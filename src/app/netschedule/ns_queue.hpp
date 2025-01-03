#ifndef NETSCHEDULE_NS_QUEUE__HPP
#define NETSCHEDULE_NS_QUEUE__HPP

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
 * Authors:  Victor Joukov
 *
 * File Description:
 *   NetSchedule queue structure and parameters
 *
 */

#include <corelib/ncbistl.hpp>
#include <corelib/ncbireg.hpp>

#include <util/thread_nonstop.hpp>
#include <util/time_line.hpp>

#include <connect/server_monitor.hpp>

#include "ns_types.hpp"
#include "ns_util.hpp"
#include "background_host.hpp"
#include "job.hpp"
#include "job_status.hpp"
#include "queue_vc.hpp"
#include "access_list.hpp"
#include "ns_affinity.hpp"
#include "ns_queue_parameters.hpp"
#include "ns_clients_registry.hpp"
#include "ns_notifications.hpp"
#include "queue_clean_thread.hpp"
#include "ns_statistics_counters.hpp"
#include "ns_group.hpp"
#include "ns_gc_registry.hpp"
#include "ns_precise_time.hpp"
#include "ns_scope.hpp"
#include "ns_server_params.hpp"

#include <map>

BEGIN_NCBI_SCOPE

class CNetScheduleServer;
class CNSRollbackInterface;
class CQueue;

// slight violation of naming convention for porting to util/time_line
typedef CTimeLine<TNSBitVector>     CJobTimeLine;

// Mutex protected Queue database with job status FSM
//
// Class holds the queue database (open files and indexes),
// thread sync mutexes and classes auxiliary queue management concepts
// (like affinity and job status bit-matrix)
class CQueue : public CObjectEx
{
public:
    enum EQueueKind {
        eKindStatic  = 0,
        eKindDynamic = 1
    };
    typedef int TQueueKind;

public:
    enum EPauseStatus {
        eNoPause              = 0,
        ePauseWithPullback    = 1,
        ePauseWithoutPullback = 2
    };
    typedef int TPauseStatus;

public:
    enum EJobReturnOption {
        eWithBlacklist    = 0,
        eWithoutBlacklist = 1,
        eRollback         = 2
    };
    typedef int TJobReturnOption;

public:
    // Constructor/destructor
    CQueue(const string&         queue_name,
           TQueueKind            queue_kind,
           CNetScheduleServer *  server,
           CQueueDataBase &      qdb);
    ~CQueue();

    void Attach(void);
    TQueueKind GetQueueKind(void) const { return m_Kind; }

    // Thread-safe parameter access
    typedef list<pair<string, string> > TParameterList;
    void SetParameters(const SQueueParameters& params);
    TParameterList GetParameters() const;
    CNSPreciseTime GetTimeout() const;
    CNSPreciseTime GetRunTimeout() const;
    CNSPreciseTime GetReadTimeout() const;
    CNSPreciseTime GetPendingTimeout() const;
    CNSPreciseTime  GetMaxPendingWaitTimeout() const;
    unsigned GetFailedRetries() const;
    bool IsSubmitAllowed(unsigned host) const;
    bool IsWorkerAllowed(unsigned host) const;
    bool IsReaderAllowed(unsigned host) const;
    bool IsProgramAllowed(const string &  program_name) const;
    void GetMaxIOSizesAndLinkedSections(
                unsigned int &  max_input_size,
                unsigned int &  max_output_size,
                map< string, map<string, string> > & linked_sections) const;
    void GetLinkedSections(map< string,
                                map<string, string> > & linked_sections) const;

    bool GetRefuseSubmits(void) const { return m_RefuseSubmits; }
    void SetRefuseSubmits(bool  val) { m_RefuseSubmits = val; }
    size_t GetAffSlotsUsed(void) const { return m_AffinityRegistry.size(); }
    size_t GetGroupSlotsUsed(void) const { return m_GroupRegistry.size(); }
    size_t GetScopeSlotsUsed(void) const { return m_ScopeRegistry.size(); }
    size_t GetClientsCount(void) const { return m_ClientsRegistry.size(); }
    size_t GetGroupsCount(void) const { return m_GroupRegistry.size(); }
    size_t GetNotifCount(void) const { return m_NotificationsList.size(); }
    size_t GetGCBacklogCount(void) const
    {
        CFastMutexGuard     guard(m_JobsToDeleteLock);
        return m_JobsToDelete.count();
    }

    const string& GetQueueName() const {
        return m_QueueName;
    }

    string DecorateJob(unsigned int    job_id) const
    { return m_QueueName + "/" + MakeJobKey(job_id); }

    // Submit job, return numeric job id
    unsigned int  Submit(const CNSClientId &        client,
                         CJob &                     job,
                         const string &             aff_token,
                         const string &             group,
                         bool                       logging,
                         CNSRollbackInterface * &   rollback_action);

    // Submit job batch
    // Returns ID of the first job, second is first_id+1 etc.
    unsigned SubmitBatch(const CNSClientId &             client,
                         vector< pair<CJob, string> > &  batch,
                         const string &                  group,
                         bool                            logging,
                         CNSRollbackInterface * &        rollback_action);

    TJobStatus  PutResult(const CNSClientId &     client,
                          const CNSPreciseTime &  curr,
                          unsigned int            job_id,
                          const string &          job_key,
                          CJob &                  job,
                          const string &          auth_token,
                          int                     ret_code,
                          const string &          output);

    bool GetJobOrWait(const CNSClientId &       client,
                      unsigned short            port, // Port the client
                                                      // will wait on
                      unsigned int              timeout,
                      const list<string> *      aff_list,
                      bool                      wnode_affinity,
                      bool                      any_affinity,
                      bool                      exclusive_new_affinity,
                      bool                      prioritized_aff,
                      bool                      new_format,
                      const list<string> *      group_list,
                      CJob *                    new_job,
                      CNSRollbackInterface * &  rollback_action,
                      string &                  added_pref_aff);

    void CancelWaitGet(const CNSClientId &  client);
    void CancelWaitRead(const CNSClientId &  client);

    list<string> ChangeAffinity(const CNSClientId &     client,
                                const list<string> &    aff_to_add,
                                const list<string> &    aff_to_del,
                                ECommandGroup           cmd_group);
    void SetAffinity(const CNSClientId &     client,
                     const list<string> &    aff,
                     ECommandGroup           cmd_group);
    int  SetClientData(const CNSClientId &  client,
                       const string &  data, int  data_version);

    TJobStatus  JobDelayExpiration(unsigned int            job_id,
                                   CJob &                  job,
                                   const CNSPreciseTime &  tm);
    TJobStatus  JobDelayReadExpiration(unsigned int            job_id,
                                       CJob &                  job,
                                       const CNSPreciseTime &  tm);

    TJobStatus  GetStatusAndLifetime(unsigned int      job_id,
                                     string &          client_ip,
                                     string &          client_sid,
                                     string &          client_phid,
                                     string &          progress_msg,
                                     CNSPreciseTime *  lifetime);
    TJobStatus  GetStatusAndLifetimeAndTouch(unsigned int      job_id,
                                             CJob &            job,
                                             CNSPreciseTime *  lifetime);

    TJobStatus  SetJobListener(unsigned int            job_id,
                               CJob &                  job,
                               unsigned int            address,
                               unsigned short          port,
                               const CNSPreciseTime &  timeout,
                               bool                    need_stolen,
                               bool                    need_progress_msg,
                               size_t *                last_event_index);

    // Worker node-specific methods
    bool PutProgressMessage(unsigned int    job_id,
                            CJob &          job,
                            const string &  msg);

    TJobStatus  ReturnJob(const CNSClientId &     client,
                          unsigned int            job_id,
                          const string &          job_key,
                          CJob &                  job,
                          const string &          auth_token,
                          string &                warning,
                          TJobReturnOption        how);
    TJobStatus  RescheduleJob(const CNSClientId &     client,
                              unsigned int            job_id,
                              const string &          job_key,
                              const string &          auth_token,
                              const string &          aff_token,
                              const string &          group,
                              bool &                  auth_token_ok,
                              CJob &                  job);
    TJobStatus  RedoJob(const CNSClientId &     client,
                        unsigned int            job_id,
                        const string &          job_key,
                        CJob &                  job);
    TJobStatus  ReadAndTouchJob(unsigned int      job_id,
                                CJob &            job,
                                CNSPreciseTime *  lifetime);

    // Cancel job execution (job stays in special Canceled state)
    // Returns the previous job status
    TJobStatus  Cancel(const CNSClientId &  client,
                       unsigned int         job_id,
                       const string &       job_key,
                       CJob &               job,
                       bool                 is_ns_rollback = false);

    unsigned int  CancelAllJobs(const CNSClientId &  client,
                                bool                 logging);
    unsigned int  CancelSelectedJobs(const CNSClientId &         client,
                                     const string &              group,
                                     const string &              aff_token,
                                     const vector<TJobStatus> &  statuses,
                                     bool                        logging,
                                     vector<string> &            warnings);

    TJobStatus GetJobStatus(unsigned job_id) const;

    bool IsEmpty() const;

    // get next job id (counter increment)
    unsigned int GetNextId();
    // Returns first id for the batch
    unsigned int GetNextJobIdForBatch(unsigned count);

    // Read-Confirm stage
    // Request done jobs for reading with timeout
    bool GetJobForReadingOrWait(const CNSClientId &       client,
                                unsigned int              port,
                                unsigned int              timeout,
                                const list<string> *      aff_list,
                                bool                      reader_affinity,
                                bool                      any_affinity,
                                bool                      exclusive_new_affinity,
                                bool                      prioritized_aff,
                                const list<string> *      group_list,
                                bool                      affinity_may_change,
                                bool                      group_may_change,
                                CJob *                    job,
                                bool *                    no_more_jobs,
                                CNSRollbackInterface * &  rollback_action,
                                string &                  added_pref_aff);
    // Confirm reading of these jobs
    TJobStatus  ConfirmReadingJob(const CNSClientId &   client,
                                  unsigned int          job_id,
                                  const string &        job_key,
                                  CJob &                job,
                                  const string &        auth_token);
    // Fail (negative acknowledge) reading of these jobs
    TJobStatus  FailReadingJob(const CNSClientId &   client,
                               unsigned int          job_id,
                               const string &        job_key,
                               CJob &                job,
                               const string &        auth_token,
                               const string &        err_msg,
                               bool                  no_retries);
    // Return jobs to unread state without reservation
    TJobStatus  ReturnReadingJob(const CNSClientId &   client,
                                 unsigned int          job_id,
                                 const string &        job_key,
                                 CJob &                job,
                                 const string &        auth_token,
                                 bool                  is_ns_rollback,
                                 bool                  blacklist,
                                 TJobStatus            target_status);

    TJobStatus  RereadJob(const CNSClientId &     client,
                          unsigned int            job_id,
                          const string &          job_key,
                          CJob &                  job,
                          bool &                  no_op);

    // Erase job from all structures, request delayed db deletion
    void EraseJob(unsigned job_id, TJobStatus  status);

    // Optimize bitvectors
    void OptimizeMem();

    TJobStatus FailJob(const CNSClientId &    client,
                       unsigned int           job_id,
                       const string &         job_key,
                       CJob &                 job,
                       const string &         auth_token,
                       const string &         err_msg,
                       const string &         output,
                       int                    ret_code,
                       bool                   no_retries,
                       string                 warning);

    string  GetAffinityTokenByID(unsigned int  aff_id) const;

    void ClearWorkerNode(const CNSClientId &  client,
                         bool &               client_was_found,
                         string &             old_session,
                         bool &               had_wn_pref_affs,
                         bool &               had_reader_pref_affs);

    void NotifyListenersPeriodically(const CNSPreciseTime &  current_time);
    CNSPreciseTime NotifyExactListeners(void);
    string PrintClientsList(bool verbose) const;
    string PrintNotificationsList(bool verbose) const;
    string PrintAffinitiesList(const CNSClientId &  client,
                               bool verbose) const;
    string PrintGroupsList(const CNSClientId &  client, bool verbose) const;
    string PrintScopesList(bool verbose) const;

    // Check execution timeout. Now checks reading timeout as well.
    // All jobs failed to execute, go back to pending
    void CheckExecutionTimeout(bool logging);

    // Checks up to given # of jobs at the given status for expiration and
    // marks up to given # of jobs for deletion. Check no further than the
    // given last_job id
    // Returns the # of performed scans, the # of jobs marked for deletion and
    // the last scanned job id.
    SPurgeAttributes CheckJobsExpiry(const CNSPreciseTime &  current_time,
                                     SPurgeAttributes        attributes,
                                     unsigned int            last_job,
                                     TJobStatus              status);

    void TimeLineMove(unsigned int            job_id,
                      const CNSPreciseTime &  old_time,
                      const CNSPreciseTime &  new_time);
    void TimeLineAdd(unsigned int            job_id,
                     const CNSPreciseTime &  job_time);
    void TimeLineRemove(unsigned int  job_id);
    void TimeLineExchange(unsigned int            remove_job_id,
                          unsigned int            add_job_id,
                          const CNSPreciseTime &  new_time);

    unsigned int  DeleteBatch(unsigned int  max_deleted);
    unsigned int  PurgeAffinities(void);
    unsigned int  PurgeGroups(void);
    void          StaleNodes(const CNSPreciseTime &  current_time);
    void          PurgeBlacklistedJobs(void);
    void          PurgeClientRegistry(const CNSPreciseTime &  current_time);

    // Dump a single job
    string PrintJobDbStat(const CNSClientId &  client,
                          unsigned int         job_id,
                          TDumpFields          dump_fields);
    // Dump all job records
    string PrintAllJobDbStat(const CNSClientId &         client,
                             const string &              group,
                             const string &              aff_token,
                             const vector<TJobStatus> &  job_statuses,
                             unsigned int                start_after_job_id,
                             unsigned int                count,
                             bool                        order_first,
                             TDumpFields                 dump_fields,
                             bool                        logging);

    unsigned CountStatus(TJobStatus) const;
    void StatusStatistics(TJobStatus                  status,
                          TNSBitVector::statistics *  st) const;

    string MakeJobKey(unsigned int  job_id) const;

    void TouchClientsRegistry(CNSClientId &  client,
                              bool &         client_was_found,
                              bool &         session_was_reset,
                              string &       old_session,
                              bool &         had_wn_pref_affs,
                              bool &         had_reader_pref_affs);
    void MarkClientAsAdmin(const CNSClientId &  client);
    void RegisterSocketWriteError(const CNSClientId &  client);
    void SetClientScope(const CNSClientId &  client);

    void PrintStatistics(size_t &  aff_count) const;
    void PrintJobCounters(void) const;
    unsigned int GetJobsToDeleteCount(void) const;
    string PrintTransitionCounters(void) const;
    string PrintJobsStat(const CNSClientId &  client,
                         const string &    group_token,
                         const string &    aff_token,
                         vector<string> &  warnings) const;
    void GetJobsPerState(const CNSClientId &  client,
                         const string &    group_token,
                         const string &    aff_token,
                         size_t *          jobs,
                         vector<string> &  warnings) const;
    void CountTransition(CNetScheduleAPI::EJobStatus  from,
                         CNetScheduleAPI::EJobStatus  to)
    { m_StatisticsCounters.CountTransition(from, to); }
    unsigned int  CountActiveJobs(void) const;
    unsigned int  CountAllJobs(void) const
    { return m_StatusTracker.Count(); }
    bool  AnyJobs(void) const
    { return m_StatusTracker.AnyJobs(); }

    TPauseStatus GetPauseStatus(void) const
    { return m_PauseStatus; }
    void SetPauseStatus(const CNSClientId &  client, TPauseStatus  status);

    // Used by NS to restore a stored status after restart
    void RestorePauseStatus(TPauseStatus  status)
    { m_PauseStatus = status; }

    void RegisterQueueResumeNotification(unsigned int  address,
                                         unsigned short  port,
                                         bool  new_format);

    void Dump(const string &  dump_dir_name);
    void RemoveDump(const string &  dump_dir_name);
    unsigned int LoadFromDump(const string &  dump_dir_name);
    bool ShouldPerfLogTransitions(void) const
    { return m_ShouldPerfLogTransitions; }
    void UpdatePerfLoggingSettings(const string &  qclass);

private:
    TJobStatus  x_ChangeReadingStatus(const CNSClientId &  client,
                                      unsigned int         job_id,
                                      const string &       job_key,
                                      CJob &               job,
                                      const string &       auth_token,
                                      const string &       err_msg,
                                      TJobStatus           target_status,
                                      bool                 is_ns_rollback,
                                      bool                 no_retries);

    struct x_SJobPick
    {
        unsigned int    job_id;
        bool            exclusive;
        unsigned int    aff_id;

        x_SJobPick() :
            job_id(0), exclusive(false), aff_id(0)
        {}
        x_SJobPick(unsigned int  jid, bool  excl, unsigned int  aid) :
            job_id(jid), exclusive(excl), aff_id(aid)
        {}
    };

    x_SJobPick
    x_FindVacantJob(const CNSClientId &           client,
                    const TNSBitVector &          explicit_affs,
                    const vector<unsigned int> &  aff_ids,
                    bool                          use_pref_affinity,
                    bool                          any_affinity,
                    bool                          exclusive_new_affinity,
                    bool                          prioritized_aff,
                    const TNSBitVector &          group_ids,
                    bool                          has_groups,
                    ECommandGroup                 cmd_group);
    x_SJobPick
    x_FindVacantJob(const CNSClientId &           client,
                    const TNSBitVector &          explicit_affs,
                    const vector<unsigned int> &  aff_ids,
                    bool                          use_pref_affinity,
                    bool                          any_affinity,
                    bool                          exclusive_new_affinity,
                    bool                          prioritized_aff,
                    const TNSBitVector &          group_ids,
                    bool                          has_groups,
                    ECommandGroup                 cmd_group,
                    const string &                scope);
    map<string, size_t> x_GetRunningJobsPerClientIP(void);
    bool x_ValidateMaxJobsPerClientIP(unsigned int  job_id,
                                      const map<string, size_t> &  jobs_per_client_ip) const;

    x_SJobPick
    x_FindOutdatedPendingJob(const CNSClientId &  client,
                             unsigned int         picked_earlier,
                             const TNSBitVector & group_ids);
    x_SJobPick
    x_FindOutdatedPendingJob(const CNSClientId &  client,
                             unsigned int         picked_earlier,
                             const TNSBitVector & group_ids,
                             const string &       scope);
    x_SJobPick
    x_FindOutdatedJobForReading(const CNSClientId &  client,
                                unsigned int         picked_earlier,
                                const TNSBitVector & group_ids);
    x_SJobPick
    x_FindOutdatedJobForReading(const CNSClientId &  client,
                                unsigned int         picked_earlier,
                                const TNSBitVector & group_ids,
                                const string &       scope);

    void x_UpdateDB_PutResultNoLock(unsigned                job_id,
                                    const string &          auth_token,
                                    const CNSPreciseTime &  curr,
                                    int                     ret_code,
                                    const string &          output,
                                    CJob &                  job,
                                    const CNSClientId &     client);

    void x_UpdateDB_ProvideJobNoLock(const CNSClientId &     client,
                                     const CNSPreciseTime &  curr,
                                     unsigned int            job_id,
                                     ECommandGroup           cmd_group,
                                     CJob &                  job);

    void x_CheckExecutionTimeout(const CNSPreciseTime &  queue_run_timeout,
                                 const CNSPreciseTime &  queue_read_timeout,
                                 unsigned                job_id,
                                 const CNSPreciseTime &  curr_time,
                                 bool                    logging);

    void x_LogSubmit(const CJob &  job);
    void x_ResetRunningDueToClear(const CNSClientId &   client,
                                  const TNSBitVector &  jobs);
    void x_ResetReadingDueToClear(const CNSClientId &   client,
                                  const TNSBitVector &  jobs);
    void x_ResetRunningDueToNewSession(const CNSClientId &   client,
                                       const TNSBitVector &  jobs);
    void x_ResetReadingDueToNewSession(const CNSClientId &   client,
                                       const TNSBitVector &  jobs);
    TJobStatus x_ResetDueTo(const CNSClientId &     client,
                            unsigned int            job_id,
                            const CNSPreciseTime &  current_time,
                            TJobStatus              status_from,
                            CJobEvent::EJobEvent    event_type);

    void x_RegisterGetListener(const CNSClientId &   client,
                               unsigned short        port,
                               unsigned int          timeout,
                               const TNSBitVector &  aff_ids,
                               bool                  wnode_aff,
                               bool                  any_aff,
                               bool                  exclusive_new_affinity,
                               bool                  new_format,
                               const TNSBitVector &  group_ids);
    void x_RegisterReadListener(const CNSClientId &   client,
                                unsigned short        port,
                                unsigned int          timeout,
                                const TNSBitVector &  aff_ids,
                                bool                  reader_aff,
                                bool                  any_aff,
                                bool                  exclusive_new_affinity,
                                const TNSBitVector &  group_ids);
    bool x_UnregisterGetListener(const CNSClientId &  client,
                                 unsigned short       port);

    /// Erase jobs from all structures, request delayed db deletion
    void x_Erase(const TNSBitVector &  job_ids,
                 TJobStatus  status);

    string x_DumpJobs(const TNSBitVector &   jobs_to_dump,
                      unsigned int           start_after_job_id,
                      unsigned int           count,
                      TDumpFields            dump_fields,
                      bool                   order_first);
    unsigned int x_CancelJobs(const CNSClientId &   client,
                              const TNSBitVector &  jobs_to_cancel,
                              bool                  logging);
    CNSPreciseTime x_GetEstimatedJobLifetime(unsigned int   job_id,
                                             TJobStatus     status) const;
    bool x_NoMoreReadJobs(const CNSClientId &   client,
                          const TNSBitVector &  aff_list,
                          bool                  reader_affinity,
                          bool                  any_affinity,
                          bool                  exclusive_new_affinity,
                          const TNSBitVector &  group_list,
                          bool                  affinity_may_change,
                          bool                  group_may_change);

    string x_GetJobsDumpFileName(const string &  dump_dname) const;
    void x_ClearQueue(void);
    void x_NotifyJobChanges(const CJob &            job,
                            const string &          job_key,
                            ENotificationReason     reason,
                            const CNSPreciseTime &  current_time);

private:
    friend class CJob;
    friend class CQueueParamAccessor;

    CNetScheduleServer *        m_Server;
    CJobStatusTracker           m_StatusTracker;    // status FSA
    CQueueDataBase &            m_QueueDB;

    map<unsigned int, CJob>     m_Jobs;             // in-memory jobs

    // Timeline object to control job execution timeout
    CJobTimeLine*               m_RunTimeLine;
    CRWLock                     m_RunTimeLineLock;

    string                      m_QueueName;
    TQueueKind                  m_Kind;            // 0 - static, 1 - dynamic

    // Lock for a queue operations
    mutable CFastMutex          m_OperationLock;

    // Registry of all the clients for the queue
    CNSClientsRegistry          m_ClientsRegistry;

    // Registry of all the job affinities
    CNSAffinityRegistry         m_AffinityRegistry;

    // Last valid id for queue
    unsigned int                m_LastId;      // Last used job ID
    unsigned int                m_SavedId;     // The ID we will start next time
                                               // the netschedule is loaded
    CFastMutex                  m_LastIdLock;

    // Lock for deleted jobs vectors
    mutable CFastMutex          m_JobsToDeleteLock;
    // Vector of jobs to be deleted from db unconditionally
    // keeps jobs still to be deleted from main DB
    TNSBitVector                m_JobsToDelete;
    unsigned int                m_JobsToDeleteOps;

    // Vector of jobs which are in a process of reading or had been already
    // read.
    TNSBitVector                m_ReadJobs;
    unsigned int                m_ReadJobsOps;

    // Configurable queue parameters
    mutable CFastMutex          m_ParamLock;
    CNSPreciseTime              m_Timeout;         // Result exp. timeout
    CNSPreciseTime              m_RunTimeout;      // Execution timeout
    CNSPreciseTime              m_ReadTimeout;
    // How many attempts to make on different nodes before failure
    unsigned                    m_FailedRetries;
    unsigned                    m_ReadFailedRetries;
    unsigned                    m_MaxJobsPerClient;
    CNSPreciseTime              m_BlacklistTime;
    CNSPreciseTime              m_ReadBlacklistTime;
    unsigned                    m_MaxInputSize;
    unsigned                    m_MaxOutputSize;
    CNSPreciseTime              m_WNodeTimeout;
    CNSPreciseTime              m_ReaderTimeout;
    CNSPreciseTime              m_PendingTimeout;
    CNSPreciseTime              m_MaxPendingWaitTimeout;
    CNSPreciseTime              m_MaxPendingReadWaitTimeout;
    // Client program version control
    CQueueClientInfoList        m_ProgramVersionList;
    // Host access list for job submission
    CNetScheduleAccessList      m_SubmHosts;
    // Host access list for job execution (workers)
    CNetScheduleAccessList      m_WnodeHosts;
    // Host access list for job reading (readers)
    CNetScheduleAccessList      m_ReaderHosts;

    CNetScheduleKeyGenerator    m_KeyGenerator;

    const bool &                m_Log;
    const bool &                m_LogBatchEachJob;

    bool                        m_RefuseSubmits;

    CStatisticsCounters         m_StatisticsCounters;
    mutable CStatisticsCounters m_StatisticsCountersLastPrinted;
    mutable CNSPreciseTime      m_StatisticsCountersLastPrintedTimestamp;

    // Notifications support
    CNSNotificationList         m_NotificationsList;
    CNSPreciseTime              m_NotifHifreqInterval;
    CNSPreciseTime              m_NotifHifreqPeriod;
    unsigned int                m_NotifLofreqMult;
    CNSPreciseTime              m_HandicapTimeout;

    unsigned int                m_DumpBufferSize;
    unsigned int                m_DumpClientBufferSize;
    unsigned int                m_DumpAffBufferSize;
    unsigned int                m_DumpGroupBufferSize;
    bool                        m_ScrambleJobKeys;
    map<string, string>         m_LinkedSections;

    // Group registry
    CNSGroupsRegistry           m_GroupRegistry;

    // Garbage collector registry
    CJobGCRegistry              m_GCRegistry;

    TPauseStatus                m_PauseStatus;

    // Client registry garbage collector settings
    CNSPreciseTime              m_ClientRegistryTimeoutWorkerNode;
    unsigned int                m_ClientRegistryMinWorkerNodes;
    CNSPreciseTime              m_ClientRegistryTimeoutAdmin;
    unsigned int                m_ClientRegistryMinAdmins;
    CNSPreciseTime              m_ClientRegistryTimeoutSubmitter;
    unsigned int                m_ClientRegistryMinSubmitters;
    CNSPreciseTime              m_ClientRegistryTimeoutReader;
    unsigned int                m_ClientRegistryMinReaders;
    CNSPreciseTime              m_ClientRegistryTimeoutUnknown;
    unsigned int                m_ClientRegistryMinUnknowns;

    CNSScopeRegistry            m_ScopeRegistry;

    bool                        m_ShouldPerfLogTransitions;

    // States from which the jobs could be taken for the READ[2] commands
    vector<CNetScheduleAPI::EJobStatus>
                                m_StatesForRead;
};


// Thread-safe parameter access. The majority of parameters are single word,
// so if you need a single parameter, it is safe to use these methods, which
// do not lock anything. In such cases, where the parameter is not single-word,
// we lock m_ParamLock for reading.
inline CNSPreciseTime CQueue::GetTimeout() const
{
    return m_Timeout;
}
inline CNSPreciseTime CQueue::GetRunTimeout() const
{
    return m_RunTimeout;
}
inline CNSPreciseTime CQueue::GetReadTimeout() const
{
    return m_ReadTimeout;
}
inline CNSPreciseTime CQueue::GetPendingTimeout() const
{
    return m_PendingTimeout;
}
inline CNSPreciseTime  CQueue::GetMaxPendingWaitTimeout() const
{
    return m_MaxPendingWaitTimeout;
}
inline unsigned CQueue::GetFailedRetries() const
{
    return m_FailedRetries;
}
inline bool CQueue::IsSubmitAllowed(unsigned host) const
{
    // The m_SubmHosts has internal lock anyway
    return host == 0  ||  m_SubmHosts.IsAllowed(host);
}
inline bool CQueue::IsWorkerAllowed(unsigned host) const
{
    // The m_WnodeHosts has internal lock anyway
    return host == 0  ||  m_WnodeHosts.IsAllowed(host);
}
inline bool CQueue::IsReaderAllowed(unsigned host) const
{
    // The m_ReaderHosts has internal lock anyway
    return host == 0  ||  m_ReaderHosts.IsAllowed(host);
}
inline bool CQueue::IsProgramAllowed(const string &  program_name) const
{
    if (!m_ProgramVersionList.IsConfigured())
        return true;    // No need to check

    if (program_name.empty())
        return false;

    try {
        CQueueClientInfo    auth_prog_info;

        ParseVersionString(program_name,
                           &auth_prog_info.client_name,
                           &auth_prog_info.version_info);
        return m_ProgramVersionList.IsMatchingClient(auth_prog_info);
    }
    catch (...) {
        // There could be parsing errors
        return false;
    }
}


END_NCBI_SCOPE

#endif /* NETSCHEDULE_NS_QUEUE__HPP */

