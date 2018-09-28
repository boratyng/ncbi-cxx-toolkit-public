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
 * Author: Frank Ludwig
 *
 * File Description:  Iterate through file names matching a given glob pattern
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbifile.hpp>

#include <objtools/import/feat_importer.hpp>

#include <objtools/import/feat_import_error.hpp>
#include "id_resolver_canonical.hpp"
#include "gtf_importer.hpp"
#include "bed_importer.hpp"
#include "feat_line_reader.hpp"
#include "feat_import_data.hpp"
#include "feat_annot_assembler.hpp"

#include <assert.h>

USING_NCBI_SCOPE;
USING_SCOPE(objects);

//  ============================================================================
CFeatImporter*
CFeatImporter::Get(
    const string& format,
    unsigned int flags)
//  ============================================================================
{
    if (format == "gtf") {
        return new CGtfImporter(flags);
    }
    if (format == "bed") {
        return new CBedImporter(flags);
    }
    return nullptr;
};


//  ============================================================================
CFeatImporter::CFeatImporter(
    unsigned int flags): mFlags(flags)
//  ============================================================================
{
    bool allIdsAsLocal = (mFlags & CFeatImporter::fAllIdsAsLocal);
    bool numericIdsAsLocal = (mFlags & CFeatImporter::fNumericIdsAsLocal);
    SetIdResolver(new CIdResolverCanonical(allIdsAsLocal, numericIdsAsLocal));
};


//  ============================================================================
CFeatImporter::~CFeatImporter()
//  ============================================================================
{
};


//  ============================================================================
void
CFeatImporter::SetIdResolver(
    CIdResolver* pIdResolver)
//  ============================================================================
{
    mpIdResolver.reset(pIdResolver);
}

//  =============================================================================
void
CFeatImporter::ReadSeqAnnot(
    CNcbiIstream& istr,
    CSeq_annot& annot,
    CFeatMessageHandler& errorReporter)
//  =============================================================================
{
    unique_ptr<CFeatLineReader> pLineReader(GetReader(istr, errorReporter));
    unique_ptr<CFeatImportData> pImportData(GetImportData(errorReporter));
    unique_ptr<CFeatAnnotAssembler> pAssembler(GetAnnotAssembler(annot, errorReporter));
    assert(pLineReader  &&  pImportData  &&  pAssembler);

    if (mFlags & fReportProgress) {
        pLineReader->SetProgressReportFrequency(5);
    }

    pAssembler->InitializeAnnot();
    while (true) {
        try {
            if (!pLineReader->GetNextRecord(*pImportData)) {
                break;
            }
            //pImportData->Serialize(cerr);
            pAssembler->ProcessRecord(*pImportData);
        }
        catch(CFeatImportError& err) {
            if (err.Code() == CFeatImportError::eEOF_NO_DATA) {
                throw;
            }
            err.SetLineNumber(pLineReader->LineCount());
            errorReporter.ReportError(err);
        }
    }
    pAssembler->FinalizeAnnot();
}


