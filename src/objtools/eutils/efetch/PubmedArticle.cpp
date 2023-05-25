/* $Id$
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
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'efetch.dtd'.
 */

 // standard includes
#include <ncbi_pch.hpp>
#include <codecvt>
#include <numeric>
#include <regex>
#include <unordered_map>
#include <objects/pubmed/Pubmed_entry.hpp>
#include <objects/medline/Medline_entry.hpp>
#include <objects/general/Date.hpp>
#include <objects/general/Date_std.hpp>
#include <objects/biblio/biblio__.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/Person_id.hpp>
#include <objects/medline/medline__.hpp>
#include <objtools/eutils/efetch/efetch__.hpp>
#include <util/unicode.hpp>


// generated includes
#include <objtools/eutils/efetch/PubmedArticle.hpp>

// generated classes

BEGIN_eutils_SCOPE // namespace eutils::

NCBI_USING_NAMESPACE_STD;
USING_NCBI_SCOPE;
USING_SCOPE(objects);

// destructor
CPubmedArticle::~CPubmedArticle(void)
{
}


BEGIN_LOCAL_NAMESPACE;

template <typename StringT>
void s_ToLower(StringT& str) {
    static const auto& s_Facet = get_ctype_facet<typename StringT::value_type>(locale());
    for (auto& cch : str) {
        cch = s_Facet.tolower(cch);
    }
}


template <typename StringT>
void s_ToUpper(StringT& str) {
    static const auto& s_Facet = get_ctype_facet<typename StringT::value_type>(locale());
    for (auto& cch : str) {
        cch = s_Facet.toupper(cch);
    }
}


static bool s_StringToInt(const string& str, int& i)
{
    static const auto& s_Facet = get_ctype_facet<char>();
    i = 0;
    if (!all_of(str.begin(), str.end(), [](char cch) -> bool { return s_Facet.is(ctype_base::digit, cch); }))
        return false;
    size_t count;
    i = stoi(str, &count, 10);
    if (count != str.size())
        return false;
    return true;
}


static int s_TranslateMonth(const string& month_str)
{
    typedef initializer_list<pair<int, const char*>> int_string_map;
    static const int_string_map s_MonthTable = {
        { 1, "Jan" }, { 1, "January" },
        { 2, "Feb" }, { 2, "February" },
        { 3, "Mar" }, { 3, "March" },
        { 4, "Apr" }, { 4, "April" },
        { 5, "May" }, { 5, "May" },
        { 6, "Jun" }, { 6, "June" },
        { 7, "Jul" }, { 7, "July" },
        { 8, "Aug" }, { 8, "August" },
        { 9, "Sep" }, { 9, "September" },
        { 10, "Oct" }, { 10, "October" },
        { 11, "Nov" }, { 11, "November" },
        { 12, "Dec" }, { 12, "December" }
    };

    if (month_str.empty()) return 0;
    int ret = 0;
    if (s_StringToInt(month_str, ret)) return ret;
    auto p = find_if(s_MonthTable.begin(), s_MonthTable.end(),
        [&month_str](const pair<int, const char*>& x) -> bool { return NStr::EqualNocase(month_str, x.second); });
    return p == s_MonthTable.end() ? 0 : p->first;
}


template <class container_type, class check_type, class proc_type>
void s_ForeachToken(container_type& container, check_type f, proc_type proc)
{
    auto first = begin(container);
    while (first != end(container))
    {
        if (f(*first))
        {
            auto p = find_if_not(first + 1, end(container), f);
            first = proc(first, p);
        }
        else
            first = find_if(first + 1, end(container), f);
    }
}


static string utf8_to_string(const string& str)
{
    string utf8;
    try {
        utf8 = CUtf8::AsUTF8(str, CUtf8::GuessEncoding(str));
    }
    catch (CStringException) {
        utf8 = str;
    }
    return utf8::UTF8ToAsciiString(utf8.c_str(), nullptr, nullptr, nullptr);
}


template<class TE>
string s_Utf8TextListToString(const list<CRef<TE>>& text_list)
{
    return utf8_to_string(s_TextListToString(text_list));
}


static string s_GetInitialsFromForeName(string fore_name)
{
    s_ToUpper(fore_name);
    vector<string> tokens;
    s_ForeachToken(fore_name, [](char x)->bool { return !isspace(x); },
        [&tokens](string::iterator p, string::iterator q) -> string::iterator {
        string s(p, q);
        if (!s.empty() && s != "D'" && s != "DE" && s != "DER" && s != "LA" && s != "IBN")
            tokens.push_back(s);
        return q;
    });
    if (tokens.size() >= 2) {
        // PM-4068
        // return tokens[0].substr(0,1) + tokens[1].substr(0,1);
        return accumulate(tokens.begin(), tokens.end(), string(),
            [](const string& x, const string& y) { return x + y.substr(0, 1); });
    }
    else if (tokens.size() == 1) {
        auto p = find(tokens.front().begin(), tokens.front().end(), '-');
        if (p != tokens.front().end() && p + 1 != tokens.front().end())
            return tokens.front().substr(0, 1) + string(1, p[1]);
        else
            return tokens.front().substr(0, 1);
    }
    return "";
}


struct pubmed_date_t {
    int _year = 0;
    int _month = 0;
    int _day = 0;
};


static pubmed_date_t s_GetPubmedDate(const CPubDate& pub_date)
{
    pubmed_date_t pubmed_date;
    static const auto& s_Facet = get_ctype_facet<char>();
    try {
        if (pub_date.IsYM()) {
            auto& ym = pub_date.GetYM();
            pubmed_date._year = NStr::StringToNumeric<int>(ym.GetYear().Get());
            if (ym.IsSetMS()) {
                if (ym.GetMS().IsMD()) {
                    auto& md = ym.GetMS().GetMD();
                    pubmed_date._month = s_TranslateMonth(md.GetMonth());
                    if (pubmed_date._month >= 1 && pubmed_date._month <= 12 && md.IsSetDay()) {
                        pubmed_date._day = NStr::StringToNumeric<int>(md.GetDay().Get());
                    }
                }
                else if (ym.GetMS().IsSeason()) {
                    auto& season = ym.GetMS().GetSeason().Get();
                    auto pos = season.find('-');
                    if (pos != NPOS) {
                        pubmed_date._month = s_TranslateMonth(season.substr(0, pos));
                    }
                    else {
                        if (NStr::EqualNocase(season, "Winter")) {
                            pubmed_date._month = 12; // Dec
                        }
                        else if (NStr::EqualNocase(season, "Spring")) {
                            pubmed_date._month = 3; // Mar
                        }
                        else if (NStr::EqualNocase(season, "Summer")) {
                            pubmed_date._month = 6; // Jun
                        }
                        else if (NStr::EqualNocase(season, "Autumn")) {
                            pubmed_date._month = 9; // Jul
                        }
                    }
                }
            }
        }
        else if (pub_date.IsMedlineDate()) {
            string medline_date = pub_date.GetMedlineDate();
            s_ForeachToken(medline_date, [](char cch) -> bool { return s_Facet.is(ctype_base::alnum, cch); },
                [&pubmed_date](string::iterator p, string::iterator q) -> string::iterator {
                string str(p, q);
                int num = 0;
                if (s_StringToInt(str, num)) {
                    if (pubmed_date._year == 0 && num >= 1800 && num <= 2100)
                        pubmed_date._year = num;
                    else if (pubmed_date._month == 0 && num >= 1 && num <= 12)
                        pubmed_date._month = num;
                    else if (pubmed_date._month != 0 && pubmed_date._day == 0 && num >= 1 && num <= 31)
                        pubmed_date._day = num;
                }
                else if (pubmed_date._month == 0) {
                    pubmed_date._month = s_TranslateMonth(str);
                }
                return q;
            });
        }
    }
    catch (...) {
    }
    return pubmed_date;
}


static CRef<CDate> s_GetDateFromArticleDate(const CArticleDate& adate)
{
    CRef<CDate> date(new CDate());
    try {
        // Try integer values
        CDate_std& std_date = date->SetStd();
        std_date.SetYear(NStr::StringToNumeric<CDate_std::TYear>(adate.GetYear().Get()));
        std_date.SetMonth(NStr::StringToNumeric<CDate_std::TMonth>(adate.GetMonth().Get()));
        std_date.SetDay(NStr::StringToNumeric<CDate_std::TDay>(adate.GetDay().Get()));
    }
    catch (...) {
        // Use string values
        string str_date = adate.GetYear();
        if (!adate.GetMonth().Get().empty()) str_date += " " + adate.GetMonth();
        if (!adate.GetDay().Get().empty()) str_date += " " + adate.GetDay();
        date->SetStr(str_date);
    }
    return date;
}


static CRef<CTitle> s_GetJournalTitle(const CPubmedArticle& pubmed_article)
{
    const CMedlineCitation& medlineCitation = pubmed_article.GetMedlineCitation();
    const CJournal& journal = medlineCitation.GetArticle().GetJournal();
    CRef<CTitle> title(new CTitle());

    if (journal.IsSetISOAbbreviation()) {
        CRef<CTitle::C_E> iso(new CTitle::C_E());
        iso->SetIso_jta(journal.GetISOAbbreviation());
        title->Set().push_back(iso);
    }

    CRef<CTitle::C_E> mljta(new CTitle::C_E());
    mljta->SetMl_jta(medlineCitation.GetMedlineJournalInfo().GetMedlineTA());
    title->Set().push_back(mljta);

    if (journal.IsSetISSN()) {
        CRef<CTitle::C_E> issn(new CTitle::C_E());
        issn->SetIssn(journal.GetISSN());
        title->Set().push_back(issn);
    }

    if (journal.IsSetTitle()) {
        CRef<CTitle::C_E> name(new CTitle::C_E());
        name->SetName(journal.GetTitle());
        title->Set().push_back(name);
    }

    return title;
}


static bool s_SetCommentCorrection(
    CRef<CImprint>& imprint,
    const CCommentsCorrectionsList& comments_corrections,
    CCitRetract::EType type,
    const string& ref_type)
{
    for (auto ref : comments_corrections.GetCommentsCorrections()) {
        string attr_ref_type = CCommentsCorrections::C_Attlist::GetTypeInfo_enum_EAttlist_RefType()->
            FindName(ref->GetAttlist().GetRefType(), false);
        if (attr_ref_type == ref_type) {
            CRef<CCitRetract> citretract(new CCitRetract);
            string temp = utf8_to_string(ref->GetRefSource());
            if (ref->IsSetNote()) temp += ". " + utf8_to_string(ref->GetNote());
            if (ref->IsSetPMID()) temp += ". PMID: " + ref->GetPMID();
            citretract->SetType(type);
            citretract->SetExp(temp);
            imprint->SetRetract(*citretract);
            return true;
        }
    }
    return false;
}


static int s_GetPublicationStatusId(const string& publication_status)
{
    static const unordered_map<string, int> s_PubStatusId {
        { "received", ePubStatus_received },
        { "accepted", ePubStatus_accepted },
        { "epublish", ePubStatus_epublish },
        { "ppublish", ePubStatus_ppublish },
        { "revised", ePubStatus_revised },
        { "pmc", ePubStatus_pmc },
        { "pmcr", ePubStatus_pmcr },
        { "pubmed", ePubStatus_pubmed },
        { "pubmedr", ePubStatus_pubmedr },
        { "aheadofprint", ePubStatus_aheadofprint },
        { "premedline", ePubStatus_premedline },
        { "medline", ePubStatus_medline },
    };
    auto p = s_PubStatusId.find(publication_status);
    return p == s_PubStatusId.end() ? ePubStatus_other : p->second;
}


static CRef<CImprint> s_GetImprint(const CPubmedArticle& pubmed_article)
{
    CRef<CImprint> imprint(new CImprint());

    const CMedlineCitation& medline_citation = pubmed_article.GetMedlineCitation();
    const CPubmedData& pubmed_data = pubmed_article.GetPubmedData();
    const CArticle& article = medline_citation.GetArticle();
    const CJournal& journal = article.GetJournal();
    const CJournalIssue& jissue = journal.GetJournalIssue();

    // Pub date
    string pub_model = CArticle::C_Attlist::GetTypeInfo_enum_EAttlist_PubModel()->
        FindName(article.GetAttlist().GetPubModel(), false);
    if (pub_model == "Electronic-Print" || pub_model == "Electronic-eCollection") {
        for (auto d : article.GetArticleDate()) {
            if (d->GetAttlist().IsSetDateType() && d->GetAttlist().GetDateType() == CArticleDate::C_Attlist::eAttlist_DateType_Electronic) {
                imprint->SetDate(*s_GetDateFromArticleDate(*d));
                break;
            }
        }
    }
    else {
        imprint->SetDate(*s_GetDateFromPubDate(jissue.GetPubDate()));
    }

    string volume = jissue.IsSetVolume() ? jissue.GetVolume().Get() : "";
    if (!volume.empty()) imprint->SetVolume(volume);

    string issue = jissue.IsSetIssue() ? jissue.GetIssue().Get() : "";
    if (!issue.empty()) imprint->SetIssue(issue);

    string pages = article.GetPE_2().IsPE() ? s_GetPagination(article.GetPE_2().GetPE().GetPagination()) : "";
    if (!pages.empty()) imprint->SetPages(pages);

    auto list_language = medline_citation.GetArticle().GetLanguage();
    string languages;
    if (!list_language.empty())
        languages = accumulate(next(list_language.begin()), list_language.end(), list_language.front()->Get(),
            [](const auto& s, const auto& e) -> string { return s + "," + e->Get(); });
    imprint->SetLanguage(languages);

    if (medline_citation.IsSetCommentsCorrectionsList()) {
        s_SetCommentCorrection(imprint, medline_citation.GetCommentsCorrectionsList(), CCitRetract::eType_retracted, "RetractionIn")
            || s_SetCommentCorrection(imprint, medline_citation.GetCommentsCorrectionsList(), CCitRetract::eType_in_error, "ErratumIn")
            || s_SetCommentCorrection(imprint, medline_citation.GetCommentsCorrectionsList(), CCitRetract::eType_erratum, "ErratumFor")
            || s_SetCommentCorrection(imprint, medline_citation.GetCommentsCorrectionsList(), CCitRetract::eType_notice, "RetractionOf");
    }

    // PubStatus
    imprint->SetPubstatus(s_GetPublicationStatusId(pubmed_data.GetPublicationStatus()));

    // History
    if (pubmed_data.IsSetHistory())
    {
        CRef<CPubStatusDateSet> date_set(new CPubStatusDateSet());
        for (auto pub_date : pubmed_data.GetHistory().GetPubMedPubDate()) {
            CRef<CPubStatusDate> pub_stat_date(new CPubStatusDate());
            string pub_status = CPubMedPubDate::C_Attlist::GetTypeInfo_enum_EAttlist_PubStatus()->
                FindName(pub_date->GetAttlist().GetPubStatus(), false);
            pub_stat_date->SetPubstatus(s_GetPublicationStatusId(pub_status));
            pub_stat_date->SetDate(*s_GetDateFromPubMedPubDate(*pub_date));
            date_set->Set().push_back(pub_stat_date);
        }
        imprint->SetHistory(*date_set);
    }

    return imprint;
}


static CRef<CCit_jour> s_GetJournalCitation(const CPubmedArticle& pubmed_article)
{
    CRef<CCit_jour> cit_journal(new CCit_jour());
    cit_journal->SetTitle(*s_GetJournalTitle(pubmed_article));
    cit_journal->SetImp(*s_GetImprint(pubmed_article));
    return cit_journal;
}


inline bool s_IsValidYN(const CAuthor& auth)
{
    return auth.GetAttlist().IsSetValidYN() &&
        auth.GetAttlist().GetValidYN() == CAuthor::C_Attlist::eAttlist_ValidYN_Y;
}


static CRef<CAuth_list> s_GetAuthorList(const CArticle& article)
{
    CRef<CAuth_list> author_list;
    if (!article.IsSetAuthorList()) return author_list;
    auto& list_author = article.GetAuthorList().GetAuthor();
    bool has_authors = any_of(list_author.begin(), list_author.end(),
        [](const CRef<CAuthor>& auth)->bool { return s_IsValidYN(*auth); });
    if (has_authors) {
        bool std_format = any_of(list_author.begin(), list_author.end(), [](const CRef<CAuthor>& auth)->bool {
            return s_IsValidYN(*auth) && (auth->GetLC().IsCollectiveName() ||
                    (auth->IsSetAffiliationInfo() && !auth->GetAffiliationInfo().empty())); });
        author_list.Reset(new CAuth_list());
        CRef<CAuth_list::C_Names> auth_names(new CAuth_list::C_Names());
        for (auto author : list_author) {
            if (s_IsValidYN(*author)) {
                if (std_format) {
                    CRef<CPerson_id> person(new CPerson_id());
                    if (author->GetLC().IsCollectiveName()) {
                        person->SetConsortium(s_Utf8TextListToString(author->GetLC().GetCollectiveName().Get()));
                    }
                    else {
                        person->SetMl(utf8_to_string(s_GetAuthorMedlineName(*author)));
                    }
                    CRef<objects::CAuthor> auth(new objects::CAuthor());
                    auth->SetName(*person);
                    auto& list_affiliation_info = author->GetAffiliationInfo();
                    if (!list_affiliation_info.empty()) {
                        list<string> affiliations;
                        for (auto affiliation_info : list_affiliation_info) {
                            string affiliation = s_Utf8TextListToString(affiliation_info->GetAffiliation().Get());
                            if (!affiliation.empty()) affiliations.emplace_back(move(affiliation));
                        }
                        if (!affiliations.empty()) {
                            CRef<CAffil> affil(new CAffil());
                            affil->SetStr(accumulate(next(affiliations.begin()), affiliations.end(), affiliations.front(),
                                [](const string& s1, const string& s2)->string { return s1 + "; " + s2; }));
                            auth->SetAffil(*affil);
                        }
                    }
                    auth_names->SetStd().push_back(auth);
                }
                else {
                    auth_names->SetMl().push_back(utf8_to_string(s_GetAuthorMedlineName(*author)));
                }
            }
        }
        if (!article.GetAuthorList().GetAttlist().IsSetCompleteYN() ||
            article.GetAuthorList().GetAttlist().GetCompleteYN() != CAuthorList::C_Attlist::eAttlist_CompleteYN_Y) {
            if (std_format) {
                CRef<CPerson_id> person(new CPerson_id());
                person->SetMl("et al");
                CRef<objects::CAuthor> auth(new objects::CAuthor());
                auth->SetName(*person);
                auth_names->SetStd().push_back(auth);
            }
            else {
                auth_names->SetMl().push_back("et al");
            }
        }
        author_list->SetNames(*auth_names);
    }
    return author_list;
}


static objects::CArticleId::E_Choice s_GetArticleIdTypeId(const CArticleId& article_id)
{
    static const unordered_map<CArticleId::C_Attlist::EAttlist_IdType, objects::CArticleId::E_Choice> s_ArticleIdTypeId = {
        { CArticleId::C_Attlist::eAttlist_IdType_pubmed, objects::CArticleId::e_Pubmed },
        { CArticleId::C_Attlist::eAttlist_IdType_medline, objects::CArticleId::e_Medline },
        { CArticleId::C_Attlist::eAttlist_IdType_doi, objects::CArticleId::e_Doi },
        { CArticleId::C_Attlist::eAttlist_IdType_pii, objects::CArticleId::e_Pii },
        { CArticleId::C_Attlist::eAttlist_IdType_pmcid, objects::CArticleId::e_Pmcid },
        { CArticleId::C_Attlist::eAttlist_IdType_pmcpid, objects::CArticleId::e_Pmcpid },
        { CArticleId::C_Attlist::eAttlist_IdType_pmpid, objects::CArticleId::e_Pmpid },
    };
    if (!article_id.GetAttlist().IsSetIdType()) return objects::CArticleId::e_not_set;
    auto id_type = article_id.GetAttlist().GetIdType();
    auto p = s_ArticleIdTypeId.find(id_type);
    return p != s_ArticleIdTypeId.end() ? p->second : objects::CArticleId::e_Other;
}


static CRef<CCit_art> s_GetCitation(const CPubmedArticle& pubmed_article)
{
    auto& medline_citation = pubmed_article.GetMedlineCitation();
    CRef<CCit_art> cit_article(new CCit_art());
    CRef<CTitle> title = s_GetTitle(medline_citation.GetArticle());
    if (title) cit_article->SetTitle(*title);
    auto author_list = s_GetAuthorList(medline_citation.GetArticle());
    if (author_list) cit_article->SetAuthors(*author_list);
    CRef<CCit_art::C_From> from(new CCit_art::C_From());
    from->SetJournal(*s_GetJournalCitation(pubmed_article));
    cit_article->SetFrom(*from);
    cit_article->SetIds(*s_GetArticleIdSet(
        pubmed_article.GetPubmedData().GetArticleIdList(),
        &pubmed_article.GetMedlineCitation().GetArticle()));
    return cit_article;
}


static void s_FillMesh(CMedline_entry::TMesh& mesh, const CMeshHeadingList& mesh_heading_list)
{
    for (auto mesh_heading_it : mesh_heading_list.GetMeshHeading()) {
        CRef<CMedline_mesh> medline_mesh(new CMedline_mesh());
        auto& desc_name = mesh_heading_it->GetDescriptorName();
        if (desc_name.GetAttlist().IsSetMajorTopicYN() &&
            desc_name.GetAttlist().GetMajorTopicYN() == CDescriptorName::C_Attlist::eAttlist_MajorTopicYN_Y)
            medline_mesh->SetMp(true);
        medline_mesh->SetTerm(desc_name.GetDescriptorName());
        for (auto qualifier_name_it : mesh_heading_it->GetQualifierName()) {
            CRef<CMedline_qual> qual(new CMedline_qual());
            if (qualifier_name_it->GetAttlist().IsSetMajorTopicYN() &&
                qualifier_name_it->GetAttlist().GetMajorTopicYN() == CQualifierName::C_Attlist::eAttlist_MajorTopicYN_Y)
                qual->SetMp(true);
            qual->SetSubh(qualifier_name_it->GetQualifierName());
            medline_mesh->SetQual().push_back(qual);
        }
        mesh.push_back(medline_mesh);
    }
}


static void s_FillChem(CMedline_entry::TSubstance& chems, const CChemicalList& chemical_list)
{
    for (auto chemical_it : chemical_list.GetChemical()) {
        CRef<CMedline_rn> chem(new CMedline_rn());
        chem->SetName(chemical_it->GetNameOfSubstance());
        // Registry number and type
        string registry_number = chemical_it->GetRegistryNumber();
        if (registry_number.empty() || registry_number == "0")
            chem->SetType(chem->eType_nameonly);
        else if (registry_number.size() > 2 && toupper(registry_number[0]) == 'E' && toupper(registry_number[1]) == 'C') {
            chem->SetCit(registry_number.c_str() + 2 + strspn(registry_number.c_str() + 2, " "));
            chem->SetType(chem->eType_ec);
        }
        else {
            chem->SetCit(registry_number);
            chem->SetType(chem->eType_cas);
        }
        chems.push_back(chem);
    }
}


static int s_GetDatabankTypeId(const string& databank_name)
{
    static const unordered_map<string, int> s_DatabankTypeId {
        { "ddbj", CMedline_si::eType_ddbj },
        { "carbbank", CMedline_si::eType_carbbank },
        { "embl", CMedline_si::eType_embl },
        { "hdb", CMedline_si::eType_hdb },
        { "genbank", CMedline_si::eType_genbank },
        { "hgml", CMedline_si::eType_hgml },
        { "mim", CMedline_si::eType_mim },
        { "msd", CMedline_si::eType_msd },
        { "pdb", CMedline_si::eType_pdb },
        { "pir", CMedline_si::eType_pir },
        { "prfseqdb", CMedline_si::eType_prfseqdb },
        { "psd", CMedline_si::eType_psd },
        { "swissprot", CMedline_si::eType_swissprot },
        { "gdb", CMedline_si::eType_gdb },
    };
    auto p = s_DatabankTypeId.find(databank_name);
    return p == s_DatabankTypeId.end() ? -1 : p->second;
}


static void s_FillXref(CMedline_entry::TXref& refs, const CDataBankList& databank_list)
{
    for (auto databank_it : databank_list.GetDataBank()) {
        string databank_name = databank_it->GetDataBankName();
        s_ToLower(databank_name);
        int type_id = s_GetDatabankTypeId(databank_name);
        if (type_id >= 0) {
            if (databank_it->IsSetAccessionNumberList()) {
                for (auto& accession_number_it : databank_it->GetAccessionNumberList().GetAccessionNumber()) {
                    CRef<CMedline_si> si(new CMedline_si());
                    si->SetType((CMedline_si::EType)type_id);
                    si->SetCit(accession_number_it->Get());
                    refs.push_back(si);
                }
            }
            else {
                CRef<CMedline_si> si(new CMedline_si());
                si->SetType((CMedline_si::EType)type_id);
                refs.push_back(si);
            }
        }
    }
}


static void s_FillGenes(CMedline_entry::TGene& mgenes, const CGeneSymbolList& gene_symbol_list)
{
    for (auto& gene_symbol_it : gene_symbol_list.GetGeneSymbol()) {
        mgenes.push_back(gene_symbol_it->Get());
    }
}


static void s_FillPubtypeList(
    CMedline_entry::TPub_type& pub_types,
    const CPublicationTypeList& publication_type_list)
{
    for (auto publication_type_it : publication_type_list.GetPublicationType()) {
        pub_types.push_back(publication_type_it->GetPublicationType());
    }
}


static CRef<CPubMedPubDate> s_FindPubDateStatus(
    const list<CRef<CPubMedPubDate>>& pubmed_pubdates,
    CPubMedPubDate::C_Attlist::EAttlist_PubStatus status)
{
    for (auto pub_date : pubmed_pubdates) {
        if (pub_date->GetAttlist().GetPubStatus() == status) {
            return pub_date;
        }
    }
    return CRef<CPubMedPubDate>();
}


static string s_GetAbstractText(const CAbstract& abstr)
{
    string abstract_text;
    for (auto abstract_text_it : abstr.GetAbstractText())
    {
        string label = abstract_text_it->GetAttlist().IsSetLabel() ?
            abstract_text_it->GetAttlist().GetLabel() + ": " : "";
        if (!abstract_text.empty()) abstract_text.append(" ");
        abstract_text.append(label + s_TextListToString(abstract_text_it->GetAbstractText()));
    }
    return abstract_text;
}


static CRef<CMedline_entry> s_GetMedlineEntry(const CPubmedArticle& pubmed_article)
{
    CRef<CMedline_entry> medline_entry(new CMedline_entry());
    CRef<CPubMedPubDate> entrez_date = s_FindPubDateStatus(
        pubmed_article.GetPubmedData().GetHistory().GetPubMedPubDate(),
        CPubMedPubDate::C_Attlist::eAttlist_PubStatus_entrez);
    if (!entrez_date) {
        entrez_date = s_FindPubDateStatus(
            pubmed_article.GetPubmedData().GetHistory().GetPubMedPubDate(),
            CPubMedPubDate::C_Attlist::eAttlist_PubStatus_pubmed);
    }
    if (entrez_date)
        medline_entry->SetEm(*s_GetDateFromPubMedPubDate(*entrez_date));
    medline_entry->SetCit(*s_GetCitation(pubmed_article));

    auto& cit = pubmed_article.GetMedlineCitation();
    if (cit.GetArticle().IsSetAbstract())
        medline_entry->SetAbstract(s_CleanupText(s_GetAbstractText(cit.GetArticle().GetAbstract())));
    if (cit.IsSetMeshHeadingList())
        s_FillMesh(medline_entry->SetMesh(), cit.GetMeshHeadingList());
    if (cit.IsSetChemicalList())
        s_FillChem(medline_entry->SetSubstance(), cit.GetChemicalList());
    if (cit.GetArticle().IsSetDataBankList())
        s_FillXref(medline_entry->SetXref(), cit.GetArticle().GetDataBankList());
    if (cit.GetArticle().IsSetGrantList())
        s_FillGrants(medline_entry->SetIdnum(), cit.GetArticle().GetGrantList());
    if (cit.IsSetGeneSymbolList())
        s_FillGenes(medline_entry->SetGene(), cit.GetGeneSymbolList());
    medline_entry->SetPmid(CPubMedId(NStr::StringToNumeric<TEntrezId>(cit.GetPMID().GetPMID(), NStr::fConvErr_NoThrow)));
    s_FillPubtypeList(medline_entry->SetPub_type(), cit.GetArticle().GetPublicationTypeList());

    CMedlineCitation::C_Attlist::TStatus status = cit.GetAttlist().GetStatus();
    if (status == CMedlineCitation::C_Attlist::eAttlist_Status_Publisher)
        medline_entry->SetStatus(medline_entry->eStatus_publisher);
    else if (status == CMedlineCitation::C_Attlist::eAttlist_Status_Completed ||
        status == CMedlineCitation::C_Attlist::eAttlist_Status_MEDLINE ||
        status == CMedlineCitation::C_Attlist::eAttlist_Status_OLDMEDLINE)
        medline_entry->SetStatus(medline_entry->eStatus_medline);
    else
        medline_entry->SetStatus(medline_entry->eStatus_premedline);
    return medline_entry;
};

END_LOCAL_NAMESPACE;


string s_CleanupText(string str)
{
    str = utf8_to_string(str);
    for (char& cch : str) {
        if (cch == '\r' || cch == '\n' || cch == '\t')
            cch = ' ';
    }
    return str;
}


CRef<CDate> s_GetDateFromPubDate(const CPubDate& pub_date)
{
    static const char* s_SeasonTab[] = { "Winter", "Spring", "Summer", "Autumn" };
    pubmed_date_t pm_pub_date = s_GetPubmedDate(pub_date);
    CRef<CDate> date(new CDate());
    CDate_std& date_std = date->SetStd();
    date_std.SetYear(pm_pub_date._year);
    if (pm_pub_date._month != 0) {
        if (pm_pub_date._month < 13) {
            date_std.SetMonth(pm_pub_date._month);
            if (pm_pub_date._day != 0)
                date_std.SetDay(pm_pub_date._day);
        }
        else {
            date_std.SetSeason(s_SeasonTab[pm_pub_date._month - 13]);
        }
    }
    return date;
}


CRef<CDate> s_GetDateFromPubMedPubDate(const CPubMedPubDate& pdate)
{
    CRef<CDate> date(new CDate());
    try {
        // Try integer values
        CDate_std& std_date = date->SetStd();
        std_date.SetYear(NStr::StringToNumeric<CDate_std::TYear>(pdate.GetYear().Get()));
        std_date.SetMonth(NStr::StringToNumeric<CDate_std::TMonth>(pdate.GetMonth().Get()));
        std_date.SetDay(NStr::StringToNumeric<CDate_std::TDay>(pdate.GetDay().Get()));
        if (pdate.IsSetHM()) {
            try {
                // Try integer time, ignore on error
                auto& hm = pdate.GetHM();
                std_date.SetHour(NStr::StringToNumeric<CDate_std::THour>(hm.GetHour().Get()));
                if (hm.IsSetMS()) {
                    auto& ms = hm.GetMS();
                    std_date.SetMinute(NStr::StringToNumeric<CDate_std::TMinute>(ms.GetMinute().Get()));
                    if (ms.IsSetSecond()) {
                        std_date.SetSecond(NStr::StringToNumeric<CDate_std::TSecond>(ms.GetSecond().Get()));
                    }
                }
            }
            catch (...) {}
        }
    }
    catch (...) {
        // Use string values
        string str_date = pdate.GetYear();
        if (!pdate.GetMonth().Get().empty()) str_date += " " + pdate.GetMonth();
        if (!pdate.GetDay().Get().empty()) str_date += " " + pdate.GetDay();
        date->SetStr(str_date);
    }
    return date;
}


string s_GetArticleTitleStr(const CArticleTitle& article_title)
{
    return article_title.IsSetArticleTitle() ?
        s_Utf8TextListToString(article_title.GetArticleTitle()) : "";
}


string s_GetVernacularTitleStr(const CVernacularTitle& vernacular_title)
{
    return s_Utf8TextListToString(vernacular_title.Get());
}


CRef<CTitle> s_MakeTitle(const string& title_str, const string& vernacular_title_str)
{
    CRef<CTitle> title;
    if (!title_str.empty() || !vernacular_title_str.empty()) {
        title.Reset(new CTitle());
        if (!title_str.empty()) {
            CRef<CTitle::C_E> name(new CTitle::C_E());
            name->SetName(title_str);
            title->Set().push_back(name);
        }
        if (!vernacular_title_str.empty()) {
            CRef<CTitle::C_E> name(new CTitle::C_E());
            name->SetTrans(vernacular_title_str);
            title->Set().push_back(name);
        }
    }
    return title;
}


string s_GetAuthorMedlineName(const CAuthor& author)
{
    string author_medline_name;
    if (author.GetLC().IsCollectiveName()) {
        author_medline_name = s_Utf8TextListToString(author.GetLC().GetCollectiveName().Get());
        if (!author_medline_name.empty() && author_medline_name.back() != '.')
            author_medline_name.append(".");
    }
    else if (author.GetLC().IsLFIS()) {
        // Personal name;
        auto& lfis = author.GetLC().GetLFIS();
        author_medline_name = utf8_to_string(lfis.GetLastName());
        // Initials
        string initials;
        if (lfis.IsSetInitials())
            initials = utf8_to_string(lfis.GetInitials());
        else if (lfis.IsSetForeName())
            initials = s_GetInitialsFromForeName(utf8_to_string(lfis.GetForeName()));
        if (!initials.empty())
            author_medline_name.append(" " + initials);
        if (lfis.IsSetSuffix())
            author_medline_name.append(" " + s_Utf8TextListToString(lfis.GetSuffix().Get()));
    }
    return author_medline_name;
}


string s_GetPagination(const CPagination& pagination)
{
    string pages;
    if (pagination.IsSEM()) {
        auto& sem = pagination.GetSEM();
        if (sem.IsSetMedlinePgn()) {
            pages = sem.GetMedlinePgn();
        }
        else {
            if (sem.IsSetStartPage()) {
                if (sem.IsSetEndPage()) return sem.GetStartPage() + "-" + sem.GetEndPage();
                else return sem.GetStartPage();
            }
            if (sem.IsSetEndPage()) return "-" + sem.GetEndPage();
            return "";
        }
    }

    if (pages.empty() && pagination.IsMedlinePgn()) pages = pagination.GetMedlinePgn();
    if (pages.empty()) return "";
    list<string> parts;
    s_ForeachToken(pages, [](char cch) -> bool { return cch != ','; },
        [&parts](string::iterator p, string::iterator q)->string::iterator {
        string x(p, q);
        x = regex_replace(x, regex("\\s+"), "");
        if (!x.empty()) {
            regex r("^(.*?)-(.*)$");
            smatch ma;
            regex_search(x, ma, r);
            if (ma.empty() || ma.str(1).length() <= ma.str(2).length())
                parts.push_back(x);
            else
                parts.push_back(ma.str(1) + "-" + ma.str(1).substr(0, ma.str(1).length() - ma.str(2).length()) + ma.str(2));
        }
        return q;
    });
    return parts.empty() ? "" : accumulate(next(parts.begin()), parts.end(), parts.front(),
            [](const string& s1, const string& s2)->string { return s1 + ',' + s2; });
}


CRef<CArticleIdSet> s_GetArticleIdSet(const CArticleIdList& article_id_list, const CArticle* article)
{
    CRef<CArticleIdSet> id_set(new CArticleIdSet());
    for (auto article_id_it : article_id_list.GetArticleId()) {
        CRef<objects::CArticleId> id(new objects::CArticleId());
        try {
            const string& str_id = article_id_it->GetArticleId();
            switch (s_GetArticleIdTypeId(*article_id_it)) {
            case objects::CArticleId::e_Pubmed:
                id->SetPubmed(CPubMedId(NStr::StringToNumeric<TEntrezId>(str_id)));
                break;
            case objects::CArticleId::e_Medline:
                continue;
            case objects::CArticleId::e_Doi:
                id->SetDoi(CDOI(str_id));
                break;
            case objects::CArticleId::e_Pii:
                id->SetPii(CPII(str_id));
                break;
            case objects::CArticleId::e_Pmcid:
                id->SetPmcid(CPmcID(NStr::StringToNumeric<TEntrezId>(str_id)));
                break;
            case objects::CArticleId::e_Pmcpid:
                id->SetPmcpid(CPmcPid(str_id));
                break;
            case objects::CArticleId::e_Pmpid:
                id->SetPmpid(CPmPid(str_id));
                break;
            case objects::CArticleId::e_Other: {
                string db = CArticleId::C_Attlist::GetTypeInfo_enum_EAttlist_IdType()->
                    FindName(article_id_it->GetAttlist().GetIdType(), false);
                CRef<CDbtag> db_tag(new CDbtag());
                db_tag->SetDb(db);
                CRef<CObject_id> obj_id(new CObject_id());
                obj_id->SetStr(str_id);
                db_tag->SetTag(*obj_id);
                id->SetOther(*db_tag);
                break;
            }
            default:
                continue;
            }
            id_set->Set().push_back(id);
        }
        catch (...) {}
    }
    if (article) {
        // JIRA: PM-966
        const list<CRef<CELocationID>>* eloc_ids = nullptr;
        if (article->GetPE_2().IsELocationID()) {
            eloc_ids = &article->GetPE_2().GetELocationID();
        }
        else {
            if (article->GetPE_2().GetPE().IsSetELocationID()) eloc_ids = &article->GetPE_2().GetPE().GetELocationID();
        }
        if (eloc_ids) {
            for (auto elocation_id_it : *eloc_ids) {
                string str_eid_type = CELocationID::C_Attlist::GetTypeInfo_enum_EAttlist_EIdType()->
                    FindName(elocation_id_it->GetAttlist().GetEIdType(), false);
                string type = "ELocationID " + str_eid_type;
                string value = elocation_id_it->GetELocationID();
                CRef<objects::CArticleId> id(new objects::CArticleId());
                CRef<CDbtag> db_tag(new CDbtag());
                db_tag->SetDb(type);
                CRef<CObject_id> obj_id(new CObject_id());
                obj_id->SetStr(value);
                db_tag->SetTag(*obj_id);
                id->SetOther(*db_tag);
                id_set->Set().push_back(id);
            }
        }
    }
    return id_set;
}


void s_FillGrants(list<string>& id_nums, const CGrantList& grant_list)
{
    for (auto grant_it : grant_list.GetGrant()) {
        string id;
        if (grant_it->IsSetGrantID())
            id = utf8_to_string(grant_it->GetGrantID());
        if (grant_it->IsSetAcronym())
            id += id.empty() ? utf8_to_string(grant_it->GetAcronym()) : "/" + utf8_to_string(grant_it->GetAcronym());
        if (grant_it->IsSetAgency() && !grant_it->GetAgency().Get().empty())
            id += id.empty() ? utf8_to_string(grant_it->GetAgency()) : "/" + utf8_to_string(grant_it->GetAgency());
        string id2 = utf8_to_string(id);
        if (!id2.empty())
            id_nums.push_back(id2);
    }
}


CRef<CPubmed_entry> CPubmedArticle::ToPubmed_entry(void) const
{
    CRef<CPubmed_entry> pubmed_entry(new CPubmed_entry());

    pubmed_entry->SetPmid(CPubMedId(NStr::StringToNumeric<TEntrezId>(
        GetMedlineCitation().GetPMID().GetPMID(), NStr::fConvErr_NoThrow)));
    pubmed_entry->SetMedent(*s_GetMedlineEntry(*this));
    return pubmed_entry;
}


END_eutils_SCOPE // namespace eutils::

/* Original file checksum: lines: 53, chars: 1696, CRC32: 3b4b74a1 */
