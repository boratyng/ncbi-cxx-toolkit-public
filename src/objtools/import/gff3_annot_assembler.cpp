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
#include <objects/general/Object_id.hpp>

#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqfeat/Gene_ref.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqloc/Seq_interval.hpp>

#include <objtools/import/feat_import_error.hpp>
#include "featid_generator.hpp"
#include "gff3_annot_assembler.hpp"

#include <assert.h>

USING_NCBI_SCOPE;
USING_SCOPE(objects);

//  ============================================================================
CGff3AnnotAssembler::CGff3AnnotAssembler(
    CFeatMessageHandler& errorReporter):
//  ============================================================================
    CFeatAnnotAssembler(errorReporter)
{
}

//  ============================================================================
CGff3AnnotAssembler::~CGff3AnnotAssembler()
//  ============================================================================
{
}

//  ============================================================================
void
CGff3AnnotAssembler::ProcessRecord(
    const CFeatImportData& record_,
    CSeq_annot& annot)
//  ============================================================================
{
    assert(dynamic_cast<const CGff3ImportData*>(&record_));
    const CGff3ImportData& record = static_cast<const CGff3ImportData&>(record_);

    auto recordId = record.Id();
    auto parentId = record.Parent();
    auto pFeature = record.GetData();
    if (!recordId.empty()) {
        mFeatureMap.AddFeature(recordId, pFeature);
    }

    switch (pFeature->GetData().GetSubtype()) {
    default:
        return xProcessFeatureDefault(recordId, parentId, pFeature, annot);
    case CSeqFeatData::eSubtype_exon:
        return xProcessFeatureExon(recordId, parentId, pFeature, annot);
    case CSeqFeatData::eSubtype_mRNA:
        return xProcessFeatureRna(recordId, parentId, pFeature, annot);
    }
}

//  ============================================================================
void
CGff3AnnotAssembler::xProcessFeatureDefault(
    const std::string& recordId,
    const std::string& parentId,
    CRef<CSeq_feat> pFeature,
    CSeq_annot& annot)
//  ============================================================================
{
    annot.SetData().SetFtable().push_back(pFeature);
}


//  ============================================================================
void
CGff3AnnotAssembler::xProcessFeatureRna(
    const std::string& recordId,
    const std::string& parentId,
    CRef<CSeq_feat> pFeature,
    CSeq_annot& annot)
//  ============================================================================
{
    annot.SetData().SetFtable().push_back(pFeature);
    pFeature->SetLocation().SetNull();

    vector<CRef<CSeq_feat>> pendingExons;
    if (!mPendingFeatures.FindPendingFeatures(recordId, pendingExons)) {
        return;
    }
    for (auto pExon: pendingExons) {
        CRef<CSeq_loc> pUpdatedLocation = pFeature->GetLocation().Add(
            pExon->GetLocation(), 
            CSeq_loc::fSortAndMerge_All, nullptr);
        pFeature->SetLocation().Assign(*pUpdatedLocation);
    }
    mPendingFeatures.MarkFeaturesDone(recordId);
}


//  ============================================================================
void
CGff3AnnotAssembler::xProcessFeatureExon(
    const std::string& recordId,
    const std::string& parentId,
    CRef<CSeq_feat> pFeature,
    CSeq_annot& annot)
//  ============================================================================
{
    auto pParentRna = mFeatureMap.FindFeature(parentId);
    if (pParentRna) {
        CRef<CSeq_loc> pUpdatedLocation = pParentRna->GetLocation().Add(
            pFeature->GetLocation(), 
            CSeq_loc::fSortAndMerge_All, nullptr);
        pParentRna->SetLocation().Assign(*pUpdatedLocation);
    }
    else {
        mPendingFeatures.AddFeature(parentId, pFeature);
    }
}

//  ============================================================================
void
CGff3AnnotAssembler::FinalizeAnnot(
    const CAnnotImportData& annotData,
    CSeq_annot& annot)
//  ============================================================================
{
}

