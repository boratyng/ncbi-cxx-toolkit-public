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
* Author:  Aaron Ucko, NCBI
*
* File Description:
*   Program to convert biological sequences between the formats the
*   C++ Toolkit supports.
*
* ===========================================================================
*/

#include <corelib/ncbiapp.hpp>

#include <serial/iterator.hpp>
#include <serial/objistr.hpp>
#include <serial/objostr.hpp>
#include <serial/serial.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seqset/Seq_entry.hpp>

#include <objmgr/gbloader.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/util/sequence.hpp>

#include <objtools/flat/flat_gbseq_formatter.hpp>
#include <objtools/flat/flat_gff_formatter.hpp>
#include <objtools/flat/flat_table_formatter.hpp>
#include <objtools/readers/fasta.hpp>
#include <objtools/readers/gff_reader.hpp>
#include <objtools/readers/readfeat.hpp>


USING_NCBI_SCOPE;
USING_SCOPE(objects);


class CConversionApp : public CNcbiApplication
{
public:
    void Init(void);
    int  Run (void);

private:
    static IFlatFormatter::EDatabase GetFlatFormat  (const string& name);
    static ESerialDataFormat         GetSerialFormat(const string& name);

    CConstRef<CSeq_entry> Read (const CArgs& args);
    void                  Write(const CSeq_entry& entry, const CArgs& args);

    CRef<CObjectManager> m_ObjMgr;
    CRef<CScope>         m_Scope;
};


void CConversionApp::Init(void)
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Convert biological sequences between formats",
                              false);

    arg_desc->AddDefaultKey("type", "AsnType", "Type of object to convert",
                            CArgDescriptions::eString, "Seq-entry");
    arg_desc->SetConstraint("type", &(*new CArgAllow_Strings,
                                      "Bioseq", "Bioseq-set", "Seq-entry"));

    arg_desc->AddDefaultKey("in", "InputFile", "File to read the object from",
                            CArgDescriptions::eInputFile, "-");
    arg_desc->AddKey("infmt", "Format", "Input format",
                     CArgDescriptions::eString);
    arg_desc->SetConstraint
        ("infmt", &(*new CArgAllow_Strings,
                    "ID", "asn", "asnb", "xml", "fasta", "gff", "tbl"));

    arg_desc->AddDefaultKey("out", "OutputFile", "File to write the object to",
                            CArgDescriptions::eOutputFile, "-");
    arg_desc->AddKey("outfmt", "Format", "Output format",
                     CArgDescriptions::eString);
    arg_desc->SetConstraint
        ("outfmt", &(*new CArgAllow_Strings,
                     "asn", "asnb", "xml", "ddbj", "embl", "genbank", "fasta",
                     "gff", "tbl", "gbseq/xml", "gbseq/asn", "gbseq/asnb"));

    SetupArgDescriptions(arg_desc.release());
}


int CConversionApp::Run(void)
{
    const CArgs& args = GetArgs();

    m_ObjMgr.Reset(new CObjectManager);
    m_ObjMgr->RegisterDataLoader(*new CGBDataLoader("ID"),
                                 CObjectManager::eDefault);

    m_Scope.Reset(new CScope(*m_ObjMgr));
    m_Scope->AddDefaults();

    CConstRef<CSeq_entry> entry = Read(args);
    if (args["infmt"].AsString() != "ID") {
        m_Scope->AddTopLevelSeqEntry(const_cast<CSeq_entry&>(*entry));
    }
    Write(*entry, args);
    return 0;
}


ESerialDataFormat CConversionApp::GetSerialFormat(const string& name)
{
    if (name == "asn") {
        return eSerial_AsnText;
    } else if (name == "asnb") {
        return eSerial_AsnBinary;
    } else if (name == "xml") {
        return eSerial_Xml;
    } else {
        return eSerial_None;
    }
}


IFlatFormatter::EDatabase CConversionApp::GetFlatFormat(const string& name)
{
    if (name == "ddbj") {
        return IFlatFormatter::eDB_DDBJ;
    } else if (name == "embl") {
        return IFlatFormatter::eDB_EMBL;
    } else {
        return IFlatFormatter::eDB_NCBI;
    }
}


CConstRef<CSeq_entry> CConversionApp::Read(const CArgs& args)
{
    const string& infmt = args["infmt"].AsString();
    const string& type  = args["type" ].AsString();

    if (infmt == "ID") {
        CSeq_id        id(args["in"].AsString());
        CBioseq_Handle h = m_Scope->GetBioseqHandle(id);
        return CConstRef<CSeq_entry>(&h.GetTopLevelSeqEntry());
    } else if (infmt == "fasta") {
        return ReadFasta(args["in"].AsInputFile());
    } else if (infmt == "gff") {
        return CGFFReader().Read(args["in"].AsInputFile(),
                                 CGFFReader::fGBQuals);
    } else if (infmt == "tbl") {
        CRef<CSeq_annot> annot = CFeature_table_reader::ReadSequinFeatureTable
            (args["in"].AsInputFile());
        CRef<CSeq_entry> entry(new CSeq_entry);
        if (type == "Bioseq") {
            CBioseq& seq = entry->SetSeq();
            for (CTypeIterator<CSeq_id> it(*annot);  it;  ++it) {
                seq.SetId().push_back(CRef<CSeq_id>(&*it));
                BREAK(it);
            }
            seq.SetInst().SetRepr(CSeq_inst::eRepr_virtual);
            seq.SetInst().SetMol(CSeq_inst::eMol_not_set);
            seq.SetAnnot().push_back(annot);
        } else {
            entry->SetSet().SetAnnot().push_back(annot);
        }
        return entry;
    } else {
        CRef<CSeq_entry> entry(new CSeq_entry);
        auto_ptr<CObjectIStream> in
            (CObjectIStream::Open(GetSerialFormat(infmt),
                                  args["in"].AsString(),
                                  eSerial_StdWhenDash));
        if (type == "Bioseq") {
            *in >> entry->SetSeq();
        } else if (type == "Bioseq-set") {
            *in >> entry->SetSet();
        } else {
            *in >> *entry;
        }
        return entry;
    }
}


void CConversionApp::Write(const CSeq_entry& entry, const CArgs& args)
{
    const string& outfmt = args["outfmt"].AsString();
    const string& type   = args["type"  ].AsString();
    if (outfmt == "genbank"  ||  outfmt == "embl"  ||  outfmt == "ddbj") {
        CFlatTextOStream ftos(args["out"].AsOutputFile());
        auto_ptr<IFlatFormatter> ff
            (CFlatTextFormatter::New(ftos, *m_Scope,
                                     IFlatFormatter::eMode_Entrez,
                                     GetFlatFormat(outfmt)));
        ff->Format(entry, *ff);
    } else if (outfmt == "fasta") {
        CFastaOstream out(args["out"].AsOutputFile());
        for (CTypeConstIterator<CBioseq> it(entry);  it;  ++it) {
            out.Write(m_Scope->GetBioseqHandle(*it));
        }
    } else if (outfmt == "gff") {
        CFlatTextOStream ftos(args["out"].AsOutputFile());
        CFlatGFFFormatter ff(ftos, *m_Scope, IFlatFormatter::eMode_Dump,
                             CFlatGFFFormatter::fGTFCompat
                             | CFlatGFFFormatter::fShowSeq);
        ff.Format(entry, ff);
    } else if (outfmt == "tbl") {
        CFlatTextOStream ftos(args["out"].AsOutputFile());
        CFlatTableFormatter ff(ftos, *m_Scope);
        ff.Format(entry, ff);
    } else if (NStr::StartsWith(outfmt, "gbseq/")) {
        CFlatGBSeqFormatter ff(*m_Scope, IFlatFormatter::eMode_Entrez);
        ff.Format(entry, ff);
        auto_ptr<CObjectOStream> out
            (CObjectOStream::Open(GetSerialFormat(outfmt.substr(6)),
                                  args["out"].AsString(),
                                  eSerial_StdWhenDash));
        *out << ff.GetGBSet();
    } else {
        auto_ptr<CObjectOStream> out
            (CObjectOStream::Open(GetSerialFormat(outfmt),
                                  args["out"].AsString(),
                                  eSerial_StdWhenDash));
        if (type == "Bioseq") {
            if (entry.IsSet()) {
                ERR_POST(Warning
                         << "Possible truncation in conversion to Bioseq");
                *out << *entry.GetSet().GetSeq_set().front();
            } else {
                *out << entry.GetSeq();
            }
        } else if (type == "Bioseq-set") {
            if (entry.IsSet()) {
                *out << entry.GetSet();
            } else {
                CBioseq_set bss;
                bss.SetSeq_set().push_back
                    (CRef<CSeq_entry>(const_cast<CSeq_entry*>(&entry)));
                *out << bss;
            }
        } else {
            *out << entry;
        }
    }
}


int main(int argc, const char* argv[])
{
    // Execute main application function
    return CConversionApp().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2003/12/03 20:58:40  ucko
* Add new universal sequence converter app.
*
*
* ===========================================================================
*/
