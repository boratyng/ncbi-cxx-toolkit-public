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
 * Author: Pavel Ivanov
 *
 */

#include "nc_pch.hpp"

#include <util/random_gen.hpp>

#include "netcached.hpp"
#include "peer_control.hpp"
#include "active_handler.hpp"
#include "periodic_sync.hpp"
#include "distribution_conf.hpp"
#include "nc_storage_blob.hpp"


BEGIN_NCBI_SCOPE


typedef map<Uint8, CNCPeerControl*> TControlMap;
// "almost" not needed, as I pre-create them all on initialization
// BUT I can add new peers on RECONF
static CMiniMutex s_MapLock;
static TControlMap s_Controls;
static CAtomicCounter s_SyncOnInit;
static CAtomicCounter s_WaitToOpenToClients;
static CAtomicCounter s_AbortedSyncClients;

static CAtomicCounter s_MirrorQueueSize;
static FILE* s_MirrorLogFile = NULL;
CAtomicCounter CNCPeerControl::sm_TotalCopyRequests;
CAtomicCounter CNCPeerControl::sm_CopyReqsRejected;

static CNCPeerShutdown* s_ShutdownListener = NULL;
static Uint4 s_ServersToSync = 0;



static CMiniMutex s_RndLock;
static CRandom s_Rnd(CRandom::TValue(time(NULL)));

static void
s_SetNextTime(Uint8& next_time, Uint8 value, bool add_random)
{
    if (add_random) {
        s_RndLock.Lock();
        value += s_Rnd.GetRand(0, kUSecsPerSecond);
        s_RndLock.Unlock();
    }
    if (next_time < value)
        next_time = value;
}


SNCMirrorProlong::SNCMirrorProlong(ENCSyncEvent typ,
                                   Uint2 slot_,
                                   const CNCBlobKeyLight& key_,
                                   Uint8 rec_no,
                                   Uint8 tm,
                                   const CNCBlobAccessor* accessor)
    : SNCMirrorEvent(typ, slot_, key_, rec_no),
      orig_time(tm)
{
    blob_sum.create_id = accessor->GetCurCreateId();
    blob_sum.create_server = accessor->GetCurCreateServer();
    blob_sum.create_time = accessor->GetCurBlobCreateTime();
    blob_sum.dead_time = accessor->GetCurBlobDeadTime();
    blob_sum.expire = accessor->GetCurBlobExpire();
    blob_sum.ver_expire = accessor->GetCurVerExpire();
    blob_sum.size = accessor->GetCurBlobSize();
}


bool
CNCPeerControl::Initialize(void)
{
    s_MirrorQueueSize.Set(0);
    if (!CNCDistributionConf::GetMirroringSizeFile().empty()) {
        s_MirrorLogFile = fopen(CNCDistributionConf::GetMirroringSizeFile().c_str(), "a");
    }
    sm_TotalCopyRequests.Set(0);
    sm_CopyReqsRejected.Set(0);

    s_ShutdownListener = new CNCPeerShutdown();
    CTaskServer::AddShutdownCallback(s_ShutdownListener);

    s_MapLock.Lock();
    NON_CONST_ITERATE(TControlMap, it, s_Controls) {
        it->second->SetRunnable();
    }
    s_MapLock.Unlock();

    return true;
}

void
CNCPeerControl::Finalize(void)
{
    if (s_MirrorLogFile)
        fclose(s_MirrorLogFile);
}

CNCPeerControl*
CNCPeerControl::Peer(Uint8 srv_id)
{
    CNCPeerControl* ctrl;
    s_MapLock.Lock();
    TControlMap::const_iterator it = s_Controls.find(srv_id);
    if (it == s_Controls.end()) {
        ctrl = new CNCPeerControl(srv_id);
        s_Controls[srv_id] = ctrl;
        // s_ShutdownListener is set during initialization
        if (s_ShutdownListener)
            ctrl->SetRunnable();
    }
    else {
        ctrl = it->second;
    }
    s_MapLock.Unlock();
    return ctrl;
}

void
CNCPeerControl::PeerHandshake(void)
{
// the answer will come some time in the future
// until that, we use backward compatible protocol
    if (AtomicCAS(m_HostProtocol, 0, 1)) {
        CNCActiveHandler* conn = GetBGConn();
        if (conn) {
            conn->AskPeerVersion();
        } else {
            m_HostProtocol = 0;
        }
    }
}

string
CNCPeerControl::GetPeerNameOrEmpty(Uint8 srv_id)
{
    CNCPeerControl *ctrl = NULL;
    s_MapLock.Lock();
    TControlMap::const_iterator it = s_Controls.find(srv_id);
    if (it != s_Controls.end()) {
        ctrl = it->second;
    }
    s_MapLock.Unlock();
    string res;
    if (ctrl != NULL) {
        res += ctrl->m_Hostname;
        res += ':';
        res += NStr::NumericToString(Uint2(ctrl->m_SrvId));
    }
    return res;
}


CNCPeerControl::CNCPeerControl(Uint8 srv_id)
    : m_SrvId(srv_id),
      m_HostIP( Uint4(m_SrvId >> 32)),
      m_FirstNWErrTime(0),
      m_NextSyncTime(0),
      m_ActiveConns(0),
      m_BGConns(0),
      m_SlotsToInitSync(0),
      m_OrigSlotsToInitSync(0),
      m_CntActiveSyncs(0),
      m_CntNWErrors(0),
      m_CntNWThrottles(0),
      m_InThrottle(false),
      m_MaybeThrottle(false),
      m_HasBGTasks(false),
      m_InitiallySynced(false)
{
    m_NextTaskSync = m_SyncList.end();

     // it MUST be "host:port", see CNCDistributionConf::Initialize
    string hostport( CNCDistributionConf::GetPeerNameOrEmpty(m_SrvId));
    if (!hostport.empty()) {
        list<CTempString> srv_fields;
        NStr::Split(hostport, ":", srv_fields);
        if (srv_fields.size() == 2) {
            m_Hostname = srv_fields.front();
        }
    }
    m_HostIPname = CTaskServer::IPToString(m_HostIP);
    m_HostAlias = CNCDistributionConf::CreateHostAlias(m_HostIP, Uint4(m_SrvId));
    m_HostProtocol = 0;
}

void
CNCPeerControl::RegisterConnError(void)
{
    CMiniMutexGuard guard(m_ObjLock);
    if (m_FirstNWErrTime == 0)
        m_FirstNWErrTime = CSrvTime::Current().AsUSec();
    m_MaybeThrottle = true;
    if (++m_CntNWErrors >= CNCDistributionConf::GetCntErrorsToThrottle()) {
        m_InThrottle = true;
        m_ThrottleStart = CSrvTime::Current().AsUSec();
        ++m_CntNWThrottles;
    }
    m_HostProtocol = 0;
    CWriteBackControl::ResetStatCounters();
}

void
CNCPeerControl::RegisterConnSuccess(void)
{
    bool ask = false;
    {
        CMiniMutexGuard guard(m_ObjLock);
        m_InThrottle = false;
        m_FirstNWErrTime = 0;
        m_CntNWErrors = 0;
        m_CntNWThrottles = 0;
        m_ThrottleStart = 0;
        ask = m_HostProtocol == 0;
    }
    if (ask) {
        PeerHandshake();
    }
}

bool
CNCPeerControl::CreateNewSocket(CNCActiveHandler* conn)
{
    if (CTaskServer::IsInHardShutdown())
        return false;
    if (m_InThrottle) {
        m_ObjLock.Lock();
        if (m_InThrottle) {
            Uint8 cur_time = CSrvTime::Current().AsUSec();
            Uint8 period = CNCDistributionConf::GetPeerThrottlePeriod();
            if (cur_time - m_ThrottleStart <= period) {
                m_ObjLock.Unlock();
                SRV_LOG(Warning, "Connection to "
                    << CNCDistributionConf::GetFullPeerName(m_SrvId) << " is throttled");
                return false;
            }
            if (m_CntNWThrottles >= CNCDistributionConf::GetCntThrottlesToIpchange()) {
                Uint4 host = CTaskServer::GetIPByHost(m_Hostname);
                if (host != 0 && m_HostIP != host) {
                    m_HostIP = host;
                    m_HostIPname = CTaskServer::IPToString(m_HostIP);
                    m_HostAlias = CNCDistributionConf::CreateHostAlias(m_HostIP, Uint4(m_SrvId));
                    m_HostProtocol = 0;
                    SRV_LOG(Warning, "IP address change: host "
                        << CNCDistributionConf::GetFullPeerName(m_SrvId));
                }
                m_CntNWThrottles = 0;
            }
            m_InThrottle = false;
            if (m_InitiallySynced)
                m_FirstNWErrTime = 0;
            m_CntNWErrors = 0;
            m_ThrottleStart = 0;
        }
        m_ObjLock.Unlock();
    }

    CNCActiveHandler_Proxy* proxy = new CNCActiveHandler_Proxy(conn);
    if (!proxy->Connect(m_HostIP, Uint2(m_SrvId))) {
        delete proxy;
        RegisterConnError();
        return false;
    }
    conn->SetProxy(proxy);
    return true;
}

CNCActiveHandler*
CNCPeerControl::x_GetPooledConnImpl(void)
{
    if (m_PooledConns.empty()  ||  CTaskServer::IsInHardShutdown())
        return NULL;

    CNCActiveHandler* conn = &m_PooledConns.back();
    m_PooledConns.pop_back();
    m_BusyConns.push_back(*conn);

    return conn;
}

CNCActiveHandler*
CNCPeerControl::GetPooledConn(void)
{
    CMiniMutexGuard guard(m_ObjLock);
    CNCActiveHandler* conn = x_GetPooledConnImpl();
    if (conn) {
        ++m_ActiveConns;
    }
    return conn;
}

inline void
CNCPeerControl::x_UpdateHasTasks(void)
{
    m_HasBGTasks = !m_SmallMirror.empty()  ||  !m_BigMirror.empty()
                   ||  !m_SyncList.empty();
}

bool
CNCPeerControl::x_ReserveBGConn(void)
{
    if (m_ActiveConns >= CNCDistributionConf::GetMaxPeerTotalConns()
        ||  m_BGConns >= CNCDistributionConf::GetMaxPeerBGConns())
    {
        return false;
    }
    ++m_ActiveConns;
    ++m_BGConns;
    return true;
}

bool
CNCPeerControl::x_ReserveBGConnNow(void)
{
    ++m_ActiveConns;
    ++m_BGConns;
    return true;
}

inline void
CNCPeerControl::x_IncBGConns(void)
{
    if (++m_BGConns > m_ActiveConns) {
        SRV_FATAL("too many BGConns");
    }
}

inline void
CNCPeerControl::x_DecBGConns(void)
{
    if (m_BGConns == 0) {
        SRV_FATAL("no BGConns");
    }
    --m_BGConns;
}

inline void
CNCPeerControl::x_DecBGConns(CNCActiveHandler* conn)
{
    if (!conn  ||  conn->IsReservedForBG()) {
        x_DecBGConns();
        if (conn)
            conn->SetReservedForBG(false);
    }
}

inline void
CNCPeerControl::x_DecActiveConns(void)
{
    if (m_ActiveConns == 0  ||  --m_ActiveConns < m_BGConns) {
        SRV_FATAL("no ActiveConns");
    }
}

inline void
CNCPeerControl::x_UnreserveBGConn(void)
{
    m_ObjLock.Lock();
    x_DecBGConns();
    if(x_DoReleaseConn(NULL)) {
        m_ObjLock.Unlock();
    }
}

CNCActiveHandler*
CNCPeerControl::x_CreateNewConn(bool for_bg)
{
    CNCActiveHandler* conn = new CNCActiveHandler(m_SrvId, this);
    conn->SetReservedForBG(for_bg);
    if (!CreateNewSocket(conn)) {
        delete conn;
        conn = NULL;
    }

    if (conn) {
        m_ObjLock.Lock();
        m_BusyConns.push_back(*conn);
        m_ObjLock.Unlock();
    }

    return conn;
}

bool
CNCPeerControl::x_AssignClientConn(CNCActiveClientHub* hub,
                                   CNCActiveHandler* conn)
{
    if (!conn)
        conn = x_GetPooledConnImpl();
    m_ObjLock.Unlock();

    if (!conn) {
        conn = x_CreateNewConn(false);
        if (!conn) {
            hub->SetErrMsg(m_InThrottle? "Connection is throttled"
                                       : "Cannot connect to peer");
            hub->SetStatus(eNCHubError);
            return false;
        }
    }
    hub->SetHandler(conn);
    conn->SetClientHub(hub);
    hub->SetStatus(eNCHubConnReady);
    return true;
}

void
CNCPeerControl::AssignClientConn(CNCActiveClientHub* hub)
{
    m_ObjLock.Lock();
    if (m_ActiveConns >= CNCDistributionConf::GetMaxPeerTotalConns()) {
        hub->SetStatus(eNCHubWaitForConn);
        m_Clients.push_back(hub);
        m_ObjLock.Unlock();
        return;
    }
    ++m_ActiveConns;
    if (!x_AssignClientConn(hub, NULL)) {
        m_ObjLock.Lock();
        if (x_DoReleaseConn(NULL))
            m_ObjLock.Unlock();
    }
}

CNCActiveHandler*
CNCPeerControl::x_GetBGConnImpl(void)
{
    CNCActiveHandler* conn = x_GetPooledConnImpl();
    m_ObjLock.Unlock();
    if (conn)
        conn->SetReservedForBG(true);
    else
        conn = x_CreateNewConn(true);
    return conn;
}

CNCActiveHandler*
CNCPeerControl::GetBGConn(void)
{
    m_ObjLock.Lock();
    if (!x_ReserveBGConn()) {
        m_ObjLock.Unlock();
        SRV_LOG(Warning, "Too many active (" << m_ActiveConns
                         << ") or background (" << m_BGConns
                         << ") connections");
        return NULL;
    }
    // x_GetBGConnImpl() releases m_ObjLock
    CNCActiveHandler* conn = x_GetBGConnImpl();
    if (!conn)
        x_UnreserveBGConn();
    return conn;
}

bool
CNCPeerControl::x_DoReleaseConn(CNCActiveHandler* conn)
{
retry:
    bool result = true;
    if (!m_Clients.empty()) {
        CNCActiveClientHub* hub = m_Clients.front();
        m_Clients.pop_front();
        if (!x_AssignClientConn(hub, conn)) {
            m_ObjLock.Lock();
            goto retry;
        }

        result = false;
    }
    else if (m_HasBGTasks
             &&  m_BGConns < CNCDistributionConf::GetMaxPeerBGConns())
    {
        x_IncBGConns();
        if (!m_SmallMirror.empty()  ||  !m_BigMirror.empty()) {
            SNCMirrorEvent* event;
            if (!m_SmallMirror.empty()) {
                event = m_SmallMirror.back();
                m_SmallMirror.pop_back();
            }
            else {
                event = m_BigMirror.back();
                m_BigMirror.pop_back();
            }
            s_MirrorQueueSize.Add(-1);
            x_UpdateHasTasks();
            m_ObjLock.Unlock();

            if (conn)
                conn->SetReservedForBG(true);
            else
                conn = x_CreateNewConn(true);
            if (conn)
                x_ProcessMirrorEvent(conn, event);
            else {
                x_DeleteMirrorEvent(event);
                m_ObjLock.Lock();
                x_DecBGConns();
                goto retry;
            }
        }
        else if (!m_SyncList.empty()) {
            CNCActiveSyncControl* sync_ctrl = *m_NextTaskSync;
            SSyncTaskInfo task_info;
            if (!sync_ctrl->GetNextTask(task_info)) {
                TNCActiveSyncListIt cur_it = m_NextTaskSync;
                ++m_NextTaskSync;
                m_SyncList.erase(cur_it);
            }
            else 
                ++m_NextTaskSync;
            if (m_NextTaskSync == m_SyncList.end())
                m_NextTaskSync = m_SyncList.begin();
            x_UpdateHasTasks();
            m_ObjLock.Unlock();

            if (conn)
                conn->SetReservedForBG(true);
            else
                conn = x_CreateNewConn(true);
            if (conn)
                sync_ctrl->ExecuteSyncTask(task_info, conn);
            else {
                sync_ctrl->CmdFinished(eSynNetworkError, eSynActionNone, NULL);
                m_ObjLock.Lock();
                x_DecBGConns();
                goto retry;
            }
        }
        else {
            SRV_FATAL("Unexpected state");
        }

        result = false;
    }
    else {
        x_DecActiveConns();
    }
    return result;
}

void
CNCPeerControl::PutConnToPool(CNCActiveHandler* conn)
{
    m_ObjLock.Lock();
    x_DecBGConns(conn);
    if (x_DoReleaseConn(conn)) {
        m_BusyConns.erase(m_BusyConns.iterator_to(*conn));
        m_PooledConns.push_back(*conn);
        m_ObjLock.Unlock();
    }
}

void
CNCPeerControl::ReleaseConn(CNCActiveHandler* conn)
{
    m_ObjLock.Lock();
    x_DecBGConns(conn);
    m_BusyConns.erase(m_BusyConns.iterator_to(*conn));
    if (x_DoReleaseConn(NULL))
        m_ObjLock.Unlock();
}

void
CNCPeerControl::x_DeleteMirrorEvent(SNCMirrorEvent* event)
{
    if (event->evt_type == eSyncWrite || event->evt_type == eSyncUpdate)
        delete event;
    else if (event->evt_type == eSyncProlong)
        delete (SNCMirrorProlong*)event;
    else {
        SRV_FATAL("Unexpected mirror event type: " << event->evt_type);
    }
}

void
CNCPeerControl::x_ProcessUpdateEvent(SNCMirrorEvent* event)
{
    m_ObjLock.Lock();
//    if (x_ReserveBGConnNow()) {
    if (x_ReserveBGConn()) {
        CNCActiveHandler* conn = x_GetBGConnImpl();
        if (conn) {
            x_ProcessMirrorEvent(conn, event);
        } else {
            x_DeleteMirrorEvent(event);
            x_UnreserveBGConn();
        }
    } else {
        m_ObjLock.Unlock();
        x_DeleteMirrorEvent(event);
    }
}

void
CNCPeerControl::x_ProcessMirrorEvent(CNCActiveHandler* conn, SNCMirrorEvent* event)
{
    if (event->evt_type == eSyncWrite) {
        conn->CopyPut(NULL, event->key, event->slot, event->orig_rec_no);
    }
    else if (event->evt_type == eSyncProlong) {
        SNCMirrorProlong* prolong = (SNCMirrorProlong*)event;
        conn->CopyProlong(prolong->key, prolong->slot, prolong->orig_rec_no,
                          prolong->orig_time, prolong->blob_sum);
    }
    else if (event->evt_type == eSyncUpdate) {
        conn->CopyUpdate(event->key, event->orig_rec_no);
    }
    else {
        SRV_FATAL("Unexpected mirror event type: " << event->evt_type);
    }
    x_DeleteMirrorEvent(event);
}

void
CNCPeerControl::x_AddMirrorEvent(SNCMirrorEvent* event, Uint8 size)
{
    sm_TotalCopyRequests.Add(1);

    m_ObjLock.Lock();
    if (x_ReserveBGConn()) {
        // x_GetBGConnImpl() releases m_ObjLock
        CNCActiveHandler* conn = x_GetBGConnImpl();
        if (conn)
            x_ProcessMirrorEvent(conn, event);
        else {
            x_DeleteMirrorEvent(event);
            x_UnreserveBGConn();
        }
    }
    else {
        TNCMirrorQueue* q;
        if (size <= CNCDistributionConf::GetSmallBlobBoundary())
            q = &m_SmallMirror;
        else
            q = &m_BigMirror;
        if (q->size() < CNCDistributionConf::GetMaxMirrorQueueSize()) {
            q->push_back(event);
            m_HasBGTasks = true;
            m_ObjLock.Unlock();

            int queue_size = s_MirrorQueueSize.Add(1);
            if (s_MirrorLogFile) {
                Uint8 cur_time = CSrvTime::Current().AsUSec();
                fprintf(s_MirrorLogFile, "%" NCBI_UINT8_FORMAT_SPEC ",%d\n",
                                         cur_time, queue_size);
            }
        }
        else {
            m_ObjLock.Unlock();
            sm_CopyReqsRejected.Add(1);
            x_DeleteMirrorEvent(event);
        }
    }
}

void
CNCPeerControl::MirrorUpdate(const CNCBlobKey& key,
                              Uint2 slot,
                              Uint8 update_time)
{
    const TServersList& servers = CNCDistributionConf::GetRawServersForSlot(slot);
    ITERATE(TServersList, it_srv, servers) {
        Uint8 srv_id = *it_srv;
        CNCPeerControl* peer = Peer(srv_id);
        if (peer->AcceptsSyncUpdate()) {
            SNCMirrorEvent* event = new SNCMirrorEvent(eSyncUpdate, slot, key, update_time);
            if (event) {
                peer->x_ProcessUpdateEvent(event);
            }
        }
    }
}

void
CNCPeerControl::MirrorWrite(const CNCBlobKey& key,
                          Uint2 slot,
                          Uint8 orig_rec_no,
                          Uint8 size)
{
    const TServersList& servers = CNCDistributionConf::GetRawServersForSlot(slot);
    ITERATE(TServersList, it_srv, servers) {
        Uint8 srv_id = *it_srv;
        CNCPeerControl* peer = Peer(srv_id);
        if (peer->AcceptsBlobKey(key)) {
            SNCMirrorEvent* event = new SNCMirrorEvent(eSyncWrite, slot, key, orig_rec_no);
            peer->x_AddMirrorEvent(event, size);
        }
    }
}

void
CNCPeerControl::MirrorProlong(const CNCBlobKey& key,
                            Uint2 slot,
                            Uint8 orig_rec_no,
                            Uint8 orig_time,
                            const CNCBlobAccessor* accessor)
{
    const TServersList& servers = CNCDistributionConf::GetRawServersForSlot(slot);
    ITERATE(TServersList, it_srv, servers) {
        Uint8 srv_id = *it_srv;
        CNCPeerControl* peer = Peer(srv_id);
        if (peer->AcceptsBlobKey(key)) {
            SNCMirrorProlong* event = new SNCMirrorProlong(eSyncProlong, slot, key.PackedKey(),
                                                   orig_rec_no, orig_time, accessor);
            peer->x_AddMirrorEvent(event, 0);
        }
    }
}

Uint8
CNCPeerControl::GetMirrorQueueSize(void)
{
    return s_MirrorQueueSize.Get();
}

Uint8
CNCPeerControl::GetMirrorQueueSize(Uint8 srv_id)
{
    CNCPeerControl* peer = Peer(srv_id);
    CMiniMutexGuard guard(peer->m_ObjLock);
    return peer->m_SmallMirror.size() + peer->m_BigMirror.size();
}

void
CNCPeerControl::ReadCurState(SNCStateStat& state)
{
    Uint4 active = 0,  bg = 0;
    s_MapLock.Lock();
    ITERATE(TControlMap, it_ctrl, s_Controls) {
        CNCPeerControl* peer = it_ctrl->second;
        active += peer->m_ActiveConns;
        bg += peer->m_BGConns;
    }
    s_MapLock.Unlock();
    state.mirror_queue_size = CNCPeerControl::GetMirrorQueueSize();
    state.peer_active_conns = active;
    state.peer_bg_conns = bg;
}

Uint4
CNCPeerControl::FindIPbyAlias(Uint4 alias)
{
    Uint4 res = 0;
    s_MapLock.Lock();
    ITERATE(TControlMap, it_ctrl, s_Controls) {
        CNCPeerControl* peer = it_ctrl->second;
        if (!peer->m_MaybeThrottle && peer->m_HostAlias == alias) {
            res = peer->m_HostIP;
            break;
        }
    }
    s_MapLock.Unlock();
    return res;
}

Uint4
CNCPeerControl::FindIPbyName(const string& alias)
{
    Uint4 res = 0;
    s_MapLock.Lock();
    ITERATE(TControlMap, it_ctrl, s_Controls) {
        CNCPeerControl* peer = it_ctrl->second;
        if (!peer->m_MaybeThrottle && peer->m_HostIPname == alias) {
            res = peer->m_HostIP;
            break;
        }
    }
    s_MapLock.Unlock();
    return res;
}

bool
CNCPeerControl::HasPeerInThrottle(void)
{
    bool res = false;
    s_MapLock.Lock();
    ITERATE(TControlMap, it_ctrl, s_Controls) {
        if (CNCDistributionConf::HasCommonSlots(it_ctrl->first) && 
            it_ctrl->second->m_MaybeThrottle) {
            res = true;
            break;
        }
    }
    s_MapLock.Unlock();
    return res;
}

void CNCPeerControl::PrintState(CSrvSocketTask& task)
{
    s_MapLock.Lock();
    TControlMap ctrl = s_Controls;
    s_MapLock.Unlock();

    string is("\": "), iss("\": \""), eol(",\n\""),  qt("\"");

    task.WriteText(eol).WriteText("peers").WriteText(is).WriteText("\n[");
    for(TControlMap::const_iterator it = ctrl.begin(); it != ctrl.end(); ++it) {
        if (it != ctrl.begin()) {
            task.WriteText(",");
        }
        task.WriteText("{\n");
        CNCPeerControl* peer = it->second;
        task.WriteText(qt).WriteText("hostname").WriteText(iss).WriteText(
                    peer->GetPeerNameOrEmpty(peer->GetSrvId())).WriteText(qt);
        task.WriteText(eol).WriteText("hostIPname").WriteText(iss).WriteText(peer->m_HostIPname).WriteText(qt);
        task.WriteText(eol).WriteText("hostProtocol").WriteText(is).WriteNumber(peer->m_HostProtocol);
        task.WriteText(eol).WriteText("healthy").WriteText(is).WriteText(
                    (peer->m_InThrottle || peer->m_MaybeThrottle) ? "false" : "true");
        task.WriteText(eol).WriteText("initiallySynced").WriteText(is).WriteText(
                    peer->m_InitiallySynced ? "true" : "false");
        task.WriteText(eol).WriteText("origSlotsToInitSync").WriteText(is).WriteNumber(peer->m_OrigSlotsToInitSync);
        task.WriteText(eol).WriteText("slotsToInitSync").WriteText(is).WriteNumber(peer->m_SlotsToInitSync);
        task.WriteText(eol).WriteText("cntActiveSyncs").WriteText(is).WriteNumber(peer->m_CntActiveSyncs);
        task.WriteText(eol).WriteText("cntNWErrors").WriteText(is).WriteNumber(peer->m_CntNWErrors);
        task.WriteText(eol).WriteText("hasBGTasks").WriteText(is).WriteText(
                    peer->m_HasBGTasks ? "true" : "false");
        task.WriteText(eol).WriteText("activeConns").WriteText(is).WriteNumber(peer->m_ActiveConns);
        task.WriteText(eol).WriteText("bGConns").WriteText(is).WriteNumber(peer->m_BGConns);

        task.WriteText("\n}");
    }
    task.WriteText("]");
}

void
CNCPeerControl::SetServersForInitSync(Uint4 cnt_servers)
{
    s_ServersToSync = cnt_servers;
    s_SyncOnInit.Set(cnt_servers);
    s_WaitToOpenToClients.Set(cnt_servers);
    s_AbortedSyncClients.Set(cnt_servers);
}

void
CNCPeerControl::ResetServersForInitSync(void)
{
    SetServersForInitSync(s_ServersToSync);
}

void
CNCPeerControl::ReconfServersForInitSync(Uint4 cnt_servers)
{
    s_ServersToSync = cnt_servers;
}

bool
CNCPeerControl::HasServersForInitSync(void)
{
    return s_SyncOnInit.Get() != 0;
}

bool
CNCPeerControl::StartActiveSync(void)
{
    CMiniMutexGuard guard(m_ObjLock);
    if (m_CntActiveSyncs >= CNCDistributionConf::GetMaxSyncsOneServer())
        return false;
    ++m_CntActiveSyncs;
    return true;
}

void
CNCPeerControl::x_SrvInitiallySynced(bool succeeded)
{
    if (!m_InitiallySynced) {
        INFO("Initial sync: for "
            << CNCDistributionConf::GetFullPeerName(m_SrvId) << " completed");
        m_InitiallySynced = true;
        s_SyncOnInit.Add(-1);
        CNCStat::InitialSyncDone(m_SrvId, succeeded);
    }
}

void
CNCPeerControl::x_SlotsInitiallySynced(Uint2 cnt_slots, bool aborted)
{
    if (cnt_slots != 0  &&  m_SlotsToInitSync != 0) {
        bool succeeded = true;
        if (cnt_slots != 1) {
            INFO("Initial sync: Server "
                << CNCDistributionConf::GetFullPeerName(m_SrvId) << " is out of reach (timeout)");
            succeeded = false;
        }
        m_SlotsToInitSync -= cnt_slots;
        if (m_SlotsToInitSync == 0) {
            x_SrvInitiallySynced(succeeded);
            if (aborted && s_AbortedSyncClients.Add(-1) == 0) {
#if 1
                SRV_LOG(Error, "Initial sync: unable to synchronize with any server");
#else
                SRV_LOG(Critical, "Initial sync: unable to synchronize with any server");
                CTaskServer::RequestShutdown(eSrvSlowShutdown);
#endif
            }
            if (s_WaitToOpenToClients.Add(-1) == 0)
                CNCServer::InitialSyncComplete();
        }
    }
}

void
CNCPeerControl::AddInitiallySyncedSlot(void)
{
    CMiniMutexGuard guard(m_ObjLock);
    x_SlotsInitiallySynced(1);
}

void
CNCPeerControl::RegisterSyncStop(bool is_passive,
                                 Uint8& next_sync_time,
                                 Uint8 next_sync_delay)
{
    CMiniMutexGuard guard(m_ObjLock);
    Uint8 now = CSrvTime::Current().AsUSec();
    Uint8 next_time = now + next_sync_delay;
    s_SetNextTime(next_sync_time, next_time, true);
    if (m_FirstNWErrTime == 0) {
        s_SetNextTime(m_NextSyncTime, now, false);
    }
    else {
        s_SetNextTime(m_NextSyncTime, next_time, true);
        if (now - m_FirstNWErrTime >= CNCDistributionConf::GetNetworkErrorTimeout())
            x_SlotsInitiallySynced(m_SlotsToInitSync, m_FirstNWErrTime == 1);
    }

    if (!is_passive)
        --m_CntActiveSyncs;
}

void CNCPeerControl::AbortInitialSync(void)
{
    m_FirstNWErrTime = 1;
}

void CNCPeerControl::SetHostProtocol(Uint8 ver)
{
    m_HostProtocol = ver;
}

bool
CNCPeerControl::AddSyncControl(CNCActiveSyncControl* sync_ctrl)
{
    bool has_more = true;
    bool task_added = false;
    SSyncTaskInfo task_info;

    m_ObjLock.Lock();
    while (has_more  &&  x_ReserveBGConn()) {
        // x_GetBGConnImpl() releases m_ObjLock
        CNCActiveHandler* conn = x_GetBGConnImpl();
        if (!conn) {
            x_UnreserveBGConn();
            if (!task_added)
                return false;
            m_ObjLock.Lock();
            break;
        }
        has_more = sync_ctrl->GetNextTask(task_info);
        sync_ctrl->ExecuteSyncTask(task_info, conn);
        task_added = true;
        m_ObjLock.Lock();
    }
    if (has_more) {
        m_SyncList.push_back(sync_ctrl);
        if (m_NextTaskSync == m_SyncList.end())
            m_NextTaskSync = m_SyncList.begin();
        m_HasBGTasks = true;
    }
    m_ObjLock.Unlock();

    return true;
}

bool
CNCPeerControl::FinishSync(CNCActiveSyncControl* sync_ctrl)
{
    m_ObjLock.Lock();
    if (x_ReserveBGConn()) {
        // x_GetBGConnImpl() releases m_ObjLock
        CNCActiveHandler* conn = x_GetBGConnImpl();
        if (!conn) {
            x_UnreserveBGConn();
            return false;
        }

        SSyncTaskInfo task_info;
        sync_ctrl->GetNextTask(task_info);
        sync_ctrl->ExecuteSyncTask(task_info, conn);
    }
    else {
        m_SyncList.push_back(sync_ctrl);
        if (m_NextTaskSync == m_SyncList.end())
            m_NextTaskSync = m_SyncList.begin();
        m_HasBGTasks = true;
        m_ObjLock.Unlock();
    }
    return true;
}

void
CNCPeerControl::ExecuteSlice(TSrvThreadNum /* thr_num */)
{
    if (CTaskServer::IsInShutdown())
        return;

// check for timeouts
    m_ObjLock.Lock();

    NON_CONST_ITERATE(TNCPeerConnsList, it, m_BusyConns) {
        it->CheckCommandTimeout();
    }

    m_ObjLock.Unlock();

    RunAfter(1);
}

bool
CNCPeerControl::GetReadyForShutdown(void)
{
    bool result = true;

    m_ObjLock.Lock();
    if (CTaskServer::IsInHardShutdown()) {
        while (!m_Clients.empty()) {
            CNCActiveClientHub* hub = m_Clients.front();
            m_Clients.pop_front();
            hub->SetErrMsg("Server is shutting down");
            hub->SetStatus(eNCHubError);
            result = false;
        }
    }
    NON_CONST_ITERATE(TNCPeerConnsList, it, m_BusyConns) {
        it->CheckCommandTimeout();
        result = false;
    }
    ERASE_ITERATE(TNCActiveSyncList, it_sync, m_SyncList) {
        CNCActiveSyncControl* sync_ctrl = *it_sync;
        m_SyncList.erase(it_sync);

        SSyncTaskInfo task_info;
        bool has_more = sync_ctrl->GetNextTask(task_info);
        sync_ctrl->CmdFinished(eSynNetworkError, eSynActionNone, NULL);
        if (has_more)
            sync_ctrl->GetNextTask(task_info);
        result = false;
    }
    m_SyncList.clear();
    m_NextTaskSync = m_SyncList.end();
    x_UpdateHasTasks();
    if (m_HasBGTasks)
        result = false;

    if (result) {
        if (CNCStat::GetCntRunningCmds() != 0) {
            result = false;
        }
        else {
            while (!m_PooledConns.empty()) {
                CNCActiveHandler* conn = &m_PooledConns.front();
                m_PooledConns.pop_front();
                conn->CloseForShutdown();
                result = false;
            }
        }
    }
    m_ObjLock.Unlock();

    return result;
}


CNCPeerShutdown::CNCPeerShutdown(void)
{}

CNCPeerShutdown::~CNCPeerShutdown(void)
{}

bool
CNCPeerShutdown::ReadyForShutdown(void)
{
    bool result = true;
    s_MapLock.Lock();
    ITERATE(TControlMap, it_ctrl, s_Controls) {
        CNCPeerControl* peer = it_ctrl->second;
        result &= peer->GetReadyForShutdown();
    }
    s_MapLock.Unlock();
    return result;
}

END_NCBI_SCOPE
