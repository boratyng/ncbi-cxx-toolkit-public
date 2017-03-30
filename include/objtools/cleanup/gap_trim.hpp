#ifndef CLEANUP___GAP_TRIM__HPP
#define CLEANUP___GAP_TRIM__HPP

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
 *   Adjusting features for gaps
 *   .......
 *
 */
#include <objmgr/scope.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqfeat/Cdregion.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSeq_entry;
class CBioseq;
class CBioseq_set;
class CSeq_annot;
class CSeq_feat;



class CSeq_entry_Handle;
class CBioseq_Handle;
class CBioseq_set_Handle;
class CSeq_annot_Handle;
class CSeq_feat_Handle;

class NCBI_CLEANUP_EXPORT CFeatGapInfo : public CObject {
public:
    CFeatGapInfo() {};
    CFeatGapInfo(CSeq_feat_Handle sf);
    ~CFeatGapInfo() {};

    void CollectGaps(const CSeq_loc& feat_loc, CScope& scope);
    void CalculateRelevantIntervals(bool unknown_length, bool known_length);
    bool HasKnown() const { return m_Known; };
    bool HasUnknown() const { return m_Unknown; };

    bool Trimmable() const;
    bool Splittable() const;
    bool ShouldRemove() const;

    void Trim(CSeq_loc& loc, bool make_partial, CScope& scope);
    typedef vector< CRef<CSeq_loc> > TLocList;
    TLocList Split(const CSeq_loc& orig, bool in_intron, bool make_partial);

    vector<CRef<CSeq_feat> > AdjustForRelevantGapIntervals(bool make_partial, bool trim, bool split, bool in_intron);
    CSeq_feat_Handle GetFeature() const { return m_Feature; };


protected:
    typedef pair<bool, pair<size_t, size_t> > TGapInterval;
    typedef vector<TGapInterval> TGapIntervalList;
    TGapIntervalList m_Gaps;

    typedef vector<pair<size_t, size_t> > TIntervalList;
    TIntervalList m_InsideGaps;
    TIntervalList m_LeftGaps;
    TIntervalList m_RightGaps;

    TSeqPos m_Start;
    TSeqPos m_Stop;

    bool m_Known;
    bool m_Unknown;

    CSeq_feat_Handle m_Feature;

    void x_AdjustOrigLabel(CSeq_feat& feat, size_t& id_offset, string& id_label, const string& qual);
    static void x_AdjustFrame(CCdregion& cdregion, TSeqPos frame_adjust);
};

typedef vector<CRef<CFeatGapInfo> > TGappedFeatList;
NCBI_CLEANUP_EXPORT TGappedFeatList ListGappedFeatures(CFeat_CI& feat_it, CScope& scope);

END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* CLEANUP___GAP_TRIM__HPP */
