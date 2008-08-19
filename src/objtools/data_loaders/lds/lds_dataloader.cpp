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
 * Author: Anatoliy Kuznetsov
 *
 * File Description:  LDS dataloader. Implementations.
 *
 */


#include <ncbi_pch.hpp>

#include <objtools/data_loaders/lds/lds_dataloader.hpp>
#include <objtools/lds/lds_reader.hpp>
#include <objtools/lds/lds_util.hpp>
#include <objtools/lds/lds.hpp>
#include <objtools/lds/lds_object.hpp>
#include <objtools/error_codes.hpp>

#include <objects/general/Object_id.hpp>

#include <bdb/bdb_util.hpp>

#include <objmgr/impl/handle_range_map.hpp>
#include <objmgr/impl/tse_info.hpp>
#include <objmgr/impl/tse_loadlock.hpp>
#include <objmgr/impl/data_source.hpp>
#include <objmgr/data_loader_factory.hpp>

#include <corelib/plugin_manager.hpp>
#include <corelib/plugin_manager_impl.hpp>
#include <corelib/plugin_manager_store.hpp>


#define NCBI_USE_ERRCODE_X   Objtools_LDS_Loader

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)

DEFINE_STATIC_FAST_MUTEX(sx_LDS_Lock);

#define LDS_GUARD() CFastMutexGuard guard(sx_LDS_Lock)



/// Objects scanner
///
/// @internal
///
class CLDS_FindSeqIdFunc
{
public:
    typedef vector<CLDS_Query::SObjectDescr>  TResult;

public:
    CLDS_FindSeqIdFunc(CLDS_Database* lds,
                       CLDS_Query& query,
                       const CHandleRangeMap&  hrmap)
        : m_LDS_db(lds),
          m_LDS_query(query),
          m_HrMap(hrmap)
        {}

    void AddResult(int object_id0, int parent_id0, int tse_id0) {
        int object_id = tse_id0 ? tse_id0 : object_id0;
        
        // check if we can extract seq-entry out of binary bioseq-set file
        //
        //   (this trick has been added by kuznets (Jan-12-2005) to read 
        //    molecules out of huge refseq files)
        CLDS_Query::SObjectDescr obj_descr =
            m_LDS_query.GetObjectDescr(m_LDS_db->GetObjTypeMap(),
                                       tse_id0, false/*do not trace to top*/);
        if ((obj_descr.is_object && obj_descr.id > 0)      &&
            (obj_descr.format == CFormatGuess::eBinaryASN) &&
            (obj_descr.type_str == "Bioseq-set")
            ) {
            obj_descr = m_LDS_query.GetTopSeqEntry(m_LDS_db->GetObjTypeMap(),
                                                   object_id);
        }
        m_Result.push_back(obj_descr);
    }

    void operator()(SLDS_ObjectDB& dbf)
    {
        if (dbf.primary_seqid.IsNull())
            return;
        int object_id = dbf.object_id;
        int parent_id = dbf.parent_object_id;
        int tse_id    = dbf.TSE_object_id;


        string seq_id_str = (const char*)dbf.primary_seqid;
        if (seq_id_str.empty())
            return;
        {{
            CRef<CSeq_id> seq_id_db;
            try {
                seq_id_db.Reset(new CSeq_id(seq_id_str));
            } catch (CSeqIdException&) {
                seq_id_db.Reset(new CSeq_id(CSeq_id::e_Local, seq_id_str));
            }

            ITERATE (CHandleRangeMap, it, m_HrMap) {
                CSeq_id_Handle seq_id_hnd = it->first;
                CConstRef<CSeq_id> seq_id = seq_id_hnd.GetSeqId();
                if (seq_id->Match(*seq_id_db)) {
                    AddResult(object_id, parent_id, tse_id);
/*
                    LOG_POST_X(1, Info << "LDS: Local object " << seq_id_str
                                       << " id=" << object_id << " matches "
                                       << seq_id->AsFastaString());
*/
                    return;
                }
            } // ITERATE
        }}

        // Primary seq_id scan gave no results
        // Trying supplemental aliases
/*
        m_db.object_attr_db.object_attr_id = object_id;
        if (m_db.object_attr_db.Fetch() != eBDB_Ok) {
            return;
        }

        if (m_db.object_attr_db.seq_ids.IsNull() || 
            m_db.object_attr_db.seq_ids.IsEmpty()) {
            return;
        }
*/
        if (dbf.seq_ids.IsNull() || 
            dbf.seq_ids.IsEmpty()) {
            return;
        }
        string attr_seq_ids((const char*)dbf.seq_ids);
        vector<string> seq_id_arr;
        
        NStr::Tokenize(attr_seq_ids, " ", seq_id_arr, NStr::eMergeDelims);

        ITERATE (vector<string>, it, seq_id_arr) {
            seq_id_str = *it;
            CRef<CSeq_id> seq_id_db;
            try {
                seq_id_db.Reset(new CSeq_id(seq_id_str));
            } catch (CSeqIdException&) {
                seq_id_db.Reset(new CSeq_id(CSeq_id::e_Local, seq_id_str));
            }

            {{
            
            ITERATE (CHandleRangeMap, it2, m_HrMap) {
                CSeq_id_Handle seq_id_hnd = it2->first;
                CConstRef<CSeq_id> seq_id = seq_id_hnd.GetSeqId();

                if (seq_id->Match(*seq_id_db)) {
                    AddResult(object_id, parent_id, tse_id);
/*
                    LOG_POST_X(2, Info << "LDS: Local object " << seq_id_str
                                       << " id=" << object_id << " matches "
                                       << seq_id->AsFastaString());
*/
                    return;
                }
            } // ITERATE
            
            }}

        } // ITERATE
    }

    void GetResult(TResult& result)
    {
        m_Result.swap(result);
    }
private:
    CLDS_Database*          m_LDS_db;      // Reference on the LDS database 
    CLDS_Query&             m_LDS_query;   // Query object
    const CHandleRangeMap&  m_HrMap;       ///< Range map of seq ids to search
    TResult                 m_Result;      ///< Search result (found objects)
};



CLDS_DataLoader::TRegisterLoaderInfo CLDS_DataLoader::RegisterInObjectManager(
    CObjectManager& om,
    CObjectManager::EIsDefault is_default,
    CObjectManager::TPriority priority)
{
    TSimpleMaker maker;
    CDataLoader::RegisterInObjectManager(om, maker, is_default, priority);
    return maker.GetRegisterInfo();
}


string CLDS_DataLoader::GetLoaderNameFromArgs(void)
{
    return "LDS_dataloader";
}


CLDS_DataLoader::TRegisterLoaderInfo CLDS_DataLoader::RegisterInObjectManager(
    CObjectManager& om,
    CLDS_Database& lds_db,
    CObjectManager::EIsDefault is_default,
    CObjectManager::TPriority priority)
{
    TDbMaker maker(lds_db);
    CDataLoader::RegisterInObjectManager(om, maker, is_default, priority);
    return maker.GetRegisterInfo();
}
string CLDS_DataLoader::GetLoaderNameFromArgs(CLDS_Database& lds_db)
{
    const string& alias = lds_db.GetAlias();
    if ( !alias.empty() ) {
        return "LDS_dataloader_" + alias;
    }
    return "LDS_dataloader_" + lds_db.GetDirName();
}

CLDS_DataLoader::CLDS_LoaderMaker::CLDS_LoaderMaker(const string& source_path,
                                                    const string& db_path,
                                                    const string& db_alias,
                                                    CLDS_Manager::ERecurse           recurse,
                                                    CLDS_Manager::EComputeControlSum csum)
    : m_SourcePath(source_path), m_DbPath(db_path), m_DbAlias(db_alias),
      m_Recurse(recurse), m_ControlSum(csum)
{
    m_Name = "LDS_dataloader_";
    m_Name += db_alias.empty() ? db_path : db_alias;
}

CDataLoader*
CLDS_DataLoader::CLDS_LoaderMaker::CreateLoader(void) const
{
    LDS_GUARD();

    CLDS_Manager mgr(m_SourcePath, m_DbPath, m_DbAlias);
    mgr.Index(m_Recurse, m_ControlSum);

    CLDS_DataLoader* dl = new CLDS_DataLoader(m_Name, mgr.ReleaseDB());
    return dl;
}

CLDS_DataLoader::TRegisterLoaderInfo CLDS_DataLoader::RegisterInObjectManager(
    CObjectManager& om,
    const string& source_path,
    const string& db_path,
    const string& db_alias,
    CLDS_Manager::ERecurse recurse,
    CLDS_Manager::EComputeControlSum csum,
    CObjectManager::EIsDefault is_default,
    CObjectManager::TPriority priority
    )
{
    CLDS_LoaderMaker maker(source_path, db_path, db_alias, recurse, csum);
    CDataLoader::RegisterInObjectManager(om, maker, is_default, priority);
    return maker.GetRegisterInfo();
}

CLDS_DataLoader::CLDS_DataLoader(void)
    : CDataLoader(GetLoaderNameFromArgs()),
      m_LDS_db(NULL),
      m_OwnDatabase(false)
{
}


CLDS_DataLoader::CLDS_DataLoader(const string& dl_name)
    : CDataLoader(dl_name),
      m_LDS_db(NULL),
      m_OwnDatabase(false)
{
}

CLDS_DataLoader::CLDS_DataLoader(const string& dl_name,
                                 CLDS_Database& lds_db)
 : CDataLoader(dl_name),
   m_LDS_db(&lds_db),
   m_OwnDatabase(false)
{
}

CLDS_DataLoader::CLDS_DataLoader(const string& dl_name,
                                 CLDS_Database* lds_db)
 : CDataLoader(dl_name),
   m_LDS_db(lds_db),
   m_OwnDatabase(true)
{
}
/*
CLDS_DataLoader::CLDS_DataLoader(const string& dl_name,
                                 const pair<string,string>& paths)
 : CDataLoader(dl_name),
   m_LDS_db(NULL),
   m_OwnDatabase(true)
{
    ///?? Should a lds db be reindexed here ?????
    //    auto_ptr<CLDS_Database> lds(new CLDS_Database(db_path, kEmptyStr));
    CLDS_Manager mgr(paths.first, paths.second);
    LDS_GUARD();
    //lds->Open();
    m_LDS_db = mgr.ReleaseDB();
    
}
*/
CLDS_DataLoader::~CLDS_DataLoader()
{
    if (m_OwnDatabase)
        delete m_LDS_db;
}


void CLDS_DataLoader::SetDatabase(CLDS_Database& lds_db,
                                  EOwnership owner,
                                  const string&  dl_name)
{
    LDS_GUARD();
    
    if (m_LDS_db && m_OwnDatabase) {
        delete m_LDS_db;
    }
    m_OwnDatabase = owner == eTakeOwnership;
    m_LDS_db = &lds_db;
    SetName(dl_name);
}

CLDS_Database& CLDS_DataLoader::GetDatabase()
{
    if( !m_LDS_db )
        throw runtime_error("LDS database is not inisialized.");
    return *m_LDS_db;
}

CDataLoader::TTSE_LockSet
CLDS_DataLoader::GetRecords(const CSeq_id_Handle& idh,
                            EChoice /* choice */)
{
    unsigned db_recovery_count = 0;
    while (true) {
        try {
            TTSE_LockSet locks;
            CHandleRangeMap hrmap;
            hrmap.AddRange(idh, CRange<TSeqPos>::GetWhole(), eNa_strand_unknown);

            CLDS_FindSeqIdFunc::TResult result;

            {{
                LDS_GUARD();
                
                SLDS_TablesCollection& db = m_LDS_db->GetTables();
                CLDS_Query lds_query(*m_LDS_db);
                
                // index screening
                CLDS_Set       cand_set;
                {{
                    CBDB_FileCursor cur_int_idx(db.obj_seqid_int_idx);
                    cur_int_idx.SetCondition(CBDB_FileCursor::eEQ);

                    CBDB_FileCursor cur_txt_idx(db.obj_seqid_txt_idx);
                    cur_txt_idx.SetCondition(CBDB_FileCursor::eEQ);

                    SLDS_SeqIdBase sbase;

                    ITERATE (CHandleRangeMap, it, hrmap) {
                        CSeq_id_Handle seq_id_hnd = it->first;
                        CConstRef<CSeq_id> seq_id = seq_id_hnd.GetSeqId();

                        LDS_GetSequenceBase(*seq_id, &sbase);

                        if (!sbase.str_id.empty()) {
                            NStr::ToUpper(sbase.str_id);
                        }

                        lds_query.ScreenSequence(sbase, 
                                                 &cand_set, 
                                                 cur_int_idx, 
                                                 cur_txt_idx);


                    } // ITER
                }}

                // finding matching sequences using pre-screened result set

                if (cand_set.any()) {
                    CLDS_FindSeqIdFunc search_func(m_LDS_db, lds_query, hrmap);
                    BDB_iterate_file(db.object_db, 
                                     cand_set.first(), cand_set.end(),
                                     search_func);
                    search_func.GetResult(result);
                }
            }}

            // load objects

            CDataSource* data_source = GetDataSource();
            _ASSERT(data_source);

            ITERATE ( CLDS_FindSeqIdFunc::TResult, it, result ) {
                const CLDS_Query::SObjectDescr& obj_descr = *it;
                int object_id = obj_descr.id;
                TBlobId blob_id(new CBlobIdInt(object_id));
                CTSE_LoadLock load_lock =
                    data_source->GetTSE_LoadLock(blob_id);
                if ( !load_lock.IsLoaded() ) {
                    CRef<CSeq_entry> seq_entry = LDS_LoadTSE(obj_descr);
                    if ( !seq_entry ) {
                        NCBI_THROW2(CBlobStateException, eBlobStateError,
                                    "cannot load blob",
                                    CBioseq_Handle::fState_no_data);
                    }
                    load_lock->SetSeq_entry(*seq_entry);
                    load_lock.SetLoaded();
                }
                locks.insert(load_lock);
            }

            return locks;
        } 
        catch (CBDB_ErrnoException& ex) {
            if ( !ex.IsRecovery() || ++db_recovery_count > 10) {
                throw;
            }
            ERR_POST_X(3, "LDS Database returned db recovery error... Reopening.");
            LDS_GUARD();
            m_LDS_db->ReOpen();
        }
    } // while
}


END_SCOPE(objects)

// ===========================================================================

USING_SCOPE(objects);

void DataLoaders_Register_LDS(void)
{
    RegisterEntryPoint<CDataLoader>(NCBI_EntryPoint_DataLoader_LDS);
}


const string kDataLoader_LDS_DriverName("lds");

class CLDS_DataLoaderCF : public CDataLoaderFactory
{
public:
    CLDS_DataLoaderCF(void)
        : CDataLoaderFactory(kDataLoader_LDS_DriverName) {}
    virtual ~CLDS_DataLoaderCF(void) {}

protected:
    virtual CDataLoader* CreateAndRegister(
        CObjectManager& om,
        const TPluginManagerParamTree* params) const;
};


CDataLoader* CLDS_DataLoaderCF::CreateAndRegister(
    CObjectManager& om,
    const TPluginManagerParamTree* params) const
{
    if ( !ValidParams(params) ) {
        // Use constructor without arguments
        return CLDS_DataLoader::RegisterInObjectManager(om).GetLoader();
    }
    // Parse params, select constructor
/*    
    const string& database_str =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_Database, false);
*/
    const string& db_path =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_DbPath, false);

    string source_path =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_SourcePath, false);

    string db_alias =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_DbAlias, false);

    string recurse_str =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_RecurseSubDir, false, "true");
    bool recurse = NStr::StringToBool(recurse_str);
    CLDS_Manager::ERecurse recurse_subdir = 
        recurse ? 
            CLDS_Manager::eRecurseSubDirs : CLDS_Manager::eDontRecurse;

    string control_sum_str =
        GetParam(GetDriverName(), params,
                 kCFParam_LDS_ControlSum, false, "true");
    bool csum = NStr::StringToBool(control_sum_str);
    CLDS_Manager::EComputeControlSum control_sum =
       csum ? 
         CLDS_Manager::eComputeControlSum : CLDS_Manager::eNoControlSum;
              
// commented out...
// suspicious code, expected somebody will pass database as a stringified pointer
// (code has a bug in casts which should always give NULL
/*
    if ( !database_str.empty() ) {
        // Use existing database
        CLDS_Database* db = dynamic_cast<CLDS_Database*>(
            static_cast<CDataLoader*>(
            const_cast<void*>(NStr::StringToPtr(database_str))));
        if ( db ) {
            return CLDS_DataLoader::RegisterInObjectManager(
                om,
                *db,
                GetIsDefault(params),
                GetPriority(params)).GetLoader();
        }
    }
*/
    if ( !db_path.empty() || !source_path.empty() ) {
        if (source_path.empty())
            source_path = db_path;
        // Use db path
        return CLDS_DataLoader::RegisterInObjectManager(
            om,
            source_path,
            db_path,
            db_alias,
            recurse_subdir,
            control_sum,
            GetIsDefault(params),
            GetPriority(params)).GetLoader();
    }
    // IsDefault and Priority arguments may be specified
    return CLDS_DataLoader::RegisterInObjectManager(
        om,
        GetIsDefault(params),
        GetPriority(params)).GetLoader();
}


void NCBI_EntryPoint_DataLoader_LDS(
    CPluginManager<CDataLoader>::TDriverInfoList&   info_list,
    CPluginManager<CDataLoader>::EEntryPointRequest method)
{
    CHostEntryPointImpl<CLDS_DataLoaderCF>::NCBI_EntryPointImpl(info_list, method);
}


void NCBI_EntryPoint_xloader_lds(
    CPluginManager<objects::CDataLoader>::TDriverInfoList&   info_list,
    CPluginManager<objects::CDataLoader>::EEntryPointRequest method)
{
    NCBI_EntryPoint_DataLoader_LDS(info_list, method);
}


END_NCBI_SCOPE
