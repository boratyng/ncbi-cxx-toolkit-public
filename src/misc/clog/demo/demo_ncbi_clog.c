/* $Id$
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
 * Author:  Vladimir Ivanov
 *
 * File Description:  Demo application for NCBI C Logging (clog.lib).
 *
 */

#include <misc/clog/ncbi_c_log.h>

#include <stdlib.h>
#include <stdio.h>



/*****************************************************************************
 *  MT-locking
 */


/* Fake MT-lock handler -- for display purposes.
*/
/* #define NCBI_DEMO_MT_LOCK_HANDLER */

#if defined(NCBI_DEMO_MT_LOCK_HANDLER)
static int s_Test_MT_Handler(void* user_data, ENcbiLog_MTLock_Action action)
{
    /*
        Your alternative MT locking implementation goes here
    */
    return 1 /*true*/;
}
#endif



/*****************************************************************************
 *  MAIN
 */

int main(int argc, const char* argv[] /*, const char* envp[]*/)
{
    /* Initialize logging API 
    */
#if defined(NCBI_DEMO_MT_LOCK_HANDLER)
    TNcbiLog_MTLock mt_lock = NcbiLog_MTLock_Create(NULL, s_Test_MT_Handler);
    NcbiLog_Init(argv[0], mt_lock, eNcbiLog_MT_TakeOwnership);
#else
    NcbiLog_InitMT(argv[0]);
#endif
    /* Or,
       NcbiLog_InitMT(argv[0]); -- use default internal MT handler
       NcbiLog_InitST(argv[0]); -- only for single-threaded applications
    */
    
    /* Create separate files for log/err/trace/perf records.
       By default only single.log file will be created.
       Don't have any effect if logging is going to stdout/stderr.
       NcbiLog_SetSplitLogFile(1);
    */

    /* Set logging destination 
    */
    {{
        ENcbiLog_Destination ds;
        ds = NcbiLog_SetDestination(eNcbiLog_Stdout);
        /* Or,
           ds = NcbiLog_SetDestination(eNcbiLog_Default); -- default, can be skipped
           ds = NcbiLog_SetDestination(eNcbiLog_Stdlog);
           ds = NcbiLog_SetDestination(eNcbiLog_Stdout);
           ds = NcbiLog_SetDestination(eNcbiLog_Stderr);
           ds = NcbiLog_SetDestination(eNcbiLog_Cwd);
           ds = NcbiLog_SetDestination(eNcbiLog_Disable);
           ds = NcbiLog_SetDestinationFile("logfile_path");
        */
        /* Check on error, especially if you use "file" destination. 
           All other standard destination can fallback to stderr.
           But you can check here that it sets really where you want.
        */
        if (ds == eNcbiLog_Disable) {
            /* error */
            perror("Error initialize destination");
            return 1;
        }
    }}
        
    /* Set host name
       NcbiLog_SetHost("SOMEHOSTNAME");
    */

    /* Set process/thread ID
       NcbiLog_SetProcessId(pid);
       NcbiLog_SetThreadId(tid);
    */

    /* Set log_site
       NcbiLog_SetLogSite("");
    */

    /* Set app-wide client
       Use value from environment variables or "UNK_CLIENT" by default.
       NcbiLog_AppSetClient("192.168.1.0");
    */

    /* Set app-wide session
       Use value from environment variables or "UNK_SESSION" by default.
       NcbiLog_AppSetSession("APP-WIDE-SESSION"); -- set from string, or
       NcbiLog_AppNewSession(); -- generate new one
    */

    /* Set app-wide hit ID
       Use value from environment variables (if any) by default.
       NcbiLog_AppSetHitID("APP-HIT-ID");
    */

    /* Start application */
    NcbiLog_AppStart(argv);
    NcbiLog_AppRun();

    /* Standard messages */
    {{
        NcbiLog_SetPostLevel(eNcbiLog_Warning);
        NcbiLog_Trace("Message");
        NcbiLog_Warning("Message");
        NcbiLog_Error("Message");
        NcbiLog_Critical("Message");
        /* NcbiLog_Fatal("Message"); */
        NcbiLog_Note(eNcbiLog_Trace, "Note message");
        NcbiLog_Note(eNcbiLog_Warning, "Note message");
        NcbiLog_Note(eNcbiLog_Error, "Note message");
    }}

    /* Standard messages with user provided time */
    {{
        time_t timer;
        NcbiLog_SetPostLevel(eNcbiLog_Trace);
        timer = time(0);
        NcbiLog_SetTime(timer, 0);
        NcbiLog_Trace("Use user provided time (1)");
        NcbiLog_Trace("Use user provided time (2)");
        NcbiLog_SetTime(0,0);
        NcbiLog_Trace("Use system local time");
    }}

    /* Request without parameters */
    {{
        /* Set request hit ID if necessary
           NcbiLog_SetHitID("REQUEST-HIT-ID");
        */
        NcbiLog_ReqStart(NULL);
        /* Next call to NcbiLog_ReqRun() can be skipped, but still recommended 
           if you want to use full functionality
        */
        NcbiLog_ReqRun();
        /* 
           ... 
           request specific logging going here 
           ...
        */
        NcbiLog_ReqStop(200, 1, 2);
    }}

    /* Message printed between requests */
    {{
        NcbiLog_Error("Message printed between requests");
    }}

    /* Request without parameters -- new request ID */
    {{
        NcbiLog_SetRequestId(10);
        NcbiLog_ReqStart(NULL);
        /* ...  */
        NcbiLog_ReqStop(403, 5, 6);
    }}
    
    /* Request with parameters */
    {{
        static const SNcbiLog_Param params[] = {
            { "k1", "v1" },
            { "k2", "v2" },
            { "",   "v3" },
            { "k4", ""   },
            { "",   ""   },
            { "k5", "v5" },
            { NULL, NULL }
        };
        NcbiLog_SetSession("session name");
        NcbiLog_SetClient("192.168.1.1");
        NcbiLog_ReqStart(params);
        NcbiLog_ReqRun();
        /* ...  */
        NcbiLog_ReqStop(500, 3, 4);
    }}

    /* Extra & performance logging */
    {{
        double timespan = 1.2345678;
        static const SNcbiLog_Param params[] = {
            { "resource", "test" },
            { "key", "value" },
            { NULL, NULL }
        };
        NcbiLog_Extra(params);
        NcbiLog_Perf(200, timespan, params);
    }}
    
    /* Stop application with exit code 0
    */
    NcbiLog_AppStop(0);
    
    /* Deinitialize logging API
    */
    NcbiLog_Destroy();
    
    return 0;
}
