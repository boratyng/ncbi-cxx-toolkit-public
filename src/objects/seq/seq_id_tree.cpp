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
* Author: Aleksey Grichenko, Eugene Vasilchenko
*
* File Description:
*   Seq-id mapper for Object Manager
*
*/

#include <objmgr/impl/seq_id_tree.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

/////////////////////////////////////////////////////////////////////////////
// to make map<string, something> case-insensitive

struct seqid_string_less
{
    bool operator()(const string& s1, const string& s2) const
    {
        return (NStr::CompareNocase(s1, s2) < 0);
    }
};

////////////////////////////////////////////////////////////////////
//
//  CSeq_id_***_Tree::
//
//    Seq-id sub-type specific trees
//

CSeq_id_Which_Tree::CSeq_id_Which_Tree(void)
{
}


CSeq_id_Which_Tree::~CSeq_id_Which_Tree(void)
{
}


bool CSeq_id_Which_Tree::IsBetterVersion(const CSeq_id_Handle& /*h1*/,
                                         const CSeq_id_Handle& /*h2*/) const
{
    return false; // No id version by default
}


CSeq_id_Info* CSeq_id_Which_Tree::CreateInfo(const CSeq_id& id)
{
    CRef<CSeq_id> id_ref(new CSeq_id);
    id_ref->Assign(id);
    return new CSeq_id_Info(id_ref);
}


void CSeq_id_Which_Tree::DropInfo(CSeq_id_Info* info)
{
    if ( info->m_Counter.Get() == 0 ) {
        TWriteLockGuard guard(m_TreeLock);
        if ( info->m_Counter.Get() == 0 ) {
            x_Unindex(info);
            delete info;
        }
    }
}


void CSeq_id_Which_Tree::Initialize(vector<CRef<CSeq_id_Which_Tree> >& v)
{
    v.resize(CSeq_id::e_Tpd+1);
    v[CSeq_id::e_not_set].Reset(new CSeq_id_not_set_Tree);
    v[CSeq_id::e_Local].Reset(new CSeq_id_Local_Tree);
    v[CSeq_id::e_Gibbsq].Reset(new CSeq_id_Gibbsq_Tree);
    v[CSeq_id::e_Gibbmt].Reset(new CSeq_id_Gibbmt_Tree);
    v[CSeq_id::e_Giim].Reset(new CSeq_id_Giim_Tree);
    // These three types share the same accessions space
    CRef<CSeq_id_Which_Tree> gb(new CSeq_id_GB_Tree);
    v[CSeq_id::e_Genbank] = gb;
    v[CSeq_id::e_Embl] = gb;
    v[CSeq_id::e_Ddbj] = gb;
    v[CSeq_id::e_Pir].Reset(new CSeq_id_Pir_Tree);
    v[CSeq_id::e_Swissprot].Reset(new CSeq_id_Swissprot_Tree);
    v[CSeq_id::e_Patent].Reset(new CSeq_id_Patent_Tree);
    v[CSeq_id::e_Other].Reset(new CSeq_id_Other_Tree);
    v[CSeq_id::e_General].Reset(new CSeq_id_General_Tree);
    v[CSeq_id::e_Gi].Reset(new CSeq_id_Gi_Tree);
    // see above    v[CSeq_id::e_Ddbj] = gb;
    v[CSeq_id::e_Prf].Reset(new CSeq_id_Prf_Tree);
    v[CSeq_id::e_Pdb].Reset(new CSeq_id_PDB_Tree);
    v[CSeq_id::e_Tpg].Reset(new CSeq_id_Tpg_Tree);
    v[CSeq_id::e_Tpe].Reset(new CSeq_id_Tpe_Tree);
    v[CSeq_id::e_Tpd].Reset(new CSeq_id_Tpd_Tree);
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_not_set_Tree
/////////////////////////////////////////////////////////////////////////////

CSeq_id_not_set_Tree::CSeq_id_not_set_Tree(void)
    : m_Info(0)
{
}


CSeq_id_not_set_Tree::~CSeq_id_not_set_Tree(void)
{
}


bool CSeq_id_not_set_Tree::Empty(void) const
{
    return !m_Info;
}


inline
bool CSeq_id_not_set_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.Which() == CSeq_id::e_not_set);
    return id.Which() == CSeq_id::e_not_set;
}


CSeq_id_Handle CSeq_id_not_set_Tree::FindInfo(const CSeq_id& /*id*/) const
{
    //_ASSERT(x_Check(id));
    TReadLockGuard guard(m_TreeLock);
    return m_Info;
}


CSeq_id_Handle CSeq_id_not_set_Tree::FindOrCreate(const CSeq_id& id)
{
    TWriteLockGuard guard(m_TreeLock);
    if ( !m_Info ) {
        m_Info = CreateInfo(id);
    }
    return m_Info;
}


void CSeq_id_not_set_Tree::x_Unindex(CSeq_id_Info* info)
{
    x_Check(info->GetSeq_id());
    _ASSERT(info && info == m_Info);
    m_Info = 0;
}


void CSeq_id_not_set_Tree::FindMatch(const CSeq_id_Handle& id,
                                     TSeq_id_MatchList& id_list) const
{
    // Only one instance of each id
    _ASSERT(id && id == m_Info);
    id_list.push_back(id);
}


void CSeq_id_not_set_Tree::FindMatchStr(string sid,
                                        TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    if ( m_Info && sid.empty() ) {
        id_list.push_back(m_Info);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_int_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_int_Tree::CSeq_id_int_Tree(void)
{
}


CSeq_id_int_Tree::~CSeq_id_int_Tree(void)
{
}


bool CSeq_id_int_Tree::Empty(void) const
{
    return m_IntMap.empty();
}


CSeq_id_Handle CSeq_id_int_Tree::FindInfo(const CSeq_id& id) const
{
    x_Check(id);
    int value = x_Get(id);

    TReadLockGuard guard(m_TreeLock);
    TIntMap::const_iterator it = m_IntMap.find(value);
    if (it != m_IntMap.end()) {
        return it->second;
    }
    return 0;
}


CSeq_id_Handle CSeq_id_int_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT(x_Check(id));
    int value = x_Get(id);

    TWriteLockGuard guard(m_TreeLock);
    pair<TIntMap::iterator, bool> ins =
        m_IntMap.insert(TIntMap::value_type(value, 0));
    if ( ins.second ) {
        ins.first->second = CreateInfo(id);
    }
    return ins.first->second;
}


void CSeq_id_int_Tree::x_Unindex(CSeq_id_Info* info)
{
    _ASSERT(x_Check(info->GetSeq_id()));
    int value = x_Get(info->GetSeq_id());

    _VERIFY(m_IntMap.erase(value));
}


void CSeq_id_int_Tree::FindMatch(const CSeq_id_Handle& id,
                                 TSeq_id_MatchList& id_list) const
{
    // Only one instance of each int id
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    id_list.push_back(id);
}


void CSeq_id_int_Tree::FindMatchStr(string sid,
                                    TSeq_id_MatchList& id_list) const
{
    int value;
    try {
        value = NStr::StringToInt(sid);
    }
    catch (const CStringException& /*ignored*/) {
        // Not an integer value
        return;
    }
    TReadLockGuard guard(m_TreeLock);
    TIntMap::const_iterator it = m_IntMap.find(value);
    if (it != m_IntMap.end()) {
        id_list.push_back(it->second);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Gi_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Gi_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsGi());
    return id.IsGi();
}


int CSeq_id_Gi_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsGi());
    return id.GetGi();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Gibbsq_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Gibbsq_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsGibbsq());
    return id.IsGibbsq();
}


int CSeq_id_Gibbsq_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsGibbsq());
    return id.GetGibbsq();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Gibbmt_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Gibbmt_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsGibbmt());
    return id.IsGibbmt();
}


int CSeq_id_Gibbmt_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsGibbmt());
    return id.GetGibbmt();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Textseq_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_Textseq_Tree::CSeq_id_Textseq_Tree(void)
{
}


CSeq_id_Textseq_Tree::~CSeq_id_Textseq_Tree(void)
{
}


bool CSeq_id_Textseq_Tree::Empty(void) const
{
    return m_ByName.empty() && m_ByAccession.empty();
}


CSeq_id_Info*
CSeq_id_Textseq_Tree::x_FindVersionEqual(const TVersions& ver_list,
                                         const CTextseq_id& tid) const
{
    ITERATE(TVersions, vit, ver_list) {
        const CTextseq_id& tid_it = x_Get((*vit)->GetSeq_id());
        if (tid.IsSetVersion()  &&  tid_it.IsSetVersion()) {
            // Compare versions
            if (tid.GetVersion() == tid_it.GetVersion()) {
                return *vit;
            }
        }
        else if (tid.IsSetRelease()  &&  tid_it.IsSetRelease()) {
            // Compare releases if no version specified
            if (tid.GetRelease() == tid_it.GetRelease()) {
                return *vit;
            }
        }
        else if (!tid.IsSetVersion()  &&  !tid.IsSetRelease()  &&
            !tid_it.IsSetVersion()  &&  !tid_it.IsSetRelease()) {
            // No version/release for both seq-ids
            return *vit;
        }
    }
    return 0;
}


CSeq_id_Info* CSeq_id_Textseq_Tree::x_FindInfo(const CTextseq_id& tid) const
{
    TStringMap::const_iterator it;
    if ( tid.IsSetAccession() ) {
        it = m_ByAccession.find(tid.GetAccession());
        if (it == m_ByAccession.end()) {
            return 0;
        }
    }
    else if ( tid.IsSetName() ) {
        it = m_ByName.find(tid.GetName());
        if (it == m_ByName.end()) {
            return 0;
        }
    }
    else {
        return 0;
    }
    return x_FindVersionEqual(it->second, tid);
}


CSeq_id_Handle CSeq_id_Textseq_Tree::FindInfo(const CSeq_id& id) const
{
    // Note: if a record is found by accession, no name is checked
    // even if it is also set.
    x_Check(id);
    const CTextseq_id& tid = x_Get(id);
    // Can not compare if no accession given
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(tid);
}

CSeq_id_Handle CSeq_id_Textseq_Tree::FindOrCreate(const CSeq_id& id)
{
    x_Check(id);
    const CTextseq_id& tid = x_Get(id);
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(tid);
    if ( !info ) {
        info = CreateInfo(id);
        if ( tid.IsSetAccession() ) {
            TVersions& ver = m_ByAccession[tid.GetAccession()];
            ITERATE(TVersions, vit, ver) {
                _ASSERT(!x_Get((*vit)->GetSeq_id()).Equals(tid));
            }
            ver.push_back(info);
        }
        if ( tid.IsSetName() ) {
            TVersions& ver = m_ByName[tid.GetName()];
            ITERATE(TVersions, vit, ver) {
                _ASSERT(!x_Get((*vit)->GetSeq_id()).Equals(tid));
            }
            ver.push_back(info);
        }
    }
    return info;
}


void CSeq_id_Textseq_Tree::x_Unindex(CSeq_id_Info* info)
{
    x_Check(info->GetSeq_id());
    const CTextseq_id& tid = x_Get(info->GetSeq_id());
    if ( tid.IsSetAccession() ) {
        TStringMap::iterator it =
            m_ByAccession.find(tid.GetAccession());
        if (it != m_ByAccession.end()) {
            NON_CONST_ITERATE(TVersions, vit, it->second) {
                if (*vit == info) {
                    it->second.erase(vit);
                    break;
                }
            }
            if (it->second.empty())
                m_ByAccession.erase(it);
        }
    }
    if ( tid.IsSetName() ) {
        TStringMap::iterator it = m_ByName.find(tid.GetName());
        if (it != m_ByName.end()) {
            NON_CONST_ITERATE(TVersions, vit, it->second) {
                if (*vit == info) {
                    it->second.erase(vit);
                    break;
                }
            }
            if (it->second.empty())
                m_ByName.erase(it);
        }
    }
}


void CSeq_id_Textseq_Tree::x_FindVersionMatch(const TVersions& ver_list,
                                              const CTextseq_id& tid,
                                              TSeq_id_MatchList& id_list) const
{
    int ver = 0;
    string rel = "";
    if ( tid.IsSetVersion() )
        ver = tid.GetVersion();
    if ( tid.IsSetRelease() )
        rel = tid.GetRelease();
    ITERATE(TVersions, vit, ver_list) {
        int ver_it = 0;
        string rel_it = "";
        const CTextseq_id& vit_ref = x_Get((*vit)->GetSeq_id());
        if ( vit_ref.IsSetVersion() )
            ver_it = vit_ref.GetVersion();
        if ( vit_ref.IsSetRelease() )
            rel_it = vit_ref.GetRelease();
        if (ver == ver_it || ver == 0) {
            id_list.push_back(*vit);
        }
        else if (rel == rel_it || rel.empty()) {
            id_list.push_back(*vit);
        }
    }
}


void CSeq_id_Textseq_Tree::FindMatch(const CSeq_id_Handle& id,
                                     TSeq_id_MatchList& id_list) const
{
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    const CTextseq_id& tid = x_Get(id.GetSeqId());
    if ( tid.IsSetAccession() ) {
        TReadLockGuard guard(m_TreeLock);
        TStringMap::const_iterator it = m_ByAccession.find(tid.GetAccession());
        if (it != m_ByAccession.end()) {
            x_FindVersionMatch(it->second, tid, id_list);
        }
    }
    else if ( tid.IsSetName() ) {
        TReadLockGuard guard(m_TreeLock);
        TStringMap::const_iterator it = m_ByName.find(tid.GetName());
        if (it != m_ByName.end()) {
            x_FindVersionMatch(it->second, tid, id_list);
        }
    }
}


void CSeq_id_Textseq_Tree::FindMatchStr(string sid,
                                        TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    // ignore '.' in the search string - cut it out
    sid = sid.substr(0, sid.find('.'));
    // Find by accession
    TStringMap::const_iterator it = m_ByAccession.find(sid);
    if (it == m_ByAccession.end()) {
        it = m_ByName.find(sid);
        if (it == m_ByName.end())
            return;
    }
    ITERATE(TVersions, vit, it->second) {
        id_list.push_back(*vit);
    }
}


bool CSeq_id_Textseq_Tree::IsBetterVersion(const CSeq_id_Handle& h1,
                                           const CSeq_id_Handle& h2) const
{
    const CSeq_id& id1 = h1.GetSeqId();
    x_Check(id1);
    const CSeq_id& id2 = h2.GetSeqId();
    x_Check(id2);
    const CTextseq_id& tid1 = x_Get(id1);
    const CTextseq_id& tid2 = x_Get(id2);
    // Compare versions. If only one of the two ids has version,
    // consider it is better.
    if ( tid1.IsSetVersion() ) {
        if ( tid2.IsSetVersion() )
            return tid1.GetVersion() > tid2.GetVersion();
        else
            return true; // Only h1 has version
    }
    return false; // h1 has no version, so it can not be better than h2
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_GB_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_GB_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsGenbank()  ||  id.IsEmbl()  ||  id.IsDdbj());
    return id.IsGenbank()  ||  id.IsEmbl()  ||  id.IsDdbj();
}


const CTextseq_id& CSeq_id_GB_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsGenbank()  ||  id.IsEmbl()  ||  id.IsDdbj());
    switch ( id.Which() ) {
    case CSeq_id::e_Genbank:
        return id.GetGenbank();
    case CSeq_id::e_Embl:
        return id.GetEmbl();
    case CSeq_id::e_Ddbj:
        return id.GetDdbj();
    default:
        THROW1_TRACE(runtime_error,
            "CSeq_id_GB_Tree::x_Get() -- Invalid seq-id type");
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Pir_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Pir_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsPir());
    return id.IsPir();
}


const CTextseq_id& CSeq_id_Pir_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsPir());
    return id.GetPir();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Swissprot_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Swissprot_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsSwissprot());
    return id.IsSwissprot();
}


const CTextseq_id& CSeq_id_Swissprot_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsSwissprot());
    return id.GetSwissprot();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Prf_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Prf_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsPrf());
    return id.IsPrf();
}


const CTextseq_id& CSeq_id_Prf_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsPrf());
    return id.GetPrf();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Tpd_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Tpg_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsTpg());
    return id.IsTpg();
}


const CTextseq_id& CSeq_id_Tpg_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsTpg());
    return id.GetTpg();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Tpe_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Tpe_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsTpe());
    return id.IsTpe();
}


const CTextseq_id& CSeq_id_Tpe_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsTpe());
    return id.GetTpe();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Tpd_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Tpd_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsTpd());
    return id.IsTpd();
}


const CTextseq_id& CSeq_id_Tpd_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsTpd());
    return id.GetTpd();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Other_Tree
/////////////////////////////////////////////////////////////////////////////

bool CSeq_id_Other_Tree::x_Check(const CSeq_id& id) const
{
    _ASSERT(id.IsOther());
    return id.IsOther();
}


const CTextseq_id& CSeq_id_Other_Tree::x_Get(const CSeq_id& id) const
{
    _ASSERT(id.IsOther());
    return id.GetOther();
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Local_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_Local_Tree::CSeq_id_Local_Tree(void)
{
}


CSeq_id_Local_Tree::~CSeq_id_Local_Tree(void)
{
}


bool CSeq_id_Local_Tree::Empty(void) const
{
    return m_ByStr.empty() && m_ById.empty();
}


CSeq_id_Info* CSeq_id_Local_Tree::x_FindInfo(const CObject_id& oid) const
{
    if ( oid.IsStr() ) {
        TByStr::const_iterator it = m_ByStr.find(oid.GetStr());
        if (it != m_ByStr.end()) {
            return it->second;
        }
    }
    else if ( oid.IsId() ) {
        TById::const_iterator it = m_ById.find(oid.GetId());
        if (it != m_ById.end()) {
            return it->second;
        }
    }
    // Not found
    return 0;
}


CSeq_id_Handle CSeq_id_Local_Tree::FindInfo(const CSeq_id& id) const
{
    _ASSERT( id.IsLocal() );
    const CObject_id& oid = id.GetLocal();
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(oid);
}


CSeq_id_Handle CSeq_id_Local_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT(id.IsLocal());
    const CObject_id& oid = id.GetLocal();
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(oid);
    if ( !info ) {
        info = CreateInfo(id);
        if ( oid.IsStr() ) {
            _VERIFY(m_ByStr.insert(TByStr::value_type(oid.GetStr(),
                                                      info)).second);
        }
        else if ( oid.IsId() ) {
            _VERIFY(m_ById.insert(TById::value_type(oid.GetId(),
                                                    info)).second);
        }
        else {
            THROW1_TRACE(runtime_error,
                         "CSeq_id_Local_Tree::FindOrCreate() -- "
                         "Can not create index for an empty local seq-id");
        }
    }
    return info;
}


void CSeq_id_Local_Tree::x_Unindex(CSeq_id_Info* info)
{
    const CSeq_id& id = info->GetSeq_id();
    _ASSERT(id.IsLocal());
    const CObject_id& oid = id.GetLocal();
    if ( oid.IsStr() ) {
        _VERIFY(m_ByStr.erase(oid.GetStr()));
    }
    else if ( oid.IsId() ) {
        _VERIFY(m_ById.erase(oid.GetId()));
    }
}


void CSeq_id_Local_Tree::FindMatch(const CSeq_id_Handle& id,
                                   TSeq_id_MatchList& id_list) const
{
    // Only one entry can match each id
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    id_list.push_back(id);
}


void CSeq_id_Local_Tree::FindMatchStr(string sid,
                                      TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    // In any case search in strings
    TByStr::const_iterator str_it = m_ByStr.find(sid);
    if (str_it != m_ByStr.end()) {
        id_list.push_back(str_it->second);
    }
    else {
        try {
            int value = NStr::StringToInt(sid);
            TById::const_iterator int_it = m_ById.find(value);
            if (int_it != m_ById.end()) {
                id_list.push_back(int_it->second);
            }
        }
        catch (const CStringException& /*ignored*/) {
            // Not an integer value
            return;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_General_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_General_Tree::CSeq_id_General_Tree(void)
{
}


CSeq_id_General_Tree::~CSeq_id_General_Tree(void)
{
}


bool CSeq_id_General_Tree::Empty(void) const
{
    return m_DbMap.empty();
}


CSeq_id_Info* CSeq_id_General_Tree::x_FindInfo(const CDbtag& dbid) const
{
    TDbMap::const_iterator db = m_DbMap.find(dbid.GetDb());
    if (db == m_DbMap.end())
        return 0;
    const STagMap& tm = db->second;
    const CObject_id& oid = dbid.GetTag();
    if ( oid.IsStr() ) {
        STagMap::TByStr::const_iterator it = tm.m_ByStr.find(oid.GetStr());
        if (it != tm.m_ByStr.end()) {
            return it->second;
        }
    }
    else if ( oid.IsId() ) {
        STagMap::TById::const_iterator it = tm.m_ById.find(oid.GetId());
        if (it != tm.m_ById.end()) {
            return it->second;
        }
    }
    // Not found
    return 0;
}


CSeq_id_Handle CSeq_id_General_Tree::FindInfo(const CSeq_id& id) const
{
    _ASSERT( id.IsGeneral() );
    const CDbtag& dbid = id.GetGeneral();
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(dbid);
}


CSeq_id_Handle CSeq_id_General_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT( id.IsGeneral() );
    const CDbtag& dbid = id.GetGeneral();
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(dbid);
    if ( !info ) {
        info = CreateInfo(id);
        STagMap& tm = m_DbMap[dbid.GetDb()];
        const CObject_id& oid = dbid.GetTag();
        if ( oid.IsStr() ) {
            _VERIFY(tm.m_ByStr.insert
                    (STagMap::TByStr::value_type(oid.GetStr(), info)).second);
        }
        else if ( oid.IsId() ) {
            _VERIFY(tm.m_ById.insert(STagMap::TById::value_type(oid.GetId(),
                                                                info)).second);
        }
        else {
            THROW1_TRACE(runtime_error,
                         "CSeq_id_General_Tree::FindOrCreate() -- "
                         "Can not create index for an empty db-tag");
        }
    }
    return info;
}


void CSeq_id_General_Tree::x_Unindex(CSeq_id_Info* info)
{
    const CSeq_id& id = info->GetSeq_id();
    _ASSERT( id.IsGeneral() );
    const CDbtag& dbid = id.GetGeneral();
    TDbMap::iterator db_it = m_DbMap.find(dbid.GetDb());
    _ASSERT(db_it != m_DbMap.end());
    STagMap& tm = db_it->second;
    const CObject_id& oid = dbid.GetTag();
    if ( oid.IsStr() ) {
        _VERIFY(tm.m_ByStr.erase(oid.GetStr()));
    }
    else if ( oid.IsId() ) {
        _VERIFY(tm.m_ById.erase(oid.GetId()));
    }
    if (tm.m_ByStr.empty()  &&  tm.m_ById.empty())
        m_DbMap.erase(db_it);
}


void CSeq_id_General_Tree::FindMatch(const CSeq_id_Handle& id,
                                     TSeq_id_MatchList& id_list) const
{
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    id_list.push_back(id);
}


void CSeq_id_General_Tree::FindMatchStr(string sid,
                                        TSeq_id_MatchList& id_list) const
{
    int value;
    bool ok;
    try {
        value = NStr::StringToInt(sid);
        ok = true;
    }
    catch (const CStringException&) {
        // Not an integer value
        value = -1;
        ok = false;
    }
    TReadLockGuard guard(m_TreeLock);
    ITERATE(TDbMap, db_it, m_DbMap) {
        // In any case search in strings
        STagMap::TByStr::const_iterator str_it =
            db_it->second.m_ByStr.find(sid);
        if (str_it != db_it->second.m_ByStr.end()) {
            id_list.push_back(str_it->second);
        }
        if ( ok ) {
            STagMap::TById::const_iterator int_it =
                db_it->second.m_ById.find(value);
            if (int_it != db_it->second.m_ById.end()) {
                id_list.push_back(int_it->second);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Giim_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_Giim_Tree::CSeq_id_Giim_Tree(void)
{
}


CSeq_id_Giim_Tree::~CSeq_id_Giim_Tree(void)
{
}


bool CSeq_id_Giim_Tree::Empty(void) const
{
    return m_IdMap.empty();
}


CSeq_id_Info* CSeq_id_Giim_Tree::x_FindInfo(const CGiimport_id& gid) const
{
    TIdMap::const_iterator id_it = m_IdMap.find(gid.GetId());
    if (id_it == m_IdMap.end())
        return 0;
    ITERATE (TGiimList, dbr_it, id_it->second) {
        const CGiimport_id& gid2 = (*dbr_it)->GetSeq_id().GetGiim();
        // Both Db and Release must be equal
        if ( !gid.Equals(gid2) ) {
            return *dbr_it;
        }
    }
    // Not found
    return 0;
}


CSeq_id_Handle CSeq_id_Giim_Tree::FindInfo(const CSeq_id& id) const
{
    _ASSERT( id.IsGiim() );
    const CGiimport_id& gid = id.GetGiim();
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(gid);
}


CSeq_id_Handle CSeq_id_Giim_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT( id.IsGiim() );
    const CGiimport_id& gid = id.GetGiim();
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(gid);
    if ( !info ) {
        info = CreateInfo(id);
        m_IdMap[gid.GetId()].push_back(info);
    }
    return info;
}


void CSeq_id_Giim_Tree::x_Unindex(CSeq_id_Info* info)
{
    const CSeq_id& id = info->GetSeq_id();
    _ASSERT( id.IsGiim() );
    const CGiimport_id& gid = id.GetGiim();
    TIdMap::iterator id_it = m_IdMap.find(gid.GetId());
    _ASSERT(id_it != m_IdMap.end());
    TGiimList& giims = id_it->second;
    NON_CONST_ITERATE(TGiimList, dbr_it, giims) {
        if (*dbr_it == info) {
            giims.erase(dbr_it);
            break;
        }
    }
    if ( giims.empty() )
        m_IdMap.erase(id_it);
}


void CSeq_id_Giim_Tree::FindMatch(const CSeq_id_Handle& id,
                                  TSeq_id_MatchList& id_list) const
{
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    id_list.push_back(id);
}


void CSeq_id_Giim_Tree::FindMatchStr(string sid,
                                     TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    try {
        int value = NStr::StringToInt(sid);
        TIdMap::const_iterator it = m_IdMap.find(value);
        if (it == m_IdMap.end())
            return;
        ITERATE(TGiimList, git, it->second) {
            id_list.push_back(*git);
        }
    }
    catch (CStringException) {
        // Not an integer value
        return;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_Patent_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_Patent_Tree::CSeq_id_Patent_Tree(void)
{
}


CSeq_id_Patent_Tree::~CSeq_id_Patent_Tree(void)
{
}


bool CSeq_id_Patent_Tree::Empty(void) const
{
    return m_CountryMap.empty();
}


CSeq_id_Info* CSeq_id_Patent_Tree::x_FindInfo(const CPatent_seq_id& pid) const
{
    const CId_pat& cit = pid.GetCit();
    TByCountry::const_iterator cntry_it = m_CountryMap.find(cit.GetCountry());
    if (cntry_it == m_CountryMap.end())
        return 0;

    const string* number;
    const SPat_idMap::TByNumber* by_number;
    if ( cit.GetId().IsNumber() ) {
        number = &cit.GetId().GetNumber();
        by_number = &cntry_it->second.m_ByNumber;
    }
    else if ( cit.GetId().IsApp_number() ) {
        number = &cit.GetId().GetApp_number();
        by_number = &cntry_it->second.m_ByApp_number;
    }
    else {
        return 0;
    }

    SPat_idMap::TByNumber::const_iterator num_it = by_number->find(*number);
    if (num_it == by_number->end())
        return 0;
    SPat_idMap::TBySeqid::const_iterator seqid_it =
        num_it->second.find(pid.GetSeqid());
    if (seqid_it != num_it->second.end()) {
        return seqid_it->second;
    }
    // Not found
    return 0;
}


CSeq_id_Handle CSeq_id_Patent_Tree::FindInfo(const CSeq_id& id) const
{
    _ASSERT( id.IsPatent() );
    const CPatent_seq_id& pid = id.GetPatent();
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(pid);
}

CSeq_id_Handle CSeq_id_Patent_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT( id.IsPatent() );
    const CPatent_seq_id& pid = id.GetPatent();
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(pid);
    if ( !info ) {
        const CId_pat& cit = pid.GetCit();
        SPat_idMap& country = m_CountryMap[cit.GetCountry()];
        if ( cit.GetId().IsNumber() ) {
            SPat_idMap::TBySeqid& num =
                country.m_ByNumber[cit.GetId().GetNumber()];
            _ASSERT(num.find(pid.GetSeqid()) == num.end());
            info = CreateInfo(id);
            num[pid.GetSeqid()] = info;
        }
        else if ( cit.GetId().IsApp_number() ) {
            SPat_idMap::TBySeqid& app = country.m_ByApp_number[
                cit.GetId().GetApp_number()];
            _ASSERT(app.find(pid.GetSeqid()) == app.end());
            info = CreateInfo(id);
            app[pid.GetSeqid()] = info;
        }
        else {
            // Can not index empty patent number
            THROW1_TRACE(runtime_error,
                         "CSeq_id_Patent_Tree: "
                         "cannot index empty patent number");
        }
    }
    return info;
}


void CSeq_id_Patent_Tree::x_Unindex(CSeq_id_Info* info)
{
    const CSeq_id& id = info->GetSeq_id();
    _ASSERT( id.IsPatent() );
    const CPatent_seq_id& pid = id.GetPatent();
    TByCountry::iterator country_it =
        m_CountryMap.find(pid.GetCit().GetCountry());
    _ASSERT(country_it != m_CountryMap.end());
    SPat_idMap& pats = country_it->second;
    if ( pid.GetCit().GetId().IsNumber() ) {
        SPat_idMap::TByNumber::iterator num_it =
            pats.m_ByNumber.find(pid.GetCit().GetId().GetNumber());
        _ASSERT(num_it != pats.m_ByNumber.end());
        SPat_idMap::TBySeqid::iterator seqid_it =
            num_it->second.find(pid.GetSeqid());
        _ASSERT(seqid_it != num_it->second.end());
        _ASSERT(seqid_it->second == info);
        num_it->second.erase(seqid_it);
        if ( num_it->second.empty() )
            pats.m_ByNumber.erase(num_it);
    }
    else if ( pid.GetCit().GetId().IsApp_number() ) {
        SPat_idMap::TByNumber::iterator app_it =
            pats.m_ByApp_number.find(pid.GetCit().GetId().GetApp_number());
        _ASSERT(app_it == pats.m_ByApp_number.end());
        SPat_idMap::TBySeqid::iterator seqid_it =
            app_it->second.find(pid.GetSeqid());
        _ASSERT(seqid_it != app_it->second.end());
        _ASSERT(seqid_it->second == info);
        app_it->second.erase(seqid_it);
        if ( app_it->second.empty() )
            pats.m_ByNumber.erase(app_it);
    }
    if (country_it->second.m_ByNumber.empty()  &&
        country_it->second.m_ByApp_number.empty())
        m_CountryMap.erase(country_it);
}


void CSeq_id_Patent_Tree::FindMatch(const CSeq_id_Handle& id,
                                    TSeq_id_MatchList& id_list) const
{
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    id_list.push_back(id);
}


void CSeq_id_Patent_Tree::FindMatchStr(string sid,
                                       TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    ITERATE (TByCountry, cit, m_CountryMap) {
        SPat_idMap::TByNumber::const_iterator nit =
            cit->second.m_ByNumber.find(sid);
        if (nit != cit->second.m_ByNumber.end()) {
            ITERATE(SPat_idMap::TBySeqid, iit, nit->second) {
                id_list.push_back(iit->second);
            }
        }
        SPat_idMap::TByNumber::const_iterator ait =
            cit->second.m_ByApp_number.find(sid);
        if (ait != cit->second.m_ByApp_number.end()) {
            ITERATE(SPat_idMap::TBySeqid, iit, nit->second) {
                id_list.push_back(iit->second);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSeq_id_PDB_Tree
/////////////////////////////////////////////////////////////////////////////


CSeq_id_PDB_Tree::CSeq_id_PDB_Tree(void)
{
}


CSeq_id_PDB_Tree::~CSeq_id_PDB_Tree(void)
{
}


bool CSeq_id_PDB_Tree::Empty(void) const
{
    return m_MolMap.empty();
}


inline string CSeq_id_PDB_Tree::x_IdToStrKey(const CPDB_seq_id& id) const
{
// this is an attempt to follow the undocumented rules of PDB
// ("documented" as code written elsewhere)
    string skey = id.GetMol().Get();
    switch (char chain = (char)id.GetChain()) {
    case '\0': skey += " ";   break;
    case '|':  skey += "VB";  break;
    default:   skey += chain; break;
    }
    return skey;
}


CSeq_id_Info* CSeq_id_PDB_Tree::x_FindInfo(const CPDB_seq_id& pid) const
{
    TMolMap::const_iterator mol_it = m_MolMap.find(x_IdToStrKey(pid));
    if (mol_it == m_MolMap.end())
        return 0;
    ITERATE(TSubMolList, it, mol_it->second) {
        if (pid.Equals((*it)->GetSeq_id().GetPdb())) {
            return *it;
        }
    }
    // Not found
    return 0;
}


CSeq_id_Handle CSeq_id_PDB_Tree::FindInfo(const CSeq_id& id) const
{
    _ASSERT( id.IsPdb() );
    const CPDB_seq_id& pid = id.GetPdb();
    TReadLockGuard guard(m_TreeLock);
    return x_FindInfo(pid);
}


CSeq_id_Handle CSeq_id_PDB_Tree::FindOrCreate(const CSeq_id& id)
{
    _ASSERT( id.IsPdb() );
    const CPDB_seq_id& pid = id.GetPdb();
    TWriteLockGuard guard(m_TreeLock);
    CSeq_id_Info* info = x_FindInfo(pid);
    if ( !info ) {
        info = CreateInfo(id);
        TSubMolList& sub = m_MolMap[x_IdToStrKey(id.GetPdb())];
        ITERATE(TSubMolList, sub_it, sub) {
            _ASSERT(!info->GetSeq_id().GetPdb()
                    .Equals((*sub_it)->GetSeq_id().GetPdb()));
        }
        sub.push_back(info);
    }
    return info;
}


void CSeq_id_PDB_Tree::x_Unindex(CSeq_id_Info* info)
{
    const CSeq_id& id = info->GetSeq_id();
    _ASSERT( id.IsPdb() );
    const CPDB_seq_id& pid = id.GetPdb();
    TMolMap::iterator mol_it = m_MolMap.find(x_IdToStrKey(pid));
    _ASSERT(mol_it != m_MolMap.end());
    NON_CONST_ITERATE(TSubMolList, it, mol_it->second) {
        if (*it == info) {
            _ASSERT(pid.Equals((*it)->GetSeq_id().GetPdb()));
            mol_it->second.erase(it);
            break;
        }
    }
    if ( mol_it->second.empty() )
        m_MolMap.erase(mol_it);
}


void CSeq_id_PDB_Tree::FindMatch(const CSeq_id_Handle& id,
                                 TSeq_id_MatchList& id_list) const
{
    //_ASSERT(id && id == FindInfo(id.GetSeqId()));
    const CPDB_seq_id& pid = id.GetSeqId().GetPdb();
    TReadLockGuard guard(m_TreeLock);
    TMolMap::const_iterator mol_it = m_MolMap.find(x_IdToStrKey(pid));
    if (mol_it == m_MolMap.end())
        return;
    ITERATE(TSubMolList, it, mol_it->second) {
        const CPDB_seq_id& pid2 = (*it)->GetSeq_id().GetPdb();
        // Ignore date if not set in id
        if ( pid.IsSetRel() ) {
            if ( !pid2.IsSetRel()  ||
                !pid.GetRel().Equals(pid2.GetRel()) )
                continue;
        }
        id_list.push_back(*it);
    }
}


void CSeq_id_PDB_Tree::FindMatchStr(string sid,
                                    TSeq_id_MatchList& id_list) const
{
    TReadLockGuard guard(m_TreeLock);
    TMolMap::const_iterator mit = m_MolMap.find(sid);
    if (mit == m_MolMap.end())
        return;
    ITERATE(TSubMolList, sub_it, mit->second) {
        id_list.push_back(*sub_it);
    }
}


END_SCOPE(objects)
END_NCBI_SCOPE



/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.2  2003/06/30 18:40:04  vasilche
* Fixed warning (unused argument).
*
* Revision 1.1  2003/06/10 19:06:35  vasilche
* Simplified CSeq_id_Mapper and CSeq_id_Handle.
*
*
* ===========================================================================
*/
