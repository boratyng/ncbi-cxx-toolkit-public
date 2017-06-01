#ifndef FEATURE_INDEXER__HPP
#define FEATURE_INDEXER__HPP

/*
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
* Author:  Jonathan Kans
*
*/

#include <corelib/ncbicntr.hpp>

#include <objects/general/Object_id.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/submit/Seq_submit.hpp>
#include <objects/submit/Submit_block.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/seq_entry_handle.hpp>
#include <objmgr/util/feature.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


// look-ahead class names
class CSeqEntryIndex;
class CBioseqIndex;
class CDescriptorIndex;
class CFeatureIndex;


// CSeqEntryIndex
//
// CSeqEntryIndex is the master, top-level Seq-entry exploration organizer.  A variable
// is created, with optional control flags, then initialized with the top-level object:
//
//   CSeqEntryIndex idx(CSeqEntryIndex::fDefaultIndexing);
//   idx.Initialize(*m_entry);
//
// A Seq-entry wrapper is created if the top-level object is a Bioseq or Bioseq-set.
// Bioseqs within the Seq-entry are then indexed and added to a vector of CBioseqIndex.
//
// Bioseqs are explored with IterateBioseqs, or selected individually by ProcessBioseq
// (given an accession, index number, or subregion):
//
//   idx.ProcessBioseq("U54469", [this](CBioseqIndex& bsx) {
//       ...
//   });
//
// The embedded lambda function statements are executed for each selected Bioseq.
//
// Internal indexing objects (CBioseqIndex, CDescriptorIndex, and CFeatureIndex) are
// generated by the indexing process, and should not be created manually.
class NCBI_XOBJUTIL_EXPORT CSeqEntryIndex : public CObject
{
    friend class CBioseqIndex;
public:

    enum EFlags {
        // general policy flags to use for all records
        fDefaultIndexing = 0,
        fSkipRemoteFeatures = 1
    };

    typedef unsigned int TFlags; // binary OR of "EFlags"

public:
    // Constructor
    CSeqEntryIndex (TFlags flags = 0);

    // Destructor
    ~CSeqEntryIndex (void);

private:
    // Prohibit copy constructor & assignment operator
    CSeqEntryIndex (const CSeqEntryIndex&);
    CSeqEntryIndex& operator= (const CSeqEntryIndex&);

public:
    // Initializers take the top-level object

    void Initialize (CSeq_entry& topsep);
    void Initialize (CBioseq_set& seqset);
    void Initialize (CBioseq& bioseq);
    void Initialize (CSeq_submit& submit);

    // Specialized initializers for streaming through release files, one component at a time

    // Submit-block obtained from top of Seq-submit release file
    void Initialize (CSeq_entry& topsep, CSubmit_block &sblock);
    // Seq-descr chain obtained from top of Bioseq-set release file
    void Initialize (CSeq_entry& topsep, CSeq_descr &descr);

    // Bioseq exploration iterator
    template<typename _Pred> int IterateBioseqs (_Pred m);

    // Process first Bioseq
    template<typename _Pred> bool ProcessBioseq (_Pred m);
    // Process Nth Bioseq
    template<typename _Pred> bool ProcessBioseq (int n, _Pred m);
    // Process Bioseq by accession
    template<typename _Pred> bool ProcessBioseq (const string& accn, _Pred m);

    // Subrange processing creates a new CBioseqIndex around a temporary delta Bioseq

    // Process Bioseq by sublocation
    template<typename _Pred> bool ProcessBioseq (const CSeq_loc& loc, _Pred m);
    // Process Bioseq by subrange, either forward strand or reverse complement
    template<typename _Pred> bool ProcessBioseq (const string& accn, int from, int to, bool rev_comp, _Pred m);

    // Getters
    CRef<CObjectManager> GetObjectManager (void) const { return m_objmgr; }
    CRef<CScope> GetScope (void) const { return m_scope; }
    CSeq_entry_Handle GetTopSEH (void) const { return m_topSEH; }
    CConstRef<CSeq_entry> GetTopSEP (void) const { return m_topSEP; }
    CConstRef<CSubmit_block> GetSbtBlk (void) const { return m_sbtBlk; }
    CConstRef<CSeq_descr> GetTopDescr (void) const { return m_topDescr; }

    // Flag to indicate failure to fetch remote sequence components or feature annotation
    bool IsFetchFailure (void) const { return m_fetchFailure; }

private:
    // Common initialization function
    void x_Init (void);

    // Populate vector of index objects for Bioseqs in Seq-entry
    void x_InitSeqs (const CSeq_entry& sep);

    CRef<CSeq_id> MakeUniqueId(void);

    // Create delta sequence referring to location, using temporary local ID
    CRef<CBioseqIndex> x_DeltaIndex(const CSeq_loc& loc);

    // Create location from range, to use in x_DeltaIndex
    CRef<CSeq_loc> x_SubRangeLoc(const string& accn, int from, int to, bool rev_comp);

private:
    CRef<CObjectManager> m_objmgr;
    CRef<CScope> m_scope;
    CSeq_entry_Handle m_topSEH;

    CConstRef<CSeq_entry> m_topSEP;
    CConstRef<CSubmit_block> m_sbtBlk;
    CConstRef<CSeq_descr> m_topDescr;

    bool m_fetchFailure;

    TFlags m_flags;

    vector<CRef<CBioseqIndex>> m_bsxList;

    // map from accession string to CBioseqIndex object
    typedef map<string, CRef<CBioseqIndex> > TAccnIndexMap;
    TAccnIndexMap m_accnIndexMap;

    mutable CAtomicCounter m_Counter;
};


// CBioseqIndex
//
// CBioseqIndex is the exploration organizer for a given Bioseq.  It provides methods to
// obtain descriptors and iterate through features that apply to the Bioseq.  (These are
// stored in vectors, which are initialized upon first request.)
//
// CBioseqIndex also maintains a CFeatTree for each Bioseq, used to find the best gene
// for each feature.
//
// Descriptors are explored with:
//
//   bsx.IterateDescriptors([this](CDescriptorIndex& sdx) {
//       ...
//   });
//
// starting with descriptors packaged on the Bioseq, then on its parent Bioseq-set, etc.
//
// Features are explored with:
//
//   bsx.IterateFeatures([this](CFeatureIndex& sfx) {
//       ...
//   });
//
// and are presented in order of position along the parent sequence.
//
// Fetching external features uses SAnnotSelector adaptive depth unless explicitly overridden.
class NCBI_XOBJUTIL_EXPORT CBioseqIndex : public CObject
{
    friend class CSeqEntryIndex;
    friend class CDescriptorIndex;
    friend class CFeatureIndex;

public:
    // Constructor
    CBioseqIndex (CBioseq_Handle bsh, const CBioseq& bsp, CSeqEntryIndex& enx);

    // Destructor
    ~CBioseqIndex (void);

private:
    // Prohibit copy constructor & assignment operator
    CBioseqIndex (const CBioseqIndex&);
    CBioseqIndex& operator= (const CBioseqIndex&);

public:
    // Descriptor exploration iterator
    template<typename _Pred> int IterateDescriptors (_Pred m);

    // Feature exploration iterator
    template<typename _Pred> int IterateFeatures (_Pred m);

    // Getters
    CBioseq_Handle GetBioseqHandle (void) const { return m_bsh; }
    const CBioseq& GetBioseq (void) const { return m_bsp; }
    CSeqEntryIndex& GetSeqEntryIndex (void) const { return m_enx; }
    CRef<CScope> GetScope (void) const { return m_scope; }
    feature::CFeatTree& GetFeatTree (void) { return m_featTree; }

    const string& GetAccession (void) const { return m_Accession; }

    // Seq-inst fields
    bool IsNA (void) const {  return m_IsNA; }
    bool IsAA (void) const { return m_IsAA; }
    CSeq_inst::TTopology GetTopology (void) const { return m_topology; }
    CSeq_inst::TLength GetLength (void) const { return m_length; }

    bool IsDelta (void) const { return m_IsDelta; }
    bool IsVirtual (void) const { return m_IsVirtual; }
    bool IsMap (void) const { return m_IsMap; }

    // Most important descriptor fields

    const string& GetTitle (void) const { return m_title; }

    CConstRef<CMolInfo> GetMolInfo (void) const { return m_molInfo; }
    CMolInfo::TBiomol GetBiomol (void) const { return m_biomol; }
    CMolInfo::TTech GetTech (void) const { return m_tech; }
    CMolInfo::TCompleteness GetCompleteness (void) const { return m_completeness; }

    CConstRef<CBioSource> GetBioSource (void) const { return m_bioSource; }

private:
    // Common descriptor collection, delayed until actually needed
    void x_InitDescs (void);

    // Common feature collection, delayed until actually needed
    void x_InitFeats (void);

    // Map from GetBestGene result to CFeatureIndex object
    CRef<CFeatureIndex> GetFeatIndex (CMappedFeat mf);

private:
    CBioseq_Handle m_bsh;
    const CBioseq& m_bsp;
    CSeqEntryIndex& m_enx;
    CRef<CScope> m_scope;

    bool m_descsInitialized;
    vector<CRef<CDescriptorIndex>> m_sdxList;

    bool m_featsInitialized;
    vector<CRef<CFeatureIndex>> m_sfxList;
    feature::CFeatTree m_featTree;

    // CFeatIndex from CMappedFeat for use with GetBestGene
    typedef map<CMappedFeat, CRef<CFeatureIndex> > TFeatIndexMap;
    TFeatIndexMap m_featIndexMap;

private:
    // Seq-id field
    string m_Accession;

    // Seq-inst fields
    bool m_IsNA;
    bool m_IsAA;
    CSeq_inst::TTopology m_topology;
    CSeq_inst::TLength m_length;

    bool m_IsDelta;
    bool m_IsVirtual;
    bool m_IsMap;

    // Instantiated title
    string m_title;

    // MolInfo fields
    CConstRef<CMolInfo> m_molInfo;
    CMolInfo::TBiomol m_biomol;
    CMolInfo::TTech m_tech;
    CMolInfo::TCompleteness m_completeness;

    // BioSource fields
    CConstRef<CBioSource> m_bioSource;

    // User object fields
    bool m_forceOnlyNearFeats;

    // Derived policy flags
    bool m_onlyNearFeats;
};


// CDescriptorIndex
//
// CDescriptorIndex stores information about an indexed descriptor
class NCBI_XOBJUTIL_EXPORT CDescriptorIndex : public CObject
{
    friend class CBioseqIndex;
public:
    // Constructor
    CDescriptorIndex (const CSeqdesc& sd, CBioseqIndex& bsx);

    // Destructor
    ~CDescriptorIndex (void);

private:
    // Prohibit copy constructor & assignment operator
    CDescriptorIndex (const CDescriptorIndex&);
    CDescriptorIndex& operator= (const CDescriptorIndex&);

public:
    // Getters
    const CSeqdesc& GetSeqDesc (void) const { return m_sd; }
    CBioseqIndex& GetBioseqIndex (void) const { return m_bsx; }

    // Get descriptor subtype (e.g., CSeqdesc::e_Molinfo)
    CSeqdesc::E_Choice GetSubtype (void) const { return m_subtype; }

private:
    const CSeqdesc& m_sd;
    CBioseqIndex& m_bsx;

    CSeqdesc::E_Choice m_subtype;
};


// CFeatureIndex
//
// CFeatureIndex stores information about an indexed feature
//
// GetBestGene returns the best (referenced or overlapping) gene, if available:
//
//   sfx.GetBestGene([this](CFeatureIndex& gnx) {
//       ...
//   });
//
class NCBI_XOBJUTIL_EXPORT CFeatureIndex : public CObject
{
    friend class CBioseqIndex;
public:
    // Constructor
    CFeatureIndex (CSeq_feat_Handle sfh, const CMappedFeat mf, CConstRef<CSeq_loc> fl, CBioseqIndex& bsx);

    // Destructor
    ~CFeatureIndex (void);

private:
    // Prohibit copy constructor & assignment operator
    CFeatureIndex (const CFeatureIndex&);
    CFeatureIndex& operator= (const CFeatureIndex&);

public:
    // Getters
    CSeq_feat_Handle GetSeqFeatHandle (void) const { return m_sfh; }
    const CMappedFeat GetMappedFeat (void) const { return m_mf; }
    CConstRef<CSeq_loc> GetMappedLocation (void) const { return m_fl; }
    CBioseqIndex& GetBioseqIndex (void) const { return m_bsx; }

    // Get feature subtype (e.g. CSeqFeatData::eSubtype_mrna)
    CSeqFeatData::ESubtype GetSubtype (void) const { return m_subtype; }

    // Map from feature to CFeatureIndex for best gene using CFeatTree in parent CBioseqIndex
    template<typename _Pred> bool GetBestGene (_Pred m);

private:
    CSeq_feat_Handle m_sfh;
    const CMappedFeat m_mf;
    CConstRef<CSeq_loc> m_fl;
    CBioseqIndex& m_bsx;

    CSeqFeatData::ESubtype m_subtype;
};


// Inline lambda function implementations

// Visit CBioseqIndex objects for all Bioseqs
template<typename _Pred>
inline
int CSeqEntryIndex::IterateBioseqs (_Pred m)

{
    int count = 0;
    for (auto& bsx : m_bsxList) {
        m(*bsx);
        count++;
    }
    return count;
}

// Find CBioseqIndex for first Bioseq
template<typename _Pred>
inline
bool CSeqEntryIndex::ProcessBioseq (_Pred m)

{
    for (auto& bsx : m_bsxList) {
        m(*bsx);
        return true;
    }
    return false;
}

// Find CBioseqIndex for Bioseq by position
template<typename _Pred>
inline
bool CSeqEntryIndex::ProcessBioseq (int n, _Pred m)

{
    for (auto& bsx : m_bsxList) {
        n--;
        if (n > 0) continue;
        m(*bsx);
        return true;
    }
    return false;
}

// Find CBioseqIndex for Bioseq by accession
template<typename _Pred>
inline
bool CSeqEntryIndex::ProcessBioseq (const string& accn, _Pred m)

{
    TAccnIndexMap::iterator it = m_accnIndexMap.find(accn);
    if (it != m_accnIndexMap.end()) {
        CRef<CBioseqIndex> bsx = it->second;
        m(*bsx);
        return true;
    }
    return false;
}

// Create CBioseqIndex and delta sequence for Bioseq subregion
template<typename _Pred>
inline
bool CSeqEntryIndex::ProcessBioseq (const CSeq_loc& loc, _Pred m)

{
    CRef<CBioseqIndex> bsx = x_DeltaIndex(loc);

    if (bsx) {
        m(*bsx);
        return true;
    }
    return false;
}

// Create CBioseqIndex and delta sequence for Bioseq subrange
template<typename _Pred>
inline
bool CSeqEntryIndex::ProcessBioseq (const string& accn, int from, int to, bool rev_comp, _Pred m)

{
    CRef<CSeq_loc> loc = x_SubRangeLoc(accn, from, to, rev_comp);

    if (loc) {
        return ProcessBioseq(*loc, m);
    }
    return false;
}

// Visit CDescriptorIndex objects for all descriptors
template<typename _Pred>
inline
int CBioseqIndex::IterateDescriptors (_Pred m)

{
    // Delay descriptor collection until first request
    if (! m_descsInitialized) {
        x_InitDescs();
    }

    int count = 0;
    for (auto& sdx : m_sdxList) {
        count++;
        m(*sdx);
    }
    return count;
}

// Visit CFeatureIndex objects for all features
template<typename _Pred>
inline
int CBioseqIndex::IterateFeatures (_Pred m)

{
    // Delay feature collection until first request
    if (! m_featsInitialized) {
        x_InitFeats();
    }

    int count = 0;
    for (auto& sfx : m_sfxList) {
        count++;
        m(*sfx);
    }
    return count;
}

// Find CFeatureIndex object for best gene using internal CFeatTree
template<typename _Pred>
inline
bool CFeatureIndex::GetBestGene (_Pred m)

{
    try {
        CMappedFeat best;
        CBioseqIndex& bsx = GetBioseqIndex();
        best = feature::GetBestGeneForFeat(m_mf, &bsx.m_featTree, 0,
                                           feature::CFeatTree::eBestGene_AllowOverlapped);
        if (best) {
            CRef<CFeatureIndex> sfx = bsx.GetFeatIndex(best);
            if (sfx) {
                m(*sfx);
                return true;
            }
        }
    } catch (CException&) {}
    return false;
}


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* FEATURE_INDEXER__HPP */
