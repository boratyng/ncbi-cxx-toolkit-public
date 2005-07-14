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
* Authors:
*           Eugene Vasilchenko
*
* File Description:
*           Structures used by CScope
*
*/

#include <ncbi_pch.hpp>
#include <objmgr/impl/scope_info.hpp>
#include <objmgr/impl/scope_impl.hpp>
#include <objmgr/scope.hpp>

#include <objmgr/impl/synonyms.hpp>
#include <objmgr/impl/data_source.hpp>

#include <objmgr/impl/tse_info.hpp>
#include <objmgr/tse_handle.hpp>
#include <objmgr/impl/seq_entry_info.hpp>
#include <objmgr/seq_entry_handle.hpp>
#include <objmgr/impl/seq_annot_info.hpp>
#include <objmgr/seq_annot_handle.hpp>
#include <objmgr/impl/bioseq_info.hpp>
#include <objmgr/bioseq_handle.hpp>
#include <objmgr/impl/bioseq_set_info.hpp>
#include <objmgr/bioseq_set_handle.hpp>

#include <corelib/ncbi_config_value.hpp>
#include <algorithm>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

#if 0
# define _TRACE_TSE_LOCK(x) _TRACE(x)
#else
# define _TRACE_TSE_LOCK(x) ((void)0)
#endif


static bool s_GetScopeAutoReleaseEnabled(void)
{
    static int sx_Value = -1;
    int value = sx_Value;
    if ( value < 0 ) {
        value = GetConfigFlag("OBJMGR", "SCOPE_AUTORELEASE", true);
        sx_Value = value;
    }
    return value != 0;
}


static unsigned s_GetScopeAutoReleaseSize(void)
{
    static unsigned sx_Value = kMax_UInt;
    unsigned value = sx_Value;
    if ( value == kMax_UInt ) {
        value = GetConfigInt("OBJMGR", "SCOPE_AUTORELEASE_SIZE", 10);
        if ( value == kMax_UInt ) {
            --value;
        }
        sx_Value = value;
    }
    return value;
}


/////////////////////////////////////////////////////////////////////////////
// CDataSource_ScopeInfo
/////////////////////////////////////////////////////////////////////////////

CDataSource_ScopeInfo::CDataSource_ScopeInfo(CScope_Impl& scope,
                                             CDataSource& ds,
                                             bool shared)
    : m_Scope(&scope),
      m_DataSource(&ds),
      m_CanBeUnloaded(s_GetScopeAutoReleaseEnabled() &&
                      ds.GetDataLoader() &&
                      ds.GetDataLoader()->CanGetBlobById()),
      m_Shared(shared),
      m_NextTSEIndex(0),
      m_TSE_UnlockQueue(s_GetScopeAutoReleaseSize())
{
}


CDataSource_ScopeInfo::~CDataSource_ScopeInfo(void)
{
    _ASSERT(!m_Scope);
    _ASSERT(!m_DataSource);
}


CScope_Impl& CDataSource_ScopeInfo::GetScopeImpl(void) const
{
    if ( !m_Scope ) {
        NCBI_THROW(CCoreException, eNullPtr,
                   "CDataSource_ScopeInfo is not attached to CScope");
    }
    return *m_Scope;
}


CDataLoader* CDataSource_ScopeInfo::GetDataLoader(void)
{
    return GetDataSource().GetDataLoader();
}


void CDataSource_ScopeInfo::DetachScope(void)
{
    if ( m_Scope ) {
        _ASSERT(m_DataSource);
        ResetDS();
        GetScopeImpl().m_ObjMgr->ReleaseDataSource(m_DataSource);
        _ASSERT(!m_DataSource);
        m_Scope = 0;
    }
}


const CDataSource_ScopeInfo::TTSE_InfoMap&
CDataSource_ScopeInfo::GetTSE_InfoMap(void) const
{
    return m_TSE_InfoMap;
}


const CDataSource_ScopeInfo::TTSE_LockSet&
CDataSource_ScopeInfo::GetTSE_LockSet(void) const
{
    return m_TSE_LockSet;
}


void CDataSource_ScopeInfo::RemoveTSE_Lock(const CTSE_Lock& lock)
{
    TTSE_LockSetMutex::TWriteLockGuard guard(m_TSE_LockSetMutex);
    _VERIFY(m_TSE_LockSet.RemoveLock(lock));
}


void CDataSource_ScopeInfo::AddTSE_Lock(const CTSE_Lock& lock)
{
    TTSE_LockSetMutex::TWriteLockGuard guard(m_TSE_LockSetMutex);
    _VERIFY(m_TSE_LockSet.AddLock(lock));
}


CDataSource_ScopeInfo::TTSE_Lock
CDataSource_ScopeInfo::GetTSE_Lock(const CTSE_Lock& lock)
{
    CTSE_ScopeUserLock ret;
    _ASSERT(lock);
    TTSE_ScopeInfo info;
    {{
        TTSE_InfoMapMutex::TWriteLockGuard guard(m_TSE_InfoMapMutex);
        STSE_Key key(*lock, m_CanBeUnloaded);
        TTSE_ScopeInfo& slot = m_TSE_InfoMap[key];
        if ( !slot ) {
            slot = info = new CTSE_ScopeInfo(*this, lock,
                                             m_NextTSEIndex++,
                                             m_CanBeUnloaded);
            if ( m_CanBeUnloaded ) {
                // add this TSE into index by SeqId
                x_IndexTSE(*info);
            }
        }
        else {
            info = slot;
        }
        _ASSERT(info->IsAttached() && &info->GetDSInfo() == this);
        info->m_TSE_LockCounter.Add(1);
        {{
            // first remove the TSE from unlock queue
            TTSE_LockSetMutex::TWriteLockGuard guard(m_TSE_UnlockQueueMutex);
            // TSE must be locked already by caller
            _ASSERT(info->m_TSE_LockCounter.Get() > 0);
            m_TSE_UnlockQueue.Erase(info);
            // TSE must be still locked by caller even after removing it
            // from unlock queue
            _ASSERT(info->m_TSE_LockCounter.Get() > 0);
        }}
        info->SetTSE_Lock(lock);
        ret.Reset(info);
        _VERIFY(info->m_TSE_LockCounter.Add(-1) > 0);
        _ASSERT(info->GetTSE_Lock() == lock);
    }}
    return ret;
}


void CDataSource_ScopeInfo::x_IndexTSE(CTSE_ScopeInfo& tse)
{
    CTSE_ScopeInfo::TBlobOrder order = tse.GetBlobOrder();
    ITERATE ( CTSE_ScopeInfo::TBioseqsIds, it, tse.GetBioseqsIds() ) {
        m_TSE_BySeqId.insert(TTSE_BySeqId::value_type(*it, Ref(&tse)));
    }
}


void CDataSource_ScopeInfo::x_UnindexTSE(const CTSE_ScopeInfo& tse)
{
    CTSE_ScopeInfo::TBlobOrder order = tse.GetBlobOrder();
    ITERATE ( CTSE_ScopeInfo::TBioseqsIds, it, tse.GetBioseqsIds() ) {
        TTSE_BySeqId::iterator tse_it = m_TSE_BySeqId.lower_bound(*it);
        while ( tse_it != m_TSE_BySeqId.end() && tse_it->first == *it ) {
            if ( tse_it->second == &tse ) {
                m_TSE_BySeqId.erase(tse_it++);
            }
            else {
                ++tse_it;
            }
        }
    }
}


CDataSource_ScopeInfo::TTSE_ScopeInfo
CDataSource_ScopeInfo::x_FindBestTSEInIndex(const CSeq_id_Handle& idh) const
{
    TTSE_ScopeInfo tse;
    for ( TTSE_BySeqId::const_iterator it = m_TSE_BySeqId.lower_bound(idh);
          it != m_TSE_BySeqId.end() && it->first == idh; ++it ) {
        if ( !tse || x_IsBetter(idh, *it->second, *tse) ) {
            tse = it->second;
        }
    }
    return tse;
}


void CDataSource_ScopeInfo::UpdateTSELock(CTSE_ScopeInfo& tse, CTSE_Lock lock)
{
    {{
        // first remove the TSE from unlock queue
        TTSE_LockSetMutex::TWriteLockGuard guard(m_TSE_UnlockQueueMutex);
        // TSE must be locked already by caller
        _ASSERT(tse.m_TSE_LockCounter.Get() > 0);
        m_TSE_UnlockQueue.Erase(&tse);
        // TSE must be still locked by caller even after removing it
        // from unlock queue
        _ASSERT(tse.m_TSE_LockCounter.Get() > 0);
    }}
    if ( !tse.GetTSE_Lock() ) {
        // OK, we need to update the lock
        if ( !lock ) { // obtain lock from CDataSource
            lock = tse.m_UnloadedInfo->LockTSE();
            _ASSERT(lock);
        }
        tse.SetTSE_Lock(lock);
        _ASSERT(tse.GetTSE_Lock() == lock);
    }
    _ASSERT(tse.m_TSE_LockCounter.Get() > 0);
    _ASSERT(tse.GetTSE_Lock());
}


// Called by destructor of CTSE_ScopeUserLock when lock counter goes to 0
void CDataSource_ScopeInfo::ReleaseTSELock(CTSE_ScopeInfo& tse)
{
    if ( tse.m_TSE_LockCounter.Get() > 0 ) {
        // relocked already
        return;
    }
    if ( !tse.GetTSE_Lock() ) {
        // already unlocked
        return;
    }
    {{
        TTSE_LockSetMutex::TWriteLockGuard guard(m_TSE_UnlockQueueMutex);
        m_TSE_UnlockQueue.Put(&tse, CTSE_ScopeInternalLock(&tse));
    }}
}


// Called by destructor of CTSE_ScopeInternalLock when lock counter goes to 0
// CTSE_ScopeInternalLocks are stored in m_TSE_UnlockQueue 
void CDataSource_ScopeInfo::ForgetTSELock(CTSE_ScopeInfo& tse)
{
    if ( tse.m_TSE_LockCounter.Get() > 0 ) {
        // relocked already
        return;
    }
    if ( !tse.GetTSE_Lock() ) {
        // already unlocked
        return;
    }
    tse.ForgetTSE_Lock();
}


void CDataSource_ScopeInfo::ResetDS(void)
{
    TTSE_InfoMapMutex::TWriteLockGuard guard1(m_TSE_InfoMapMutex);
    {{
        TTSE_LockSetMutex::TWriteLockGuard guard2(m_TSE_UnlockQueueMutex);
        m_TSE_UnlockQueue.Clear();
    }}
    NON_CONST_ITERATE ( TTSE_InfoMap, it, m_TSE_InfoMap ) {
        it->second->DropTSE_Lock();
        it->second->x_DetachDS();
    }
    m_TSE_InfoMap.clear();
    m_TSE_BySeqId.clear();
    {{
        TTSE_LockSetMutex::TWriteLockGuard guard2(m_TSE_LockSetMutex);
        m_TSE_LockSet.clear();
    }}
    m_NextTSEIndex = 0;
}


void CDataSource_ScopeInfo::ResetHistory(int action_if_locked)
{
    if ( action_if_locked == CScope_Impl::eRemoveIfLocked ) {
        // no checks -> fast reset
        ResetDS();
        return;
    }
    TTSE_InfoMapMutex::TWriteLockGuard guard1(m_TSE_InfoMapMutex);
    typedef vector< CRef<CTSE_ScopeInfo> > TTSEs;
    TTSEs tses;
    tses.reserve(m_TSE_InfoMap.size());
    ITERATE ( TTSE_InfoMap, it, m_TSE_InfoMap ) {
        tses.push_back(it->second);
    }
    ITERATE ( TTSEs, it, tses ) {
        it->GetNCObject().RemoveFromHistory(action_if_locked);
    }
}


void CDataSource_ScopeInfo::RemoveFromHistory(CTSE_ScopeInfo& tse)
{
    TTSE_InfoMapMutex::TWriteLockGuard guard1(m_TSE_InfoMapMutex);
    if ( tse.CanBeUnloaded() ) {
        x_UnindexTSE(tse);
    }
    _VERIFY(m_TSE_InfoMap.erase(tse));
    tse.m_TSE_LockCounter.Add(1); // to prevent storing into m_TSE_UnlockQueue
    // remove TSE lock completely
    {{
        TTSE_LockSetMutex::TWriteLockGuard guard2(m_TSE_UnlockQueueMutex);
        m_TSE_UnlockQueue.Erase(&tse);
    }}
    tse.ResetTSE_Lock();
    tse.x_DetachDS();
    tse.m_TSE_LockCounter.Add(-1); // restore lock counter
    _ASSERT(!tse.GetTSE_Lock());
    _ASSERT(!tse.m_DS_Info);
}


CDataSource_ScopeInfo::TTSE_Lock
CDataSource_ScopeInfo::FindTSE_Lock(const CSeq_entry& tse)
{
    CDataSource::TTSE_Lock lock;
    {{
        TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
        lock = GetDataSource().FindTSE_Lock(tse, m_TSE_LockSet);
    }}
    if ( lock ) {
        return GetTSE_Lock(lock);
    }
    return TTSE_Lock();
}


CDataSource_ScopeInfo::TSeq_entry_Lock
CDataSource_ScopeInfo::FindSeq_entry_Lock(const CSeq_entry& entry)
{
    CDataSource::TSeq_entry_Lock lock;
    {{
        TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
        lock = GetDataSource().FindSeq_entry_Lock(entry, m_TSE_LockSet);
    }}
    if ( lock.first ) {
        return TSeq_entry_Lock(lock.first, GetTSE_Lock(lock.second));
    }
    return TSeq_entry_Lock();
}


CDataSource_ScopeInfo::TSeq_annot_Lock
CDataSource_ScopeInfo::FindSeq_annot_Lock(const CSeq_annot& annot)
{
    CDataSource::TSeq_annot_Lock lock;
    {{
        TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
        lock = GetDataSource().FindSeq_annot_Lock(annot, m_TSE_LockSet);
    }}
    if ( lock.first ) {
        return TSeq_annot_Lock(lock.first, GetTSE_Lock(lock.second));
    }
    return TSeq_annot_Lock();
}


CDataSource_ScopeInfo::TBioseq_set_Lock
CDataSource_ScopeInfo::FindBioseq_set_Lock(const CBioseq_set& seqset)
{
    CDataSource::TBioseq_set_Lock lock;
    {{
        TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
        lock = GetDataSource().FindBioseq_set_Lock(seqset, m_TSE_LockSet);
    }}
    if ( lock.first ) {
        return TBioseq_set_Lock(lock.first, GetTSE_Lock(lock.second));
    }
    return TBioseq_set_Lock();
}


CDataSource_ScopeInfo::TBioseq_Lock
CDataSource_ScopeInfo::FindBioseq_Lock(const CBioseq& bioseq)
{
    CDataSource::TBioseq_Lock lock;
    {{
        TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
        lock = GetDataSource().FindBioseq_Lock(bioseq, m_TSE_LockSet);
    }}
    if ( lock.first ) {
        return GetTSE_Lock(lock.second)->GetBioseqLock(null, lock.first);
    }
    return TBioseq_Lock();
}


CDataSource_ScopeInfo::STSE_Key::STSE_Key(const CTSE_Info& tse,
                                          bool can_be_unloaded)
{
    if ( can_be_unloaded ) {
        m_Loader = tse.GetDataSource().GetDataLoader();
        _ASSERT(m_Loader);
        m_BlobId = tse.GetBlobId();
        _ASSERT(m_BlobId);
        _ASSERT(m_BlobId.GetPointer() != &tse);
    }
    else {
        m_Loader = 0;
        m_BlobId = &tse;
    }
}


CDataSource_ScopeInfo::STSE_Key::STSE_Key(const CTSE_ScopeInfo& tse)
{
    if ( tse.CanBeUnloaded() ) {
        m_Loader = tse.GetDSInfo().GetDataSource().GetDataLoader();
        _ASSERT(m_Loader);
    }
    else {
        m_Loader = 0;
    }
    m_BlobId = tse.GetBlobId();
    _ASSERT(m_BlobId);
}


bool CDataSource_ScopeInfo::STSE_Key::operator<(const STSE_Key& tse2) const
{
    _ASSERT(m_Loader == tse2.m_Loader);
    if ( m_Loader ) {
        return m_Loader->LessBlobId(m_BlobId, tse2.m_BlobId);
    }
    else {
        return m_BlobId < tse2.m_BlobId;
    }
}


SSeqMatch_Scope CDataSource_ScopeInfo::BestResolve(const CSeq_id_Handle& idh,
                                                   int get_flag)
{
    SSeqMatch_Scope ret = x_GetSeqMatch(idh);
    if ( !ret && get_flag == CScope::eGetBioseq_All ) {
        // Try to load the sequence from the data source
        SSeqMatch_DS ds_match = GetDataSource().BestResolve(idh);
        if ( ds_match ) {
            x_SetMatch(ret, ds_match);
        }
    }
#ifdef _DEBUG
    if ( ret ) {
        _ASSERT(ret.m_Seq_id);
        _ASSERT(ret.m_Bioseq);
        _ASSERT(ret.m_TSE_Lock);
        _ASSERT(ret.m_Bioseq == ret.m_TSE_Lock->m_TSE_Lock->FindBioseq(ret.m_Seq_id));
    }
#endif
    return ret;
}


SSeqMatch_Scope CDataSource_ScopeInfo::x_GetSeqMatch(const CSeq_id_Handle& idh)
{
    SSeqMatch_Scope ret = x_FindBestTSE(idh);
    if ( !ret && idh.HaveMatchingHandles() ) {
        CSeq_id_Handle::TMatches ids;
        idh.GetMatchingHandles(ids);
        ITERATE ( CSeq_id_Handle::TMatches, it, ids ) {
            if ( *it == idh ) // already checked
                continue;
            if ( ret && ret.m_Seq_id.IsBetter(*it) ) // worse hit
                continue;
            ret = x_FindBestTSE(*it);
        }
    }
    return ret;
}


SSeqMatch_Scope CDataSource_ScopeInfo::x_FindBestTSE(const CSeq_id_Handle& idh)
{
    SSeqMatch_Scope ret;
    if ( m_CanBeUnloaded ) {
        // We have full index of static TSEs.
        TTSE_InfoMapMutex::TReadLockGuard guard(GetTSE_InfoMapMutex());
        TTSE_ScopeInfo tse = x_FindBestTSEInIndex(idh);
        if ( tse ) {
            x_SetMatch(ret, *tse, idh);
        }
    }
    else {
        // We have to ask data source about it.
        CDataSource::TSeqMatches matches;
        {{
            TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_LockSetMutex);
            CDataSource::TSeqMatches matches2 =
                GetDataSource().GetMatches(idh, m_TSE_LockSet);
            matches.swap(matches2);
        }}
        ITERATE ( CDataSource::TSeqMatches, it, matches ) {
            SSeqMatch_Scope nxt;
            x_SetMatch(nxt, *it);
            if ( !ret || x_IsBetter(idh, *nxt.m_TSE_Lock, *ret.m_TSE_Lock) ) {
                ret = nxt;
            }
        }
    }
    return ret;
}


bool CDataSource_ScopeInfo::x_IsBetter(const CSeq_id_Handle& idh,
                                       const CTSE_ScopeInfo& tse1,
                                       const CTSE_ScopeInfo& tse2)
{
    // First of all we check if we already resolve bioseq with this id.
    bool resolved1 = tse1.HasResolvedBioseq(idh);
    bool resolved2 = tse2.HasResolvedBioseq(idh);
    if ( resolved1 != resolved2 ) {
        return resolved1;
    }
    // Now check TSEs' orders.
    CTSE_ScopeInfo::TBlobOrder order1 = tse1.GetBlobOrder();
    CTSE_ScopeInfo::TBlobOrder order2 = tse2.GetBlobOrder();
    if ( order1 != order2 ) {
        return order1 < order2;
    }

    // Now we have very similar TSE's so we'll prefer the first one added.
    return tse1.GetLoadIndex() < tse2.GetLoadIndex();
}


void CDataSource_ScopeInfo::x_SetMatch(SSeqMatch_Scope& match,
                                       CTSE_ScopeInfo& tse,
                                       const CSeq_id_Handle& idh) const
{
    match.m_Seq_id = idh;
    match.m_TSE_Lock = CTSE_ScopeUserLock(&tse);
    match.m_Bioseq = match.m_TSE_Lock->GetTSE_Lock()->FindBioseq(idh);
    _ASSERT(match.m_Bioseq);
}


void CDataSource_ScopeInfo::x_SetMatch(SSeqMatch_Scope& match,
                                       const SSeqMatch_DS& ds_match)
{
    match.m_Seq_id = ds_match.m_Seq_id;
    match.m_TSE_Lock = GetTSE_Lock(ds_match.m_TSE_Lock);
    match.m_Bioseq = ds_match.m_Bioseq;
    _ASSERT(match.m_Bioseq);
    _ASSERT(match.m_Bioseq == match.m_TSE_Lock->GetTSE_Lock()->FindBioseq(match.m_Seq_id));
}


bool CDataSource_ScopeInfo::TSEIsInQueue(const CTSE_ScopeInfo& tse) const
{
    TTSE_LockSetMutex::TReadLockGuard guard(m_TSE_UnlockQueueMutex);
    return m_TSE_UnlockQueue.Contains(&tse);
}


/////////////////////////////////////////////////////////////////////////////
// CTSE_ScopeLocker
/////////////////////////////////////////////////////////////////////////////

void CTSE_ScopeLocker::Lock(CTSE_ScopeInfo* tse) const
{
    _TRACE_TSE_LOCK("CTSE_ScopeLocker("<<this<<") "<<tse<<" lock");
    CObjectCounterLocker::Lock(tse);
    tse->x_LockTSE();
}


void CTSE_ScopeInternalLocker::Unlock(CTSE_ScopeInfo* tse) const
{
    _TRACE_TSE_LOCK("CTSE_ScopeInternalLocker("<<this<<") "<<tse<<" unlock");
    tse->x_InternalUnlockTSE();
    CObjectCounterLocker::Unlock(tse);
}


void CTSE_ScopeUserLocker::Unlock(CTSE_ScopeInfo* tse) const
{
    _TRACE_TSE_LOCK("CTSE_ScopeUserLocker("<<this<<") "<<tse<<" unlock");
    tse->x_UserUnlockTSE();
    CObjectCounterLocker::Unlock(tse);
}


/////////////////////////////////////////////////////////////////////////////
// CTSE_ScopeInfo
/////////////////////////////////////////////////////////////////////////////


CTSE_ScopeInfo::SUnloadedInfo::SUnloadedInfo(const CTSE_Lock& tse_lock)
    : m_Loader(tse_lock->GetDataSource().GetDataLoader()),
      m_BlobId(tse_lock->GetBlobId()),
      m_BlobOrder(tse_lock->GetBlobOrder())
{
    _ASSERT(m_Loader);
    _ASSERT(m_BlobId);
    // copy all bioseq ids
    tse_lock->GetBioseqsIds(m_BioseqsIds);
}


CTSE_Lock CTSE_ScopeInfo::SUnloadedInfo::LockTSE(void)
{
    _ASSERT(m_Loader);
    _ASSERT(m_BlobId);
    return m_Loader->GetBlobById(m_BlobId);
}


CTSE_ScopeInfo::CTSE_ScopeInfo(CDataSource_ScopeInfo& ds_info,
                               const CTSE_Lock& lock,
                               int load_index,
                               bool can_be_unloaded)
    : m_DS_Info(&ds_info),
      m_LoadIndex(load_index),
      m_UsedByTSE(0)
{
    m_TSE_LockCounter.Set(0);
    _ASSERT(lock);
    if ( can_be_unloaded ) {
        _ASSERT(lock->GetBlobId());
        m_UnloadedInfo.reset(new SUnloadedInfo(lock));
    }
    else {
        // permanent lock
        _TRACE_TSE_LOCK("CTSE_ScopeInfo("<<this<<") perm lock");
        m_TSE_LockCounter.Add(1);
        x_SetTSE_Lock(lock);
        _ASSERT(m_TSE_Lock == lock);
    }
}


CTSE_ScopeInfo::~CTSE_ScopeInfo(void)
{
    if ( !CanBeUnloaded() ) {
        // remove permanent lock
        _TRACE_TSE_LOCK("CTSE_ScopeInfo("<<this<<") perm unlock: "<<m_TSE_LockCounter.Get());
        _VERIFY(m_TSE_LockCounter.Add(-1) == 0);
    }
    x_DetachDS();
    _TRACE_TSE_LOCK("CTSE_ScopeInfo("<<this<<") final: "<<m_TSE_LockCounter.Get());
    _ASSERT(m_TSE_LockCounter.Get() == 0);
    _ASSERT(!m_TSE_Lock);
}


CTSE_ScopeInfo::TBlobOrder CTSE_ScopeInfo::GetBlobOrder(void) const
{
    if ( CanBeUnloaded() ) {
        _ASSERT(m_UnloadedInfo.get());
        return m_UnloadedInfo->m_BlobOrder;
    }
    else {
        _ASSERT(m_TSE_Lock);
        return m_TSE_Lock->GetBlobOrder();
    }
}


CTSE_ScopeInfo::TBlobId CTSE_ScopeInfo::GetBlobId(void) const
{
    if ( CanBeUnloaded() ) {
        _ASSERT(m_UnloadedInfo.get());
        return m_UnloadedInfo->m_BlobId;
    }
    else {
        _ASSERT(m_TSE_Lock);
        return TBlobId(&*m_TSE_Lock);
    }
}


const CTSE_ScopeInfo::TBioseqsIds& CTSE_ScopeInfo::GetBioseqsIds(void) const
{
    _ASSERT(CanBeUnloaded());
    return m_UnloadedInfo->m_BioseqsIds;
}


void CTSE_ScopeInfo::x_LockTSE(void)
{
    m_TSE_LockCounter.Add(1);
    if ( !m_TSE_Lock ) {
        GetDSInfo().UpdateTSELock(*this, CTSE_Lock());
    }
    _ASSERT(m_TSE_Lock);
}

/*
void CTSE_ScopeInfo::x_LockTSE(const CTSE_Lock& lock)
{
    m_TSE_LockCounter.Add(1);
    if ( !m_TSE_Lock ) {
        GetDSInfo().UpdateTSELock(*this, lock);
    }
    _ASSERT(m_TSE_Lock);
}
*/

void CTSE_ScopeInfo::x_UserUnlockTSE(void)
{
    if ( m_TSE_LockCounter.Add(-1) == 0 ) {
        _ASSERT(CanBeUnloaded());
        GetDSInfo().ReleaseTSELock(*this);
    }
}


void CTSE_ScopeInfo::x_InternalUnlockTSE(void)
{
    if ( m_TSE_LockCounter.Add(-1) == 0 ) {
        _ASSERT(CanBeUnloaded());
        GetDSInfo().ForgetTSELock(*this);
    }
}

/*
void CTSE_ScopeInfo::x_ReleaseTSE(void)
{
    _ASSERT( !LockedMoreThanOnce() );
    m_TSE_LockCounter.Add(-1);
    if ( CanBeUnloaded() ) {
        x_ForgetLocks();
        _ASSERT(!m_TSE_Lock);
    }
}
*/

bool CTSE_ScopeInfo::x_SameTSE(const CTSE_Info& tse) const
{
    return m_TSE_LockCounter.Get() > 0 && m_TSE_Lock && &*m_TSE_Lock == &tse;
}


bool CTSE_ScopeInfo::AddUsedTSE(const CTSE_ScopeUserLock& used_tse) const
{
    CTSE_ScopeInfo& add_info = const_cast<CTSE_ScopeInfo&>(*used_tse);
    if ( m_TSE_LockCounter.Get() == 0 || // this one is unlocked
         &add_info == this || // the same TSE
         !add_info.CanBeUnloaded() || // permanentrly locked
         &add_info.GetDSInfo() != &GetDSInfo() || // another data source
         add_info.m_UsedByTSE ) { // already used
        return false;
    }
    CDataSource_ScopeInfo::TTSE_LockSetMutex::TWriteLockGuard
        guard(GetDSInfo().GetTSE_LockSetMutex());
    if ( m_TSE_LockCounter.Get() == 0 || // this one is unlocked
         add_info.m_UsedByTSE ) { // already used
        return false;
    }
    // check if used TSE uses this TSE indirectly
    for ( const CTSE_ScopeInfo* p = m_UsedByTSE; p; p = p->m_UsedByTSE ) {
        _ASSERT(&p->GetDSInfo() == &GetDSInfo());
        if ( p == &add_info ) {
            return false;
        }
    }
    add_info.m_UsedByTSE = this;
    _VERIFY(m_UsedTSE_Set.insert(CTSE_ScopeInternalLock(&add_info)).second);
    return true;
}


void CTSE_ScopeInfo::x_SetTSE_Lock(const CTSE_Lock& lock)
{
    _ASSERT(lock);
    if ( !m_TSE_Lock ) {
        m_TSE_Lock = lock;
        GetDSInfo().AddTSE_Lock(lock);
    }
    _ASSERT(m_TSE_Lock == lock);
}


void CTSE_ScopeInfo::x_ResetTSE_Lock(void)
{
    if ( m_TSE_Lock ) {
        CTSE_Lock lock;
        lock.Swap(m_TSE_Lock);
        GetDSInfo().RemoveTSE_Lock(lock);
    }
    _ASSERT(!m_TSE_Lock);
}


void CTSE_ScopeInfo::SetTSE_Lock(const CTSE_Lock& lock)
{
    _ASSERT(lock);
    if ( !m_TSE_Lock ) {
        CMutexGuard guard(m_TSE_LockMutex);
        x_SetTSE_Lock(lock);
    }
    _ASSERT(m_TSE_Lock == lock);
}


void CTSE_ScopeInfo::ResetTSE_Lock(void)
{
    if ( m_TSE_Lock ) {
        CMutexGuard guard(m_TSE_LockMutex);
        x_ResetTSE_Lock();
    }
    _ASSERT(!m_TSE_Lock);
}


void CTSE_ScopeInfo::DropTSE_Lock(void)
{
    if ( m_TSE_Lock ) {
        CMutexGuard guard(m_TSE_LockMutex);
        m_TSE_Lock.Reset();
    }
    _ASSERT(!m_TSE_Lock);
}


// Action A4.
void CTSE_ScopeInfo::ForgetTSE_Lock(void)
{
    if ( !m_TSE_Lock ) {
        return;
    }
    CMutexGuard guard(m_TSE_LockMutex);
    if ( !m_TSE_Lock ) {
        return;
    }
    {{
        ITERATE ( TUsedTSE_LockSet, it, m_UsedTSE_Set ) {
            _ASSERT((*it)->m_UsedByTSE == this);
            (*it)->m_UsedByTSE = 0;
        }
        m_UsedTSE_Set.clear();
    }}
    NON_CONST_ITERATE ( TScopeInfoMap, it, m_ScopeInfoMap ) {
        _ASSERT(!it->second->m_TSE_Handle.m_TSE);
        it->second->m_ObjectInfo.Reset();
        if ( it->second->IsTemporary() ) {
            it->second->x_DetachTSE(this);
        }
    }
    m_ScopeInfoMap.clear();
    x_ResetTSE_Lock();
}


void CTSE_ScopeInfo::x_DetachDS(void)
{
    if ( !m_DS_Info ) {
        return;
    }
    CMutexGuard guard(m_TSE_LockMutex);
    {{
        // release all used TSEs
        ITERATE ( TUsedTSE_LockSet, it, m_UsedTSE_Set ) {
            _ASSERT((*it)->m_UsedByTSE == this);
            (*it)->m_UsedByTSE = 0;
        }
        m_UsedTSE_Set.clear();
    }}
    NON_CONST_ITERATE ( TScopeInfoMap, it, m_ScopeInfoMap ) {
        it->second->m_TSE_Handle.Reset();
        it->second->m_ObjectInfo.Reset();
        it->second->x_DetachTSE(this);
    }
    m_ScopeInfoMap.clear();
    m_TSE_Lock.Reset();
    m_DS_Info = 0;
}


int CTSE_ScopeInfo::x_GetDSLocksCount(void) const
{
    int max_locks = CanBeUnloaded() ? 0 : 1;
    if ( GetDSInfo().TSEIsInQueue(*this) ) {
        // Extra-lock from delete queue allowed
        ++max_locks;
    }
    return max_locks;
}


bool CTSE_ScopeInfo::IsLocked(void) const
{
    return m_TSE_LockCounter.Get() > x_GetDSLocksCount();
}


bool CTSE_ScopeInfo::LockedMoreThanOnce(void) const
{
    return m_TSE_LockCounter.Get() > x_GetDSLocksCount() + 1;
}


void CTSE_ScopeInfo::RemoveFromHistory(int action_if_locked)
{
    if ( IsLocked() ) {
        switch ( action_if_locked ) {
        case CScope_Impl::eKeepIfLocked:
            return;
        case CScope_Impl::eThrowIfLocked:
            NCBI_THROW(CObjMgrException, eLockedData,
                       "Cannot remove TSE from scope's history "
                       "because it's locked");
        default: // forced removal
            break;
        }
    }
    GetDSInfo().RemoveFromHistory(*this);
}


bool CTSE_ScopeInfo::HasResolvedBioseq(const CSeq_id_Handle& id) const
{
    return m_BioseqById.find(id) != m_BioseqById.end();
}


bool CTSE_ScopeInfo::ContainsBioseq(const CSeq_id_Handle& id) const
{
    if ( CanBeUnloaded() ) {
        return binary_search(m_UnloadedInfo->m_BioseqsIds.begin(),
                             m_UnloadedInfo->m_BioseqsIds.end(),
                             id);
    }
    else {
        return m_TSE_Lock->ContainsBioseq(id);
    }
}


bool CTSE_ScopeInfo::ContainsMatchingBioseq(const CSeq_id_Handle& id) const
{
    if ( CanBeUnloaded() ) {
        if ( ContainsBioseq(id) ) {
            return true;
        }
        if ( id.HaveMatchingHandles() ) {
            CSeq_id_Handle::TMatches ids;
            id.GetMatchingHandles(ids);
            ITERATE ( CSeq_id_Handle::TMatches, it, ids ) {
                if ( *it != id ) {
                    if ( ContainsBioseq(*it) ) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    else {
        return m_TSE_Lock->ContainsMatchingBioseq(id);
    }
}

// Action A5.
CTSE_ScopeInfo::TSeq_entry_Lock
CTSE_ScopeInfo::GetScopeLock(const CTSE_Handle& tse,
                             const CSeq_entry_Info& info)
{
    CMutexGuard guard(m_TSE_LockMutex);
    _ASSERT(x_SameTSE(tse.x_GetTSE_Info()));
    CRef<CSeq_entry_ScopeInfo> scope_info;
    TScopeInfoMapKey key(&info);
    TScopeInfoMap::iterator iter = m_ScopeInfoMap.lower_bound(key);
    if ( iter == m_ScopeInfoMap.end() || iter->first != key ) {
        scope_info = new CSeq_entry_ScopeInfo(tse, info);
        TScopeInfoMapValue value(scope_info);
        m_ScopeInfoMap.insert(iter, TScopeInfoMap::value_type(key, value));
        value->m_ObjectInfo = &info;
    }
    else {
        _ASSERT(iter->second->HasObject());
        _ASSERT(&iter->second->GetObjectInfo_Base() == &info);
        scope_info = &dynamic_cast<CSeq_entry_ScopeInfo&>(*iter->second);
    }
    if ( !scope_info->m_TSE_Handle.m_TSE ) {
        scope_info->m_TSE_Handle = tse.m_TSE;
    }
    _ASSERT(scope_info->IsAttached());
    _ASSERT(scope_info->m_TSE_Handle.m_TSE);
    _ASSERT(scope_info->HasObject());
    return TSeq_entry_Lock(*scope_info);
}

// Action A5.
CTSE_ScopeInfo::TSeq_annot_Lock
CTSE_ScopeInfo::GetScopeLock(const CTSE_Handle& tse,
                             const CSeq_annot_Info& info)
{
    CMutexGuard guard(m_TSE_LockMutex);
    _ASSERT(x_SameTSE(tse.x_GetTSE_Info()));
    CRef<CSeq_annot_ScopeInfo> scope_info;
    TScopeInfoMapKey key(&info);
    TScopeInfoMap::iterator iter = m_ScopeInfoMap.lower_bound(key);
    if ( iter == m_ScopeInfoMap.end() || iter->first != key ) {
        scope_info = new CSeq_annot_ScopeInfo(tse, info);
        TScopeInfoMapValue value(scope_info);
        m_ScopeInfoMap.insert(iter, TScopeInfoMap::value_type(key, value));
        value->m_ObjectInfo = &info;
    }
    else {
        _ASSERT(iter->second->HasObject());
        _ASSERT(&iter->second->GetObjectInfo_Base() == &info);
        scope_info = &dynamic_cast<CSeq_annot_ScopeInfo&>(*iter->second);
    }
    if ( !scope_info->m_TSE_Handle.m_TSE ) {
        scope_info->m_TSE_Handle = tse.m_TSE;
    }
    _ASSERT(scope_info->IsAttached());
    _ASSERT(scope_info->m_TSE_Handle.m_TSE);
    _ASSERT(scope_info->HasObject());
    return TSeq_annot_Lock(*scope_info);
}

// Action A5.
CTSE_ScopeInfo::TBioseq_set_Lock
CTSE_ScopeInfo::GetScopeLock(const CTSE_Handle& tse,
                             const CBioseq_set_Info& info)
{
    CMutexGuard guard(m_TSE_LockMutex);
    _ASSERT(x_SameTSE(tse.x_GetTSE_Info()));
    CRef<CBioseq_set_ScopeInfo> scope_info;
    TScopeInfoMapKey key(&info);
    TScopeInfoMap::iterator iter = m_ScopeInfoMap.lower_bound(key);
    if ( iter == m_ScopeInfoMap.end() || iter->first != key ) {
        scope_info = new CBioseq_set_ScopeInfo(tse, info);
        TScopeInfoMapValue value(scope_info);
        m_ScopeInfoMap.insert(iter, TScopeInfoMap::value_type(key, value));
        value->m_ObjectInfo = &info;
    }
    else {
        _ASSERT(iter->second->HasObject());
        _ASSERT(&iter->second->GetObjectInfo_Base() == &info);
        scope_info = &dynamic_cast<CBioseq_set_ScopeInfo&>(*iter->second);
    }
    if ( !scope_info->m_TSE_Handle.m_TSE ) {
        scope_info->m_TSE_Handle = tse.m_TSE;
    }
    _ASSERT(scope_info->IsAttached());
    _ASSERT(scope_info->m_TSE_Handle.m_TSE);
    _ASSERT(scope_info->HasObject());
    return TBioseq_set_Lock(*scope_info);
}

// Action A5.
CTSE_ScopeInfo::TBioseq_Lock
CTSE_ScopeInfo::GetBioseqLock(CRef<CBioseq_ScopeInfo> info,
                              CConstRef<CBioseq_Info> bioseq)
{
    CMutexGuard guard(m_TSE_LockMutex);
    CTSE_ScopeUserLock tse(this);
    _ASSERT(m_TSE_Lock);
    if ( !info ) {
        // find CBioseq_ScopeInfo
        _ASSERT(bioseq);
        _ASSERT(bioseq->BelongsToTSE_Info(*m_TSE_Lock));
        const CBioseq_Info::TId& ids = bioseq->GetId();
        if ( !ids.empty() ) {
            // named bioseq, look in Seq-id index only
            info = x_FindBioseqInfo(ids);
            if ( !info ) {
                info = x_CreateBioseqInfo(ids);
            }
        }
        else {
            // unnamed bioseq, look in object map, create if necessary
            TScopeInfoMapKey key(bioseq);
            TScopeInfoMap::iterator iter = m_ScopeInfoMap.lower_bound(key);
            if ( iter == m_ScopeInfoMap.end() || iter->first != key ) {
                info = new CBioseq_ScopeInfo(*this);
                TScopeInfoMapValue value(info);
                iter = m_ScopeInfoMap
                    .insert(iter, TScopeInfoMap::value_type(key, value));
                value->m_ObjectInfo = &*bioseq;
            }
            else {
                _ASSERT(iter->second->HasObject());
                _ASSERT(&iter->second->GetObjectInfo_Base() == &*bioseq);
                info.Reset(&dynamic_cast<CBioseq_ScopeInfo&>(*iter->second));
            }
            if ( !info->m_TSE_Handle.m_TSE ) {
                info->m_TSE_Handle = tse;
            }
            _ASSERT(info->IsAttached());
            _ASSERT(info->m_TSE_Handle.m_TSE);
            _ASSERT(info->HasObject());
            return TBioseq_Lock(*info);
        }
    }
    _ASSERT(info);
    _ASSERT(!info->IsDetached());
    // update CBioseq_ScopeInfo object
    if ( !info->HasObject() ) {
        if ( !bioseq ) {
            const CBioseq_ScopeInfo::TIds& ids = info->GetIds();
            if ( !ids.empty() ) {
                const CSeq_id_Handle& id = *ids.begin();
                bioseq = m_TSE_Lock->FindBioseq(id);
                _ASSERT(bioseq);
            }
            else {
                // unnamed bioseq without object - error,
                // this situation must be prevented by code.
                _ASSERT(0 && "CBioseq_ScopeInfo without ids and bioseq");
            }
        }
        _ASSERT(bioseq);
        _ASSERT(bioseq->GetId() == info->GetIds());
        TScopeInfoMapKey key(bioseq);
        TScopeInfoMapValue value(info);
        _VERIFY(m_ScopeInfoMap
                .insert(TScopeInfoMap::value_type(key, value)).second);
        info->m_ObjectInfo = &*bioseq;
        info->x_SetLock(tse, *bioseq);
    }
    if ( !info->m_TSE_Handle.m_TSE ) {
        info->m_TSE_Handle = tse;
    }
    _ASSERT(info->HasObject());
    _ASSERT(info->GetObjectInfo().BelongsToTSE_Info(*m_TSE_Lock));
    _ASSERT(m_ScopeInfoMap.find(TScopeInfoMapKey(&info->GetObjectInfo()))->second == info);
    _ASSERT(info->IsAttached());
    _ASSERT(info->m_TSE_Handle.m_TSE);
    _ASSERT(info->HasObject());
    return TBioseq_Lock(*info);
}


// Action A1
void CTSE_ScopeInfo::RemoveLastInfoLock(CScopeInfo_Base& info)
{
    if ( !info.m_TSE_Handle.m_TSE ) {
        // already unlocked
        return;
    }
    CRef<CTSE_ScopeInfo> self;
    {{
        CMutexGuard guard(m_TSE_LockMutex);
        if ( info.m_LockCounter.Get() > 0 ) {
            // already locked again
            return;
        }
        self = this; // to prevent deletion of this while mutex is locked.
        info.m_TSE_Handle.Reset();
    }}
}


// Find scope bioseq info by match: CConstRef<CBioseq_Info> & CSeq_id_Handle
// The problem is that CTSE_Info and CBioseq_Info may be unloaded and we
// cannot store pointers to them.
// However, we have to find the same CBioseq_ScopeInfo object.
// It is stored in m_BioseqById map under one of Bioseq's ids.
CRef<CBioseq_ScopeInfo>
CTSE_ScopeInfo::GetBioseqInfo(const SSeqMatch_Scope& match)
{
    _ASSERT(&*match.m_TSE_Lock == this);
    _ASSERT(match.m_Seq_id);
    _ASSERT(match.m_Bioseq);
    CRef<CBioseq_ScopeInfo> info;
    const CBioseq_Info::TId& ids = match.m_Bioseq->GetId();
    _ASSERT(find(ids.begin(), ids.end(), match.m_Seq_id) != ids.end());

    CMutexGuard guard(m_TSE_LockMutex);
    
    info = x_FindBioseqInfo(ids);
    if ( !info ) {
        info = x_CreateBioseqInfo(ids);
    }
    return info;
}


CRef<CBioseq_ScopeInfo>
CTSE_ScopeInfo::x_FindBioseqInfo(const TBioseqsIds& ids) const
{
    if ( !ids.empty() ) {
        const CSeq_id_Handle& id = *ids.begin();
        for ( TBioseqById::const_iterator it(m_BioseqById.lower_bound(id));
              it != m_BioseqById.end() && it->first == id; ++it ) {
            if ( it->second->GetIds() == ids ) {
                return it->second;
            }
        }
    }
    return null;
}


CRef<CBioseq_ScopeInfo>
CTSE_ScopeInfo::x_CreateBioseqInfo(const TBioseqsIds& ids)
{
    return Ref(new CBioseq_ScopeInfo(*this, ids));
}


void CTSE_ScopeInfo::x_IndexBioseq(const CSeq_id_Handle& id,
                                   CBioseq_ScopeInfo* info)
{
    m_BioseqById.insert(TBioseqById::value_type(id, Ref(info)));
}


void CTSE_ScopeInfo::x_UnindexBioseq(const CSeq_id_Handle& id,
                                     const CBioseq_ScopeInfo* info)
{
    for ( TBioseqById::iterator it = m_BioseqById.lower_bound(id);
          it != m_BioseqById.end() && it->first == id; ++it ) {
        if ( it->second == info ) {
            m_BioseqById.erase(it);
            return;
        }
    }
    _ASSERT(0 && "UnindexBioseq: CBioseq_ScopeInfo is not in index");
}

// Action A2.
void CTSE_ScopeInfo::ResetEntry(CSeq_entry_ScopeInfo& info)
{
    _ASSERT(info.IsAttached());
    CMutexGuard guard(m_TSE_LockMutex);
    info.GetNCObjectInfo().Reset();
    x_CleanRemovedObjects();
}

// Action A2.
void CTSE_ScopeInfo::RemoveEntry(CSeq_entry_ScopeInfo& info)
{
    _ASSERT(info.IsAttached());
    CMutexGuard guard(m_TSE_LockMutex);
    CSeq_entry_Info& entry = info.GetNCObjectInfo();
    entry.GetParentBioseq_set_Info().RemoveEntry(Ref(&entry));
    x_CleanRemovedObjects();
    _ASSERT(info.IsDetached());
}

// Action A2.
void CTSE_ScopeInfo::RemoveAnnot(CSeq_annot_ScopeInfo& info)
{
    _ASSERT(info.IsAttached());
    CMutexGuard guard(m_TSE_LockMutex);
    CSeq_annot_Info& annot = info.GetNCObjectInfo();
    annot.GetParentBioseq_Base_Info().RemoveAnnot(Ref(&annot));
    x_CleanRemovedObjects();
    _ASSERT(info.IsDetached());
}

// Action A3.
void CTSE_ScopeInfo::x_CleanRemovedObjects(void)
{
    _ASSERT(!m_UnloadedInfo);
    _ASSERT(m_TSE_Lock);
    for ( TScopeInfoMap::iterator it = m_ScopeInfoMap.begin();
          it != m_ScopeInfoMap.end(); ) {
        if ( !it->first->BelongsToTSE_Info(*m_TSE_Lock) ) {
            it->second->m_TSE_Handle.Reset();
            it->second->x_DetachTSE(this);
            m_ScopeInfoMap.erase(it++);
        }
        else {
            ++it;
        }
    }
    _ASSERT(m_TSE_Lock);
#ifdef _DEBUG
    ITERATE ( TBioseqById, it, m_BioseqById ) {
        _ASSERT(!it->second->IsDetached());
        _ASSERT(&it->second->x_GetTSE_ScopeInfo() == this);
        _ASSERT(!it->second->HasObject() || dynamic_cast<const CTSE_Info_Object&>(it->second->GetObjectInfo_Base()).BelongsToTSE_Info(*m_TSE_Lock));
    }
#endif
}

// Action A7.
void CTSE_ScopeInfo::AddEntry(CBioseq_set_ScopeInfo& parent,
                              CSeq_entry_ScopeInfo& child,
                              int index)
{
    _ASSERT(parent.IsAttached());
    _ASSERT(parent.m_LockCounter.Get() > 0);
    _ASSERT(child.IsDetached());
    _ASSERT(child.HasObject());
    _ASSERT(child.m_LockCounter.Get() > 0);
    _ASSERT(x_SameTSE(parent.GetTSE_Handle().x_GetTSE_Info()));

    CMutexGuard guard(m_TSE_LockMutex);
    parent.GetNCObjectInfo().AddEntry(Ref(&child.GetNCObjectInfo()), index);
    child.x_AttachTSE(this);
    TScopeInfoMapKey key(&child.GetObjectInfo());
    TScopeInfoMapValue value(&child);
    _VERIFY(m_ScopeInfoMap.insert(TScopeInfoMap::value_type(key, value)).second);
    child.m_TSE_Handle = parent.m_TSE_Handle;

    _ASSERT(child.IsAttached());
    _ASSERT(child.m_TSE_Handle.m_TSE);
    _ASSERT(child.HasObject());
}


// Action A7.
void CTSE_ScopeInfo::AddAnnot(CSeq_entry_ScopeInfo& parent,
                              CSeq_annot_ScopeInfo& child)
{
    _ASSERT(parent.IsAttached());
    _ASSERT(parent.m_LockCounter.Get() > 0);
    _ASSERT(child.IsDetached());
    _ASSERT(child.HasObject());
    _ASSERT(child.m_LockCounter.Get() > 0);
    _ASSERT(x_SameTSE(parent.GetTSE_Handle().x_GetTSE_Info()));

    CMutexGuard guard(m_TSE_LockMutex);
    parent.GetNCObjectInfo().AddAnnot(Ref(&child.GetNCObjectInfo()));
    child.x_AttachTSE(this);
    TScopeInfoMapKey key(&child.GetObjectInfo());
    TScopeInfoMapValue value(&child);
    _VERIFY(m_ScopeInfoMap.insert(TScopeInfoMap::value_type(key, value)).second);
    child.m_TSE_Handle = parent.m_TSE_Handle;

    _ASSERT(child.IsAttached());
    _ASSERT(child.m_TSE_Handle.m_TSE);
    _ASSERT(child.HasObject());
}


// Action A7.
void CTSE_ScopeInfo::SelectSet(CSeq_entry_ScopeInfo& parent,
                               CBioseq_set_ScopeInfo& child)
{
    _ASSERT(parent.IsAttached());
    _ASSERT(parent.m_LockCounter.Get() > 0);
    _ASSERT(parent.GetObjectInfo().Which() == CSeq_entry::e_not_set);
    _ASSERT(child.IsDetached());
    _ASSERT(child.HasObject());
    _ASSERT(child.m_LockCounter.Get() > 0);
    _ASSERT(x_SameTSE(parent.GetTSE_Handle().x_GetTSE_Info()));

    CMutexGuard guard(m_TSE_LockMutex);
    parent.GetNCObjectInfo().SelectSet(child.GetNCObjectInfo());
    child.x_AttachTSE(this);
    TScopeInfoMapKey key(&child.GetObjectInfo());
    TScopeInfoMapValue value(&child);
    _VERIFY(m_ScopeInfoMap.insert(TScopeInfoMap::value_type(key, value)).second);
    child.m_TSE_Handle = parent.m_TSE_Handle;

    _ASSERT(child.IsAttached());
    _ASSERT(child.m_TSE_Handle.m_TSE);
    _ASSERT(child.HasObject());
}


// Action A7.
void CTSE_ScopeInfo::SelectSeq(CSeq_entry_ScopeInfo& parent,
                               CBioseq_ScopeInfo& child)
{
    _ASSERT(parent.IsAttached());
    _ASSERT(parent.m_LockCounter.Get() > 0);
    _ASSERT(parent.GetObjectInfo().Which() == CSeq_entry::e_not_set);
    _ASSERT(child.IsDetached());
    _ASSERT(child.HasObject());
    _ASSERT(child.m_LockCounter.Get() > 0);
    _ASSERT(x_SameTSE(parent.GetTSE_Handle().x_GetTSE_Info()));

    CMutexGuard guard(m_TSE_LockMutex);
    parent.GetNCObjectInfo().SelectSeq(child.GetNCObjectInfo());
    child.x_AttachTSE(this);
    TScopeInfoMapKey key(&child.GetObjectInfo());
    TScopeInfoMapValue value(&child);
    _VERIFY(m_ScopeInfoMap.insert(TScopeInfoMap::value_type(key, value)).second);
    child.m_TSE_Handle = parent.m_TSE_Handle;

    _ASSERT(child.IsAttached());
    _ASSERT(child.m_TSE_Handle.m_TSE);
    _ASSERT(child.HasObject());
}


/////////////////////////////////////////////////////////////////////////////
// CBioseq_ScopeInfo
/////////////////////////////////////////////////////////////////////////////


CBioseq_ScopeInfo::CBioseq_ScopeInfo(TBlobStateFlags flags)
    : m_BlobState(flags)
{
}


CBioseq_ScopeInfo::CBioseq_ScopeInfo(CTSE_ScopeInfo& tse)
    : m_BlobState(CBioseq_Handle::fState_none)
{
    x_AttachTSE(&tse);
}


CBioseq_ScopeInfo::CBioseq_ScopeInfo(CTSE_ScopeInfo& tse,
                                     const TIds& ids)
    : m_Ids(ids),
      m_BlobState(CBioseq_Handle::fState_none)
{
    x_AttachTSE(&tse);
}


CBioseq_ScopeInfo::~CBioseq_ScopeInfo(void)
{
}


const CBioseq_ScopeInfo::TIndexIds* CBioseq_ScopeInfo::GetIndexIds(void) const
{
    const TIds& ids = GetIds();
    return ids.empty()? 0: &ids;
}


bool CBioseq_ScopeInfo::HasBioseq(void) const
{
    return (GetBlobState() & CBioseq_Handle::fState_no_data) == 0;
}


CBioseq_ScopeInfo::TBioseq_Lock
CBioseq_ScopeInfo::GetLock(CConstRef<CBioseq_Info> bioseq)
{
    return x_GetTSE_ScopeInfo().GetBioseqLock(Ref(this), bioseq);
}


void CBioseq_ScopeInfo::x_AttachTSE(CTSE_ScopeInfo* tse)
{
    CScopeInfo_Base::x_AttachTSE(tse);
    ITERATE ( TIds, it, GetIds() ) {
        tse->x_IndexBioseq(*it, this);
    }
}

void CBioseq_ScopeInfo::x_DetachTSE(CTSE_ScopeInfo* tse)
{
    m_SynCache.Reset();
    m_BioseqAnnotRef_Info.Reset();
    ITERATE ( TIds, it, GetIds() ) {
        tse->x_UnindexBioseq(*it, this);
    }
    CScopeInfo_Base::x_DetachTSE(tse);
}


void CBioseq_ScopeInfo::x_ForgetTSE(CTSE_ScopeInfo* tse)
{
    m_SynCache.Reset();
    m_BioseqAnnotRef_Info.Reset();
    CScopeInfo_Base::x_ForgetTSE(tse);
}


string CBioseq_ScopeInfo::IdString(void) const
{
    CNcbiOstrstream os;
    const TIds& ids = GetIds();
    ITERATE ( TIds, it, ids ) {
        if ( it != ids.begin() )
            os << " | ";
        os << it->AsString();
    }
    return CNcbiOstrstreamToString(os);
}


void CBioseq_ScopeInfo::ResetId(void)
{
    _ASSERT(HasObject());
    const_cast<CBioseq_Info&>(GetObjectInfo()).ResetId();
    ITERATE ( TIds, it, GetIds() ) {
        x_GetTSE_ScopeInfo().x_UnindexBioseq(*it, this);
    }
    m_Ids.clear();
}


bool CBioseq_ScopeInfo::AddId(const CSeq_id_Handle& id)
{
    _ASSERT(HasObject());
    if ( !const_cast<CBioseq_Info&>(GetObjectInfo()).AddId(id) ) {
        return false;
    }
    m_Ids.push_back(id);
    x_GetTSE_ScopeInfo().x_IndexBioseq(id, this);
    return true;
}


bool CBioseq_ScopeInfo::RemoveId(const CSeq_id_Handle& id)
{
    _ASSERT(HasObject());
    if ( !const_cast<CBioseq_Info&>(GetObjectInfo()).RemoveId(id) ) {
        return false;
    }
    TIds::iterator it = find(m_Ids.begin(), m_Ids.end(), id);
    _ASSERT(it != m_Ids.end());
    m_Ids.erase(it);
    x_GetTSE_ScopeInfo().x_UnindexBioseq(id, this);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// SSeq_id_ScopeInfo
/////////////////////////////////////////////////////////////////////////////

SSeq_id_ScopeInfo::SSeq_id_ScopeInfo(void)
{
}

SSeq_id_ScopeInfo::~SSeq_id_ScopeInfo(void)
{
}

/////////////////////////////////////////////////////////////////////////////
// CSynonymsSet
/////////////////////////////////////////////////////////////////////////////

CSynonymsSet::CSynonymsSet(void)
{
}


CSynonymsSet::~CSynonymsSet(void)
{
}


CSeq_id_Handle CSynonymsSet::GetSeq_id_Handle(const const_iterator& iter)
{
    return (*iter)->first;
}


CBioseq_Handle CSynonymsSet::GetBioseqHandle(const const_iterator& iter)
{
    return CBioseq_Handle((*iter)->first, *(*iter)->second.m_Bioseq_Info);
}


bool CSynonymsSet::ContainsSynonym(const CSeq_id_Handle& id) const
{
   ITERATE ( TIdSet, iter, m_IdSet ) {
        if ( (*iter)->first == id ) {
            return true;
        }
    }
    return false;
}


void CSynonymsSet::AddSynonym(const value_type& syn)
{
    _ASSERT(!ContainsSynonym(syn->first));
    m_IdSet.push_back(syn);
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.20  2005/07/14 17:04:14  vasilche
* Fixed detaching from data loader.
* Implemented 'Removed' handles.
* Use 'Removed' handles when transferring object from one place to another.
* Fixed MT locking when removing/unlocking handles, clearing scope's history.
*
* Revision 1.19  2005/06/27 18:17:04  vasilche
* Allow getting CBioseq_set_Handle from CBioseq_set.
*
* Revision 1.18  2005/06/24 19:14:13  vasilche
* Disabled excessive _TRACE messages.
*
* Revision 1.17  2005/06/22 14:27:31  vasilche
* Implemented copying of shared Seq-entries at edit request.
* Added invalidation of handles to removed objects.
*
* Revision 1.16  2005/04/20 15:45:36  vasilche
* Fixed removal of TSE from scope's history.
*
* Revision 1.15  2005/03/15 19:14:27  vasilche
* Correctly update and check  bioseq ids in split blobs.
*
* Revision 1.14  2005/03/14 18:17:15  grichenk
* Added CScope::RemoveFromHistory(), CScope::RemoveTopLevelSeqEntry() and
* CScope::RemoveDataLoader(). Added requested seq-id information to
* CTSE_Info.
*
* Revision 1.13  2005/03/14 17:05:56  vasilche
* Thread safe retrieval of configuration variables.
*
* Revision 1.12  2005/01/05 18:45:57  vasilche
* Added GetConfigXxx() functions.
*
* Revision 1.11  2004/12/28 18:39:46  vasilche
* Added lost scope lock in CTSE_Handle.
*
* Revision 1.10  2004/12/22 15:56:26  vasilche
* Used deep copying of CPriorityTree in AddScope().
* Implemented auto-release of unused TSEs in scope.
* Introduced CTSE_Handle.
* Fixed annotation collection.
* Removed too long cvs log.
*
* Revision 1.9  2004/10/25 16:53:32  vasilche
* Added suppord for orphan annotations.
*
* Revision 1.8  2004/08/04 14:53:26  vasilche
* Revamped object manager:
* 1. Changed TSE locking scheme
* 2. TSE cache is maintained by CDataSource.
* 3. CObjectManager::GetInstance() doesn't hold CRef<> on the object manager.
* 4. Fixed processing of split data.
*
* Revision 1.7  2004/05/21 21:42:12  gorelenk
* Added PCH ncbi_pch.hpp
*
* Revision 1.6  2003/11/17 16:03:13  grichenk
* Throw exception in CBioseq_Handle if the parent scope has been reset
*
* Revision 1.5  2003/09/30 16:22:03  vasilche
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
* Revision 1.4  2003/06/19 19:48:16  vasilche
* Removed unnecessary copy constructor of SSeq_id_ScopeInfo.
*
* Revision 1.3  2003/06/19 19:31:23  vasilche
* Added missing CBioseq_ScopeInfo destructor.
*
* Revision 1.2  2003/06/19 19:08:55  vasilche
* Added explicit constructor/destructor.
*
* Revision 1.1  2003/06/19 18:23:46  vasilche
* Added several CXxx_ScopeInfo classes for CScope related information.
* CBioseq_Handle now uses reference to CBioseq_ScopeInfo.
* Some fine tuning of locking in CScope.
*
*
* ===========================================================================
*/
