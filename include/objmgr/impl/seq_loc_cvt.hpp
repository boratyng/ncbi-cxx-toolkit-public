#ifndef SEQ_LOC_CVT__HPP
#define SEQ_LOC_CVT__HPP

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

#include <corelib/ncbiobj.hpp>

#include <util/range.hpp>
#include <util/rangemap.hpp>

#include <objmgr/seq_id_handle.hpp>
#include <objmgr/impl/heap_scope.hpp>

#include <objects/seqloc/Na_strand.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_point.hpp>
#include <objects/seqloc/Seq_interval.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSeqMap_CI;
class CScope;
class CSeq_align_Mapper;
class CAnnotObject_Ref;

class CSeq_id;
class CSeq_loc;
class CSeq_interval;
class CSeq_point;

class CSeq_align;
class CDense_seg;
class CPacked_seg;
class CSeq_align_set;


/////////////////////////////////////////////////////////////////////////////
// CSeq_loc_Conversion
/////////////////////////////////////////////////////////////////////////////

class CSeq_loc_Conversion : public CObject
{
public:
    typedef CRange<TSeqPos> TRange;

    CSeq_loc_Conversion(CSeq_loc& master_loc_empty,
                        const CSeqMap_CI& seg,
                        const CSeq_id_Handle& src_id,
                        CScope* scope);
    // Create conversion, mapping an ID to itself
    CSeq_loc_Conversion(const CSeq_id_Handle& master_id,
                        CScope* scope);

    ~CSeq_loc_Conversion(void);

    TSeqPos ConvertPos(TSeqPos src_pos);

    bool GoodSrcId(const CSeq_id& id);
    bool MinusStrand(void) const
        {
            return m_Reverse;
        }

    bool ConvertPoint(TSeqPos src_pos, ENa_strand src_strand);
    bool ConvertPoint(const CSeq_point& src);

    bool ConvertInterval(TSeqPos src_from, TSeqPos src_to,
                         ENa_strand src_strand);
    bool ConvertInterval(const CSeq_interval& src);

    enum EConvertFlag {
        eCnvDefault,
        eCnvAlways
    };
    enum ELocationType {
        eLocation,
        eProduct
    };

    bool Convert(const CSeq_loc& src, CRef<CSeq_loc>* dst,
                 EConvertFlag flag = eCnvDefault);

    void Convert(const CSeq_align& src, CRef<CSeq_align_Mapper>* dst);

    void Convert(CAnnotObject_Ref& obj, ELocationType loctype);

    void Reset(void);

    bool IsPartial(void) const
        {
            return m_Partial;
        }

    void SetSrcId(const CSeq_id_Handle& src)
        {
            m_Src_id_Handle = src;
        }
    void SetConversion(const CSeqMap_CI& seg);

    const TRange& GetTotalRange(void) const
        {
            return m_TotalRange;
        }
    
    ENa_strand ConvertStrand(ENa_strand strand) const;

    void SetMappedLocation(CAnnotObject_Ref& ref, ELocationType loctype);

private:
    void CheckDstInterval(void);
    void CheckDstPoint(void);

    CRef<CSeq_interval> GetDstInterval(void);
    CRef<CSeq_point> GetDstPoint(void);

    void SetDstLoc(CRef<CSeq_loc>* loc);

    bool IsSpecialLoc(void) const;

    CSeq_loc& GetDstLocEmpty(void)
        {
            return *m_Dst_loc_Empty;
        }
    CSeq_id& GetDstId(void)
        {
            return m_Dst_loc_Empty->SetEmpty();
        }

    // Translation parameters:
    //   Source id and bounds:
    CSeq_id_Handle m_Src_id_Handle;
    TSeqPos        m_Src_from;
    TSeqPos        m_Src_to;

    //   Source to destination shift:
    TSignedSeqPos  m_Shift;
    bool           m_Reverse;

    //   Destination id:
    CRef<CSeq_id>  m_Dst_id;
    CRef<CSeq_loc> m_Dst_loc_Empty;

    // Results:
    //   Cumulative results on destination:
    TRange         m_TotalRange;
    bool           m_Partial;
    
    //   Last Point, Interval or other simple location's conversion result:
    enum EMappedObjectType {
        eMappedObjType_not_set,
        eMappedObjType_Seq_loc,
        eMappedObjType_Seq_point,
        eMappedObjType_Seq_interval
    };
    EMappedObjectType m_LastType;
    TRange         m_LastRange;
    ENa_strand     m_LastStrand;

    // Scope for id resolution:
    CHeapScope     m_Scope;

    friend class CSeq_loc_Conversion_Set;
    friend class CSeq_align_Mapper;
};


class CSeq_loc_Conversion_Set
{
public:
    CSeq_loc_Conversion_Set(void);

    typedef CRange<TSeqPos> TRange;
    typedef CRangeMultimap<CRef<CSeq_loc_Conversion>, TSeqPos> TRangeMap;
    typedef TRangeMap::iterator TRangeIterator;
    typedef map<CSeq_id_Handle, TRangeMap> TIdMap;

    void Add(CSeq_loc_Conversion& cvt);
    TRangeIterator BeginRanges(CSeq_id_Handle id, TSeqPos from, TSeqPos to);
    void Convert(CAnnotObject_Ref& obj,
                 CSeq_loc_Conversion::ELocationType loctype);
    bool Convert(const CSeq_loc& src, CRef<CSeq_loc>* dst);
    void Convert(const CSeq_align& src, CRef<CSeq_align_Mapper>* dst);
    void SetScope(CHeapScope& scope)
        {
            m_Scope = scope;
        }

private:
    bool ConvertPoint(const CSeq_point& src, CRef<CSeq_loc>* dst);
    bool ConvertInterval(const CSeq_interval& src, CRef<CSeq_loc>* dst);

    TIdMap         m_IdMap;
    bool           m_Partial;
    TRange         m_TotalRange;
    CHeapScope     m_Scope;
};


inline
bool CSeq_loc_Conversion::IsSpecialLoc(void) const
{
    return m_LastType == eMappedObjType_Seq_point ||
        m_LastType == eMappedObjType_Seq_interval;
}


inline
void CSeq_loc_Conversion::Reset(void)
{
    _ASSERT(!IsSpecialLoc());
    m_TotalRange = TRange::GetEmpty();
    m_Partial = false;
}


inline
TSeqPos CSeq_loc_Conversion::ConvertPos(TSeqPos src_pos)
{
    if ( src_pos < m_Src_from || src_pos > m_Src_to ) {
        m_Partial = true;
        return kInvalidSeqPos;
    }
    TSeqPos dst_pos;
    if ( !m_Reverse ) {
        dst_pos = m_Shift + src_pos;
    }
    else {
        dst_pos = m_Shift - src_pos;
    }
    return dst_pos;
}


inline
bool CSeq_loc_Conversion::GoodSrcId(const CSeq_id& id)
{
    bool good = (m_Src_id_Handle == id);
    if ( !good ) {
        m_Partial = true;
    }
    return good;
}


inline
ENa_strand CSeq_loc_Conversion::ConvertStrand(ENa_strand strand) const
{
    if ( m_Reverse ) {
        strand = Reverse(strand);
    }
    return strand;
}


inline
bool CSeq_loc_Conversion::ConvertPoint(const CSeq_point& src)
{
    ENa_strand strand = src.IsSetStrand()? src.GetStrand(): eNa_strand_unknown;
    return GoodSrcId(src.GetId()) && ConvertPoint(src.GetPoint(), strand);
}


inline
bool CSeq_loc_Conversion::ConvertInterval(const CSeq_interval& src)
{
    ENa_strand strand = src.IsSetStrand()? src.GetStrand(): eNa_strand_unknown;
    return GoodSrcId(src.GetId()) && ConvertInterval(src.GetFrom(), src.GetTo(), strand);
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.16  2004/05/10 18:26:37  grichenk
* Fixed 'not used' warnings
*
* Revision 1.15  2004/03/30 21:21:09  grichenk
* Reduced number of includes.
*
* Revision 1.14  2004/03/30 15:42:33  grichenk
* Moved alignment mapper to separate file, added alignment mapping
* to CSeq_loc_Mapper.
*
* Revision 1.13  2004/03/16 15:47:27  vasilche
* Added CBioseq_set_Handle and set of EditHandles
*
* Revision 1.12  2004/02/23 15:23:16  grichenk
* Removed unused members
*
* Revision 1.11  2004/02/06 16:07:26  grichenk
* Added CBioseq_Handle::GetSeq_entry_Handle()
* Fixed MapLocation()
*
* Revision 1.10  2004/01/30 15:25:44  grichenk
* Fixed alignments mapping and sorting
*
* Revision 1.9  2004/01/28 20:54:35  vasilche
* Fixed mapping of annotations.
*
* Revision 1.8  2004/01/23 16:14:46  grichenk
* Implemented alignment mapping
*
* Revision 1.7  2003/11/10 18:11:03  grichenk
* Moved CSeq_loc_Conversion_Set to seq_loc_cvt
*
* Revision 1.6  2003/11/04 16:21:36  grichenk
* Updated CAnnotTypes_CI to map whole features instead of splitting
* them by sequence segments.
*
* Revision 1.5  2003/10/27 20:07:10  vasilche
* Started implementation of full annotations' mapping.
*
* Revision 1.4  2003/09/30 16:22:01  vasilche
* Updated internal object manager classes to be able to load ID2 data.
* SNP blobs are loaded as ID2 split blobs - readers convert them automatically.
* Scope caches results of requests for data to data loaders.
* Optimized CSeq_id_Handle for gis.
* Optimized bioseq lookup in scope.
* Reduced object allocations in annotation iterators.
* CScope is allowed to be destroyed before other objects using this scope are
* deleted (feature iterators, bioseq handles etc).
* Optimized lookup for matching Seq-ids in CSeq_id_Mapper.
* Added 'adaptive' option to objmgr_demo application.
*
* Revision 1.3  2003/08/27 14:28:51  vasilche
* Reduce amount of object allocations in feature iteration.
*
* Revision 1.2  2003/08/15 19:19:15  vasilche
* Fixed memory leak in string packing hooks.
* Fixed processing of 'partial' flag of features.
* Allow table packing of non-point SNP.
* Allow table packing of SNP with long alleles.
*
* Revision 1.1  2003/08/14 20:05:18  vasilche
* Simple SNP features are stored as table internally.
* They are recreated when needed using CFeat_CI.
*
* ===========================================================================
*/

#endif  // ANNOT_TYPES_CI__HPP
