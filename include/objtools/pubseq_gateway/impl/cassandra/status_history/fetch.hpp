#ifndef OBJTOOLS__PUBSEQ_GATEWAY__CASSANDRA__STATUS_HISTORY__FETCH_HPP
#define OBJTOOLS__PUBSEQ_GATEWAY__CASSANDRA__STATUS_HISTORY__FETCH_HPP

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
 * Authors: Dmitrii Saprykin
 *
 * File Description:
 *
 * Cassandra fetch status history record task.
 *
 */

#include <corelib/request_status.hpp>
#include <corelib/ncbidiag.hpp>

#include <memory>
#include <optional>
#include <string>

#include <objtools/pubseq_gateway/impl/cassandra/cass_blob_op.hpp>
#include <objtools/pubseq_gateway/impl/cassandra/IdCassScope.hpp>

#include <objtools/pubseq_gateway/impl/cassandra/status_history/record.hpp>

BEGIN_IDBLOB_SCOPE
USING_NCBI_SCOPE;

class CCassStatusHistoryTaskFetch
    : public CCassBlobWaiter
{
    enum EStatusHistoryFetchState {
        eInit = 0,
        eFetchStarted,
        eDone = CCassBlobWaiter::eDone,
        eError = CCassBlobWaiter::eError
    };
 public:
    CCassStatusHistoryTaskFetch(
        shared_ptr<CCassConnection> conn,
        const string & keyspace,
        int32_t sat_key,
        int64_t done_when,
        TDataErrorCallback data_error_cb
    );

    CCassStatusHistoryTaskFetch(
        shared_ptr<CCassConnection> conn,
        const string & keyspace,
        int32_t sat_key,
        TDataErrorCallback data_error_cb
    );
    unique_ptr<CBlobStatusHistoryRecord> Consume();
 protected:
    void Wait1() override;
 private:
    optional<int64_t> m_DoneWhen;
    unique_ptr<CBlobStatusHistoryRecord> m_Record;
};

END_IDBLOB_SCOPE

#endif  // OBJTOOLS__PUBSEQ_GATEWAY__CASSANDRA__STATUS_HISTORY__FETCH_HPP
