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
* Author:  Aleksey Grichenko
*
* File Description:
*   C++ object manager performance test.
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <common/test_data_path.h>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <util/random_gen.hpp>
#include <connect/ncbi_core_cxx.hpp>
#include <dbapi/driver/drivers.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/object_manager.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objtools/data_loaders/genbank/readers.hpp>
#include <sstream>


BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


/////////////////////////////////////////////////////////////////////////////
//
//  Test application
//


class CPerfTestApp : public CNcbiApplication
{
public:
    virtual void Init(void);
    virtual int  Run (void);

    void TestIds(void);
    void TestId(CSeq_id_Handle idh);

private:
    typedef set<CSeq_id_Handle> TIds;

    void x_LoadIds(CNcbiIstream& in);

    void x_ParseResults(istream& istr, bool csv);

    CSeq_id_Handle x_NextId(void) {
        CFastMutexGuard guard(m_IdsMutex);
        CSeq_id_Handle ret;
        if (m_NextId != m_Ids.end()) {
            ret = *m_NextId;
            ++m_NextId;
        }
        return ret;
    }

    bool m_GetInfo = true;
    bool m_GetData = true;
    bool m_PrintInfo = false;
    bool m_PrintBlobId = false;
    bool m_PrintData = false;
    bool m_AllIds = false;

    CFastMutex m_IdsMutex;
    CRef<CScope> m_Scope;
    TIds m_Ids;
    TIds::const_iterator m_NextId;
    char m_TimeStat = 'r';
};


class CTestThread : public CThread
{
public:
    CTestThread(CPerfTestApp& app) : m_App(app) {}

protected:
    void* Main(void) override;

private:
    CPerfTestApp& m_App;
};


class CAtomicOut : public stringstream
{
public:
    ~CAtomicOut() {
        CFastMutexGuard guard(sm_Mutex);
        cout << str();
    }

private:
    static CFastMutex sm_Mutex;
};


CFastMutex CAtomicOut::sm_Mutex;


void* CTestThread::Main(void)
{
    m_App.TestIds();
    return nullptr;
}


void CPerfTestApp::Init(void)
{
    CONNECT_Init(&GetConfig());

    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // Input: list of ids or range of GIs.
    arg_desc->AddOptionalKey("ids", "IdsFile",
        "file with seq-ids to load",
        CArgDescriptions::eInputFile);
    arg_desc->AddOptionalKey("gi_from", "GiFrom",
        "starting GI",
        CArgDescriptions::eIntId);
    arg_desc->AddOptionalKey("gi_to", "GiTo",
        "ending GI",
        CArgDescriptions::eIntId);
    arg_desc->SetDependency("ids", CArgDescriptions::eExcludes, "gi_from");
    arg_desc->SetDependency("ids", CArgDescriptions::eExcludes, "gi_to");
    arg_desc->SetDependency("gi_from", CArgDescriptions::eRequires, "gi_to");
    arg_desc->SetDependency("gi_to", CArgDescriptions::eRequires, "gi_from");

    arg_desc->AddDefaultKey("count", "RepeatCount",
        "repeat test RepeatCount times",
        CArgDescriptions::eInteger, "1");
    arg_desc->AddDefaultKey("threads", "Threads",
        "number of threads to run (0 to run main thread only)",
        CArgDescriptions::eInteger, "0");

    // Data loader: genbank vs psg
    arg_desc->AddDefaultKey("loader", "DataLoader",
        "data loader and reader to use",
        CArgDescriptions::eString,
        "gb");
    arg_desc->SetConstraint("loader", &(*new CArgAllow_Strings,
        "gb", "psg"));

    // For genbank use id1/id2 vs pubseqos/pubseqos2
    arg_desc->AddFlag("pubseqos", "use pubseqos (with genbank only)");

    // Split vs unsplit (id1/id2, pubseqos/pubseqos2, psg tse option).
    arg_desc->AddFlag("no_split", "get only unsplit data");

    arg_desc->AddFlag("skip_info", "do not get sequence info");
    arg_desc->AddFlag("skip_data", "do not get complete TSE");
    arg_desc->AddFlag("print_info", "print sequence info");
    arg_desc->AddFlag("print_blob_id", "print blob-id");
    arg_desc->AddFlag("print_data", "print TSE");
    arg_desc->AddFlag("all_ids", "fetch all seq-ids from each thread");

    arg_desc->AddFlag("bulk_gi", "test bulk GetGis()");
    arg_desc->AddFlag("bulk_acc", "test bulk GetAccVers()");
    arg_desc->SetDependency("bulk_gi", CArgDescriptions::eExcludes, "bulk_acc");

    arg_desc->AddOptionalKey("stat", "StatFile",
        "File with performace test outputs",
        CArgDescriptions::eInputFile);
    arg_desc->SetDependency("ids", CArgDescriptions::eExcludes, "stat");
    arg_desc->SetDependency("ids", CArgDescriptions::eExcludes, "stat");
    arg_desc->SetDependency("gi_from", CArgDescriptions::eExcludes, "stat");
    arg_desc->SetDependency("gi_to", CArgDescriptions::eExcludes, "stat");
    arg_desc->AddDefaultKey("t", "time", "time to show", CArgDescriptions::eString, "r");
    arg_desc->SetConstraint("t", &(*new CArgAllow_Strings, "r", "u", "s"));
    arg_desc->SetDependency("t", CArgDescriptions::eRequires, "stat");
    arg_desc->AddFlag("csv", "Produce comma separated output");
    arg_desc->SetDependency("csv", CArgDescriptions::eRequires, "stat");

    string prog_description = "C++ object manager performance test\n";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              prog_description, false);

    SetupArgDescriptions(arg_desc.release());
}

int CPerfTestApp::Run(void)
{
    if (GetEnvironment().Get("NCBI_RUN_UNDER_INSPXE") == "1") {
        cout << "The test is disabled under Intel Inspector." << endl;
        return 0;
    }

    const CArgs& args = GetArgs();

    if (args["stat"]) {
        m_TimeStat = args["t"].AsString()[0];
        x_ParseResults(args["stat"].AsInputFile(), args["csv"]);
        return 0;
    }

    bool bulk_gi = args["bulk_gi"];
    bool bulk_acc = args["bulk_acc"];
    int repeat_count = args["count"].AsInteger();
    if (repeat_count <= 0) repeat_count = 1;
    int thread_count = args["threads"].AsInteger();
    if (thread_count < 0) thread_count = 0;
    m_GetInfo = !args["skip_info"];
    m_GetData = !args["skip_data"];
    m_PrintInfo = args["print_info"];
    m_PrintBlobId = args["print_blob_id"];
    m_PrintData = args["print_data"];
    m_AllIds = args["all_ids"];

    CRef<CObjectManager> om = CObjectManager::GetInstance();
    m_Scope.Reset(new CScope(*om));

#if defined(NCBI_OS_MSWIN)
    // Windows does not have unsetenv(), but putting an empty value works fine.
    putenv("GENBANK_LOADER_PSG=");
#else
    // On Linux putting an empty value results in empty string being fetched by CParam, not NULL.
    unsetenv("GENBANK_LOADER_PSG");
#endif
    string loader = args["loader"].AsString();
    if (loader == "gb") {
        string reader;
        if (args["pubseqos"]) {
#ifdef HAVE_PUBSEQ_OS
            DBAPI_RegisterDriver_FTDS();
            if (args["no_split"]) {
                reader = "pubseqos";
                GenBankReaders_Register_Pubseq();
            }
            else {
                reader = "pubseqos2";
                GenBankReaders_Register_Pubseq2();
            }
#else
            ERR_POST("PubseqOS reader not supported");
            return 0;
#endif
        }
        else {
            if (args["no_split"]) {
                GenBankReaders_Register_Id1();
                reader = "id1";
            }
            else {
                GenBankReaders_Register_Id2();
                reader = "id2";
            }
        }
        CGBDataLoader::RegisterInObjectManager(*om, reader);
    }
    else if (loader == "psg") {
        GetRWConfig().Set("genbank", "loader_psg", "t");
        GetRWConfig().Set("psg_loader", "no_split", args["no_split"] ? "t" : "f");
        CGBDataLoader::RegisterInObjectManager(*om);
    }
    m_Scope->AddDefaults();

    if (args["ids"]) {
        string ids_file = args["ids"].AsString();
        if (!ids_file.empty() && ids_file[0] == '_') {
            string path = CFile::MakePath(
                CFile::MakePath(NCBI_GetTestDataPath(), "pubseq_gateway"),
                string("ids") + ids_file);
            if (CDirEntry(path).Exists()) {
                CNcbiIfstream in(path.c_str());
                x_LoadIds(in);
            }
            else {
                ERR_POST("Input file does not exist: " << path);
                return -1;
            }
        }
        else {
            x_LoadIds(args["ids"].AsInputFile());
        }
    }
    else {
        if (!args["gi_from"] || !args["gi_to"]) {
            ERR_POST("No input data specified. Either ids or gi_from/gi_to arguments must be provided.");
            return -1;
        }
        TGi gi_from = args["gi_from"].AsIntId();
        TGi gi_to = args["gi_to"].AsIntId();
        for (TGi gi = gi_from; gi < gi_to; ++gi) {
            m_Ids.insert(CSeq_id_Handle::GetGiHandle(gi));
        }
    }

    m_NextId = m_Ids.begin();
    cout << "Testing " << m_Ids.size() << " seq-ids" << endl;

    CStopWatch sw;
    sw.Start();
    for (int i = 0; i < repeat_count; ++i) {
        CStopWatch sw2;
        sw2.Start();

        if (bulk_gi) {
            CScope::TIds bulk_ids;
            bulk_ids.insert(bulk_ids.end(), m_Ids.begin(), m_Ids.end());
            CScope::TGIs gis = m_Scope->GetGis(bulk_ids);
            if (m_PrintData) {
                ITERATE(CScope::TGIs, gi, gis) {
                    cout << *gi << endl;
                }
            }
        }
        else if (bulk_acc) {
            CScope::TIds bulk_ids;
            bulk_ids.insert(bulk_ids.end(), m_Ids.begin(), m_Ids.end());
            CScope::TIds ids = m_Scope->GetAccVers(bulk_ids);
            if (m_PrintData) {
                ITERATE(CScope::TIds, id, ids) {
                    cout << id->AsString() << endl;
                }
            }
        }
        else {
            if (thread_count == 0) {
                TestIds();
            }
            else {
                vector<CRef<CThread>> threads;
                for (int i = 0; i < thread_count; ++i) {
                    CRef<CThread> thr(new CTestThread(*this));
                    threads.push_back(thr);
                    thr->Run();
                }
                for (int i = 0; i < thread_count; ++i) {
                    threads[i]->Join(nullptr);
                }
            }
        }

        if (repeat_count > 1) {
            cout << "Cycle " << i << " finished in " << sw2.AsSmartString() << endl;
        }
    }

    double t_real, t_user, t_sys;
    CCurrentProcess::GetTimes(&t_real, &t_user, &t_sys);

    cout << "Done: r=" << t_real << "; u=" << t_user << "; s=" << t_sys << "; ";
    if (args["ids"]) {
        cout << "'" << args["ids"].AsString() << "'";
    }
    else {
        cout << args["gi_from"].AsIntId() << ".." << args["gi_to"].AsIntId();
    }
    cout << "; " << loader;
    if (loader == "gb") cout << (args["pubseqos"] ? "/pubseqos" : "/id");
    cout << (args["no_split"] ? "/no_split" : "/split");
    cout << "; " << m_Ids.size() << " ids; ";
    if (bulk_gi) {
        cout << "bulk gi ";
    }
    else if (bulk_acc) {
        cout << "bulk acc ";
    }
    else {
        cout << thread_count << " thr; ";
    }
    cout << repeat_count << " rep";
    cout << endl;

    return 0;
}


void CPerfTestApp::x_LoadIds(CNcbiIstream& in)
{
    CStopWatch sw;
    sw.Start();

    while (in.good() && !in.eof()) {
        string line;
        getline(in, line);
        NStr::TruncateSpacesInPlace(line);
        if (line.empty()) continue;
        CSeq_id id(line);
        CSeq_id_Handle idh = CSeq_id_Handle::GetHandle(id);
        m_Ids.insert(idh);
    }
}


double ParseDouble(const string& s, size_t& pos)
{
    pos = s.find('=', pos);
    if (pos == NPOS) return -1;
    size_t pos2 = s.find(';', pos);
    if (pos2 == NPOS) return -1;
    double ret = NStr::StringToDouble(s.substr(pos + 1, pos2 - pos - 1));
    pos = pos2 + 1;
    return ret;
}


void CPerfTestApp::x_ParseResults(istream& istr, bool csv)
{
    struct SPerfKey {
        string data;
        string rep;
        string threads;
        string split;
        string loader;
        string id_count;

        SPerfKey(const string& key)
        {
            vector<string> parts;
            NStr::Split(key, " ;/'", parts, NStr::fSplit_Tokenize);
            if (parts.size() < 9) {
                data = key;
                return;
            }
            size_t idx = 0;
            data = parts[idx++];
            loader = parts[idx++];
            if (loader == "gb") {
                loader += "/" + parts[idx++];
            }
            split = parts[idx++];
            id_count = parts[idx++];
            idx++;
            threads = parts[idx++];
            idx++;
            rep = parts[idx++];
        }

        static string GetTitleRow(bool csv)
        {
            stringstream os;
            if (csv) {
                os << "name,rep,thr,loader,split,ids";
            }
            else {
                os << left
                    << setw(21) << "name"
                    << setw(6) << "rep"
                    << setw(6) << "thr"
                    << setw(13) << "loader"
                    << setw(10) << "split"
                    << setw(7) << "ids";
            }
            return os.str();
        }

        string AsString(bool csv) const
        {
            stringstream os;
            if (csv) {
                os << data << ',' << rep << ',' << threads << ',' << loader << ',' << split << ',' << id_count;
            }
            else {
                os << left
                    << setw(20) << data << ' '
                    << setw(5) << rep << ' '
                    << setw(5) << threads << ' '
                    << setw(12) << loader << ' '
                    << setw(9) << split << ' '
                    << setw(6) << id_count;
            }
            return os.str();
        }

        bool operator<(const SPerfKey& other) const
        {
            if (data != other.data) return data < other.data;
            if (rep != other.rep) return rep < other.rep;
            if (threads != other.threads) return threads < other.threads;
            if (split != other.split) return split < other.split;
            if (loader != other.loader) return loader  < other.loader;
            if (id_count != other.id_count) return id_count < other.id_count;
            return false;
        }
    };

    // Done: r=1.07493; u=0.265625; s=0.109375; 'perf_ids1_acc'; gb/id/no_split; 7967 ids; 0 thr; 1 rep
    struct SPerfStat {
        double r;
        double u;
        double s;

        SPerfStat(double ar = 0, double au = 0, double as = 0) : r(ar), u(au), s(as) {}

        void Add(const SPerfStat& ps) {
            r += ps.r;
            u += ps.u;
            s += ps.s;
        }
    };

    typedef map<SPerfKey, vector<SPerfStat>> TStats;
    TStats stats;
    size_t name_len = 0;
    while (!istr.eof() && istr.good()) {
        string line;
        getline(istr, line);
        if (line.empty()) continue;
        if (!NStr::StartsWith(line, "Done: r=")) continue;
        size_t pos = 0;
        SPerfStat stat;
        stat.r = ParseDouble(line, pos);
        stat.u = ParseDouble(line, pos);
        stat.s = ParseDouble(line, pos);
        SPerfKey key(line.substr(pos + 1));
        stats[key].push_back(stat);
        string test_name = key.AsString(csv);
        if (test_name.size() > name_len) name_len = test_name.size();
    }
    if (csv) {
        cout << SPerfKey::GetTitleRow(csv)
            << ",min,max,avg,med,p75,count" << endl;
    }
    else {
        cout << left << setw(name_len + 2) << SPerfKey::GetTitleRow(csv)
            << right
            << setw(8) << "min"
            << setw(10) << "max"
            << setw(10) << "avg"
            << setw(10) << "med"
            << setw(10) << "p75"
            << setw(10) << "count" << endl;
    }
    NON_CONST_ITERATE(TStats, test_it, stats) {
        size_t count = test_it->second.size();
        if (count == 0) continue;
        vector<double> st;
        st.reserve(count);
        double avg = 0;
        ITERATE(vector<SPerfStat>, it, test_it->second) {
            double v;
            if (m_TimeStat == 'r') {
                v = it->r;
            }
            else if (m_TimeStat == 'u') {
                v = it->u;
            }
            else {
                v = it->s;
            }
            st.push_back(v);
            avg += v;
        }
        avg /= count;
        sort(st.begin(), st.end());

        double p50;
        size_t idx50 = count / 2;
        if (idx50 + 1 >= count) {
            p50 = st[idx50];
        }
        else if (count % 2 == 0) {
            p50 = (st[idx50] + st[idx50 + 1]) / 2;
        }
        else {
            ++idx50;
            p50 = st[idx50];
        }

        double p75;
        size_t idx75 = count * 0.75;
        if (idx75 + 1 >= count) {
            p75 = st[idx75];
        }
        else if (count % 4 == 0) {
            p75 = (st[idx75] + st[idx75 + 1]) / 2;
        }
        else {
            ++idx75;
            p75 = st[idx75];
        }

        if (csv) {
            cout << test_it->first.AsString(csv)
                << ','
                << fixed << setprecision(3)
                << st[0] << ','
                << st[count - 1] << ','
                << avg << ','
                << p50 << ','
                << p75 << ','
                << count
                << endl;
        }
        else {
            cout << left << setw(name_len + 2) << test_it->first.AsString(csv)
                << right << fixed << setprecision(3)
                << setw(8) << st[0] << "  "
                << setw(8) << st[count - 1] << "  "
                << setw(8) << avg << "  "
                << setw(8) << p50 << "  "
                << setw(8) << p75 << "  "
                << setw(8) << count
                << endl;
        }
    }
}


void CPerfTestApp::TestIds()
{
    if (m_AllIds) {
        ITERATE(TIds, it, m_Ids) {
            TestId(*it);
        }
    }
    else {
        while (true) {
            CSeq_id_Handle idh = x_NextId();
            if (!idh) break;
            TestId(idh);
        }
    }
}


void CPerfTestApp::TestId(CSeq_id_Handle idh)
{
    _ASSERT(idh);
    CAtomicOut atomic_out;
    if (m_PrintInfo || m_PrintData) {
        atomic_out << "ID: " << idh.AsString() << endl;
    }
    try {
        if (m_GetInfo) {
            CScope::TIds ids = m_Scope->GetIds(idh);
            if (ids.empty()) {
                atomic_out << "Failed to get synonyms for " << idh.AsString() << endl;
                return;
            }
            CSeq_inst::TMol moltype = m_Scope->GetSequenceType(idh);
            TSeqPos len = m_Scope->GetSequenceLength(idh);
            int taxid = m_Scope->GetTaxId(idh);
            int hash = m_Scope->GetSequenceHash(idh);
            vector<string> synonyms;
            ITERATE(CScope::TIds, it, ids) {
                synonyms.push_back(it->AsString());
            }
            sort(synonyms.begin(), synonyms.end());
            if (m_PrintInfo) {
                atomic_out << "  Synonyms:";
                ITERATE(vector<string>, it, synonyms) {
                    atomic_out << " " << *it;
                }
                atomic_out << endl;
                atomic_out << "  mol-type=" << moltype << "; len=" << len << "; taxid=" << taxid << "; hash=" << hash << endl;
            }
        }
        if (m_GetData) {
            CBioseq_Handle bh = m_Scope->GetBioseqHandle(idh);
            if (!bh) {
                atomic_out << "Failed to load bioseq " << idh.AsString() << endl;
                return;
            }
            if (m_PrintBlobId) {
                atomic_out << "  Blob-id: " << bh.GetTSE_Handle().GetBlobId() << endl;
            }
            CConstRef<CSeq_entry> entry = bh.GetTopLevelEntry().GetCompleteSeq_entry();
            if (m_PrintData) {
                atomic_out << MSerial_AsnText << *entry;
            }
        }
    }
    catch (CException& ex) {
        ERR_POST("Error testing seq-id " << idh.AsString() << " : " << ex);
    }
    if (m_PrintInfo || m_PrintData) {
        atomic_out << endl;
    }
    return;
}


END_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
//  MAIN


USING_NCBI_SCOPE;

int main(int argc, const char* argv[])
{
    return CPerfTestApp().AppMain(argc, argv);
}
