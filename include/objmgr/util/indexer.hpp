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
#include <objmgr/seq_vector.hpp>
#include <objmgr/util/feature.hpp>
#include <objmgr/util/create_defline.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


// look-ahead class names
class CSeqEntryIndex;
class CSeqsetIndex;
class CBioseqIndex;
class CDescriptorIndex;
class CFeatureIndex;


// CSeqEntryIndex
//
// CSeqEntryIndex is the master, top-level Seq-entry exploration organizer.  A variable
// is created, with an optional fetch policy, an optional feature exploration depth (for
// the default adaptive fetch policy), and initialized with the top-level object:
//
//   CSeqEntryIndex idx(*m_entry, CSeqEntryIndex::fAdaptive);
//
// A Seq-entry wrapper is created if the top-level object is a Bioseq or Bioseq-set.
// Bioseqs within the Seq-entry are then indexed and added to a vector of CBioseqIndex.
//
// Bioseqs are explored with IterateBioseqs, or selected individually by GetBioseqIndex
// (given an accession, index number, or subregion):
//
//   idx.IterateBioseqs("U54469", [this](CBioseqIndex& bsx) {
//       ...
//   });
//
// The embedded lambda function statements are executed for each selected Bioseq.
//
// Internal indexing objects (i.e., CSeqsetIndex, CBioseqIndex, CDescriptorIndex, and
// CFeatureIndex) are generated by the indexing process, and should not be created by
// the application.
class NCBI_XOBJUTIL_EXPORT CSeqEntryIndex : public CObjectEx
{
public:

    enum EPolicy {
        // far feature fetch policy
        fAdaptive = 0,
        fInternal = 1,
        fExternal = 2,
        fExhaustive = 3
    };

public:
    // Constructors take the top-level object
    CSeqEntryIndex (CSeq_entry& topsep, EPolicy policy = fAdaptive, int depth = -1);
    CSeqEntryIndex (CBioseq_set& seqset, EPolicy policy = fAdaptive, int depth = -1);
    CSeqEntryIndex (CBioseq& bioseq, EPolicy policy = fAdaptive, int depth = -1);
    CSeqEntryIndex (CSeq_submit& submit, EPolicy policy = fAdaptive, int depth = -1);

    // Specialized constructors for streaming through release files, one component at a time

    // Submit-block obtained from top of Seq-submit release file
    CSeqEntryIndex (CSeq_entry& topsep, CSubmit_block &sblock, EPolicy policy = fAdaptive, int depth = -1);
    // Seq-descr chain obtained from top of Bioseq-set release file
    CSeqEntryIndex (CSeq_entry& topsep, CSeq_descr &descr, EPolicy policy = fAdaptive, int depth = -1);

private:
    // Prohibit copy constructor & assignment operator
    CSeqEntryIndex (const CSeqEntryIndex&) = delete;
    CSeqEntryIndex& operator= (const CSeqEntryIndex&) = delete;

public:
    // Bioseq exploration iterator
    template<typename Fnc> size_t IterateBioseqs (Fnc m);

    // Get first Bioseq index
    CRef<CBioseqIndex> GetBioseqIndex (void);
    // Get Nth Bioseq index
    CRef<CBioseqIndex> GetBioseqIndex (int n);
    // Get Bioseq index by accession
    CRef<CBioseqIndex> GetBioseqIndex (const string& accn);

    // Subrange processing creates a new CBioseqIndex around a temporary delta Bioseq
    // Get Bioseq index by sublocation
    CRef<CBioseqIndex> GetBioseqIndex (const CSeq_loc& loc);
    // Get Bioseq index by subrange
    CRef<CBioseqIndex> GetBioseqIndex (const string& accn, int from, int to, bool rev_comp);
    CRef<CBioseqIndex> GetBioseqIndex (int from, int to, bool rev_comp);

    // Seqset exploration iterator
    template<typename Fnc> size_t IterateSeqsets (Fnc m);

    // Getters
    CRef<CObjectManager> GetObjectManager (void) const { return m_Objmgr; }
    CRef<CScope> GetScope (void) const { return m_Scope; }
    CSeq_entry_Handle GetTopSEH (void) const { return m_Tseh; }
    CConstRef<CSeq_entry> GetTopSEP (void) const { return m_Tsep; }
    CConstRef<CSubmit_block> GetSbtBlk (void) const { return m_SbtBlk; }
    CConstRef<CSeq_descr> GetTopDescr (void) const { return m_TopDescr; }

    const vector<CRef<CBioseqIndex>>& GetBioseqIndices(void);

    // Check all Bioseqs for failure to fetch remote sequence components or feature annotation
    bool IsFetchFailure(void);

private:
    // Common initialization function called by each Initialize variant
    void x_Init (void);

    // Recursive exploration to populate vector of index objects for Bioseqs in Seq-entry
    void x_InitSeqs (const CSeq_entry& sep, CRef<CSeqsetIndex> prnt);

    CRef<CSeq_id> x_MakeUniqueId(void);

    // Create delta sequence referring to location, using temporary local ID
    CRef<CBioseqIndex> x_DeltaIndex(const CSeq_loc& loc);

    // Create location from range, to use in x_DeltaIndex
    CConstRef<CSeq_loc> x_SubRangeLoc(const string& accn, int from, int to, bool rev_comp);

private:
    CRef<CObjectManager> m_Objmgr;
    CRef<CScope> m_Scope;
    CSeq_entry_Handle m_Tseh;

    CConstRef<CSeq_entry> m_Tsep;
    CConstRef<CSubmit_block> m_SbtBlk;
    CConstRef<CSeq_descr> m_TopDescr;

    EPolicy m_Policy;
    int m_Depth;

    vector<CRef<CBioseqIndex>> m_BsxList;

    // map from accession string to CBioseqIndex object
    typedef map<string, CRef<CBioseqIndex> > TAccnIndexMap;
    TAccnIndexMap m_AccnIndexMap;

    vector<CRef<CSeqsetIndex>> m_SsxList;

    mutable CAtomicCounter m_Counter;
};


// CSeqsetIndex
//
// CSeqsetIndex stores information about an element in the Bioseq-set hierarchy
class NCBI_XOBJUTIL_EXPORT CSeqsetIndex : public CObjectEx
{
public:
    // Constructor
    CSeqsetIndex (CBioseq_set_Handle ssh,
                  const CBioseq_set& bssp,
                  CRef<CSeqsetIndex> prnt);

private:
    // Prohibit copy constructor & assignment operator
    CSeqsetIndex (const CSeqsetIndex&) = delete;
    CSeqsetIndex& operator= (const CSeqsetIndex&) = delete;

public:
    // Getters
    CBioseq_set_Handle GetSeqsetHandle (void) const { return m_Ssh; }
    const CBioseq_set& GetSeqset (void) const { return m_Bssp; }
    CRef<CSeqsetIndex> GetParent (void) const { return m_Prnt; }

    CBioseq_set::TClass GetClass (void) const { return m_Class; }

private:
    CBioseq_set_Handle m_Ssh;
    const CBioseq_set& m_Bssp;
    CRef<CSeqsetIndex> m_Prnt;

    CBioseq_set::TClass m_Class;
};


// CBioseqIndex
//
// CBioseqIndex is the exploration organizer for a given Bioseq.  It provides methods to
// obtain descriptors and iterate through features that apply to the Bioseq.  (These are
// stored in vectors, which are initialized upon first request.)
//
// CBioseqIndex also maintains a CFeatTree for its Bioseq, used to find the best gene for
// each feature.
//
// Descriptors are explored with:
//
//   bsx.IterateDescriptors([this](CDescriptorIndex& sdx) {
//       ...
//   });
//
// and are presented based on the order of the descriptor chain hierarchy, starting with
// descriptors packaged on the Bioseq, then on its parent Bioseq-set, etc.
//
// Features are explored with:
//
//   bsx.IterateFeatures([this](CFeatureIndex& sfx) {
//       ...
//   });
//
// and are presented in order of biological position along the parent sequence.
//
// Fetching external features uses SAnnotSelector adaptive depth unless explicitly overridden.
class NCBI_XOBJUTIL_EXPORT CBioseqIndex : public CObjectEx
{
public:
    // Constructor
    CBioseqIndex (CBioseq_Handle bsh,
                  const CBioseq& bsp,
                  CBioseq_Handle obsh,
                  CRef<CSeqsetIndex> prnt,
                  CSeq_entry_Handle tseh,
                  CRef<CScope> scope,
                  CSeqEntryIndex::EPolicy policy,
                  int depth,
                  bool surrogate);

private:
    // Prohibit copy constructor & assignment operator
    CBioseqIndex (const CBioseqIndex&) = delete;
    CBioseqIndex& operator= (const CBioseqIndex&) = delete;

public:
    // Descriptor exploration iterator
    template<typename Fnc> size_t IterateDescriptors (Fnc m);

    // Feature exploration iterator
    template<typename Fnc> size_t IterateFeatures (Fnc m);

    // Getters
    CBioseq_Handle GetBioseqHandle (void) const { return m_Bsh; }
    const CBioseq& GetBioseq (void) const { return m_Bsp; }
    CBioseq_Handle GetOrigBioseqHandle (void) const { return m_OrigBsh; }
    CRef<CSeqsetIndex> GetParent (void) const { return m_Prnt; }
    CRef<CScope> GetScope (void) const { return m_Scope; }
    feature::CFeatTree& GetFeatTree (void) { return m_FeatTree; }
    CRef<CSeqVector> GetSeqVector (void) const { return m_SeqVec; }

    const string& GetAccession (void) const { return m_Accession; }

    // Seq-inst fields
    bool IsNA (void) const {  return m_IsNA; }
    bool IsAA (void) const { return m_IsAA; }
    CSeq_inst::TTopology GetTopology (void) const { return m_Topology; }
    CSeq_inst::TLength GetLength (void) const { return m_Length; }

    bool IsDelta (void) const { return m_IsDelta; }
    bool IsVirtual (void) const { return m_IsVirtual; }
    bool IsMap (void) const { return m_IsMap; }

    // Most important descriptor fields

    const string& GetTitle (void);

    CConstRef<CMolInfo> GetMolInfo (void);
    CMolInfo::TBiomol GetBiomol (void);
    CMolInfo::TTech GetTech (void);
    CMolInfo::TCompleteness GetCompleteness (void);

    CConstRef<CBioSource> GetBioSource (void);
    const string& GetTaxname (void);

    // Run definition line generator
    const string& GetDefline (sequence::CDeflineGenerator::TUserFlags = 0);

    // Get sequence letters from Bioseq
    string GetSequence (void);
    void GetSequence (string& buffer);
    // Get sequence letters from Bioseq subrange
    string GetSequence (int from, int to);
    void GetSequence (int from, int to, string& buffer);

    // Map from GetBestGene result to CFeatureIndex object
    CRef<CFeatureIndex> GetFeatIndex (CMappedFeat mf);

    const vector<CRef<CDescriptorIndex>>& GetDescriptorIndices(void);

    const vector<CRef<CFeatureIndex>>& GetFeatureIndices(void);

    // Flag to indicate failure to fetch remote sequence components or feature annotation
    bool IsFetchFailure (void) const { return m_FetchFailure; }

    void SetFetchFailure (bool fails) { m_FetchFailure = fails; }

private:
    // Common descriptor collection, delayed until actually needed
    void x_InitDescs (void);

    // Common feature collection, delayed until actually needed
    void x_InitFeats (void);

private:
    CBioseq_Handle m_Bsh;
    const CBioseq& m_Bsp;
    CBioseq_Handle m_OrigBsh;
    CRef<CSeqsetIndex> m_Prnt;
    CSeq_entry_Handle m_Tseh;
    CRef<CScope> m_Scope;

    bool m_DescsInitialized;
    vector<CRef<CDescriptorIndex>> m_SdxList;

    bool m_FeatsInitialized;
    vector<CRef<CFeatureIndex>> m_SfxList;
    feature::CFeatTree m_FeatTree;

    // CFeatIndex from CMappedFeat for use with GetBestGene
    typedef map<CMappedFeat, CRef<CFeatureIndex> > TFeatIndexMap;
    TFeatIndexMap m_FeatIndexMap;

    CRef<CSeqVector> m_SeqVec;

    CSeqEntryIndex::EPolicy m_Policy;
    int m_Depth;

    bool m_FetchFailure;

private:
    // Seq-id field
    string m_Accession;

    // Seq-inst fields
    bool m_IsNA;
    bool m_IsAA;
    CSeq_inst::TTopology m_Topology;
    CSeq_inst::TLength m_Length;

    bool m_IsDelta;
    bool m_IsVirtual;
    bool m_IsMap;

    // Instantiated title
    string m_Title;

    // MolInfo fields
    CConstRef<CMolInfo> m_MolInfo;
    CMolInfo::TBiomol m_Biomol;
    CMolInfo::TTech m_Tech;
    CMolInfo::TCompleteness m_Completeness;

    // BioSource fields
    CConstRef<CBioSource> m_BioSource;
    string m_Taxname;

    // User object fields
    bool m_ForceOnlyNearFeats;

    bool m_Surrogate;

    // Generated definition line
    string m_Defline;
    sequence::CDeflineGenerator::TUserFlags m_Dlflags;
};


// CDescriptorIndex
//
// CDescriptorIndex stores information about an indexed descriptor
class NCBI_XOBJUTIL_EXPORT CDescriptorIndex : public CObjectEx
{
public:
    // Constructor
    CDescriptorIndex (const CSeqdesc& sd,
                      CBioseqIndex& bsx);

private:
    // Prohibit copy constructor & assignment operator
    CDescriptorIndex (const CDescriptorIndex&) = delete;
    CDescriptorIndex& operator= (const CDescriptorIndex&) = delete;

public:
    // Getters
    const CSeqdesc& GetSeqDesc (void) const { return m_Sd; }

    // Get parent Bioseq index
    CWeakRef<CBioseqIndex> GetBioseqIndex (void) const { return m_Bsx; }

    // Get descriptor subtype (e.g., CSeqdesc::e_Molinfo)
    CSeqdesc::E_Choice GetSubtype (void) const { return m_Subtype; }

private:
    const CSeqdesc& m_Sd;
    CWeakRef<CBioseqIndex> m_Bsx;

    CSeqdesc::E_Choice m_Subtype;
};


// CFeatureIndex
//
// CFeatureIndex stores information about an indexed feature
class NCBI_XOBJUTIL_EXPORT CFeatureIndex : public CObjectEx
{
public:
    // Constructor
    CFeatureIndex (CSeq_feat_Handle sfh,
                   const CMappedFeat mf,
                   CBioseqIndex& bsx);

private:
    // Prohibit copy constructor & assignment operator
    CFeatureIndex (const CFeatureIndex&) = delete;
    CFeatureIndex& operator= (const CFeatureIndex&) = delete;

public:
    // Getters
    CSeq_feat_Handle GetSeqFeatHandle (void) const { return m_Sfh; }
    const CMappedFeat GetMappedFeat (void) const { return m_Mf; }
    CRef<CSeqVector> GetSeqVector (void) const { return m_SeqVec; }

    CConstRef<CSeq_loc> GetMappedLocation(void);

    // Get parent Bioseq index
    CWeakRef<CBioseqIndex> GetBioseqIndex (void) const { return m_Bsx; }

    // Get feature subtype (e.g. CSeqFeatData::eSubtype_mrna)
    CSeqFeatData::ESubtype GetSubtype (void) const { return m_Subtype; }

    // Get sequence letters under feature intervals
    string GetSequence (void);
    void GetSequence (string& buffer);
    // Get sequence letters under feature subrange
    string GetSequence (int from, int to);
    void GetSequence (int from, int to, string& buffer);

    // Map from feature to CFeatureIndex for best gene using CFeatTree in parent CBioseqIndex
    CRef<CFeatureIndex> GetBestGene (void);

private:
    void SetFetchFailure (bool fails);

private:
    CSeq_feat_Handle m_Sfh;
    const CMappedFeat m_Mf;
    CConstRef<CSeq_loc> m_Fl;
    CRef<CSeqVector> m_SeqVec;
    CWeakRef<CBioseqIndex> m_Bsx;

    CSeqFeatData::ESubtype m_Subtype;
};


// CWordPairIndexer
//
// CWordPairIndexer generates normalized terms and adjacent word pairs for Entrez indexing
class NCBI_XOBJUTIL_EXPORT CWordPairIndexer
{
public:
    // Constructor
    CWordPairIndexer (void) { }

private:
    // Prohibit copy constructor & assignment operator
    CWordPairIndexer (const CWordPairIndexer&) = delete;
    CWordPairIndexer& operator= (const CWordPairIndexer&) = delete;

public:
    void PopulateWordPairIndex (string str);

    template<typename Fnc> void IterateNorm (Fnc m);
    template<typename Fnc> void IteratePair (Fnc m);

public:
    static string ConvertUTF8ToAscii(const string& str);
    static string TrimPunctuation (const string& str);
    static string TrimMixedContent (const string& str);
    static bool IsStopWord(const string& str);

    const vector<string>& GetNorm (void) const { return m_Norm; }
    const vector<string>& GetPair (void) const { return m_Pair; }

private:
    string x_AddToWordPairIndex (string item, string prev);

    vector<string> m_Norm;
    vector<string> m_Pair;
};


// Inline lambda function implementations

// Visit CBioseqIndex objects for all Bioseqs
template<typename Fnc>
inline
size_t CSeqEntryIndex::IterateBioseqs (Fnc m)

{
    int count = 0;
    for (auto& bsx : m_BsxList) {
        m(*bsx);
        count++;
    }
    return count;
}

// Visit CSeqsetIndex objects for all Seqsets
template<typename Fnc>
inline
size_t CSeqEntryIndex::IterateSeqsets (Fnc m)

{
    int count = 0;
    for (auto& ssx : m_SsxList) {
        m(*ssx);
        count++;
    }
    return count;
}

// Visit CDescriptorIndex objects for all descriptors
template<typename Fnc>
inline
size_t CBioseqIndex::IterateDescriptors (Fnc m)

{
    int count = 0;
    try {
        // Delay descriptor collection until first request
        if (! m_DescsInitialized) {
            x_InitDescs();
        }

        for (auto& sdx : m_SdxList) {
            count++;
            m(*sdx);
        }
    }
    catch (CException& e) {
        LOG_POST(Error << "Error in CBioseqIndex::IterateDescriptors: " << e.what());
    }
    return count;
}

// Visit CFeatureIndex objects for all features
template<typename Fnc>
inline
size_t CBioseqIndex::IterateFeatures (Fnc m)

{
    int count = 0;
    try {
        // Delay feature collection until first request
        if (! m_FeatsInitialized) {
            x_InitFeats();
        }

        for (auto& sfx : m_SfxList) {
            count++;
            m(*sfx);
        }
    }
    catch (CException& e) {
        LOG_POST(Error << "Error in CBioseqIndex::IterateFeatures: " << e.what());
    }
    return count;
}

template<typename Fnc>
inline
void CWordPairIndexer::IterateNorm (Fnc m)

{
    for (auto& str : m_Norm) {
        m(str);
    }
}

template<typename Fnc>
inline
void CWordPairIndexer::IteratePair (Fnc m)

{
    for (auto& str : m_Pair) {
        m(str);
    }
}


END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* FEATURE_INDEXER__HPP */
