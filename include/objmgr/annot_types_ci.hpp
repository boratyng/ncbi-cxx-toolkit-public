#ifndef ANNOT_TYPES_CI__HPP
#define ANNOT_TYPES_CI__HPP

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
* Author: Aleksey Grichenko, Michael Kimelman, Eugene Vasilchenko
*
* File Description:
*   Object manager iterators
*
*/

#include <objects/seqloc/Na_strand.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objmgr/annot_selector.hpp>
#include <objmgr/impl/annot_object.hpp>
#include <corelib/ncbiobj.hpp>
#include <set>
#include <vector>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CScope;
class CTSE_Info;
class CSeq_loc;
class CSeqMap_CI;
class CSeq_id_Handle;
class CAnnotObject_Info;
class CSeq_loc_Conversion;
class CBioseq_Handle;
class CAnnotTypes_CI;
class CHandleRangeMap;
class CHandleRange;
struct SAnnotObject_Index;
class CSeq_annot_SNP_Info;
struct SSNP_Info;
class CSeq_loc_Conversion;

class NCBI_XOBJMGR_EXPORT CAnnotObject_Ref
{
public:
    typedef CRange<TSeqPos> TRange;

    CAnnotObject_Ref(void);
    CAnnotObject_Ref(const CAnnotObject_Info& object);
    CAnnotObject_Ref(const CSeq_annot_SNP_Info& snp_info, TSeqPos index);
    ~CAnnotObject_Ref(void);

    enum EObjectType {
        eType_null,
        eType_AnnotObject_Info,
        eType_Seq_annot_SNP_Info
    };

    EObjectType GetObjectType(void) const;

    const CAnnotObject_Info& GetAnnotObject_Info(void) const;
    const CSeq_annot_SNP_Info& GetSeq_annot_SNP_Info(void) const;
    const SSNP_Info& GetSNP_Info(void) const;

    TSeqPos GetFrom(void) const;
    TSeqPos GetToOpen(void) const;
    const TRange& GetTotalRange(void) const;

    bool IsPartial(void) const;
    bool MinusStrand(void) const;
    int GetMappedIndex(void) const;

    const CSeq_annot& GetSeq_annot(void) const;
    bool IsFeat(void) const;
    bool IsSNPFeat(void) const;
    bool IsGraph(void) const;
    bool IsAlign(void) const;
    const CSeq_feat& GetFeat(void) const;
    const CSeq_graph& GetGraph(void) const;
    const CSeq_align& GetAlign(void) const;

    const CSeq_loc* GetMappedLocation(void) const;
    TSeqPos GetSNP_Index(void) const;

    void SetAnnotObjectRange(const TRange& range);
    void SetAnnotObjectRange(const TRange& range,
                             bool partial, const CRef<CSeq_loc>& loc);
    void SetSNP_Point(const SSNP_Info& snp, CSeq_loc_Conversion* cvt);

    void SetPartial(bool value);
    void SetMinusStrand(bool value);
    void SetMappedIndex(int index);

private:
    CConstRef<CObject>      m_Object;
    CRef<CSeq_loc>          m_MappedLocation; // master sequence coordinates
    TRange                  m_TotalRange;
    Int2                    m_MappedIndex;
    Int1                    m_ObjectType; // EObjectType
    // Partial flag for mapped feature (same as in features)
    // if object type is eType_Seq_annot_SNP_Info and m_MappedLocation is null
    // m_Partual contains 'minus_strand' flag
    enum FFlags {
        fPartial     = (1<<0),
        fMinusStrand = (1<<1)
    };
    typedef Int1 TFlags;
    TFlags                  m_Flags;
    TSeqPos                 m_SNP_Index;  // for eType_Seq_annot_SNP_Info
};


class CSeq_annot_Handle;


// Base class for specific annotation iterators
class NCBI_XOBJMGR_EXPORT CAnnotTypes_CI : public SAnnotSelector
{
public:
    CAnnotTypes_CI(void);

    // short methods
    CAnnotTypes_CI(CScope& scope,
                   const CSeq_loc& loc,
                   const SAnnotSelector& params);
    CAnnotTypes_CI(const CBioseq_Handle& bioseq,
                   TSeqPos start, TSeqPos stop,
                   const SAnnotSelector& params);
    // Iterate everything from the seq-annot
    CAnnotTypes_CI(const CSeq_annot_Handle& annot,
                   const SAnnotSelector& params);
    // Iterate everything from the seq-entry
    CAnnotTypes_CI(CScope& scope,
                   const CSeq_entry& entry,
                   const SAnnotSelector& params);

    // Search all TSEs in all datasources, by default get annotations defined
    // on segments in the same TSE (eResolve_TSE method).
    CAnnotTypes_CI(CScope& scope,
                   const CSeq_loc& loc,
                   SAnnotSelector selector,
                   EOverlapType overlap_type,
                   EResolveMethod resolve,
                   const CSeq_entry* entry = 0);
    // Search only in TSE, containing the bioseq, by default get annotations
    // defined on segments in the same TSE (eResolve_TSE method).
    // If "entry" is set, search only annotations from the seq-entry specified
    // (but no its sub-entries or parent entry).
    CAnnotTypes_CI(const CBioseq_Handle& bioseq,
                   TSeqPos start, TSeqPos stop,
                   SAnnotSelector selector,
                   EOverlapType overlap_type,
                   EResolveMethod resolve,
                   const CSeq_entry* entry = 0);
    CAnnotTypes_CI(const CAnnotTypes_CI& it);
    virtual ~CAnnotTypes_CI(void);

    CAnnotTypes_CI& operator= (const CAnnotTypes_CI& it);

    // Rewind annot iterator to point to the very first annot object,
    // the same as immediately after construction.
    void Rewind(void);

    typedef CConstRef<CTSE_Info> TTSE_Lock;
    typedef set<TTSE_Lock>       TTSE_LockSet;

    const CSeq_annot& GetSeq_annot(void) const;

    // Get number of annotations
    size_t GetSize(void) const;

protected:
    // Check if a datasource and an annotation are selected.
    bool IsValid(void) const;
    // Move to the next valid position
    void Next(void);
    void Prev(void);
    // Return current annotation
    const CAnnotObject_Ref& Get(void) const;
    CScope& GetScope(void) const;

private:
    typedef vector<CAnnotObject_Ref> TAnnotSet;

    void x_Clear(void);
    void x_Initialize(const CBioseq_Handle& bioseq,
                      TSeqPos start, TSeqPos stop);
    void x_Initialize(const CHandleRangeMap& master_loc);
    void x_Initialize(const CObject& limit_info);
    void x_SearchMapped(const CSeqMap_CI& seg,
                        const CSeq_id_Handle& master_id,
                        const CHandleRange& master_hr);
    void x_Search(const CHandleRangeMap& loc,
                  CSeq_loc_Conversion* cvt);
    void x_Search(const CSeq_id_Handle& id,
                  const CHandleRange& hr,
                  CSeq_loc_Conversion* cvt);
    void x_Search(const CObject& limit_info);
    
    // Release all locked resources TSE etc
    void x_ReleaseAll(void);

    bool x_NeedSNPs(void) const;
    bool x_MatchLimitObject(const CAnnotObject_Info& annot_info) const;
    bool x_MatchType(const CAnnotObject_Info& annot_info) const;
    bool x_MatchRange(const CHandleRange& hr,
                      const CRange<TSeqPos>& range,
                      const SAnnotObject_Index& index) const;

    // Set of all the annotations found
    TAnnotSet                    m_AnnotSet;
    // Current annotation
    TAnnotSet::const_iterator    m_CurAnnot;
    // TSE set to keep all the TSEs locked
    TTSE_LockSet                 m_TSE_LockSet;
    mutable CRef<CScope>         m_Scope;
};


/////////////////////////////////////////////////////////////////////////////
// CAnnotObject_Ref
/////////////////////////////////////////////////////////////////////////////

inline
CAnnotObject_Ref::CAnnotObject_Ref(void)
    : m_ObjectType(eType_null)
{
}


inline
CAnnotObject_Ref::~CAnnotObject_Ref(void)
{
}


inline
CAnnotObject_Ref::EObjectType CAnnotObject_Ref::GetObjectType(void) const
{
    return EObjectType(m_ObjectType);
}


inline
const CAnnotObject_Info& CAnnotObject_Ref::GetAnnotObject_Info(void) const
{
    _ASSERT(m_ObjectType == eType_AnnotObject_Info);
    return static_cast<const CAnnotObject_Info&>(*m_Object);
}


inline
const CSeq_annot_SNP_Info& CAnnotObject_Ref::GetSeq_annot_SNP_Info(void) const
{
    _ASSERT(m_ObjectType == eType_Seq_annot_SNP_Info);
    return *reinterpret_cast<const CSeq_annot_SNP_Info*>(m_Object.GetPointer());
}


inline
TSeqPos CAnnotObject_Ref::GetFrom(void) const
{
    return m_TotalRange.GetFrom();
}


inline
TSeqPos CAnnotObject_Ref::GetToOpen(void) const
{
    return m_TotalRange.GetToOpen();
}


inline
const CAnnotObject_Ref::TRange& CAnnotObject_Ref::GetTotalRange(void) const
{
    return m_TotalRange;
}


inline
TSeqPos CAnnotObject_Ref::GetSNP_Index(void) const
{
    _ASSERT(m_ObjectType == eType_Seq_annot_SNP_Info);
    return m_SNP_Index;
}


inline
bool CAnnotObject_Ref::IsPartial(void) const
{
    return (m_Flags & fPartial) != 0;
}


inline
bool CAnnotObject_Ref::MinusStrand(void) const
{
    return (m_Flags & fMinusStrand) != 0;
}


inline
const CSeq_loc* CAnnotObject_Ref::GetMappedLocation(void) const
{
    return m_MappedLocation.GetPointerOrNull();
}


inline
int CAnnotObject_Ref::GetMappedIndex(void) const
{
    return m_MappedIndex;
}


inline
bool CAnnotObject_Ref::IsFeat(void) const
{
    return m_ObjectType == eType_Seq_annot_SNP_Info ||
        m_ObjectType == eType_AnnotObject_Info &&
        GetAnnotObject_Info().IsFeat();
}


inline
bool CAnnotObject_Ref::IsSNPFeat(void) const
{
    return m_ObjectType == eType_Seq_annot_SNP_Info;
}


inline
bool CAnnotObject_Ref::IsGraph(void) const
{
    return m_ObjectType == eType_AnnotObject_Info &&
        GetAnnotObject_Info().IsGraph();
}


inline
bool CAnnotObject_Ref::IsAlign(void) const
{
    return m_ObjectType == eType_AnnotObject_Info &&
        GetAnnotObject_Info().IsAlign();
}


inline
const CSeq_feat& CAnnotObject_Ref::GetFeat(void) const
{
    return GetAnnotObject_Info().GetFeat();
}


inline
const CSeq_graph& CAnnotObject_Ref::GetGraph(void) const
{
    return GetAnnotObject_Info().GetGraph();
}


inline
const CSeq_align& CAnnotObject_Ref::GetAlign(void) const
{
    return GetAnnotObject_Info().GetAlign();
}


inline
void CAnnotObject_Ref::SetMappedIndex(int index)
{
    m_MappedIndex = index;
}


inline
void CAnnotObject_Ref::SetAnnotObjectRange(const TRange& range)
{
    m_TotalRange = range;
    m_Flags = 0;
    m_MappedLocation.Reset();
}


inline
void CAnnotObject_Ref::SetAnnotObjectRange(const TRange& range,
                                           bool partial,
                                           const CRef<CSeq_loc>& loc)
{
    m_TotalRange = range;
    m_Flags = partial? fPartial: 0;
    m_MappedLocation = loc;
}


/////////////////////////////////////////////////////////////////////////////
// CAnnotTypes_CI
/////////////////////////////////////////////////////////////////////////////


inline
bool CAnnotTypes_CI::IsValid(void) const
{
    return m_CurAnnot != m_AnnotSet.end();
}


inline
void CAnnotTypes_CI::Rewind(void)
{
    m_CurAnnot = m_AnnotSet.begin();
}


inline
void CAnnotTypes_CI::Next(void)
{
    ++m_CurAnnot;
}


inline
void CAnnotTypes_CI::Prev(void)
{
    --m_CurAnnot;
}


inline
const CAnnotObject_Ref& CAnnotTypes_CI::Get(void) const
{
    _ASSERT( IsValid() );
    return *m_CurAnnot;
}


inline
const CSeq_annot& CAnnotTypes_CI::GetSeq_annot(void) const
{
    return Get().GetSeq_annot();
}


inline
size_t CAnnotTypes_CI::GetSize(void) const
{
    return m_AnnotSet.size();
}

inline
CScope& CAnnotTypes_CI::GetScope(void) const
{
    return *m_Scope;
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.47  2003/08/22 14:58:55  grichenk
* Added NoMapping flag (to be used by CAnnot_CI for faster fetching).
* Added GetScope().
*
* Revision 1.46  2003/08/15 19:19:15  vasilche
* Fixed memory leak in string packing hooks.
* Fixed processing of 'partial' flag of features.
* Allow table packing of non-point SNP.
* Allow table packing of SNP with long alleles.
*
* Revision 1.45  2003/08/14 20:05:18  vasilche
* Simple SNP features are stored as table internally.
* They are recreated when needed using CFeat_CI.
*
* Revision 1.44  2003/08/04 17:02:57  grichenk
* Added constructors to iterate all annotations from a
* seq-entry or seq-annot.
*
* Revision 1.43  2003/07/18 19:39:44  vasilche
* Workaround for GCC 3.0.4 bug.
*
* Revision 1.42  2003/07/17 20:07:55  vasilche
* Reduced memory usage by feature indexes.
* SNP data is loaded separately through PUBSEQ_OS.
* String compression for SNP data.
*
* Revision 1.41  2003/06/25 20:56:29  grichenk
* Added max number of annotations to annot-selector, updated demo.
*
* Revision 1.40  2003/06/02 16:01:36  dicuccio
* Rearranged include/objects/ subtree.  This includes the following shifts:
*     - include/objects/alnmgr --> include/objtools/alnmgr
*     - include/objects/cddalignview --> include/objtools/cddalignview
*     - include/objects/flat --> include/objtools/flat
*     - include/objects/objmgr/ --> include/objmgr/
*     - include/objects/util/ --> include/objmgr/util/
*     - include/objects/validator --> include/objtools/validator
*
* Revision 1.39  2003/04/29 19:51:12  vasilche
* Fixed interaction of Data Loader garbage collector and TSE locking mechanism.
* Made some typedefs more consistent.
*
* Revision 1.38  2003/04/24 16:12:37  vasilche
* Object manager internal structures are splitted more straightforward.
* Removed excessive header dependencies.
*
* Revision 1.37  2003/03/27 19:40:10  vasilche
* Implemented sorting in CGraph_CI.
* Added Rewind() method to feature/graph/align iterators.
*
* Revision 1.36  2003/03/21 19:22:48  grichenk
* Redesigned TSE locking, replaced CTSE_Lock with CRef<CTSE_Info>.
*
* Revision 1.35  2003/03/18 21:48:27  grichenk
* Removed obsolete class CAnnot_CI
*
* Revision 1.34  2003/03/05 20:56:42  vasilche
* SAnnotSelector now holds all parameters of annotation iterators.
*
* Revision 1.33  2003/02/28 19:27:20  vasilche
* Cleaned Seq_loc conversion class.
*
* Revision 1.32  2003/02/27 20:56:51  vasilche
* Use one method for lookup on main sequence and segments.
*
* Revision 1.31  2003/02/27 14:35:32  vasilche
* Splitted PopulateTSESet() by logically independent parts.
*
* Revision 1.30  2003/02/26 17:54:14  vasilche
* Added cached total range of mapped location.
*
* Revision 1.29  2003/02/24 21:35:21  vasilche
* Reduce checks in CAnnotObject_Ref comparison.
* Fixed compilation errors on MS Windows.
* Removed obsolete file src/objects/objmgr/annot_object.hpp.
*
* Revision 1.28  2003/02/24 18:57:20  vasilche
* Make feature gathering in one linear pass using CSeqMap iterator.
* Do not use feture index by sub locations.
* Sort features at the end of gathering in one vector.
* Extracted some internal structures and classes in separate header.
* Delay creation of mapped features.
*
* Revision 1.27  2003/02/13 14:34:31  grichenk
* Renamed CAnnotObject -> CAnnotObject_Info
* + CSeq_annot_Info and CAnnotObject_Ref
* Moved some members of CAnnotObject to CSeq_annot_Info
* and CAnnotObject_Ref.
* Added feat/align/graph to CAnnotObject_Info map
* to CDataSource.
*
* Revision 1.26  2003/02/04 21:44:10  grichenk
* Convert seq-loc instead of seq-annot to the master coordinates
*
* Revision 1.25  2002/12/26 20:51:35  dicuccio
* Added Win32 export specifier
*
* Revision 1.24  2002/12/26 16:39:21  vasilche
* Object manager class CSeqMap rewritten.
*
* Revision 1.23  2002/12/24 15:42:44  grichenk
* CBioseqHandle argument to annotation iterators made const
*
* Revision 1.22  2002/12/06 15:35:57  grichenk
* Added overlap type for annot-iterators
*
* Revision 1.21  2002/11/04 21:28:58  grichenk
* Fixed usage of const CRef<> and CRef<> constructor
*
* Revision 1.20  2002/11/01 20:46:41  grichenk
* Added sorting to set< CRef<CAnnotObject> >
*
* Revision 1.19  2002/10/08 18:57:27  grichenk
* Added feature sorting to the iterator class.
*
* Revision 1.18  2002/07/08 20:50:56  grichenk
* Moved log to the end of file
* Replaced static mutex (in CScope, CDataSource) with the mutex
* pool. Redesigned CDataSource data locking.
*
* Revision 1.17  2002/05/31 17:52:58  grichenk
* Optimized for better performance (CTSE_Info uses atomic counter,
* delayed annotations indexing, no location convertions in
* CAnnot_Types_CI if no references resolution is required etc.)
*
* Revision 1.16  2002/05/24 14:58:53  grichenk
* Fixed Empty() for unsigned intervals
* SerialAssign<>() -> CSerialObject::Assign()
* Improved performance for eResolve_None case
*
* Revision 1.15  2002/05/06 03:30:35  vakatov
* OM/OM1 renaming
*
* Revision 1.14  2002/05/03 21:28:01  ucko
* Introduce T(Signed)SeqPos.
*
* Revision 1.13  2002/04/30 14:30:41  grichenk
* Added eResolve_TSE flag in CAnnot_Types_CI, made it default
*
* Revision 1.12  2002/04/22 20:06:15  grichenk
* Minor changes in private interface
*
* Revision 1.11  2002/04/17 21:11:58  grichenk
* Fixed annotations loading
* Set "partial" flag in features if necessary
* Implemented most seq-loc types in reference resolving methods
* Fixed searching for annotations within a signle TSE
*
* Revision 1.10  2002/04/11 12:07:28  grichenk
* Redesigned CAnnotTypes_CI to resolve segmented sequences correctly.
*
* Revision 1.9  2002/04/05 21:26:16  grichenk
* Enabled iteration over annotations defined on segments of a
* delta-sequence.
*
* Revision 1.8  2002/03/07 21:25:31  grichenk
* +GetSeq_annot() in annotation iterators
*
* Revision 1.7  2002/03/05 16:08:12  grichenk
* Moved TSE-restriction to new constructors
*
* Revision 1.6  2002/03/04 15:07:46  grichenk
* Added "bioseq" argument to CAnnotTypes_CI constructor to iterate
* annotations from a single TSE.
*
* Revision 1.5  2002/02/21 19:27:00  grichenk
* Rearranged includes. Added scope history. Added searching for the
* best seq-id match in data sources and scopes. Updated tests.
*
* Revision 1.4  2002/02/15 20:36:29  gouriano
* changed implementation of HandleRangeMap
*
* Revision 1.3  2002/02/07 21:27:33  grichenk
* Redesigned CDataSource indexing: seq-id handle -> TSE -> seq/annot
*
* Revision 1.2  2002/01/16 16:26:35  gouriano
* restructured objmgr
*
* Revision 1.1  2002/01/11 19:03:59  gouriano
* restructured objmgr
*
*
* ===========================================================================
*/

#endif  // ANNOT_TYPES_CI__HPP
