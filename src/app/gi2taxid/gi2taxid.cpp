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
 * Authors:  Mike DiCuccio
 *
 * File Description:
 *
 */


#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbireg.hpp>

#include <objects/taxon1/taxon1.hpp>
#include <objects/id1/id1_client.hpp>

USING_SCOPE(ncbi);
USING_SCOPE(ncbi::objects);


class CGi2TaxIdApp : public CNcbiApplication
{
public:
    virtual void Init(void);
    virtual int  Run (void);

};


void CGi2TaxIdApp::Init()
{
    // Prepare command line descriptions
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->AddDefaultKey("gi", "GI",
                            "gi to test",
                            CArgDescriptions::eInteger,
                            "0");

    arg_desc->AddOptionalKey("file", "InputFile",
                             "Input file to test, one gi or accession per line",
                             CArgDescriptions::eInputFile);

    arg_desc->AddFlag("show_acc",
                      "Show the passed accession as well as the gi");

    // Pass argument descriptions to the application
    //
    SetupArgDescriptions(arg_desc.release());
}


int CGi2TaxIdApp::Run()
{
    CArgs args = GetArgs();

    bool show = args["show_acc"];

    vector<string> id_list;
    id_list.push_back("gi|" + NStr::IntToString(args["gi"].AsInteger()));

    if (args["file"]) {
        CNcbiIstream& istr = args["file"].AsInputFile();
        string acc;
        while (istr >> acc) {
            id_list.push_back(acc);
        }
    }

    CID1Client id1_client;
    CTaxon1 tax;
    tax.Init();

    ITERATE (vector<string>, iter, id_list) {
        if ( iter->empty() ) {
            LOG_POST(Info << "ignoring empty accession: ");
            continue;
        }

        // resolve the id to a gi
        int gi = 0;
        try {
            gi = NStr::StringToInt(*iter);
        }
        catch (...) {
            CSeq_id id(*iter);
            if ( id.Which() != CSeq_id::e_not_set) {
                gi = id1_client.AskGetgi(id);
            }
        }

        if (gi == 0) {
            LOG_POST(Error << "don't know anything about accession/id: "
                << *iter);
            continue;
        }

        int tax_id = 0;
        tax.GetTaxId4GI(gi, tax_id);

        if (show) {
            cout << *iter << " ";
        }
        cout << gi << " " << tax_id << endl;
    }

    return 0;
}


int main(int argc, const char* argv[])
{
    return CGi2TaxIdApp().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2004/05/21 21:41:40  gorelenk
 * Added PCH ncbi_pch.hpp
 *
 * Revision 1.1  2004/02/05 13:33:57  dicuccio
 * Moved from taxon1/demo - this now makes use of CID1Client
 *
 * Revision 1.1  2003/10/16 16:13:20  dicuccio
 * Initial revision
 *
 * ===========================================================================
 */
