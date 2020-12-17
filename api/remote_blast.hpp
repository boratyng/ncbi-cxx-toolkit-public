#ifndef ALGO_BLAST_API___REMOTE_BLAST__HPP
#define ALGO_BLAST_API___REMOTE_BLAST__HPP

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
 * Authors:  Kevin Bealer
 *
 */

/// @file remote_blast.hpp
/// Declares the CRemoteBlast class.

#include <algo/blast/api/blast_aux.hpp>
#include <algo/blast/api/blast_options_handle.hpp>
#include <algo/blast/api/uniform_search.hpp>
#include <objects/blast/blast__.hpp>
#include <objects/blast/names.hpp>
#include <util/format_guess.hpp> 

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
    /// forward declaration of ASN.1 object containing PSSM (scoremat.asn)
    class CPssmWithParameters;
    class CBioseq_set;
    class CSeq_loc;
    class CSeq_id;
    class CSeq_align_set;
END_SCOPE(objects)

BEGIN_SCOPE(blast)

struct SInteractingOptions;

/// Exception class for the CRemoteBlast class
class CRemoteBlastException : public CBlastException
{
public:
    /// Types of exceptions generated by the CRemoteBlast class
    enum EErrCode {
        eServiceNotAvailable,   ///< Service is not available
        eIncompleteConfig       ///< Remote BLAST object not fully configured
    };

    /// Translate from the error code value to its string representation.
    virtual const char* GetErrCodeString(void) const override {
        switch (GetErrCode()) {
        case eServiceNotAvailable: return "eServiceNotAvailable";
        case eIncompleteConfig: return "eIncompleteConfig";
        default:                return CException::GetErrCodeString();
        }
    }
#ifndef SKIP_DOXYGEN_PROCESSING
    NCBI_EXCEPTION_DEFAULT(CRemoteBlastException, CBlastException);
#endif /* SKIP_DOXYGEN_PROCESSING */
};


/// API for Remote Blast Requests
///
/// API Class to facilitate submission of Remote Blast requests.
/// Provides an interface to build a Remote Blast request given an
/// object of a subclass of CBlastOptionsHandle.

class NCBI_XBLAST_EXPORT CRemoteBlast : public CObject
{
public:
    /// Use the specified RID to get results for an existing search.
    CRemoteBlast(const string & RID);

    /// Uses the file to populate results.
    /// The file may be text or binary ASN.1 or XML. type is automatically detected.
    ///@param f istream to archive file
    CRemoteBlast(CNcbiIstream& f);

    /// Create a search using any kind of options handle.
    CRemoteBlast(CBlastOptionsHandle * any_opts);
    
    /// Create a sequence search and set options, queries, and database.
    /// @param queries Queries corresponding to Seq-loc-list or Bioseq-set.
    /// @param opts_handle Blast options handle.
    /// @param db Database used for this search.
    CRemoteBlast(CRef<IQueryFactory>         queries,
                 CRef<CBlastOptionsHandle>   opts_handle,
                 const CSearchDatabase     & db);
    
    /// Create a search and set options, queries, and subject sequences.
    /// @param queries Queries corresponding to Seq-loc-list or Bioseq-set.
    /// @param opts_handle Blast options handle.
    /// @param subjects Subject corresponding to Seq-loc-list or Bioseq-set.
    CRemoteBlast(CRef<IQueryFactory>       queries,
                 CRef<CBlastOptionsHandle> opts_handle,
                 CRef<IQueryFactory>       subjects);
    
    /// Create a PSSM search and set options, queries, and database.
    /// @param pssm Search matrix for a PSSM search.
    /// @param opts_handle Blast options handle.
    /// @param db Database used for this search.
    CRemoteBlast(CRef<objects::CPssmWithParameters>   pssm,
                 CRef<CBlastOptionsHandle>            opts_handle,
                 const CSearchDatabase              & db);
    
    /// Destruct the search object.
    ~CRemoteBlast();
    
    /// This restricts the subject database to this list of GIs (this is not
    /// supported yet on the server end).
    /// @param gi_list list of GIs to restrict the search to [in]
    void SetGIList(const list<TGi> & gi_list);

    /// This excludes the provided GIs from the subject database (this is not
    /// supported yet on the server end).
    /// @param gi_list list of GIs to exclude [in]
    void SetNegativeGIList(const list<TGi> & gi_list);
    
    /// Sets the filtering algorithm ID to be applied to the BLAST database
    /// (not supported by server yet)
    /// @param algo_id algorithm ID to use (ignored if -1)
    void SetDbFilteringAlgorithmId(int algo_id, ESubjectMaskingType mask_type=eSoftSubjMasking);

    /// Sets the filtering algorithm key to be applied to the BLAST database
    /// (not supported by server yet)
    /// @param algo_id algorithm ID to use (ignored if -1)
    void SetDbFilteringAlgorithmKey(string algo_key, ESubjectMaskingType mask_type=eSoftSubjMasking);

    ESubjectMaskingType GetSubjectMaskingType() const;

    /// Set the name of the database to search against.
    void SetDatabase(const string & x);
    
    /// Set a list of subject sequences to search against.
    void SetSubjectSequences(CRef<IQueryFactory> subj);
    
    /// Set a list of subject sequences to search against.
    void SetSubjectSequences(const list< CRef<objects::CBioseq> > & subj);
    
    /// Restrict search to sequences matching this Entrez query.
    void SetEntrezQuery(const char * x);
    
    /// Set the query as a Bioseq_set.
    void SetQueries(CRef<objects::CBioseq_set> bioseqs);

    /// Convert a TSeqLocInfoVector to a list< CRef<CBlast4_mask> > objects
    /// @param masking_locations Masks to convert [in]
    /// @param program CORE BLAST program type [in]
    /// @param warnings optional argument where warnings will be returned
    /// [in|out]
    static objects::CBlast4_get_search_results_reply::TMasks
    ConvertToRemoteMasks(const TSeqLocInfoVector& masking_locations,
                         EBlastProgramType program,
                         vector<string>* warnings = NULL);
    
    /// Set the query as a Bioseq_set along with the corresponding masking
    /// locations.
    /// @param bioseqs Query sequence data [in]
    /// @param masking_locations Masked regions for the queries above [in]
    void SetQueries(CRef<objects::CBioseq_set> bioseqs,
                    const TSeqLocInfoVector& masking_locations);
    
    /// Set the masking locations for queries.
    /// @param masking_locations Masked regions for the queries above [in]
    void SetQueryMasks(const TSeqLocInfoVector& masking_locations);

    /// Typedef for a list of Seq-locs
    typedef list< CRef<objects::CSeq_loc> > TSeqLocList;
    
    /// Set the query as a list of Seq_locs.
    /// @param seqlocs One interval Seq_loc or a list of whole Seq_locs.
    void SetQueries(TSeqLocList& seqlocs);
    
    /// Set the query as a list of Seq_locs.
    /// @param seqlocs One interval Seq_loc or a list of whole Seq_locs.
    /// @param masking_locations Masked regions for the queries above [in]
    void SetQueries(TSeqLocList& seqlocs,
                    const TSeqLocInfoVector& masking_locations);

    /// Set a PSSM query (as for PSI blast), which must include a bioseq set.
    void SetQueries(CRef<objects::CPssmWithParameters> pssm);
    
    
    /* Getting Results */
    
    
    /// Submit the search (if necessary) and return the results.
    /// @return Search results.
    CRef<CSearchResultSet> GetResultSet();
    
    /// This submits the search (if necessary) and polls for results.
    ///
    /// If a new search is configured, and not already submitted, this will
    /// submit it.  It then polls for results until either completion or error
    /// state is reached, or until the search times out.  The polling is done
    /// at an increasing interval, which starts out at 10 seconds, increasing
    /// by 30% after each check to a maximum of 300 seconds per sleep.
    ///
    /// @return true if the search was submitted, otherwise false.
    bool SubmitSync(void);
    
    /// This submits the search (if necessary) and polls for results.
    ///
    /// If a new search is configured, and not already submitted, this will
    /// submit it.  It then polls for results until either completion or error
    /// state is reached, or until the search times out.  The polling is done
    /// at an increasing interval, which starts out at 10 seconds, increasing
    /// by 30% after each check to a maximum of 300 seconds per sleep.
    ///
    /// @param timeout Search timeout specified as a number of seconds.
    /// @return true if the search was submitted, otherwise false.
    bool SubmitSync(int timeout);
    
    /// This submits the search (if necessary) and returns immediately.
    ///
    /// If a new search is configured, and not already submitted, this will
    /// submit it.  It then polls for results until either completion or error
    /// state is reached, or until the search times out.  The polling is done
    /// at an increasing interval, which starts out at 10 seconds, increasing
    /// by 30% after each check to a maximum of 300 seconds per sleep.
    ///
    /// @return true if the search was submitted, otherwise false.
    bool Submit(void);

    /// Represents the status of previously submitted search/RID
    enum ESearchStatus {
        /// Never submitted or purged from the system
        eStatus_Unknown,   
        /// Completed successfully
        eStatus_Done,      
        /// Not completed yet
        eStatus_Pending,   
        /// Completed but failed, call GetErrors/GetErrorVector()
        eStatus_Failed     
    };

    /// Returns the status of a previously submitted search/RID
    ESearchStatus CheckStatus();
    
    /// Check whether the search has completed.
    ///
    /// This checks the status of the search.  Please delay at least
    /// 10 seconds between subsequent calls.  If this function returns
    /// true, it will already have gotten the results as part of its
    /// processing.  With the common technique of polling with
    /// CheckDone before calling GetAlignments (or other results
    /// access operations), the first CheckDone call after results are
    /// available will perform the CPU, network, and memory intensive
    /// processing, and the GetAlignments() (for example) call will
    /// simply return a pointer to part of this data.
    /// @return true If search is not still running.
    bool CheckDone(void);
    
    /// This returns a string containing any errors that were produced by the
    /// search.  A successful search should return an empty string here.
    ///
    /// @return An empty string or a newline seperated list of errors.
    string GetErrors(void);
    
    /// This returns any warnings encountered.  These do not necessarily
    /// indicate an error or even a potential error; some warnings are always
    /// returned from certain types of searches.  (This is a debugging feature
    /// and warnings should probably not be returned to the end-user).
    ///
    /// @return Empty string or newline seperated list of warnings.
    string GetWarnings(void);
    
    /// This returns any warnings encountered as a vector of strings.
    /// @sa CRemoteBlast::GetWarnings
    /// @return Reference to a vector of warnings.
    const vector<string> & GetWarningVector();
    
    /// This returns any errors encountered as a vector of strings.
    /// @sa CRemoteBlast::GetErrors
    /// @return Reference to a vector of errors.
    const vector<string> & GetErrorVector();
    
    /* Getting Results */
    
    /// Gets the request id (RID) associated with the search.
    ///
    /// If the search was not successfully submitted, this will be empty.
    const string & GetRID(void);
    
    /// Get the seqalign set from the results.
    ///
    /// This method returns the alignment data as a seq align set.  If
    /// this search contains multiple queries, this method will return
    /// all data as a single set.  Most users will probably prefer to
    /// call GetSeqAlignSets() in this case.
    ///
    /// @return Reference to a seqalign set.
    CRef<objects::CSeq_align_set> GetAlignments(void);
    
    /// Get the seqalign vector from the results.
    ///
    /// This method returns the alignment data from the search as a
    /// TSeqAlignVector, which is a vector of CSeq_align_set objects.
    /// For multiple query searches, this method will normally return
    /// each alignment as a seperate element of the TSeqAlignVector.
    /// Note that in some cases, the TSeqAlignVector will not have an
    /// entry for some queries.  If the vector contains fewer
    /// alignments than there were queries, it may be necessary for
    /// the calling code to match queries to alignments by comparing
    /// Seq_ids.  This normally happens only if the same query is
    /// specified multiple times, or if one of the searches does not
    /// find any matches.
    ///
    /// @return A seqalign vector.
    ///
    /// @todo Separate the results for each query into discontinuous seq-aligns
    /// separated by the subject sequence. Also, using the upcoming feature of
    /// retrieving the query sequences, insert empty seqaligns into vector
    /// elements where there are no results for a given query (use
    /// x_CreateEmptySeq_align_set from blast_seqalign.cpp)
    TSeqAlignVector GetSeqAlignSets();
    
    /// Get the results of a PHI-Align request, if PHI pattern was set.
    /// @return Reference to PHI alignment set.
    CRef<objects::CBlast4_phi_alignments> GetPhiAlignments(void);
    
    /// Convenience typedef for a list of CRef<CBlast4_ka_block>
    typedef list< CRef<objects::CBlast4_ka_block > > TKarlinAltschulBlocks;

    /// Get the Karlin/Altschul parameter blocks produced by the search.
    /// @return List of references to KA blocks.
    TKarlinAltschulBlocks GetKABlocks(void);

    /// Get the queries' masked locations
    TSeqLocInfoVector GetMasks(void);
    
    /// Get the search statistics block as a list of strings.
    /// 
    /// Search statistics describe the data flow during each step of a BLAST
    /// search.  They are subject to change, and are not formally defined, but
    /// can sometimes provide insight when investigating software problems.
    /// 
    /// @return List of strings, each contains one line of search stats block.
    list< string > GetSearchStats(void);
    
    /// Get the PSSM produced by the search.
    /// @return Reference to a Score-matrix-parameters object.
    CRef<objects::CPssmWithParameters> GetPSSM(void);
    
    /// Debugging support can be turned on with eDebug or off with eSilent.
    enum EDebugMode {
        eDebug = 0,
        eSilent
    };
    
    /// Adjust the debugging level.
    ///
    /// This causes debugging trace data to be dumped to standard output,
    /// along with ASN.1 objects used during the search and other text.  It
    /// produces a great deal of output, none of which is expected to be
    /// useful to the end-user.
    void SetVerbose(EDebugMode verb = eDebug);

    /// Defines a std::vector of CRef<CSeq_interval>
    typedef vector< CRef<objects::CSeq_interval> > TSeqIntervalVector;
    /// Defines a std::vector of CRef<CSeq_data>
    typedef vector< CRef<objects::CSeq_data> > TSeqDataVector;

    /// Return values states whether GetDatabases or GetSubjectSequences
    /// call should be used.
    /// @return true indicates that GetDatabases should be used.
    bool IsDbSearch();

    /// Get the database used by the search.
    ///
    /// An object is returned, describing the name and type of
    /// database used as the subject of this search.
    ///
    /// @return An object describing the searched database(s).
    CRef<objects::CBlast4_database> GetDatabases();

    /// Returns subject sequences if "bl2seq" mode used.
    /// @return a list of bioseqs
    list< CRef<objects::CBioseq> > GetSubjectSequences();

    CBlast4_subject::TSeq_loc_list GetSubjectSeqLocs();

    /// Get the program used for this search.
    /// @return The value of the program parameter.
    string GetProgram();
    
    /// Get the service used for this search.
    /// @return The value of the service parameter.
    string GetService();
    
    /// Get the created-by string associated with this search.
    ///
    /// The created by string for this search will be returned.
    ///
    /// @return The value of the created-by string.
    string GetCreatedBy();
    
    /// Get the queries used for this search.
    ///
    /// The queries specified for this search will be returned.  The
    /// returned object will include either a list of seq-locs, a
    /// CBioseq, or a PSSM query.
    ///
    /// @return The queries used for this search.
    CRef<objects::CBlast4_queries> GetQueries();
    
    /// Get the search options used for this search.
    ///
    /// This returns the CBlastOptionsHandle for this search.  If this
    /// object was constructed with an RID, a CBlastOptionsHandle will
    /// be constructed from the search options stored on the remote
    /// server.  In this case the returned CBlastOptionsHandle will
    /// have a concrete type that corresponds to the program+service,
    /// and a locality of "eLocal".
    CRef<CBlastOptionsHandle> GetSearchOptions();

    /// Fetch the search strategy for this object without submitting the search
    CRef<objects::CBlast4_request> GetSearchStrategy();

    /// Returns the filtering algorithm ID used in the database
    Int4 GetDbFilteringAlgorithmId() const {
        return m_DbFilteringAlgorithmId;
    }

    /// Returns the filtering algorithm key used in the database
    string GetDbFilteringAlgorithmKey() const {
        return m_DbFilteringAlgorithmKey;
    }

    /// Returns the task used to create the remote search (if any)
    string GetTask() const {
        return m_Task;
    }

    /// Retrieves the client ID used by this object to send requests
    string GetClientId() const { return m_ClientId; }
    /// Sets the client ID used by this object to send requests
    void SetClientId(const string& client_id) { m_ClientId = client_id; }

    /// Loads next chunk of archive from file.
    bool LoadFromArchive();

    /// Get the title assigned for this search. 
    string GetTitle(void);

    /// Controls disk cache usage for results retrieval
    void EnableDiskCacheUse() { m_use_disk_cache = true;  }
    void DisableDiskCacheUse(){ m_use_disk_cache = false; }
    bool IsDiskCacheActive(void) { return m_use_disk_cache; } 
    /// disk cache error handling
    // m_disk_cache_error_flag
    bool IsDiskCacheError(void) { return m_disk_cache_error_flag; }
    void ClearDiskCacheError(void){ m_disk_cache_error_flag = false;}
    string GetDiskCacheErrorMessahe(void) { return m_disk_cache_error_msg; }
    // ask for search stats to check status.
    CRef<objects::CBlast4_reply> x_GetSearchStatsOnly(void);
    // actual code to get results using disk cache intermediate storage
    CRef<objects::CBlast4_reply> x_GetSearchResultsHTTP(void);

    // For Psiblast
    unsigned int GetPsiNumberOfIterations(void);

    bool IsErrMsgArchive(void);

    set<TTaxId> & GetTaxidList() { return m_TaxidList; };
    set<TTaxId> & GetNegativeTaxidList() { return m_NegativeTaxidList; };

private:

    bool x_HasRetrievedSubjects() const {
        return !m_SubjectSeqLocs.empty() || !m_SubjectSequences.empty();
    }

    /// Retrieve the request body for a search submission
    CRef<objects::CBlast4_request_body> x_GetBlast4SearchRequestBody();

    /// Sets a subset (only m_Dbs) of what the public SetDatabase sets.
    ///@param x name of database.
    void x_SetDatabase(const string & x);

    /// Set a list of subject sequences to search against (only
    /// m_SubjectSequences)
    ///@param subj subject bioseqs
    void x_SetSubjectSequences(const list< CRef<objects::CBioseq> > & subj);

    /// Value list.
    typedef list< CRef<objects::CBlast4_parameter> > TValueList;
    
    /// An alias for the most commonly used part of the Blast4 search results.
    typedef objects::CBlast4_get_search_results_reply TGSRR;

    /// Get the query masks from the results.
    /// @return list of references to Blast4_mask object.
    TGSRR::TMasks x_GetMasks(void);
    
    /// Various states the search can be in.
    ///
    /// eStart  Not submitted, can still be configured.
    /// eFailed An error was encountered.
    /// eWait   The search is still running.
    /// eDone   Results are available.
    enum EState {
        eStart = 0,
        eFailed,
        eWait,
        eDone
    };
    
    /// Indicates whether to use async mode.
    enum EImmediacy {
        ePollAsync = 0,
        ePollImmed
    };
    
    /// This class attempts to verify whether all necessary configuration is
    /// complete before attempting to submit the search.
    enum ENeedConfig {
        eNoConfig = 0x0,
        eProgram  = 0x1,
        eService  = 0x2,
        eQueries  = 0x4,
        eSubject  = 0x8,
        eNeedAll  = 0xF
    };
    
    /// The default timeout is 3.5 hours.
    int x_DefaultTimeout(void);

    /// Uses the file to populate results.
    /// The file may be text or binary ASN.1 or XML. type is automatically detected.
    ///@param f istream to archive file
    void x_Init(CNcbiIstream& f);
    
    /// Called by new search constructors: initialize a new search.
    void x_Init(CBlastOptionsHandle * algo_opts,
                const string        & program,
                const string        & service);
    
    /// Called by new search constructors: initialize a new search.
    void x_Init(CBlastOptionsHandle * algo_opts);
    
    /// Called by RID constructor: set up monitoring of existing search.
    void x_Init(const string & RID);
    
    /// Initialize a search with a database and options handle.
    /// @param opts_handle Blast options handle.
    /// @param db Database used for this search.
    void x_Init(CRef<CBlastOptionsHandle>   opts_handle,
                const CSearchDatabase     & db);
    
    /// Initialize queries based on a query factory.
    /// @param queries Query factory from which to pull queries.
    void x_InitQueries(CRef<IQueryFactory> queries);

    /// Initialize disk caching 
    void x_InitDiskCache(void);
    
    /// Configure new search from options handle passed to constructor.
    void x_SetAlgoOpts(void);
    
    /// Set an integer parameter (not used yet).
    /// @param field CBlast4Field object corresponding to option.
    /// @param value Pointer to integer value to use.
    void x_SetOneParam(objects::CBlast4Field & field, const int * value);
    
    /// Set a list of integers.
    /// @param field CBlast4Field object corresponding to option.
    /// @param value Pointer to list of integers to use.
    void x_SetOneParam(objects::CBlast4Field & field, const list<int> * value);
    
    /// Set a list of 8 byte integers.
    /// @param field CBlast4Field object corresponding to option.
    /// @param value Pointer to list of integers to use.
    void x_SetOneParam(objects::CBlast4Field & field, const list<Int8> * value);

    /// Set a string parameter.
    /// @param field CBlast4Field object corresponding to option.
    /// @param value Pointer to pointer to null delimited string.
    void x_SetOneParam(objects::CBlast4Field & field, const char ** value);

    /// Set a masking location for query
    /// @param field CBlast4Field object corresponding to option.
    /// @param mask masking location [in]
    void x_SetOneParam(objects::CBlast4Field & field, CRef<objects::CBlast4_mask> mask);
    
    /// Determine what state the search is in.
    EState x_GetState(void);
    
    /// Determine if this is an unknown RID.
    /// @note caller must have contacted the server for this method to work
    /// (e.g.: via CheckDone());
    bool x_IsUnknownRID(void);

    /// Poll until done, return the CBlast4_get_search_results_reply.
    /// @return Pointer to GSR reply object or NULL if search failed.
    TGSRR * x_GetGSRR(void);
    
    /// Send a Blast4 request and get a reply.
    /// @return The blast4 server response.
    CRef<objects::CBlast4_reply>
    x_SendRequest(CRef<objects::CBlast4_request_body> body);
    
    /// Try to get the search results.
    /// @return The blast4 server response.
    CRef<objects::CBlast4_reply>
    x_GetSearchResults(void);
    
    /// Verify that search object contains mandatory fields.
    void x_CheckConfig(void);
    
    /// Submit the search and process results (of submit action).
    void x_SubmitSearch(void);
    
    /// Try to get and process results.
    void x_CheckResults(void);
    
    /// Try to get and process results using disk cache.
    void x_CheckResultsDC(void);

    
    /// Iterate over error list, splitting into errors and warnings.
    void x_SearchErrors(CRef<objects::CBlast4_reply> reply);
    
    /// Poll until results are found, error occurs, or timeout expires.
    void x_PollUntilDone(EImmediacy poll_immed, int seconds);
    
    /// Fetch the request info (wait for completion if necessary).
    void x_GetRequestInfo();
    /// Fetch the requested info from splitd.
    void x_GetRequestInfoFromRID();
    /// Fetch the requested info from an archive file.
    void x_GetRequestInfoFromFile();

    /// Extract the query IDs from the CBlast4_queries for a given search
    /// @param query_ids the query IDs to be returned [in|out]
    void x_ExtractQueryIds(CSearchResultSet::TQueryIdVector& query_ids);
    
    /// Set the masking locations AFTER the queries have been set in the 
    /// m_QSR field
    void x_SetMaskingLocationsForQueries(const TSeqLocInfoVector&
                                         masking_locations);

    /// Extract the user specified masking locations from the query factory
    /// @note this method only extracts masking locations for
    /// CObjMgr_QueryFactory objects, for other types use the SetQueries method
    /// @param query_factory source of query sequence data [in]
    /// @param masks masking locations extracted [out]
    void
    x_ExtractUserSpecifiedMasks(CRef<IQueryFactory> query_factory, 
                                TSeqLocInfoVector& masks);

    /// Converts the provided query masking locations (if any) to the network
    /// representation following the BLAST 4 ASN.1 spec
    void x_QueryMaskingLocationsToNetwork();
    
    /// Add an algorithm or program option to the provided handle.
    void x_ProcessOneOption(CBlastOptionsHandle          & opts,
                            const string                 & nm,
                            const objects::CBlast4_value & v,
                            struct SInteractingOptions   & io);
    
    /// Add algorithm and program options to the provided handle.
    void x_ProcessOptions(CBlastOptionsHandle          & opts,
                          const TValueList             & L,
                          struct SInteractingOptions   & io);
    
    /// Adjust the EProgram based on option values.
    ///
    /// The blast4 protocol uses a notion of program and service to
    /// represent the type of search to do.  However, for some values
    /// of program and service, it is necessary to look at options
    /// values in order to determine the precise EProgram value.  This
    /// is particularly true when dealing with discontiguous megablast
    /// for example.
    ///
    /// @param L The list of options used for this search.
    /// @param pstr The program string used by the blast4 protocol.
    /// @param program The EProgram suggested by program+service.
    /// @return The EProgram value as adjusted by options.
    EProgram x_AdjustProgram(const TValueList & L,
                             const string     & pstr,
                             EProgram           program);

    // Retrieve the subject sequences using the get-search-info functionality
    void   x_GetSubjects(void);
    
    string x_GetStringFromSearchInfoReply(CRef<CBlast4_reply> reply,
                                          const string& name,
                                          const string& value);

    // Get psi iterations for an RID
    unsigned int x_GetPsiIterationsFromServer(void);
    /// Prohibit copy construction.
    CRemoteBlast(const CRemoteBlast &);
    
    /// Prohibit assignment.
    CRemoteBlast & operator=(const CRemoteBlast &);
    
    
    // Data
    
    /// Options for new search.
    CRef<blast::CBlastOptionsHandle>   m_CBOH;
    
    /// Request object for new search.
    CRef<objects::CBlast4_queue_search_request> m_QSR;
    
    /// Results of BLAST search.
    CRef<objects::CBlast4_reply>                m_Reply;
    
    /// Archive of BLAST search and results.
    CRef<objects::CBlast4_archive>                m_Archive;

    /// true if a CBlast4_archive should be read in.
    bool m_ReadFile;

    /// Use to ready CBlast4_archive
    unique_ptr<CObjectIStream> m_ObjectStream;

    /// Type of object CBlast4_archive as determined by CFormatGuess
    CFormatGuess::EFormat m_ObjectType;
    
    /// List of errors encountered.
    vector<string> m_Errs;
    
    /// List of warnings encountered.
    vector<string> m_Warn;

    /// Request ID of submitted or pre-existing search.
    string         m_RID;
    
    /// Count of server glitches (communication errors) to ignore.
    int         m_ErrIgn;
    
    /// Pending state: indicates that search still needs to be queued.
    bool        m_Pending;
    
    /// Verbosity mode: whether to produce debugging info on stdout.
    EDebugMode  m_Verbose;
    
    /// Bitfield to track whether all necessary configuration is done.
    ENeedConfig m_NeedConfig;
    
    
    // "Get request info" fields.
    
    /// Databases
    CRef<objects::CBlast4_database> m_Dbs;
    
    /// Subject Sequences
    list< CRef<objects::CBioseq> > m_SubjectSequences;
    /// This field is populated when dealing with a remote bl2seq search (e.g.:
    /// when formatting an RID using blast_formatter)
    CBlast4_subject::TSeq_loc_list m_SubjectSeqLocs;
    
    /// Program value used when submitting this search
    string m_Program;
    
    /// Service value used when submitting this search
    string m_Service;
    
    /// Created-by attribution for this search.
    string m_CreatedBy;
    
    /// Queries associated with this search.
    CRef<objects::CBlast4_queries> m_Queries;
    
    /// Options relevant to the search algorithm.
    CRef<objects::CBlast4_parameters> m_AlgoOpts;
    
    /// Options relevant to the search application.
    CRef<objects::CBlast4_parameters> m_ProgramOpts;

    /// Options relevant to the format application.
    CRef<objects::CBlast4_parameters> m_FormatOpts;

    /// Masking locations for queries.
    TSeqLocInfoVector m_QueryMaskingLocations;
    
    /// Entrez Query, if any.
    string m_EntrezQuery;
    
    /// First database sequence.
    string m_FirstDbSeq;
    
    /// Final database sequence.
    string m_FinalDbSeq;
    
    /// GI list.
    list<TGi> m_GiList;

    /// Negative GI list.
    list<TGi> m_NegativeGiList;

    /// filtering algorithm to use in the database
    int m_DbFilteringAlgorithmId;

    /// filtering algorithm key to use in the database
    string m_DbFilteringAlgorithmKey;

    ESubjectMaskingType m_SubjectMaskingType;

    /// Task used when the search was submitted (recovered via
    /// CBlastOptionsBuilder)
    string m_Task;

    /// Client ID submitting requests throw this interface
    string m_ClientId;

    /// Use disk cache for retrieving results
    /// default: false
    bool m_use_disk_cache;
    /// disk cache error flag
    bool m_disk_cache_error_flag;
    /// disk cache error message
    string m_disk_cache_error_msg;
    set<TTaxId> m_TaxidList;
    set<TTaxId> m_NegativeTaxidList;
};

/** Converts the return value of CSeqLocInfo::GetFrame into the
 * Blast4-frame-type field. Note that for non-translated queries, this value
 * should be set to notset (value = 0).
 * @param frame frame as specified by CSeqLocInfo::ETranslationFrame
 * @param program CORE BLAST program type
 */
NCBI_XBLAST_EXPORT
objects::EBlast4_frame_type
FrameNumber2NetworkFrame(int frame, EBlastProgramType program);


/** Converts Blast4-frame-type into CSeqLocInfo::ETranslationFrame 
 * @param frame frame as specified by objects::EBlast4_frame_type
 * @param program CORE BLAST program type
 */
NCBI_XBLAST_EXPORT
CSeqLocInfo::ETranslationFrame
NetworkFrame2FrameNumber(objects::EBlast4_frame_type frame, 
                         EBlastProgramType program);


/// Function to convert from program and service name into the CORE BLAST
/// program type
/// This is based on the values set in the various CBlastOptionsHandle 
/// subclasses (look for SetRemoteProgramAndService_Blast3 in include tree)
/// @note This function needs to be updated if the program/service name
/// combinations change
NCBI_XBLAST_EXPORT
EBlastProgramType
NetworkProgram2BlastProgramType(const string& program, const string& service);

/// Extract a Blast4-request (a.k.a.: a search strategy) from an input stream.
/// This function supports reading binary and text ASN.1 as well as XML.
/// A Blast4-get-search-strategy-reply will be attempted to read first (output
/// of BLAST SOAP server), then a Blast4-request (output of BLAST C++ command
/// line binaries).
/// @param in stream to read the data from [in]
/// @throws CException if input cannot be recognized
NCBI_XBLAST_EXPORT
CRef<objects::CBlast4_request> 
ExtractBlast4Request(CNcbiIstream& in);

void
FlattenBioseqSet(const CBioseq_set & bss, list< CRef<CBioseq> > & seqs);

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */

#endif  /* ALGO_BLAST_API___REMOTE_BLAST__HPP */
