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
 *   Government have not placed any restriction on its use or reproduction.
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
 * Authors:  Maxim Didenko, Dmitry Kazimirov
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>

#include "netservice_api_impl.hpp"

#include <connect/services/srv_connections_expt.hpp>
#include <connect/services/netservice_api_expt.hpp>
#include <connect/services/netservice_params.hpp>
#include <connect/services/error_codes.hpp>

#include <connect/ncbi_conn_exception.hpp>

#include <corelib/ncbi_system.hpp>


#define NCBI_USE_ERRCODE_X   ConnServ_Connection


BEGIN_NCBI_SCOPE

///////////////////////////////////////////////////////////////////////////
SNetServerCmdOutputImpl::~SNetServerCmdOutputImpl()
{
    if (!m_ReadCompletely) {
        m_Connection->Close();
        ERR_POST_X(7, "Multiline command output was not completely read");
    }
}

bool CNetServerCmdOutput::ReadLine(string& output)
{
    _ASSERT(!m_Impl->m_ReadCompletely);

    if (!m_Impl->m_NetCacheCompatMode)
        output = m_Impl->m_Connection->ReadCmdOutputLine();
    else {
        try {
            output = m_Impl->m_Connection->ReadCmdOutputLine();
        }
        catch (CNetServiceException& e) {
            if (e.GetErrCode() != CNetServiceException::eCommunicationError)
                throw;

            m_Impl->m_ReadCompletely = true;
            return false;
        }
    }

    if (output != "END")
        return true;
    else {
        m_Impl->m_ReadCompletely = true;
        return false;
    }
}

inline SNetServerConnectionImpl::SNetServerConnectionImpl(
    SNetServerImpl* server) :
        m_Server(server),
        m_NextFree(NULL)
{
    if (TServConn_UserLinger2::GetDefault()) {
        STimeout zero = {0,0};
        m_Socket.SetTimeout(eIO_Close,&zero);
    }
}

void SNetServerConnectionImpl::Delete()
{
    // Return this connection to the pool.
    if (m_Server->m_Service->m_PermanentConnection != eOff &&
            m_Socket.GetStatus(eIO_Open) == eIO_Success) {
        // we need to check if all data has been read.
        // if it has not then we have to close the connection
        STimeout to = {0, 0};

        if (m_Socket.Wait(eIO_Read, &to) == eIO_Success) {
            Abort();
            ERR_POST_X(8, "Socket not fully read while returning to the pool");
        } else {
            TFastMutexGuard guard(m_Server->m_FreeConnectionListLock);

            int upper_limit = TServConn_MaxConnPoolSize::GetDefault();

            if (upper_limit == 0 ||
                    m_Server->m_FreeConnectionListSize < upper_limit) {
                m_NextFree = m_Server->m_FreeConnectionListHead;
                m_Server->m_FreeConnectionListHead = this;
                ++m_Server->m_FreeConnectionListSize;
                m_Server = NULL;
                return;
            }
        }
    }

    // Could not return the connection to the pool, delete it.
    delete this;
}

string SNetServerConnectionImpl::ReadCmdOutputLine()
{
    string result;

    EIO_Status io_st = m_Socket.ReadLine(result);

    switch (io_st)
    {
    case eIO_Success:
        break;
    case eIO_Timeout:
        Abort();
        NCBI_THROW(CNetSrvConnException, eReadTimeout,
            "Communication timeout reading from " +
            m_Server->GetAddressAsString());
        break;
    default: // invalid socket or request, bailing out
        Abort();
        NCBI_THROW(CNetServiceException, eCommunicationError,
            "Communication error reading from " +
            m_Server->GetAddressAsString());
    }

    if (NStr::StartsWith(result, "OK:")) {
        result.erase(0, sizeof("OK:") - 1);
    } else if (NStr::StartsWith(result, "ERR:")) {
        result.erase(0, sizeof("ERR:") - 1);
        result = NStr::ParseEscapes(result);
        m_Server->m_Service->m_Listener->OnError(result, m_Server);
    }
    return result;
}

/*
bool SNetServerConnectionImpl::IsConnected()
{
    EIO_Status st = m_Socket.GetStatus(eIO_Open);
    if (st != eIO_Success)
        return false;

    STimeout zero = {0, 0}, tmo;
    const STimeout* tmp = m_Socket.GetTimeout(eIO_Read);
    if (tmp) {
        tmo = *tmp;
        tmp = &tmo;
    }
    if (m_Socket.SetTimeout(eIO_Read, &zero) != eIO_Success)
        return false;

    st = m_Socket.Read(0,1,0,eIO_ReadPeek);
    bool ret = false;
    switch (st) {
         case eIO_Closed:
            m_Socket.Close();
            return false;
         case eIO_Success:
            if (m_Socket.GetStatus(eIO_Read) != eIO_Success) {
                m_Socket.Close();
                break;
            }
         case eIO_Timeout:
            ret = true;
         default:
            break;
    }

    if (m_Socket.SetTimeout(eIO_Read, tmp) != eIO_Success)
        return false;

    return ret;
}
*/

void SNetServerConnectionImpl::Close()
{
    m_Socket.Close();
}

void SNetServerConnectionImpl::Abort()
{
    m_Socket.Abort();
}

SNetServerConnectionImpl::~SNetServerConnectionImpl()
{
    Close();
}

void SNetServerConnectionImpl::WriteLine(const string& line)
{
    // TODO change to "\n" when no old NS/NC servers remain.
    string str(line + "\r\n");

    const char* buf = str.data();
    size_t len = str.size();

    while (len > 0) {
        size_t n_written;

        EIO_Status io_st = m_Socket.Write(buf, len, &n_written);

        if (io_st != eIO_Success) {
            Abort();

            CIO_Exception io_ex(DIAG_COMPILE_INFO, 0,
                (CIO_Exception::EErrCode) io_st, "IO error.");

            NCBI_THROW(CNetSrvConnException, eWriteFailure,
                "Failed to write to " + m_Server->GetAddressAsString() +
                ": " + string(io_ex.what()));
        }
        len -= n_written;
        buf += n_written;
    }
}

void SNetServerConnectionImpl::WaitForServer()
{
    EIO_Status io_st = m_Socket.Wait(eIO_Read, &m_Server->m_Service->m_Timeout);

    if (io_st == eIO_Timeout) {
        Abort();

        NCBI_THROW(CNetSrvConnException, eResponseTimeout,
            "No response from " + m_Server->GetAddressAsString());
    }
}

string CNetServerConnection::Exec(const string& cmd)
{
    m_Impl->WriteLine(cmd);
    m_Impl->WaitForServer();

    return m_Impl->ReadCmdOutputLine();
}

CNetServerCmdOutput CNetServerConnection::ExecMultiline(const string& cmd)
{
    m_Impl->WriteLine(cmd);
    m_Impl->WaitForServer();

    return new SNetServerCmdOutputImpl(m_Impl);
}

const string& CNetServerConnection::GetHost() const
{
    return m_Impl->m_Server->m_Host;
}

unsigned int CNetServerConnection::GetPort() const
{
    return m_Impl->m_Server->m_Port;
}


/*************************************************************************/
SNetServerImplReal::SNetServerImplReal(const string& host,
    unsigned short port) : SNetServerImpl(host, port)
{
    m_FreeConnectionListHead = NULL;
    m_FreeConnectionListSize = 0;
}

void SNetServerImpl::Delete()
{
    // Before resetting the m_Service pointer, verify that nobody
    // has acquired a reference to this server object yet (between
    // the time the reference counter went to zero, and the
    // current moment when m_Service is about to be reset).

    CFastMutexGuard g(m_Service->m_ConnectionMutex);

    if (m_Refs.Get() == 0)
        m_Service = NULL;
}

string SNetServerImpl::GetAddressAsString() const
{
    string address =
        CSocketAPI::gethostbyaddr(CSocketAPI::gethostbyname(m_Host));

    address += ':';
    address += NStr::UIntToString(m_Port);

    return address;
}

SNetServerImplReal::~SNetServerImplReal()
{
    // No need to lock the mutex in the destructor.
    SNetServerConnectionImpl* impl = m_FreeConnectionListHead;
    while (impl != NULL) {
        SNetServerConnectionImpl* next_impl = impl->m_NextFree;
        delete impl;
        impl = next_impl;
    }
}

string CNetServer::GetHost() const
{
    return m_Impl->m_Host;
}

unsigned short CNetServer::GetPort() const
{
    return m_Impl->m_Port;
}

CNetServerConnection CNetServer::Connect()
{
    {{
        // Get an existing connection object from the connection pool.

        TFastMutexGuard guard(m_Impl->m_FreeConnectionListLock);

        if (m_Impl->m_FreeConnectionListSize > 0) {
            CNetServerConnection conn = m_Impl->m_FreeConnectionListHead;

            m_Impl->m_FreeConnectionListHead = conn->m_NextFree;
            --m_Impl->m_FreeConnectionListSize;
            conn->m_Server = m_Impl;

            return conn;
        }
    }}

    CNetServerConnection conn = new SNetServerConnectionImpl(m_Impl);

    unsigned conn_repeats = 0;

    EIO_Status io_st;

    while ((io_st = conn->m_Socket.Connect(m_Impl->m_Host, m_Impl->m_Port,
        &m_Impl->m_Service->m_Timeout, eOn)) != eIO_Success) {

        if (io_st == eIO_Unknown) {

            conn->m_Socket.Close();

            // most likely this is an indication we have too many
            // open ports on the client side
            // (this kernel limitation manifests itself on Linux)
            if (++conn_repeats > m_Impl->m_Service->m_MaxRetries) {
                if (io_st != eIO_Success) {
                    CIO_Exception io_ex(DIAG_COMPILE_INFO,
                        0, (CIO_Exception::EErrCode)io_st, "IO error.");
                    NCBI_THROW(CNetSrvConnException, eConnectionFailure,
                        "Failed to connect to " + m_Impl->GetAddressAsString() +
                            ": " + string(io_ex.what()));
                }
            }
            // give system a chance to recover

            SleepMilliSec(1000 * conn_repeats);
        } else {
            CIO_Exception io_ex(DIAG_COMPILE_INFO,  0, (CIO_Exception::EErrCode)io_st,  "IO error.");
            NCBI_THROW(CNetSrvConnException, eConnectionFailure,
                "Failed to connect to " + m_Impl->GetAddressAsString() +
                ": " + string(io_ex.what()));
        }
    }

    conn->m_Socket.SetDataLogging(eDefault);
    conn->m_Socket.SetTimeout(eIO_ReadWrite, &m_Impl->m_Service->m_Timeout);
    conn->m_Socket.DisableOSSendDelay();
    conn->m_Socket.SetReuseAddress(eOn);

    if (m_Impl->m_Service->m_Listener)
        m_Impl->m_Service->m_Listener->OnConnected(conn);

    return conn;
}

END_NCBI_SCOPE
