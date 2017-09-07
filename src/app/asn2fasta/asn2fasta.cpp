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
* Author:  Aaron Ucko, Mati Shomrat, Jonathan Kans, NCBI
*
* File Description:
*   fasta-file generator application
*
* ===========================================================================
*/
#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <connect/ncbi_core_cxx.hpp>
#include <serial/serial.hpp>
#include <serial/objistr.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/bioseq_ci.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objtools/data_loaders/genbank/readers.hpp>

#include <objects/seqset/gb_release_file.hpp>
#include <objects/seqres/Seq_graph.hpp>
#include <objects/seqres/Byte_graph.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seq/seq_macros.hpp>

#include <objects/submit/Seq_submit.hpp>

#ifdef HAVE_NCBI_VDB
#  include <sra/data_loaders/wgs/wgsloader.hpp>
#endif

#include <objtools/cleanup/cleanup.hpp>

#include <objmgr/util/sequence.hpp>
#include <util/compress/zlib.hpp>
#include <util/compress/stream.hpp>
#include <dbapi/driver/drivers.hpp>

#include <objtools/writers/fasta_writer.hpp>


BEGIN_NCBI_SCOPE
USING_SCOPE(objects);



//  ==========================================================================
class CAsn2FastaApp:
    public CNcbiApplication, public CGBReleaseFile::ISeqEntryHandler
//  ==========================================================================
{
public:
    CAsn2FastaApp (void);
    ~CAsn2FastaApp (void);

    void Init(void);
    int  Run (void);

    bool HandleSeqEntry(CRef<CSeq_entry>& se);
    bool HandleSeqEntry(CSeq_entry_Handle& se);
    bool HandleSeqID( const string& seqID );

    CSeq_entry_Handle ObtainSeqEntryFromSeqEntry(CObjectIStream& is);
    CSeq_entry_Handle ObtainSeqEntryFromBioseq(CObjectIStream& is);
    CSeq_entry_Handle ObtainSeqEntryFromBioseqSet(CObjectIStream& is);
    CSeq_entry_Handle ObtainSeqEntryFromSeqSubmit(CObjectIStream& is);

    CFastaOstreamEx* OpenFastaOstream(const string& argname, const string& strname, bool use_stdout);

    void PrintQualityScores(const CBioseq& bioseq, CNcbiOstream& ostream);

private:
    CFastaOstreamEx* x_GetFastaOstream(CBioseq_Handle& handle);
    CObjectIStream* x_OpenIStream(const CArgs& args);
    bool x_IsOtherFeat(const CSeq_feat& feat) const;
    void x_WriteScoreHeader(const CBioseq& bioseq, CNcbiOstream& ostream) const;

    // data
    CRef<CObjectManager>        m_Objmgr;       // Object Manager
    CRef<CScope>                m_Scope;

    auto_ptr<CFastaOstreamEx>     m_Os;           // all sequence output stream

    auto_ptr<CFastaOstreamEx>     m_On;           // nucleotide output stream
    auto_ptr<CFastaOstreamEx>     m_Og;           // genomic output stream
    auto_ptr<CFastaOstreamEx>     m_Or;           // RNA output stream
    auto_ptr<CFastaOstreamEx>     m_Op;           // protein output stream
    auto_ptr<CFastaOstreamEx>     m_Ou;           // unknown output stream
    CNcbiOstream*               m_Oq;           // quality score output stream

    string                      m_OgHead;
    string                      m_OgTail;
    int                         m_OgIndex;
    TSeqPos                     m_OgMax;
    TSeqPos                     m_OgCurrLen;

    bool                        m_OnlyNucs;
    bool                        m_OnlyProts;
    bool                        m_DeflineOnly;
    bool                        m_do_cleanup;
    bool                        m_AllFeats;
    bool                        m_CDS;
    bool                        m_TranslateCDS;
    bool                        m_Gene;
    bool                        m_RNA;
    bool                        m_OtherFeats;
    CArgValue::TStringArray     m_FeatureSelection;
};

// constructor
CAsn2FastaApp::CAsn2FastaApp (void)
{
   SetVersionByBuild(1);
}

// destructor
CAsn2FastaApp::~CAsn2FastaApp (void)

{
}

//  --------------------------------------------------------------------------
void CAsn2FastaApp::Init(void)
//  --------------------------------------------------------------------------
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);
    arg_desc->SetUsageContext(
        GetArguments().GetProgramBasename(),
        "Convert an ASN.1 Seq-entry into a FASTA report",
        false);

    // input
    {{
        // name
        arg_desc->AddOptionalKey("i", "InputFile",
            "Input file name", CArgDescriptions::eInputFile);

        // input file serial format (AsnText\AsnBinary\XML, default: AsnText)
        arg_desc->AddOptionalKey("serial", "SerialFormat", "Input file format",
            CArgDescriptions::eString);
        arg_desc->SetConstraint("serial", &(*new CArgAllow_Strings,
            "text", "binary", "XML"));
        // id
        arg_desc->AddOptionalKey("id", "ID",
            "Specific ID to display", CArgDescriptions::eString);

        // input type:
        arg_desc->AddDefaultKey( "type", "AsnType", "ASN.1 object type",
            CArgDescriptions::eString, "any" );
        arg_desc->SetConstraint( "type",
            &( *new CArgAllow_Strings, "any", "seq-entry", "bioseq", "bioseq-set", "seq-submit" ) );
    }}


    // batch processing
    {{
        arg_desc->AddFlag("batch", "Process NCBI release file");
        // compression
        arg_desc->AddFlag("c", "Compressed file");
        // propogate top descriptors
        arg_desc->AddFlag("p", "Propogate top descriptors");

        arg_desc->AddFlag("defline-only",
                          "Only output the defline");
        string feat_desc = "Comma-separated list of feature types.\n";
        feat_desc += " Allowed types are:\n";
        feat_desc += "   fasta_cds_na,\n";
        feat_desc += "   fasta_cds_aa,\n";
        feat_desc += "   gene_fasta,\n";
        feat_desc += "   rna_fasta,\n";
        feat_desc += "   other_fasta,\n";
        feat_desc += "   all";
        arg_desc->AddOptionalKey("feats", "String", feat_desc, CArgDescriptions::eString);
        arg_desc->SetDependency("feats", CArgDescriptions::eExcludes,
                                "prots-only");
    }}

    // output
    {{
        arg_desc->AddFlag("show-mods", "Show FASTA header mods (e.g. [strain=abc])");

        arg_desc->AddOptionalKey("gap-mode", "GapMode", "\
 Gap mode:\n\
   letters:  letters will show gaps as runs of Ns (default mode)\n\
   count:    count shows gaps as '>?123'\n\
   dashes:   will show gaps as runs of dashes\n\
   one-dash: shows single gap literal as one das\n\
",
            CArgDescriptions::eString);
        arg_desc->SetConstraint("gap-mode", &(*new CArgAllow_Strings,
            "letters", "count", "dashes", "one-dash"));

        arg_desc->AddOptionalKey("width", "CHARS", "Output FASTA with an alternate number of columns", CArgDescriptions::eInteger);
        arg_desc->SetConstraint("width", new CArgAllow_Integers(1, kMax_Int));

        // single output name
        arg_desc->AddOptionalKey("o", "SingleOutputFile",
            "Single output file name", CArgDescriptions::eOutputFile);

        // filtering options for use with old style single output file:
        arg_desc->AddFlag("nucs-only", "Only emit nucleotide sequences");
        arg_desc->SetDependency("nucs-only", CArgDescriptions::eRequires, "o");

        arg_desc->AddFlag("prots-only", "Only emit protein sequences");
        arg_desc->SetDependency("prots-only", CArgDescriptions::eRequires, "o");
        arg_desc->SetDependency("prots-only", CArgDescriptions::eExcludes,
                                "nucs-only");

        // file names
        arg_desc->AddOptionalKey("on", "NucleotideOutputFile",
            "Nucleotide output file name", CArgDescriptions::eOutputFile);
        arg_desc->SetDependency("on", CArgDescriptions::eExcludes, "o");

        arg_desc->AddOptionalKey("og", "GenomicOutputFile",
            "Genomic output file name", CArgDescriptions::eOutputFile);
        arg_desc->SetDependency("og", CArgDescriptions::eExcludes, "o");
        arg_desc->SetDependency("og", CArgDescriptions::eExcludes, "on");

        arg_desc->AddOptionalKey("og_head", "GenomicOutputFileHead",
            "Genomic output file name stem",
            CArgDescriptions::eString);
        arg_desc->SetDependency("og_head", CArgDescriptions::eExcludes, "o");
        arg_desc->SetDependency("og_head", CArgDescriptions::eExcludes, "on");
        arg_desc->SetDependency("og_head", CArgDescriptions::eExcludes, "og");

        arg_desc->AddOptionalKey("og_tail", "GenomicOutputFileTail",
            "Genomic output file name suffix",
            CArgDescriptions::eString);
        arg_desc->SetDependency("og_tail", CArgDescriptions::eExcludes, "o");
        arg_desc->SetDependency("og_tail", CArgDescriptions::eExcludes, "on");
        arg_desc->SetDependency("og_tail", CArgDescriptions::eExcludes, "og");

        arg_desc->AddDefaultKey("x", "GenomeFileMaxSize",
                                "Maximum size of each genomic fasta file in Mb",
                                CArgDescriptions::eInteger, "1000");

        arg_desc->AddOptionalKey("or", "RNAOutputFile",
            "RNA output file name", CArgDescriptions::eOutputFile);
        arg_desc->SetDependency("or", CArgDescriptions::eExcludes, "o");
        arg_desc->SetDependency("or", CArgDescriptions::eExcludes, "on");

        arg_desc->AddOptionalKey("op", "ProteinOutputFile",
            "Protein output file name", CArgDescriptions::eOutputFile);
        arg_desc->SetDependency("op", CArgDescriptions::eExcludes, "o");

        arg_desc->AddOptionalKey("ou", "UnknownOutputFile",
            "Unknown output file name", CArgDescriptions::eOutputFile);
        arg_desc->SetDependency("ou", CArgDescriptions::eExcludes, "o");

        arg_desc->AddOptionalKey("oq", "QualityScoreOutputFile",
            "Quality score output file name", CArgDescriptions::eOutputFile);
        arg_desc->AddFlag("enable-gi", "Enable GI output in defline");

    }}

    // misc
    {{
         // cleanup
         arg_desc->AddFlag("cleanup",
                           "Do internal data cleanup prior to formatting");
     }}

    SetupArgDescriptions(arg_desc.release());
}



//  --------------------------------------------------------------------------
CFastaOstreamEx* CAsn2FastaApp::OpenFastaOstream(const string& argname, const string& strname, bool use_stdout)
//  --------------------------------------------------------------------------
{
    const CArgs& args = GetArgs();

    if (! use_stdout) {
        if (strname.empty()) {
            if (argname.empty()) return NULL;
            if (! args[argname]) return NULL;
        }
    }

    CNcbiOstream* os;
    if (use_stdout) {
        os = &NcbiCout;
    } else if ( !strname.empty() ) {
        // NB: This stream will leak unless explicitly managed.
        // (An app-wide smart pointer should suffice.)
        os = new CNcbiOfstream(strname.c_str());
    } else {
        if (args[argname]) {
            os = &args[argname].AsOutputFile();
        } else {
            return NULL;
        }
    }

    auto_ptr<CFastaOstreamEx> fasta_os(new CFastaOstreamEx(*os));
    fasta_os->SetAllFlags(        
        CFastaOstreamEx::fInstantiateGaps |
        CFastaOstreamEx::fAssembleParts |
        CFastaOstreamEx::fNoDupCheck |
        CFastaOstreamEx::fKeepGTSigns |
        CFastaOstreamEx::fNoExpensiveOps |
        CFastaOstreamEx::fHideGenBankPrefix );

    if (GetArgs()["enable-gi"])
    {
        fasta_os->SetFlag(CFastaOstreamEx::fEnableGI);
    }

    if( GetArgs()["gap-mode"] ) {
        fasta_os->SetFlag(
            CFastaOstreamEx::fInstantiateGaps);
        string gapmode = GetArgs()["gap-mode"].AsString();
        if ( gapmode == "one-dash" ) {
            fasta_os->SetGapMode(CFastaOstreamEx::eGM_one_dash);
        } else if ( gapmode == "dashes" ) {
            fasta_os->SetGapMode(CFastaOstreamEx::eGM_dashes);
        } else if ( gapmode == "letters" ) {
            fasta_os->SetGapMode(CFastaOstreamEx::eGM_letters);
        } else if ( gapmode == "count" ) {
            fasta_os->SetGapMode(CFastaOstreamEx::eGM_count);
        }
    }

    if( args["show-mods"] ) {
        fasta_os->SetFlag(CFastaOstreamEx::fShowModifiers);
    }
    if( args["width"] ) {
        fasta_os->SetWidth( GetArgs()["width"].AsInteger() );
    }

    m_do_cleanup = ( args["cleanup"]);

    return fasta_os.release();
}

//  --------------------------------------------------------------------------
int CAsn2FastaApp::Run(void)
//  --------------------------------------------------------------------------
{
    // initialize conn library
    CONNECT_Init(&GetConfig());
#ifdef HAVE_PUBSEQ_OS
    // we may require PubSeqOS readers at some point, so go ahead and make
    // sure they are properly registered
    GenBankReaders_Register_Pubseq();
    GenBankReaders_Register_Pubseq2();
    DBAPI_RegisterDriver_FTDS();
#endif

    const CArgs&   args = GetArgs();

    // create object manager
    m_Objmgr = CObjectManager::GetInstance();
    if ( !m_Objmgr ) {
        NCBI_THROW(CException, eUnknown,
                   "Could not create object manager");
    }
    CGBDataLoader::RegisterInObjectManager(*m_Objmgr);
#ifdef HAVE_NCBI_VDB
    CWGSDataLoader::RegisterInObjectManager(*m_Objmgr,
                                            CObjectManager::eDefault,
                                            88);
#endif
    m_Scope.Reset(new CScope(*m_Objmgr));
    m_Scope->AddDefaults();

    // open the output streams
    m_Os.reset( OpenFastaOstream ("o", "", false) );
    m_On.reset( OpenFastaOstream ("on", "", false) );
    m_Og.reset( OpenFastaOstream ("og", "", false) );
    m_Or.reset( OpenFastaOstream ("or", "", false) );
    m_Op.reset( OpenFastaOstream ("op", "", false) );
    m_Ou.reset( OpenFastaOstream ("ou", "", false) );
    m_Oq = args["oq"] ? &(args["oq"].AsOutputFile()) : NULL;

    m_OnlyNucs = args["nucs-only"];
    m_OnlyProts = args["prots-only"];

    m_OgHead = "";
    m_OgTail = "";
    m_OgIndex = 0;
    m_OgMax = 0;
    m_OgCurrLen = 0;
    if (args["og_head"] && args["og_tail"] && args["x"]) {
        m_OgHead = args["og_head"].AsString();
        m_OgTail = args["og_tail"].AsString();
        m_OgMax = TSeqPos (args["x"].AsInteger() * 1000000);
    }

    if (! m_OgHead.empty() && ! m_OgTail.empty()) {
        m_OgIndex++;
        string ogx = m_OgHead + NStr::IntToString(m_OgIndex) + m_OgTail;
        m_Og.reset( OpenFastaOstream ("", ogx, false) );
    }

    if (m_Os.get() == NULL  &&  m_On.get() == NULL  &&  m_Og.get() == NULL  &&
        m_Or.get() == NULL  &&  m_Op.get() == NULL  &&  m_Ou.get() == NULL  &&
        m_Oq == NULL) {
        // No output (-o*) argument given - default to stdout
        m_Os.reset( OpenFastaOstream ("", "", true) );
    }

    auto_ptr<CObjectIStream> is;
    is.reset( x_OpenIStream( args ) );
    if (is.get() == NULL) {
        string msg = args["i"]? "Unable to open input file" + args["i"].AsString() :
                        "Unable to read data from stdin";
        NCBI_THROW(CException, eUnknown, msg);
    }


    m_DeflineOnly = args["defline-only"];

    // Default is not to look at features
    m_AllFeats = m_CDS 
               = m_TranslateCDS 
               = m_Gene 
               = m_RNA 
               = m_OtherFeats
               = false;

    if ( args["feats"] ) {
        m_OnlyNucs = true; // feature only supported on nucleotide sequences

        list<string> feat_list;
        NStr::Split(args["feats"].AsString(),
                    ",",
                    feat_list,
                    NStr::fSplit_MergeDelimiters | NStr::fSplit_Truncate);

        if (feat_list.empty()) {
            m_AllFeats = true;
        } else{
            for(const auto feat_name : feat_list) {
                if (feat_name == "fasta_cds_na") {
                    m_CDS = true;
                } 
                else
                if (feat_name == "fasta_cds_aa") {
                    m_CDS = true;
                    m_TranslateCDS = true;
                } 
                else 
                if (feat_name == "gene_fasta") {
                    m_Gene = true;
                } 
                else 
                if (feat_name == "rna_fasta") {
                    m_RNA = true;
                } 
                else 
                if (feat_name == "other_fasta") {
                    m_OtherFeats = true;
                }
                else 
                if (feat_name == "all") {
                    m_AllFeats = true;
                }
                else {
                    NCBI_THROW(CException, eUnknown,
                        "Unrecognized feature type: "+feat_name );
                }
            }
        }
        m_CDS   |= m_AllFeats;
        m_Gene  |= m_AllFeats;
        m_RNA   |= m_AllFeats;
        m_OtherFeats |= m_AllFeats;
    }

    if ( args["batch"] ) {
        CGBReleaseFile in(*is.release());
        in.RegisterHandler(this);
        in.Read();  // HandleSeqEntry will be called from this function
    }
    else {

        if ( args["id"] ) {

            //
            //  Implies gbload; otherwise this feature would be pretty
            //  useless...
            //
            m_Scope->AddDefaults();
            string seqID = args["id"].AsString();
            HandleSeqID( seqID );

        } else {
            string asn_type = args["type"].AsString();
            CSeq_entry_Handle seh;

            if ( asn_type == "seq-entry" ) {
                //
                //  Straight through processing: Read a seq_entry, then process
                //  a seq_entry:
                //
                seh = ObtainSeqEntryFromSeqEntry(*is);
                while (seh) {
                    HandleSeqEntry(seh);

                    m_Scope->RemoveTopLevelSeqEntry(seh);
                    m_Scope->ResetHistory();
                    seh = ObtainSeqEntryFromSeqEntry(*is);
                }
                return 0;
            }
            else if ( asn_type == "bioseq" ) {
                //
                //  Read object as a bioseq, wrap it into a seq_entry, then
                //  process the wrapped bioseq as a seq_entry:
                //
                seh = ObtainSeqEntryFromBioseq(*is);
                if ( !seh ) {
                    NCBI_THROW(CException, eUnknown,
                               "Unable to construct Seq-entry object" );
                }
                HandleSeqEntry(seh);
            }
            else if ( asn_type == "bioseq-set" ) {
                //
                //  Read object as a bioseq_set, wrap it into a seq_entry, then
                //  process the wrapped bioseq_set as a seq_entry:
                //
                seh = ObtainSeqEntryFromBioseqSet(*is);
                if ( !seh ) {
                    NCBI_THROW(CException, eUnknown,
                               "Unable to construct Seq-entry object" );
                }
                HandleSeqEntry(seh);
            }
            else if ( asn_type == "seq-submit" ) {
                //
                //  Read object as a seq_submit, then process the first seq_entry:
                //
                seh = ObtainSeqEntryFromSeqSubmit(*is);
                if ( !seh ) {
                    NCBI_THROW(CException, eUnknown,
                               "Unable to construct Seq-entry object" );
                }
                HandleSeqEntry(seh);
            }
            else if ( asn_type == "any" ) {
                //
                //  Try the first three in turn:
                //

                set<TTypeInfo> known_types, matching_types;
                known_types.insert(CSeq_entry::GetTypeInfo());
                known_types.insert(CSeq_submit::GetTypeInfo());
                known_types.insert(CBioseq_set::GetTypeInfo());
                known_types.insert(CBioseq::GetTypeInfo());

                while (!is->EndOfData()) {
                    matching_types = is->GuessDataType(known_types);
                    if (matching_types.empty()) {
                        NCBI_THROW(CException, eUnknown,
                            "Unidentifiable object" );
                    } else if (matching_types.size() > 1) {
                        NCBI_THROW(CException, eUnknown,
                            "Ambiguous object" );
                    } else if (*matching_types.begin() == CSeq_entry::GetTypeInfo()) {
                        seh = ObtainSeqEntryFromSeqEntry(*is);
                    } else if (*matching_types.begin() == CSeq_submit::GetTypeInfo()) {
                        seh = ObtainSeqEntryFromSeqSubmit(*is);
                    } else if (*matching_types.begin() == CBioseq_set::GetTypeInfo()) {
                        seh = ObtainSeqEntryFromBioseqSet(*is);
                    } else if (*matching_types.begin() == CBioseq::GetTypeInfo()) {
                        seh = ObtainSeqEntryFromBioseq(*is);
                    }
                    if (!seh) {
                        NCBI_THROW(CException, eUnknown,
                            "Unable to construct Seq-entry object" );
                    }
                    HandleSeqEntry(seh);
                    m_Scope->RemoveTopLevelSeqEntry(seh);
                    m_Scope->ResetHistory();
                }
            }
        }
    }

    return 0;
}

//  --------------------------------------------------------------------------
CSeq_entry_Handle CAsn2FastaApp::ObtainSeqEntryFromSeqEntry(
    CObjectIStream& is)
//  --------------------------------------------------------------------------
{
    try {
        CRef<CSeq_entry> se(new CSeq_entry);
        is >> *se;
        if (se->Which() == CSeq_entry::e_not_set) {
            NCBI_THROW(CException, eUnknown,
                       "provided Seq-entry is empty");
        }
        return m_Scope->AddTopLevelSeqEntry(*se);
    }
    catch (CEofException&) { // ignore
    }
    catch (CException& e) {
        ERR_POST(Error << e);
    }
    return CSeq_entry_Handle();
}

//  --------------------------------------------------------------------------
CSeq_entry_Handle CAsn2FastaApp::ObtainSeqEntryFromBioseq(
    CObjectIStream& is)
//  --------------------------------------------------------------------------
{
    try {
        CRef<CBioseq> bs(new CBioseq);
        is >> *bs;
        CBioseq_Handle bsh = m_Scope->AddBioseq(*bs);
        return bsh.GetTopLevelEntry();
    }
    catch (CEofException&) { // ignore
    }
    catch (CException& e) {
        ERR_POST(Error << e);
    }
    return CSeq_entry_Handle();
}

//  --------------------------------------------------------------------------
CSeq_entry_Handle CAsn2FastaApp::ObtainSeqEntryFromBioseqSet(
    CObjectIStream& is)
//  --------------------------------------------------------------------------
{
    try {
        CRef<CSeq_entry> entry(new CSeq_entry);
        is >> entry->SetSet();
        return m_Scope->AddTopLevelSeqEntry(*entry);
    }
    catch (CEofException&) { // ignore
    }
    catch (CException& e) {
        ERR_POST(Error << e);
    }
    return CSeq_entry_Handle();
}

//  --------------------------------------------------------------------------
CSeq_entry_Handle CAsn2FastaApp::ObtainSeqEntryFromSeqSubmit(
    CObjectIStream& is)
//  --------------------------------------------------------------------------
{
    try {
        CRef<CSeq_entry> se(new CSeq_entry);
        CRef<CSeq_submit> sub(new CSeq_submit());
        is >> *sub;
        se.Reset( sub->SetData().SetEntrys().front() );
        return m_Scope->AddTopLevelSeqEntry(*se);
    }
    catch (CEofException&) { // ignore
    }
    catch (CException& e) {
        ERR_POST(Error << e);
    }
    return CSeq_entry_Handle();
}

//  --------------------------------------------------------------------------
bool CAsn2FastaApp::HandleSeqID( const string& seq_id )
//  --------------------------------------------------------------------------
{
    CSeq_id id(seq_id);
    CBioseq_Handle bsh = m_Scope->GetBioseqHandle( id );
    if ( ! bsh ) {
        ERR_FATAL("Unable to obtain data on ID \"" << seq_id << "\".");
    }

    //
    //  ... and use that to generate the flat file:
    //
    CSeq_entry_Handle seh = bsh.GetTopLevelEntry();
    return HandleSeqEntry(seh);
}

//  --------------------------------------------------------------------------
bool CAsn2FastaApp::HandleSeqEntry(CRef<CSeq_entry>& se)
//  --------------------------------------------------------------------------
{
    CSeq_entry_Handle seh = m_Scope->AddTopLevelSeqEntry(*se);
    bool ret = HandleSeqEntry(seh);
    m_Scope->RemoveTopLevelSeqEntry(seh);
    m_Scope->ResetHistory();
    return ret;
}

namespace {

bool s_GetMaxMin(const vector<char>& values, int& max, int& min) 
{
    if (values.empty()) {
        return false;
    }
    max = values[0];
    min = values[0];
    for (auto i=1; i<values.size(); ++i) {
        int current_value = static_cast<int>(values[i]);
        if (current_value > max) {
            max = current_value;
        } 
        else if(current_value < min) {
            min = current_value;
        }
    }
    return true;
}

};

//  --------------------------------------------------------------------------
void CAsn2FastaApp::x_WriteScoreHeader(const CBioseq& bioseq, CNcbiOstream& ostream) const
//  --------------------------------------------------------------------------
{
    const TSeqPos length = bioseq.IsSetLength() ? bioseq.GetLength() : 0;

    string header;
    if (!bioseq.IsSetAnnot()) {
        return;
    }

    string score_header;
    string graph_title;
    int min=256;
    int max=0;
    bool have_title = false;
    bool has_byte_graph = false;
    for (const CRef<CSeq_annot>& pAnnot : bioseq.GetAnnot()) {
        if (!pAnnot->IsGraph()) {
            continue;
        }

        for (const CRef<CSeq_graph>& pGraph : pAnnot->GetData().GetGraph()) {
            if (!have_title &&
                pGraph->IsSetTitle()) {
                graph_title = pGraph->GetTitle();
                have_title = true;
            }
            const auto& graph_data = pGraph->GetGraph();
            if (graph_data.Which() == CSeq_graph::TGraph::e_Byte) {
                has_byte_graph = true;

                const CByte_graph& byte_graph = graph_data.GetByte();

                int local_max;
                int local_min;

                if (s_GetMaxMin(byte_graph.GetValues(), local_max, local_min)) {
                    min = (local_min<min) ? local_min : min;
                    max = (local_max>max) ? local_max : max;
                }
            }
        }
    }

    if (!has_byte_graph) { // Nothing to do
        return;
    }

    score_header = graph_title;
    if (!NStr::IsBlank(score_header)) {
        score_header += " ";
    }

    if (length>0) {
        score_header += "(Length: ";
        score_header += NStr::IntToString(length);
        score_header += ", Min: ";
    }
    else {
        score_header += "(Min: "; 
    }
    score_header += NStr::IntToString(min);
    score_header += ", Max: ";
    score_header += NStr::IntToString(max);
    score_header += ")";

    unique_ptr<CFastaOstreamEx> fasta_os(new CFastaOstreamEx(ostream));

    fasta_os->SetAllFlags(        
        CFastaOstreamEx::fHideGenBankPrefix );

    if (GetArgs()["enable-gi"])
    {
        fasta_os->SetFlag(CFastaOstreamEx::fEnableGI);
    }

    fasta_os->WriteTitle(bioseq, 0, false, score_header);
}

static void s_Advance(int& column, CNcbiOstream& ostream, const int num_columns)
{
    if (column == num_columns) {
        ostream << '\n';
        column = 1;
        return;
    }
    ++column;
}

//  --------------------------------------------------------------------------
void CAsn2FastaApp::PrintQualityScores(const CBioseq& bioseq,
        CNcbiOstream& ostream) 
//  --------------------------------------------------------------------------
{
    TSeqPos current_pos=0;
    TSeqPos length=0;
    int column=1; 
    int num_columns=20;

    if (bioseq.GetLength()) {
        length = bioseq.GetLength();
    }
    bool has_graph = false;
    x_WriteScoreHeader(bioseq, ostream);

    if (bioseq.IsSetAnnot()) {
        for (CRef<CSeq_annot> pAnnot : bioseq.GetAnnot()) {
            if (!pAnnot->IsGraph()) {
                continue;
            }

            has_graph = true;

            for (CRef<CSeq_graph> pGraph : pAnnot->GetData().GetGraph()) {
                if (!pGraph->GetGraph().IsByte()) {
                    continue;
                }

                if (pGraph->IsSetLoc()) {
                    TSeqPos left = pGraph->GetLoc().GetStart(eExtreme_Positional);
                    while (current_pos < left) {
                        ostream << " -1";
                        s_Advance(column, ostream, num_columns);
                        ++current_pos;
                    }
                }

                const CByte_graph& byte_graph = pGraph->GetGraph().GetByte();
                if (byte_graph.IsSetValues()) {
                    for (char ch : byte_graph.GetValues()) {
                        ostream << " " << setw(2) << static_cast<int>(ch);
                        s_Advance(column, ostream, num_columns);
                        ++current_pos;
                    }
                }
            }
        }
    }
    
    if (has_graph) {
        while (current_pos < length) {
            ostream << " -1";
            s_Advance(column, ostream, num_columns);
            ++current_pos;
        }
    }

    if (column > 1) {
        ostream << '\n';
    }
}

/*
void CAsn2FastaApp::PrintQualityScores(const CBioseq& bsp, CNcbiOstream* out_stream)
{
    TSeqPos curpos = 0;
    TSeqPos len = 0;
    int j = 0;
    bool first = true;

    if (bsp.IsSetLength()) {
        len = bsp.GetLength();
    }

    x_WriteScoreHeader(bsp, *out_stream); 


    FOR_EACH_SEQANNOT_ON_BIOSEQ (sa_itr, bsp) {
        const CSeq_annot& annot = **sa_itr;
        if (! annot.IsGraph()) continue;
        FOR_EACH_SEQGRAPH_ON_SEQANNOT (gr_itr, annot) {
            const CSeq_graph& graph = **gr_itr;
            const CSeq_graph::TGraph& src_data = graph.GetGraph();

            if (first) { // Strange logic here
                first = false;
            }

            switch (src_data.Which()) {
                case CSeq_graph::TGraph::e_Byte:
                    {
                    if (graph.IsSetLoc()) {
                        const CSeq_loc& loc = graph.GetLoc();
                        TSeqPos left = loc.GetStart(eExtreme_Positional);
                        while (curpos < left) {
                            *out_stream << " -1";
                            if (j < 19) {
                                j++;
                            } else {
                                *out_stream << '\n';
                                j = 0;
                            }
                            curpos++;
                        }
                    }
                    const CByte_graph& byte_graph = src_data.GetByte();
                    if (byte_graph.IsSetValues()) {
                        const CByte_graph::TValues& bytes = byte_graph.GetValues();
                        for (auto i = 0; i < bytes.size(); i++) {
                            *out_stream << " ";
                            char ch = bytes [i];
                            *out_stream << setw(2) << static_cast<int>(ch);
                            if (j==19) {
                                j = 0;
                                *out_stream << '\n';
                            } 
                            else {
                                ++j;
                            }
                            ++curpos;
                        }
                    }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    if (! first) {
        while (curpos < len) {
            *out_stream << " -1";
            if (j < 19) {
                j++;
            } else {
                *out_stream << '\n';
                j = 0;
            }
            curpos++;
        }
    }
    if (j > 0) {
        *out_stream << '\n';
    }
}
*/

CFastaOstreamEx* CAsn2FastaApp::x_GetFastaOstream(CBioseq_Handle& bsh)
{
    CConstRef<CBioseq> bsr = bsh.GetCompleteBioseq();

    CFastaOstreamEx* fasta_os = NULL;

    bool is_genomic = false;
    bool is_RNA = false;

    CConstRef<CSeqdesc> closest_molinfo = bsr->GetClosestDescriptor(CSeqdesc::e_Molinfo);
    if (closest_molinfo) {
        const CMolInfo& molinf = closest_molinfo->GetMolinfo();
        CMolInfo::TBiomol biomol = molinf.GetBiomol();
        switch (biomol) {
        case NCBI_BIOMOL(genomic):
        case NCBI_BIOMOL(other_genetic):
        case NCBI_BIOMOL(genomic_mRNA):
        case NCBI_BIOMOL(cRNA):
        is_genomic = true;
        break;
        case NCBI_BIOMOL(pre_RNA):
        case NCBI_BIOMOL(mRNA):
        case NCBI_BIOMOL(rRNA):
        case NCBI_BIOMOL(tRNA):
        case NCBI_BIOMOL(snRNA):
        case NCBI_BIOMOL(scRNA):
        case NCBI_BIOMOL(snoRNA):
        case NCBI_BIOMOL(transcribed_RNA):
        case NCBI_BIOMOL(ncRNA):
        case NCBI_BIOMOL(tmRNA):
        is_RNA = true;
        break;
        case NCBI_BIOMOL(other):
            {
                CBioseq_Handle::TMol mol = bsh.GetSequenceType();
                switch (mol) {
                    case CSeq_inst::eMol_dna:
                        is_genomic = true;
                        break;
                    case CSeq_inst::eMol_rna:
                        is_RNA = true;
                        break;
                    case CSeq_inst::eMol_na:
                        is_genomic = true;
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
        }
    }

    if ( m_Oq != NULL && bsh.IsNa() ) {
        PrintQualityScores (*bsr, *m_Oq);
    }

    if ( m_Os.get() != NULL ) {
        if ( m_OnlyNucs && ! bsh.IsNa() ) return NULL;
        if ( m_OnlyProts && ! bsh.IsAa() ) return NULL;
        fasta_os = m_Os.get();
    } else if ( bsh.IsNa() ) {
        if ( m_On.get() != NULL ) {
            fasta_os = m_On.get();
        } else if ( (is_genomic || ! closest_molinfo) && m_Og.get() != NULL ) {
            fasta_os = m_Og.get();
            if (! m_OgHead.empty() && ! m_OgTail.empty()) {
                TSeqPos len = bsr->GetLength();
                if ( m_OgCurrLen > 0 && m_OgCurrLen + len > m_OgMax ) {
                    m_OgIndex++;
                    m_OgCurrLen = 0;
                    string ogx = m_OgHead + NStr::IntToString(m_OgIndex) + m_OgTail;
                    m_Og.reset( OpenFastaOstream ("", ogx, false) );
                    fasta_os = m_Og.get();
                }
                m_OgCurrLen += len;
            }
        } else if ( is_RNA && m_Or.get() != NULL ) {
            fasta_os = m_Or.get();
        } else {
            return NULL;
        }
    } else if ( bsh.IsAa() ) {
        if ( m_Op.get() != NULL ) {
            fasta_os = m_Op.get();
        }
    } else {
        if ( m_Ou.get() != NULL ) {
            fasta_os = m_Ou.get();
        } else if ( m_On.get() != NULL ) {
            fasta_os = m_On.get();
        } else {
            return NULL;
        }
    }

    return fasta_os;
}


//  --------------------------------------------------------------------------
bool CAsn2FastaApp::x_IsOtherFeat(const CSeq_feat& feat) const
//  --------------------------------------------------------------------------
{
    return !(feat.GetData().IsCdregion() ||
            feat.GetData().IsGene() ||
            feat.GetData().IsRna());
}

//  --------------------------------------------------------------------------
bool CAsn2FastaApp::HandleSeqEntry(CSeq_entry_Handle& seh)
//  --------------------------------------------------------------------------
{

    if ( m_do_cleanup ) {
        CSeq_entry_EditHandle tseh = seh.GetTopLevelEntry().GetEditHandle();
        CBioseq_set_EditHandle bseth;
        CBioseq_EditHandle bseqh;
        CRef<CSeq_entry> tmp_se(new CSeq_entry);
        if ( tseh.IsSet() ) {
            bseth = tseh.SetSet();
            CConstRef<CBioseq_set> bset = bseth.GetCompleteObject();
            bseth.Remove(bseth.eKeepSeq_entry);
            tmp_se->SetSet(const_cast<CBioseq_set&>(*bset));
        }
        else {
            bseqh = tseh.SetSeq();
            CConstRef<CBioseq> bseq = bseqh.GetCompleteObject();
            bseqh.Remove(bseqh.eKeepSeq_entry);
            tmp_se->SetSeq(const_cast<CBioseq&>(*bseq));
        }

        CCleanup cleanup;
        cleanup.BasicCleanup( *tmp_se );

        if ( tmp_se->IsSet() ) {
            tseh.SelectSet(bseth);
        }
        else {
            tseh.SelectSeq(bseqh);
        }
    }

    bool any_feats = m_AllFeats
                   | m_CDS 
                   | m_TranslateCDS
                   | m_Gene 
                   | m_RNA 
                   | m_OtherFeats;


    if(!any_feats) {
        for (CBioseq_CI bioseq_it(seh);  bioseq_it;  ++bioseq_it) {
            CBioseq_Handle bsh = *bioseq_it;
            if (!bsh)
                continue;
            CFastaOstreamEx* fasta_os = x_GetFastaOstream(bsh);

            if ( fasta_os == NULL) continue;

            if ( m_DeflineOnly ) {
                fasta_os->WriteTitle(bsh);
            } else {
                fasta_os->Write(bsh);
            }
        }
        return true;
    }

    // Iterate over features
    SAnnotSelector sel;
    sel.SetAdaptiveDepth(true);
    sel.SetResolveAll();
    sel.SetSortOrder(SAnnotSelector::eSortOrder_Normal);
    sel.SetResolveDepth(100);
    sel.ExcludeNamedAnnots("SNP");

    auto& scope = seh.GetScope();

    for (CFeat_CI feat_it(seh, sel); feat_it; ++feat_it) {

        const CSeq_feat& feat = feat_it->GetOriginalFeature();
        auto bsh = scope.GetBioseqHandle(feat.GetLocation());
        CFastaOstreamEx* fasta_os = x_GetFastaOstream(bsh);

        if (feat.IsSetData() &&
            feat.GetData().IsCdregion()) {
        }

        if (!feat.IsSetData() ||
            (feat.GetData().IsCdregion() && !m_CDS) ||
            (feat.GetData().IsGene() && !m_Gene) ||
            (feat.GetData().IsRna() && !m_RNA) ||
            (x_IsOtherFeat(feat) && !m_OtherFeats)) {
                continue;
        }
        if ( m_DeflineOnly ) {
            fasta_os->WriteFeatureTitle(feat, scope, m_TranslateCDS);
        } else {
            fasta_os->WriteFeature(feat, scope, m_TranslateCDS);
        }
    }
    return true;
}

//  --------------------------------------------------------------------------
CObjectIStream* CAsn2FastaApp::x_OpenIStream(const CArgs& args)
//  --------------------------------------------------------------------------
{

    // determine the file serialization format.
    // default for batch files is binary, otherwise text.
    ESerialDataFormat serial = args["batch"] ? eSerial_AsnBinary :eSerial_AsnText;
    if ( args["serial"] ) {
        const string& val = args["serial"].AsString();
        if ( val == "text" ) {
            serial = eSerial_AsnText;
        } else if ( val == "binary" ) {
            serial = eSerial_AsnBinary;
        } else if ( val == "XML" ) {
            serial = eSerial_Xml;
        }
    }

    // make sure of the underlying input stream. If -i was given on the command line
    // then the input comes from a file. Otherwise, it comes from stdin:
    CNcbiIstream* pInputStream = &NcbiCin;
    bool bDeleteOnClose = false;
    if ( args["i"] ) {
        pInputStream = new CNcbiIfstream( args["i"].AsString().c_str(), ios::binary  );
        bDeleteOnClose = true;
    }

    // if -c was specified then wrap the input stream into a gzip decompressor before
    // turning it into an object stream:
    CObjectIStream* pI = NULL;
    if ( args["c"] ) {
        CZipStreamDecompressor* pDecompressor = new CZipStreamDecompressor(
            512, 512, kZlibDefaultWbits, CZipCompression::fCheckFileHeader );
        CCompressionIStream* pUnzipStream = new CCompressionIStream(
            *pInputStream, pDecompressor, CCompressionIStream::fOwnProcessor );
        pI = CObjectIStream::Open( serial, *pUnzipStream, eTakeOwnership );
    }
    else {
        pI = CObjectIStream::Open(
            serial, *pInputStream, (bDeleteOnClose ? eTakeOwnership : eNoOwnership));
    }

    if ( 0 != pI ) {
        pI->UseMemoryPool();
    }
    return pI;
}

END_NCBI_SCOPE

USING_NCBI_SCOPE;

//  ==========================================================================
int main(int argc, const char** argv)
//  ==========================================================================
{
    return CAsn2FastaApp().AppMain(argc, argv);
}
