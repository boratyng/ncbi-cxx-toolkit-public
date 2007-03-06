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
 * Author:  Maxim Didenko
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistr.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/blob_storage.hpp>

#include <connect/services/ns_client_factory.hpp>
#include <connect/services/blob_storage_netcache.hpp>

#include <misc/grid_cgi/grid_cgiapp.hpp>


#include <vector>

BEGIN_NCBI_SCOPE

CGridCgiContext::CGridCgiContext(CHTMLPage& page, CCgiContext& ctx)
    : m_Page(page), m_CgiContext(ctx), m_NeedRenderPage(true)
{
    const CCgiRequest& req = ctx.GetRequest();
    string query_string = req.GetProperty(eCgi_QueryString);
    CCgiRequest::ParseEntries(query_string, m_ParsedQueryString);
}

CGridCgiContext::~CGridCgiContext()
{
}

string CGridCgiContext::GetSelfURL() const
{
    string url = m_CgiContext.GetSelfURL();
    bool first = true;
    TPersistedEntries::const_iterator it;
    for (it = m_PersistedEntries.begin(); 
         it != m_PersistedEntries.end(); ++it) {
        const string& name = it->first;
        const string& value = it->second;
        if (!name.empty() && !value.empty()) {
            if (first) {
                url += '?';
                first = false;
            }
            else
                url += '&';
            url += name + '=' + URL_EncodeString(value);
        }
    }
    return url;
}

string CGridCgiContext::GetHiddenFields() const
{
    string ret;
    TPersistedEntries::const_iterator it;
    for (it = m_PersistedEntries.begin(); 
         it != m_PersistedEntries.end(); ++it) {
        const string& name = it->first;
        const string& value = it->second;
        ret += "<INPUT TYPE=\"HIDDEN\" NAME=\"" + name 
             + "\" VALUE=\"" + value + "\">\n";
    }        
    return ret;
}

void CGridCgiContext::SetJobKey(const string& job_key)
{
    PersistEntry("job_key", job_key);

}
const string& CGridCgiContext::GetJobKey(void) const
{
    return GetEntryValue("job_key");
}

const string& CGridCgiContext::GetEntryValue(const string& entry_name) const
{
    TPersistedEntries::const_iterator it = m_PersistedEntries.find(entry_name);
    if (it != m_PersistedEntries.end())
        return it->second;
    return kEmptyStr;
}

void CGridCgiContext::PersistEntry(const string& entry_name)
{
    string value = kEmptyStr;
    ITERATE(TCgiEntries, eit, m_ParsedQueryString) {
        if (NStr::CompareNocase(entry_name, eit->first) == 0 ) {
            string v = eit->second;
            if (!v.empty())
                value = v;
        }
    }
    if (value.empty()) {
        const TCgiEntries entries = m_CgiContext.GetRequest().GetEntries();
        ITERATE(TCgiEntries, eit, entries) {
            if (NStr::CompareNocase(entry_name, eit->first) == 0 ) {
                string v = eit->second;
                if (!v.empty())
                    value = v;
            }
        }
    }
    PersistEntry(entry_name, value);
}
void CGridCgiContext::PersistEntry(const string& entry_name, 
                                   const string& value)
{   
    if (value.empty()) {
        TPersistedEntries::iterator it = 
              m_PersistedEntries.find(entry_name);
        if (it != m_PersistedEntries.end())
            m_PersistedEntries.erase(it);
    } else {
        m_PersistedEntries[entry_name] = value;
    }
}

void CGridCgiContext::Clear()
{
    m_PersistedEntries.clear();
}

void CGridCgiContext::SetCompleteResponse(CNcbiIstream& is)
{
    m_CgiContext.GetResponse().out() << is.rdbuf();
    m_NeedRenderPage = false;
}

/////////////////////////////////////////////////////////////////////////////
//  CGridCgiSampleApplication::
//


void CGridCgiApplication::Init()
{
    // Standard CGI framework initialization
    CCgiApplication::Init();
    InitGridClient();

}

void CGridCgiApplication::InitGridClient()
{
    // hack!!! It needs to be removed when we know how to deal with unresolved
    // symbols in plugins.
    BlobStorage_RegisterDriver_NetCache(); 
    m_RefreshDelay = 
        GetConfig().GetInt("grid_cgi", "refresh_delay", 5, IRegistry::eReturn);
    m_FirstDelay = 
        GetConfig().GetInt("grid_cgi", "expect_complete", 5, IRegistry::eReturn);
    if (m_FirstDelay > 10 ) m_FirstDelay = 10;
    if (m_FirstDelay < 0) m_FirstDelay = 0;

    bool automatic_cleanup = 
        GetConfig().GetBool("grid_cgi", "automatic_cleanup", true, IRegistry::eReturn);
    bool use_progress = 
        GetConfig().GetBool("grid_cgi", "use_progress", true, IRegistry::eReturn);

    if (!m_NSClient.get()) {
        CNetScheduleClientFactory cf(GetConfig());
        m_NSClient.reset(cf.CreateInstance());
        m_NSClient->SetProgramVersion(GetProgramVersion());
    }
    if( !m_NSStorage.get()) {
        CBlobStorageFactory cf(GetConfig());
        m_NSStorage.reset(cf.CreateInstance());
    }
    bool use_embedded_input = false;
    if (!GetConfig().Get(kNetScheduleAPIDriverName, "use_embedded_storage").empty())
        use_embedded_input = GetConfig().
            GetBool(kNetScheduleAPIDriverName, "use_embedded_storage", false, 0, 
                    CNcbiRegistry::eReturn);
    else
        use_embedded_input = GetConfig().
            GetBool(kNetScheduleAPIDriverName, "use_embedded_input", false, 0, 
                    CNcbiRegistry::eReturn);

    m_GridClient.reset(new CGridClient(m_NSClient->GetSubmitter(), *m_NSStorage,
                                       automatic_cleanup? 
                                            CGridClient::eAutomaticCleanup  :
                                            CGridClient::eManualCleanup,
                                       use_progress? 
                                            CGridClient::eProgressMsgOn :
                                            CGridClient::eProgressMsgOff,
                                       use_embedded_input));

}

void CGridCgiApplication::OnQueueIsBusy(CGridCgiContext& ctx)
{
    OnJobFailed("NetSchedule Queue is busy", ctx);
}

const string kGridCgiForm = "<FORM METHOD=\"GET\" ACTION=\"<@SELF_URL@>\">\n"
                            "<@HIDDEN_FIELDS@>\n<@STAT_VIEW@>\n"
                            "</FORM>";

int CGridCgiApplication::ProcessRequest(CCgiContext& ctx)
{
    // Given "CGI context", get access to its "HTTP request" and
    // "HTTP response" sub-objects
    //const CCgiRequest& request  = ctx.GetRequest();
    CCgiResponse& response = ctx.GetResponse();
    m_Response = &response;


    // Create a HTML page (using template HTML file "grid_cgi_sample.html")
    auto_ptr<CHTMLPage> page;
    try {
        page.reset(new CHTMLPage(GetPageTitle(), GetPageTemplate()));
        CHTMLText* stat_view = new CHTMLText(kGridCgiForm);
        page->AddTagMap("VIEW", stat_view);
    } catch (exception& e) {
        ERR_POST("Failed to create " << GetPageTitle()
                                     << " HTML page: " << e.what());
        return 2;
    }
    CGridCgiContext grid_ctx(*page, ctx);
    grid_ctx.PersistEntry("job_key");
    grid_ctx.PersistEntry("Cancel");
    string job_key = grid_ctx.GetEntryValue("job_key");
    try {
        try {
        OnBeginProcessRequest(grid_ctx);

        if (!job_key.empty()) {
        
            bool finished = x_CheckJobStatus(grid_ctx);
            if (x_JobStopRequested(grid_ctx)) 
                GetGridClient().CancelJob(job_key);

            if (finished) 
                grid_ctx.Clear();
            else
                RenderRefresh(*page, grid_ctx.GetSelfURL(), m_RefreshDelay);
        }        
        else {
            if (CollectParams(grid_ctx)) {
                bool finished = false;
                // Get a job submiter
                CGridJobSubmitter& job_submiter = GetGridClient().GetJobSubmitter();
                // Submit a job
                try {
                    PrepareJobData(job_submiter);
                    string job_key = job_submiter.Submit();
                    grid_ctx.SetJobKey(job_key);

                    unsigned long sleep_time = m_FirstDelay*1000;
                    const unsigned long interval = 500;
                    long count = sleep_time / interval;
                    finished = x_CheckJobStatus(grid_ctx);
                    for(; count > 0; --count) {
                        if (finished)
                            break;
                        SleepMilliSec(interval);
                        finished = x_CheckJobStatus(grid_ctx);
                    }
                    if( !finished ) {
                        OnJobSubmitted(grid_ctx);
                        RenderRefresh(*page, grid_ctx.GetSelfURL(), m_RefreshDelay);
                    }        
                } 
                catch (CNetScheduleException& ex) {
                    if (ex.GetErrCode() == 
                        CNetScheduleException::eTooManyPendingJobs)
                        OnQueueIsBusy(grid_ctx);
                    else
                        OnJobFailed(ex.what(), grid_ctx);
                    finished = true;
                }
                catch (exception& ex) {
                    OnJobFailed(ex.what(), grid_ctx);
                    finished = true;
                }
                if (finished)
                    grid_ctx.Clear();

            }
            else {
                ShowParamsPage(grid_ctx);
            }
        }
        } // try
        catch (/*CNetServiceException*/ exception& ex) {
            OnJobFailed(ex.what(), grid_ctx);
        }       
        CHTMLPlainText* self_url =
            new CHTMLPlainText(grid_ctx.GetSelfURL(),true);
        page->AddTagMap("SELF_URL", self_url);
        CHTMLPlainText* hidden_fields = 
            new CHTMLPlainText(grid_ctx.GetHiddenFields(),true);
        page->AddTagMap("HIDDEN_FIELDS", hidden_fields);

        OnEndProcessRequest(grid_ctx);
    } //try
    catch (exception& e) {
        ERR_POST("Failed to populate " << GetPageTitle() 
                                       << " HTML page: " << e.what());
        return 3;
    }

    if (!grid_ctx.NeedRenderPage())
        return 0;
    // Compose and flush the resultant HTML page
    try {
        response.WriteHeader();
        page->Print(response.out(), CNCBINode::eHTML);
    } catch (exception& e) {
        ERR_POST("Failed to compose/send " << GetPageTitle() 
                 <<" HTML page: " << e.what());
        return 4;
    }


    return 0;
}

bool CGridCgiApplication::x_JobStopRequested(const CGridCgiContext& ctx) const
{
    if (JobStopRequested())
        return true;
    if (!ctx.GetEntryValue("Cancel").empty()) 
        return true;
    return false;
}

void CGridCgiApplication::RenderRefresh(CHTMLPage& page,
                                        const string& url,
                                        int idelay)
{
    if (idelay >= 0) {
        CHTMLText* redirect = new CHTMLText(
                    "<META HTTP-EQUIV=Refresh " 
                    "CONTENT=\"<@REDIRECT_DELAY@>; URL=<@REDIRECT_URL@>\">");
        page.AddTagMap("REDIRECT", redirect);

        CHTMLPlainText* delay = new CHTMLPlainText(NStr::IntToString(idelay));
        page.AddTagMap("REDIRECT_DELAY",delay);               
    }

    CHTMLPlainText* h_url = new CHTMLPlainText(url,true);
    page.AddTagMap("REDIRECT_URL",h_url);               
    m_Response->SetHeaderValue("NCBI-RCGI-RetryURL", url);
}


bool CGridCgiApplication::x_CheckJobStatus(CGridCgiContext& grid_ctx)
{
    string job_key = grid_ctx.GetEntryValue("job_key");
    CGridJobStatus& job_status = GetGridClient().GetJobStatus(job_key);

    CNetScheduleAPI::EJobStatus status;
    status = job_status.GetStatus();
    grid_ctx.SetJobInput(job_status.GetJobInput());
    grid_ctx.SetJobOutput(job_status.GetJobOutput());
            
    bool finished = false;
    grid_ctx.GetCGIContext().GetResponse().
        SetHeaderValue("NCBI-RCGI-JobStatus", CNetScheduleAPI::StatusToString(status));
    switch (status) {
    case CNetScheduleAPI::eDone:
        // a job is done
        OnJobDone(job_status, grid_ctx);
        finished = true;
        break;
    case CNetScheduleAPI::eFailed:
        // a job has failed
        OnJobFailed(job_status.GetErrorMessage(), grid_ctx);
        finished = true;
        break;

    case CNetScheduleAPI::eCanceled :
        // A job has been canceled
        OnJobCanceled(grid_ctx);
        finished = true;
        break;
            
    case CNetScheduleAPI::eJobNotFound:
        // A lost job
        OnJobFailed("Job is not found.", grid_ctx);
        finished = true;
        break;
                
    case CNetScheduleAPI::ePending :
    case CNetScheduleAPI::eReturned:
        // A job is in the Netscheduler's Queue
        OnJobPending(grid_ctx);
        break;
        
    case CNetScheduleAPI::eRunning:
        // A job is being processed by a worker node
        grid_ctx.SetJobProgressMessage(job_status.GetProgressMessage());
        OnJobRunning(grid_ctx);
        break;
        
    default:
        _ASSERT(0);
    }
    return finished;
}



/////////////////////////////////////////////////////////////////////////////

END_NCBI_SCOPE
