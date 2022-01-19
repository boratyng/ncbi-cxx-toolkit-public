#ifndef OBJTOOLS__PUBSEQ_GATEWAY__CLIENT__IMPL__MISC__HPP
#define OBJTOOLS__PUBSEQ_GATEWAY__CLIENT__IMPL__MISC__HPP

/*  $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Authors: Rafael Sadyrov
 *
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <corelib/ncbitime.hpp>
#include <corelib/ncbi_param.hpp>

#include <connect/impl/connect_misc.hpp>

BEGIN_NCBI_SCOPE

template <>
struct SThreadSafe<void>
{
protected:
    mutex m_Mutex;
};

template <typename TType = void>
struct SPSG_CV : SThreadSafe<TType>
{
public:
    void NotifyOne() volatile { GetThis().x_NotifyOne(); }
    void NotifyAll() volatile { GetThis().x_NotifyAll(); }

    template <class... TArgs>
    bool WaitUntil(TArgs&&... args) volatile
    {
        return GetThis().x_WaitUntil(forward<TArgs>(args)...);
    }

    bool Reset() volatile { return GetThis().x_Reset(); }

private:
    using clock = chrono::system_clock;

    void x_NotifyOne() { x_Signal(); m_CV.notify_one(); }
    void x_NotifyAll() { x_Signal(); m_CV.notify_all(); }

    bool x_WaitUntil(const CDeadline& deadline)
    {
        return deadline.IsInfinite() ? x_Wait() : x_Wait(x_GetTP(deadline));
    }

    template <typename T = bool>
    bool x_WaitUntil(const volatile atomic<T>& a, const CDeadline& deadline, T v = false, bool rv = false)
    {
        constexpr auto kWait = chrono::milliseconds(100);
        const auto until = deadline.IsInfinite() ? clock::time_point::max() : x_GetTP(deadline);

        do {
            const auto max = clock::now() + kWait;

            if (until < max) {
                return x_Wait(until);
            }

            if (x_Wait(max)) {
                return true;
            }
        }
        while (a == v);

        return rv;
    }

    static clock::time_point x_GetTP(const CDeadline& d)
    {
        time_t seconds;
        unsigned int nanoseconds;

        d.GetExpirationTime(&seconds, &nanoseconds);
        const auto ns = chrono::duration_cast<clock::duration>(chrono::nanoseconds(nanoseconds));
        return clock::from_time_t(seconds) + ns;
    }

    template <class... TArgs>
    bool x_Wait(TArgs&&... args)
    {
        unique_lock<mutex> lock(SThreadSafe<TType>::m_Mutex);

        if (!x_CvWait(lock, forward<TArgs>(args)...)) return false;

        m_Signal--;
        return true;
    }

    bool x_CvWait(unique_lock<mutex>& l, const clock::time_point& t)
    {
        return m_CV.wait_until(l, t, [&](){ return m_Signal > 0; });
    }

    bool x_CvWait(unique_lock<mutex>& l)
    {
        m_CV.wait(l, [&](){ return m_Signal > 0; });
        return true;
    }

    void x_Signal()
    {
        lock_guard<mutex> lock(SThreadSafe<TType>::m_Mutex);
        m_Signal++;
    }

    bool x_Reset()
    {
        lock_guard<mutex> lock(SThreadSafe<TType>::m_Mutex);
        return exchange(m_Signal, 0);
    }

    SPSG_CV& GetThis() volatile { return const_cast<SPSG_CV&>(*this); }

    condition_variable m_CV;
    int m_Signal = 0;
};

template <class TValue>
struct CPSG_Stack
{
private:
    struct TElement
    {
        shared_ptr<TElement> next;
        TValue value;

        template <class... TArgs>
        TElement(shared_ptr<TElement> n, TArgs&&... args) : next(n), value(forward<TArgs>(args)...) {}
    };

public:
    ~CPSG_Stack() { Clear(); }

    template <class... TArgs>
    void Emplace(TArgs&&... args)
    {
        auto head = make_shared<TElement>(atomic_load(&m_Head), forward<TArgs>(args)...);

        while (!atomic_compare_exchange_weak(&m_Head, &head->next, head));
    }

    void Push(TValue value)
    {
        auto head = make_shared<TElement>(atomic_load(&m_Head), move(value));

        while (!atomic_compare_exchange_weak(&m_Head, &head->next, head));
    }

    bool Pop(TValue& value)
    {
        while (auto head = atomic_load(&m_Head)) {
            if (atomic_compare_exchange_weak(&m_Head, &head, head->next)) {
                value = move(head->value);
                return true;
            }
        }

        return false;
    }

    void Clear()
    {
        while (auto head = atomic_load(&m_Head)) {
            if (atomic_compare_exchange_weak(&m_Head, &head, {})) {
                while (auto old_head = head) {
                    head = head->next;
                }
            }
        }
    }

    bool Empty() const { return !atomic_load(&m_Head); }

private:
    shared_ptr<TElement> m_Head;
};

template <class TValue>
struct CPSG_WaitingStack : private CPSG_Stack<TValue>
{
    CPSG_WaitingStack() : m_Stopped(false) {}

    template <class... TArgs>
    void Emplace(TArgs&&... args)
    {
        if (m_Stopped) return;

        CPSG_Stack<TValue>::Emplace(forward<TArgs>(args)...);
        m_CV.NotifyOne();
    }

    void Push(TValue value)
    {
        if (m_Stopped) return;

        CPSG_Stack<TValue>::Push(move(value));
        m_CV.NotifyOne();
    }

    bool Pop(TValue& value, const CDeadline& deadline = CDeadline::eInfinite)
    {
        do {
            if (CPSG_Stack<TValue>::Pop(value)) {
                return true;
            }
        }
        while (m_CV.WaitUntil(m_Stopped, deadline));

        return false;
    }

    enum EStop { eDrain, eClear };
    void Stop(EStop stop)
    {
        m_Stopped.store(true);
        if (stop == eClear) CPSG_Stack<TValue>::Clear();
        m_CV.NotifyAll();
    }

    const atomic_bool& Stopped() const { return m_Stopped; }
    SPSG_CV<>& CV() { return m_CV; }
    bool Empty() const { return m_Stopped && CPSG_Stack<TValue>::Empty(); }

private:
    SPSG_CV<> m_CV;
    atomic_bool m_Stopped;
};

template <class TParam>
struct SPSG_ParamValue
{
    using TValue = typename TParam::TValueType;

    // Getting default incurs some performance penalty, so this ctor is explicit
    enum EGetDefault { eGetDefault };
    explicit SPSG_ParamValue(EGetDefault) : m_Value(TParam::GetDefault()) { _DEBUG_ARG(sm_Used = true); }

    operator TValue() const { return m_Value; }

    static TValue GetDefault() { return TParam::GetDefault(); }

    template <typename T>
    static void SetDefault(const T& value)
    {
        // Forbid setting after it's already used
        _ASSERT(!sm_Used);

        TParam::SetDefault(static_cast<TValue>(value));
    }

    static void SetDefault(const string& value)
    {
        SetDefaultImpl(TParam(), value);
    }

    // Overriding default but only if it's not configured explicitly
    template <typename T>
    static void SetImplicitDefault(const T& value)
    {
        bool sourcing_complete;
        typename TParam::EParamSource param_source;
        TParam::GetDefault();
        TParam::GetState(&sourcing_complete, &param_source);

        if (sourcing_complete && (param_source == TParam::eSource_Default)) {
            SetDefault(value);
        }
    }

private:
    // TDescription is not publicly available in CParam, but it's needed for string to enum conversion.
    // This templated method circumvents that shortcoming.
    template <class TDescription>
    static void SetDefaultImpl(const CParam<TDescription>&, const string& value)
    {
        SetDefault(CParam<TDescription>::TParamParser::StringToValue(value, TDescription::sm_ParamDescription));
    }

    TValue m_Value;
    _DEBUG_ARG(static bool sm_Used);
};

_DEBUG_ARG(template <class TParam> bool SPSG_ParamValue<TParam>::sm_Used = false);

#define PSG_PARAM_VALUE_TYPE(section, name) SPSG_ParamValue<NCBI_PARAM_TYPE(section, name)>

NCBI_PARAM_DECL(unsigned, PSG, rd_buf_size);
typedef NCBI_PARAM_TYPE(PSG, rd_buf_size) TPSG_RdBufSize;

NCBI_PARAM_DECL(size_t, PSG, wr_buf_size);
typedef NCBI_PARAM_TYPE(PSG, wr_buf_size) TPSG_WrBufSize;

NCBI_PARAM_DECL(unsigned, PSG, max_concurrent_streams);
typedef NCBI_PARAM_TYPE(PSG, max_concurrent_streams) TPSG_MaxConcurrentStreams;

NCBI_PARAM_DECL(unsigned, PSG, max_sessions);
typedef NCBI_PARAM_TYPE(PSG, max_sessions) TPSG_MaxSessions;

NCBI_PARAM_DECL(unsigned, PSG, num_io);
typedef NCBI_PARAM_TYPE(PSG, num_io) TPSG_NumIo;

NCBI_PARAM_DECL(unsigned, PSG, reader_timeout);
typedef NCBI_PARAM_TYPE(PSG, reader_timeout) TPSG_ReaderTimeout;

NCBI_PARAM_DECL(double, PSG, rebalance_time);
typedef NCBI_PARAM_TYPE(PSG, rebalance_time) TPSG_RebalanceTime;

NCBI_PARAM_DECL(unsigned, PSG, request_timeout);
using TPSG_RequestTimeout = PSG_PARAM_VALUE_TYPE(PSG, request_timeout);

NCBI_PARAM_DECL(size_t, PSG, requests_per_io);
using TPSG_RequestsPerIo = PSG_PARAM_VALUE_TYPE(PSG, requests_per_io);

NCBI_PARAM_DECL(unsigned, PSG, request_retries);
using TPSG_RequestRetries = PSG_PARAM_VALUE_TYPE(PSG, request_retries);

NCBI_PARAM_DECL(string, PSG, request_user_args);
typedef NCBI_PARAM_TYPE(PSG, request_user_args) TPSG_RequestUserArgs;

NCBI_PARAM_DECL(bool, PSG, user_request_ids);
using TPSG_UserRequestIds = PSG_PARAM_VALUE_TYPE(PSG, user_request_ids);

NCBI_PARAM_DECL(unsigned, PSG, localhost_preference);
typedef NCBI_PARAM_TYPE(PSG, localhost_preference) TPSG_LocalhostPreference;

NCBI_PARAM_DECL(bool, PSG, fail_on_unknown_items);
typedef NCBI_PARAM_TYPE(PSG, fail_on_unknown_items) TPSG_FailOnUnknownItems;

NCBI_PARAM_DECL(bool, PSG, https);
typedef NCBI_PARAM_TYPE(PSG, https) TPSG_Https;

NCBI_PARAM_DECL(double, PSG, no_servers_retry_delay);
typedef NCBI_PARAM_TYPE(PSG, no_servers_retry_delay) TPSG_NoServersRetryDelay;

NCBI_PARAM_DECL(bool, PSG, stats);
typedef NCBI_PARAM_TYPE(PSG, stats) TPSG_Stats;

NCBI_PARAM_DECL(double, PSG, stats_period);
typedef NCBI_PARAM_TYPE(PSG, stats_period) TPSG_StatsPeriod;

NCBI_PARAM_DECL(double, PSG, throttle_relaxation_period);
using TPSG_ThrottlePeriod = NCBI_PARAM_TYPE(PSG, throttle_relaxation_period);

NCBI_PARAM_DECL(unsigned, PSG, throttle_by_consecutive_connection_failures);
using TPSG_ThrottleMaxFailures = PSG_PARAM_VALUE_TYPE(PSG, throttle_by_consecutive_connection_failures);

NCBI_PARAM_DECL(bool, PSG, throttle_hold_until_active_in_lb);
using TPSG_ThrottleUntilDiscovery = PSG_PARAM_VALUE_TYPE(PSG, throttle_hold_until_active_in_lb);

NCBI_PARAM_DECL(string, PSG, throttle_by_connection_error_rate);
using TPSG_ThrottleThreshold = NCBI_PARAM_TYPE(PSG, throttle_by_connection_error_rate);

enum class EPSG_DebugPrintout { eNone, eSome, eAll };
NCBI_PARAM_ENUM_DECL(EPSG_DebugPrintout, PSG, debug_printout);
using TPSG_DebugPrintout = PSG_PARAM_VALUE_TYPE(PSG, debug_printout);

enum class EPSG_UseCache { eDefault, eNo, eYes };
NCBI_PARAM_ENUM_DECL(EPSG_UseCache, PSG, use_cache);
using TPSG_UseCache = PSG_PARAM_VALUE_TYPE(PSG, use_cache);

// Performance reporting/request IDs for psg_client app
enum class EPSG_PsgClientMode { eOff, ePerformance };
NCBI_PARAM_ENUM_DECL(EPSG_PsgClientMode, PSG, internal_psg_client_mode);
using TPSG_PsgClientMode = PSG_PARAM_VALUE_TYPE(PSG, internal_psg_client_mode);

END_NCBI_SCOPE

#endif
