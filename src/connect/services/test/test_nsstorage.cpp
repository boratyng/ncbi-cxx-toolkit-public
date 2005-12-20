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
 * Authors:  Maxim Didenko
 *
 * File Description:  NetSchedule Storage test
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbireg.hpp>

#include <connect/services/blob_storage_netcache.hpp>

#include <test/test_assert.h>  /* This header must go last */


USING_NCBI_SCOPE;

    
///////////////////////////////////////////////////////////////////////


/// Test application
///
/// @internal
///
class CTestNSStorage : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};



void CTestNSStorage::Init(void)
{
    // Setup command line arguments and parameters

    // Create command-line argument descriptions class
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // Specify USAGE context
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "NetSchedule storage");
    
    arg_desc->AddPositional("hostname", 
                            "NetCache host name.", CArgDescriptions::eString);

    arg_desc->AddPositional("port",
                            "Port number.", 
                            CArgDescriptions::eInteger);
    
    // Setup arg.descriptions for this application
    SetupArgDescriptions(arg_desc.release());

//    CONNECT_Init(&GetConfig());

    SetDiagPostLevel(eDiag_Info);
    SetDiagTrace(eDT_Enable);
}



int CTestNSStorage::Run(void)
{
    CArgs args = GetArgs();
    const string& host  = args["hostname"].AsString();
    unsigned short port = args["port"].AsInteger();

    auto_ptr<IBlobStorage> storage1(
                   new CBlobStorage_NetCache( 
                          new CNetCacheClient( host, port, "client1" )
                          )
                   );

    auto_ptr<IBlobStorage> storage2(
                   new CBlobStorage_NetCache( 
                          new CNetCacheClient( host, port, "client2" )
                          )
                   );

    string blobid;
    CNcbiOstream& os = storage1->CreateOStream(blobid);
    os << "Test_date";
    size_t blobsize;

    try {
        CNcbiIstream& is = storage2->GetIStream(blobid, &blobsize,
                               IBlobStorage::eLockNoWait );
    } catch( CBlobStorageException& ex ) {
        if( ex.GetErrCode() == CBlobStorageException::eBlocked ) {
            cout << "Blob : " << blobid << " is blocked" << endl;
        } else {
            throw;
        }
    }
    

    storage1.reset(0);

    cout << "Second attempt." << endl;

    CNcbiIstream& is = storage2->GetIStream(blobid, &blobsize);
    string res;
    is >> res;
    cout << res;

    return 0;
}


int main(int argc, const char* argv[])
{
    return CTestNSStorage().AppMain(argc, argv);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 6.3  2005/12/20 17:26:22  didenko
 * Reorganized netschedule storage facility.
 * renamed INetScheduleStorage to IBlobStorage and moved it to corelib
 * renamed INetScheduleStorageFactory to IBlobStorageFactory and moved it to corelib
 * renamed CNetScheduleNSStorage_NetCache to CBlobStorage_NetCache and moved it
 * to separate files
 * Moved CNetScheduleClientFactory to separate files
 *
 * Revision 6.2  2005/10/27 13:15:45  kuznets
 * Fixed bug with exception handling and memory management
 *
 * Revision 6.1  2005/10/26 16:37:44  didenko
 * Added for non-blocking read for netschedule storage
 *
 * ===========================================================================
 */
