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
* Author:  Aaron Ucko
*
* File Description:
*   test code for CScope::GetTitle
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.1  2002/01/11 19:06:28  gouriano
* restructured objmgr
*
* Revision 1.4  2001/12/07 19:39:19  butanaev
* Minor code improvements.
*
* Revision 1.3  2001/12/07 16:43:35  butanaev
* Switching to new reader interfaces.
*
* Revision 1.2  2001/10/17 20:51:54  ucko
* Pass flags to GetTitle.
*
* Revision 1.1  2001/10/12 21:20:37  ucko
* Add test for CScope::GetTitle
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>

#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>

#include <objects/objmgr/reader_id1.hpp>
#include <objects/objmgr/objmgr.hpp>

// (BEGIN_NCBI_SCOPE must be followed by END_NCBI_SCOPE later in this file)
BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

class CTitleTester : public CNcbiApplication
{
    virtual void Init(void);
    virtual int  Run(void);
};


void CTitleTester::Init(void)
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Show a sequence's title", false);

    arg_desc->AddKey("gi", "SeqEntryID", "GI id of the Seq-Entry to examine",
                     CArgDescriptions::eInteger);
    arg_desc->AddFlag("reconstruct", "Reconstruct title");
    arg_desc->AddFlag("accession", "Prepend accession");
    arg_desc->AddFlag("organism", "Append organism name");

    SetupArgDescriptions(arg_desc.release());
}


int CTitleTester::Run(void)
{
    const CArgs&   args = GetArgs();
    CObjectManager objmgr;
    CScope         scope = objmgr.CreateScope();
    CSeq_id        id;
    CId1Reader     reader;
    
    id.SetGi(args["gi"].AsInteger());

#if 0 // DataLoader != Loader
    objmgr.AddLoader(reader, &scope);
#else
    bool first = true;
    for(CIStream srs(reader.SeqrefStreamBuf(id)); ! srs.Eof(); first = false)
    {
      CSeqref *seqRef = reader.RetrieveSeqref(srs);
      if(first)
      {
        CBlobClass cl;
        for(CIStream bs(seqRef->BlobStreamBuf(0, 0, cl)); ! bs.Eof(); )
        {
          CId1Blob *blob = static_cast<CId1Blob *>(seqRef->RetrieveBlob(bs));
          objmgr.AddEntry(*blob->Seq_entry(), &scope);
          delete blob;
        }
      }
      delete seqRef;
    }
#endif

    CBioseqHandle          handle = scope.GetBioseqHandle(id);
    CScope::TGetTitleFlags flags  = 0;
    if (args["reconstruct"]) {
        flags |= CScope::fGetTitle_Reconstruct;
    }
    if (args["accession"]) {
        flags |= CScope::fGetTitle_Accession;
    }
    if (args["organism"]) {
        flags |= CScope::fGetTitle_Organism;
    }
    NcbiCout << scope.GetTitle(handle, flags) << NcbiEndl;
    return 0;
}


// (END_NCBI_SCOPE must be preceded by BEGIN_NCBI_SCOPE)
END_NCBI_SCOPE

USING_NCBI_SCOPE;

int main(int argc, const char** argv)
{
    return CTitleTester().AppMain(argc, argv);
}
