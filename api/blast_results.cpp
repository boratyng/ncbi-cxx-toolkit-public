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
 * Author: Christiam Camacho
 *
 */

/** @file blast_results.cpp
 * Implementation of classes which constitute the results of running a BLAST
 * search
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = 
    "$Id$";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <algo/blast/api/blast_results.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqloc/Seq_id.hpp>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)

CSearchResults::CSearchResults(CConstRef<objects::CSeq_id> query,
                               CRef<objects::CSeq_align_set> align,
                               const TQueryMessages& errs,
                               CRef<CBlastAncillaryData> ancillary_data,
                               const TMaskedQueryRegions* query_masks)
: m_QueryId(query), m_Alignment(align), m_Errors(errs), 
  m_AncillaryData(ancillary_data)
{
    if (query_masks)
        SetMaskedQueryRegions(*query_masks);
}

void
CSearchResults::GetMaskedQueryRegions
    (TMaskedQueryRegions& flt_query_regions) const
{
    flt_query_regions = m_Masks;
}

void
CSearchResults::SetMaskedQueryRegions
    (const TMaskedQueryRegions& flt_query_regions)
{
    m_Masks.clear();
    copy(flt_query_regions.begin(), flt_query_regions.end(), 
         back_inserter(m_Masks));
}

TQueryMessages
CSearchResults::GetErrors(int min_severity) const
{
    TQueryMessages errs;
    
    ITERATE(TQueryMessages, iter, m_Errors) {
        if ((**iter).GetSeverity() >= min_severity) {
            errs.push_back(*iter);
        }
    }
    
    return errs;
}

bool
CSearchResults::HasAlignments() const
{
    if (m_Alignment.Empty()) {
        return false;
    }

    return m_Alignment->Get().size() != 0  &&
         m_Alignment->Get().front()->IsSetSegs();
}

CConstRef<CSeq_id>
CSearchResults::GetSeqId() const
{
    return m_QueryId;
}

CSearchResults&
CSearchResultSet::GetResults(size_type qi, size_type si)
{
    if (m_ResultType != eSequenceComparison) {
        NCBI_THROW(CBlastException, eNotSupported, "Invalid method accessed");
    }
    return *m_Results[qi * m_NumQueries + si];
}

const CSearchResults&
CSearchResultSet::GetResults(size_type qi, size_type si) const
{
    return const_cast<CSearchResults&>(GetResults(qi, si));
}

CConstRef<CSearchResults>
CSearchResultSet::operator[](const objects::CSeq_id & ident) const
{
    if (m_ResultType != eDatabaseSearch) {
        NCBI_THROW(CBlastException, eNotSupported, "Invalid method accessed");
    }
    for( size_t i = 0;  i < m_Results.size();  i++ ) {
        if ( CSeq_id::e_YES == ident.Compare(*m_Results[i]->GetSeqId()) ) {
            return m_Results[i];
        }
    }
    
    return CConstRef<CSearchResults>();
}

CRef<CSearchResults>
CSearchResultSet::operator[](const objects::CSeq_id & ident)
{
    if (m_ResultType != eDatabaseSearch) {
        NCBI_THROW(CBlastException, eNotSupported, "Invalid method accessed");
    }
    for( size_t i = 0;  i < m_Results.size();  i++ ) {
        if ( CSeq_id::e_YES == ident.Compare(*m_Results[i]->GetSeqId()) ) {
            return m_Results[i];
        }
    }
    
    return CRef<CSearchResults>();
}

/// Find the first alignment in a set of blast results, and
//  return the sequence identifier of the first sequence in the alignment.
//  All alignments in the blast results are assumed to contain the
//  same identifier
// @param align_set The blast results
// @return The collection of sequence ID's corresponding to the
//         first sequence of the first alignment
static CConstRef<CSeq_id>
s_ExtractSeqId(CConstRef<CSeq_align_set> align_set)
{
    CConstRef<CSeq_id> retval;
    
    if (! (align_set.Empty() || align_set->Get().empty())) {
        // index 0 = query, index 1 = subject
        const int kQueryIndex = 0;
        
        CRef<CSeq_align> align = align_set->Get().front();

        if (align->GetSegs().IsDisc() == true)
        {
        
            if (align->GetSegs().GetDisc().Get().empty())
                return retval;
        
            CRef<CSeq_align> first_align = align->GetSegs().GetDisc().Get().front();
            retval.Reset(& align->GetSeq_id(kQueryIndex));
        }
        else
        {
            retval.Reset(& align->GetSeq_id(kQueryIndex));
        }
    }
    
    _ASSERT(retval.NotEmpty());
    return retval;
}

CSearchResultSet::CSearchResultSet(TQueryIdVector               queries,
                                   TSeqAlignVector              aligns,
                                   TSearchMessages              msg_vec,
                                   TAncillaryVector             ancillary_data,
                                   const TSeqLocInfoVector*     query_masks,
                                   EResultType                  res_type)
: m_ResultType(res_type)
{
    if (ancillary_data.empty()) {
        ancillary_data.resize(aligns.size());
    }
    x_Init(queries, aligns, msg_vec, ancillary_data, query_masks);
}

CSearchResultSet::CSearchResultSet(TSeqAlignVector aligns,
                                   TSearchMessages msg_vec,
                                   EResultType res_type)
: m_ResultType(res_type)
{
    vector< CConstRef<CSeq_id> > queries;
    TAncillaryVector ancillary_data(aligns.size()); // no ancillary_data
    
    for(size_t i = 0; i < aligns.size(); i++) {
        queries.push_back(s_ExtractSeqId(aligns[i]));
    }
    
    x_Init(queries, aligns, msg_vec, ancillary_data, NULL);
}

void CSearchResultSet::x_Init(vector< CConstRef<objects::CSeq_id> > queries,
                              TSeqAlignVector                    aligns,
                              TSearchMessages                    msg_vec,
                              TAncillaryVector                   ancillary_data,
                              const TSeqLocInfoVector*           query_masks)
{
    _ASSERT(queries.size() == aligns.size());
    _ASSERT(aligns.size() == msg_vec.size());
    _ASSERT(aligns.size() == ancillary_data.size());

    m_NumQueries = queries.size();
    m_Results.resize(aligns.size());
    
    for(size_t i = 0; i < aligns.size(); i++) {
        if (query_masks) {
            m_Results[i].Reset(new CSearchResults(queries[i],
                                                  aligns[i],
                                                  msg_vec[i],
                                                  ancillary_data[i],
                                                  &(*query_masks)[i]));
        } else {
            m_Results[i].Reset(new CSearchResults(queries[i],
                                                  aligns[i],
                                                  msg_vec[i],
                                                  ancillary_data[i]));
        }
    }
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */
