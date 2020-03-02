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
 * Author:  Colleen Bollin
 *
 * File Description:
 *   check biosource and structured comment descriptors against biosample database
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbistre.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbiutil.hpp>

#include <serial/serial.hpp>
#include <serial/objistr.hpp>
#include <serial/objectio.hpp>

#include <connect/ncbi_core_cxx.hpp>
#include <connect/ncbi_util.h>
#include <connect/ncbi_http_session.hpp>

// Objects includes
#include <objects/general/Object_id.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/PCRReactionSet.hpp>
#include <objects/seqfeat/PCRReaction.hpp>
#include <objects/seqfeat/PCRPrimer.hpp>
#include <objects/seqfeat/PCRPrimerSet.hpp>
#include <objects/seqfeat/PCRPrimerName.hpp>
#include <objects/seqfeat/PCRPrimerSeq.hpp>
#include <objects/seq/Pubdesc.hpp>
#include <objects/pub/Pub.hpp>
#include <objects/pub/Pub_equiv.hpp>
#include <objects/biblio/Cit_sub.hpp>
#include <objects/biblio/Cit_gen.hpp>
#include <objects/biblio/Auth_list.hpp>
#include <objects/biblio/Author.hpp>
#include <objects/biblio/Affil.hpp>
#include <objects/general/Person_id.hpp>
#include <objects/general/Name_std.hpp>
#include <objects/submit/Seq_submit.hpp>
#include <objects/submit/Submit_block.hpp>
#include <objects/submit/Contact_info.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objtools/cleanup/cleanup.hpp>
#include <objects/seqtable/SeqTable_multi_data.hpp>
#include <objects/seqtable/SeqTable_column_info.hpp>
#include <util/line_reader.hpp>
#include <util/compress/stream_util.hpp>
#include <util/format_guess.hpp>

#include <objects/seqset/Bioseq_set.hpp>

// Object Manager includes
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_descr_ci.hpp>
#include <objmgr/bioseq_handle.hpp>
#include <objmgr/bioseq_ci.hpp>
#include <objmgr/seqdesc_ci.hpp>

#include <objtools/data_loaders/genbank/gbloader.hpp>
#ifdef HAVE_NCBI_VDB
#  include <sra/data_loaders/wgs/wgsloader.hpp>
#endif
#include <misc/jsonwrapp/jsonwrapp.hpp>
#include <misc/xmlwrapp/xmlwrapp.hpp>


#include <misc/biosample_util/biosample_util.hpp>
#include <misc/biosample_util/struc_table_column.hpp>

#include <common/test_assert.h>  /* This header must go last */


using namespace ncbi;
using namespace objects;
using namespace xml;
using namespace biosample_util;

const char * BSDIFF_APP_VER = "1.0";

//  ----------------------------------------------------------------------------
void
PrintField(
    CNcbiOstream& ostr,
    int width,
    const string& value = "")
//  ----------------------------------------------------------------------------
{
    string printField = (value + string(width, ' ')).substr(0, width-1) + ' ';
    ostr << printField;
}


//  ----------------------------------------------------------------------------
void
PrintDiffList(
    const string& source,
    const TBiosampleFieldDiffList& diffList,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    if (diffList.empty()) {
        return;
    }
    bool headerDone = false;
    for (auto diff: diffList) {
        if (!headerDone) {
            PrintField(ostr, 20, source);
            PrintField(ostr, 40, diff->GetBioSample());
            PrintField(ostr, 40, diff->GetSequenceId());
            ostr << endl << string(100, '-') << endl;
            headerDone = true;
        }
        auto sampleVal(diff->GetSampleVal().empty() ? "---" : diff->GetSampleVal());
        auto sourceVal(diff->GetSrcVal().empty() ? "---" : diff->GetSrcVal());
        PrintField(ostr, 20, diff->GetFieldName());
        PrintField(ostr, 40, sampleVal);
        PrintField(ostr, 40, sourceVal);
        ostr << endl;
    }
    ostr << endl;
}

#define DEBUG_DATA false
//  ----------------------------------------------------------------------------
void
SaveBiosample(
    const string& filename,
    const CBioSource& bioSource)
//  -----------------------------------------------------------------------------
{
    if (!DEBUG_DATA) {
        return;
    }
    CNcbiOfstream ostr(filename);
    MSerial_Format_AsnText asnText;
    ostr << asnText << bioSource;
    ostr.close();
}

//  ----------------------------------------------------------------------------
CRef<CSeq_descr>
LoadBioSampleFromAcc(
    const string& bioSampleAcc)
//  ----------------------------------------------------------------------------
{
    CRef<CSeq_descr> pSeqDescrs = biosample_util::GetBiosampleData(
        bioSampleAcc, false, nullptr);
    return pSeqDescrs;
}


//  ----------------------------------------------------------------------------
void DumpBioSample(
    CNcbiOstream& ostr,
    const CSeq_descr& bioSample)
//  ----------------------------------------------------------------------------
{
    ostr << MSerial_Format_AsnText() << bioSample;
}


//  ----------------------------------------------------------------------------
void
GenerateDiffListFromDescriptors(
    const CSeq_descr& bioSample, 
    const CSeq_descr& pDescriptorSet,
    TBiosampleFieldDiffList& diffs)
//  ----------------------------------------------------------------------------
{
    diffs.clear();
    for (auto pSourceDesc: pDescriptorSet.Get()) {
        const CSeqdesc& sourceDesc = *pSourceDesc; 
        if (!sourceDesc.IsSource()) {
            continue;
        }
        return biosample_util::GenerateDiffListFromBioSource(
            bioSample, sourceDesc.GetSource(), diffs);
    }
}


//  ----------------------------------------------------------------------------
CRef<CSeq_descr> 
LoadBioSampleFromFile(
    const string& fileName)
//  ----------------------------------------------------------------------------
{
    auto_ptr<CNcbiIfstream> pInStr(new CNcbiIfstream(fileName.c_str(), ios::binary));
    CObjectIStream* pI = CObjectIStream::Open(
        eSerial_AsnText, *pInStr, eTakeOwnership);    
    CRef<CSeq_descr> pDescrs(new CSeq_descr);
    try {
        pI->Read(ObjectInfo(*pDescrs));
    }
    catch (CException&) {
        return pDescrs;
    }
    return pDescrs;
}


//  ----------------------------------------------------------------------------
void CompareBioSampleAccessionToDescriptors(
    const string& bioSampleAcc,
    CRef<CSeq_descr> pDescriptorSet)
//  ----------------------------------------------------------------------------
{
    CRef<CSeq_descr> pBioSample = LoadBioSampleFromAcc(bioSampleAcc);
    if (!pBioSample) {
        return;
    }
    {{
        CNcbiOfstream sampleOut("biosample.asn1");
        DumpBioSample(sampleOut, *pBioSample);
        sampleOut.close();
    }}
    TBiosampleFieldDiffList diffs;
    GenerateDiffListFromDescriptors(*pBioSample, *pDescriptorSet, diffs);
    PrintDiffList(bioSampleAcc, diffs, cout);
}

//  ----------------------------------------------------------------------------
void CompareBioSampleAccessionToBioSource(
    const string& bioSampleAcc,
    const CBioSource& bioSource)
//  ----------------------------------------------------------------------------
{
    //CRef<CSeq_descr> pBioSample = LoadBioSampleFromAcc(bioSampleAcc);
    TBiosampleFieldDiffList diffs;
    CBioSource sampleSource;
    
    if (biosample_util::GenerateDiffListFromBioSource(
        bioSampleAcc, bioSource, sampleSource, diffs)) {
        PrintDiffList(bioSampleAcc, diffs, cout);
    }
    
}


//  ----------------------------------------------------------------------------
void CompareBioSampleFileToDescriptors(
    const string& bioSampleFile,
    CRef<CSeq_descr> pDescriptorSet)
//  ----------------------------------------------------------------------------
{
    CRef<CSeq_descr> pBioSample = LoadBioSampleFromFile(bioSampleFile);
    TBiosampleFieldDiffList diffs;
    GenerateDiffListFromDescriptors(*pBioSample, *pDescriptorSet, diffs);
    PrintDiffList("Source", diffs, cout);
}


//  ----------------------------------------------------------------------------
CRef<CSeq_descr> 
LoadBioSource(
    const string& inFile)
//  ----------------------------------------------------------------------------
{
    auto_ptr<CNcbiIfstream> pInStr(new CNcbiIfstream(inFile.c_str(), ios::binary));
    CObjectIStream* pI = CObjectIStream::Open(
        eSerial_AsnText, *pInStr, eTakeOwnership);    
    CRef<CSeq_descr> pDescrs(new CSeq_descr);
    try {
        pI->Read(ObjectInfo(*pDescrs));
    }
    catch (CException&) {
        return pDescrs;
    }
    return pDescrs;
}


//  ============================================================================
class CBsDiffApp : public CNcbiApplication
//  ============================================================================
{
public:
    CBsDiffApp(void);

    virtual void Init(void);
    virtual int  Run (void);

private:
    void xCompareSeqEntryAccession(
        const string&);
    void xCompareSeqEntryFile(
        const string&);

    CRef<CSeq_entry> xLoadSeqEntry(
        const string&);

    CRef<CBioSource> xGetBioSource(
        CRef<CSeq_entry>);
    vector<string> xGetBioSampleAccs(
        CRef<CSeq_entry>);
        
    CRef<CObjectManager> mpObjmgr;
    CRef<CScope> mpScope;
};


//  ----------------------------------------------------------------------------
CBsDiffApp::CBsDiffApp(void)
//  ----------------------------------------------------------------------------
{
    CONNECT_Init(&GetConfig());
    mpObjmgr = CObjectManager::GetInstance();
    if (!mpObjmgr) {
        NCBI_THROW(CException, eUnknown, "Could not create object manager");
    }
    CGBDataLoader::RegisterInObjectManager(*mpObjmgr);
    mpScope.Reset(new CScope(*mpObjmgr));
    mpScope->AddDefaults();
}


//  ----------------------------------------------------------------------------
void CBsDiffApp::Init()
//  ----------------------------------------------------------------------------
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->AddDefaultKey(
        "biosource", 
        "biosource_asn1", 
        "BioSource info to compare", 
        CArgDescriptions::eString,
        "");

    arg_desc->AddDefaultKey(
        "biosample-acc",
        "biosample_accession",
        "biosample, retrieve online by accession",
        CArgDescriptions::eString,
        "");

    arg_desc->AddDefaultKey(
        "biosample-file",
        "biosample_local_filename",
        "biosample, retrieve locally from filename",
        CArgDescriptions::eString,
        "");

    arg_desc->AddDefaultKey(
        "seq-entry-acc",
        "seq_entry_accession",
        "seq-entry, to be used for both biosource and biosample",
        CArgDescriptions::eString,
        "");

    arg_desc->AddDefaultKey(
        "seq-entry-file",
        "seq_entry_accession_file",
        "seq_entry_file, containing multiple seq entry accessions",
        CArgDescriptions::eString,
        "");

    string prog_description = "BioSample Checker\n";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
        prog_description, false);

    // Pass argument descriptions to the application
    SetupArgDescriptions(arg_desc.release());

}


//  ----------------------------------------------------------------------------
int CBsDiffApp::Run()
//  ----------------------------------------------------------------------------
{
    const CArgs& args = GetArgs();

    string bioSampleAcc = args["biosample-acc"].AsString();
    string bioSampleFile = args["biosample-file"].AsString();
    string bioSourceFile = args["biosource"].AsString();
    string seqEntryAcc = args["seq-entry-acc"].AsString();
    string seqEntryFile = args["seq-entry-file"].AsString();

    if (seqEntryAcc.empty()  &&  seqEntryFile.empty()) {
        if (bioSampleAcc.empty()  &&  bioSampleFile.empty()) {
            cerr << "Bad arguments: Need to uniquely specify biosample." << endl;
            return 1;
        }
        if (!bioSampleAcc.empty()  &&  !bioSampleFile.empty()) {
            cerr << "Bad arguments: Need to uniquely specify biosample." << endl;
            return 1;
        }
        if (bioSourceFile.empty()) {
            cerr << "Bad arguments: Need to supply biosource." << endl;
            return 1;
        }
    }

    if (!seqEntryFile.empty()  &&  !seqEntryAcc.empty()) {
        auto otherStuff = bioSampleAcc + bioSampleFile + bioSourceFile;
        if (!otherStuff.empty()) {
            cerr << "Bad arguments: seq-entry-acc or seq-entry-file cannot go with anything else."
                 << endl;
            return 1;
        }
    }

    if (!seqEntryAcc.empty()) {
        xCompareSeqEntryAccession(seqEntryAcc);
        return 0;
    }

    if (!seqEntryFile.empty()) {
        xCompareSeqEntryFile(seqEntryFile);
        return 0;
    }

    CRef<CSeq_descr> pBioSource = LoadBioSource(bioSourceFile);
    if (!bioSampleAcc.empty()) {
        CompareBioSampleAccessionToDescriptors(bioSampleAcc, pBioSource);
        return 0;
    }
    if (!bioSampleFile.empty()) {
        CompareBioSampleFileToDescriptors(bioSampleFile, pBioSource);
        return 0;
    }
    cerr << "Internal error: utility completed without doing anything." << endl;
    return 1;
}


//  ----------------------------------------------------------------------------
CRef<CSeq_entry>
CBsDiffApp::xLoadSeqEntry(
    const string& accession)
//  ----------------------------------------------------------------------------
{
    try {
        mpScope->ResetDataAndHistory();
        CSeq_id_Handle idh = CSeq_id_Handle::GetHandle(accession);
        CBioseq_Handle bsh = mpScope->GetBioseqHandle(idh);
        CSeq_entry_Handle seh = bsh.GetTopLevelEntry();

        CRef<CSeq_entry> pSeqEntry(new CSeq_entry());
        pSeqEntry->Assign(*seh.GetCompleteSeq_entry());
        return pSeqEntry;
    }
    catch (CException&) {
    }
    return CRef<CSeq_entry>();
}


//  ----------------------------------------------------------------------------
void
CBsDiffApp::xCompareSeqEntryFile(
    const string& filename)
//  ----------------------------------------------------------------------------
{
    CNcbiIfstream ifstr(filename.c_str());
    string accession;
    int counter = 0;
    while (!ifstr.eof()) {
        std::getline(ifstr, accession);
        if (!accession.empty()) {
            ++counter;
            cout << "Processing accession \"" << accession << "\" (" 
                 << counter << ") ---" << endl << endl;
            xCompareSeqEntryAccession(accession);
        }
    }
}

//  ----------------------------------------------------------------------------
void
CBsDiffApp::xCompareSeqEntryAccession(
    const string& accession)
//  ----------------------------------------------------------------------------
{
    CRef<CSeq_entry> pSeqEntry;
    try {
        pSeqEntry = xLoadSeqEntry(accession);
    }
    catch (CException&) {
        cerr << "Loader: Unable to retrieve seq_entry \"" << accession
                << "\"." << endl;
        exit(1);
    }
    auto pBioSource = xGetBioSource(pSeqEntry);
    if (!pBioSource) {
        cerr << "Differ: Given sequence does not have a biosource." << endl;
        exit(1);
    }
    SaveBiosample("biosample.asn1", *pBioSource);
    auto bioSampleAccessions = xGetBioSampleAccs(pSeqEntry);
    if (bioSampleAccessions.empty()) {
        cerr << "Differ: Given sequence does not contain biosample links."
                << endl;
        exit(1);
    }
    CBioSource fusedSource;
    TBiosampleFieldDiffList diffs;
    for (auto bioSampleAcc: bioSampleAccessions) {
        if (biosample_util::GenerateDiffListFromBioSource(
                bioSampleAcc, *pBioSource, fusedSource, diffs)) {
            PrintDiffList(bioSampleAcc, diffs, cout);
            SaveBiosample("fusedsource.asn1", fusedSource);
        }      
    }
}


//  ----------------------------------------------------------------------------
CRef<CBioSource>
CBsDiffApp::xGetBioSource(
    CRef<CSeq_entry> pSeqEntry)
//  ----------------------------------------------------------------------------
{
    assert(pSeqEntry);
    CRef<CBioSource> pBioSource;
    auto descriptors = pSeqEntry->GetDescr().Get();
    for (auto descriptor: descriptors) {
        if (descriptor->IsSource()) {
            pBioSource.Reset(new CBioSource);
            pBioSource->Assign(descriptor->GetSource());
            return pBioSource;
        }
    }
    return pBioSource;
}


//  ----------------------------------------------------------------------------
vector<string>
CBsDiffApp::xGetBioSampleAccs(
    CRef<CSeq_entry> pSeqEntry)
//  ----------------------------------------------------------------------------
{
    assert(pSeqEntry);
    vector<string> bioSampleAccs;
    auto descriptors = pSeqEntry->GetDescr().Get();
    for (auto descriptor: descriptors) {
        if (!descriptor->IsUser()  ||  
                descriptor->GetUser().GetType().GetStr() != "DBLink") {
            continue;
        }
        auto descriptorData = descriptor->GetUser().GetData();
        for (auto entry: descriptorData) {
            if (!entry->CanGetLabel()  ||  entry->GetLabel().GetStr() != "BioSample") {
                continue;
            }
            if (!entry->CanGetData()) {
                continue;
            }
            auto& data = entry->GetData();
            if (data.IsStr()) {
                bioSampleAccs.push_back(data.GetStr());
                break;
            }
            if (data.IsStrs()) {
                bioSampleAccs.insert(
                    bioSampleAccs.end(), data.GetStrs().begin(), data.GetStrs().end());
                break;
            }
        }
    }
    return bioSampleAccs;
}


//  ============================================================================
int main(int argc, const char* argv[])
//  ============================================================================
{
    return CBsDiffApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
