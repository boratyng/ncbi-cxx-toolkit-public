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
 *   'valid.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/valid/Comment_set.hpp>
#include <objects/valid/Comment_rule.hpp>

// for default structured comment rules file
#include "validrules.inc"
#include <util/util_misc.hpp>
#include <util/line_reader.hpp>
#include <serial/enumvalues.hpp>
#include <serial/serialimpl.hpp>


// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


static bool  s_StructuredCommentRulesInitialized = false;
DEFINE_STATIC_FAST_MUTEX(s_StructuredCommentRulesMutex);
static CRef<CComment_set> s_CommentRules;

// destructor
CComment_set::~CComment_set(void)
{
}

CConstRef<CComment_rule> CComment_set::FindCommentRuleEx (const string& prefix) const
{
    string search = prefix;
    CComment_rule::NormalizePrefix(search);
    ITERATE (CComment_set::Tdata, it, Get()) {
        const CComment_rule& rule = **it;
        string this_prefix = rule.GetPrefix();
        CComment_rule::NormalizePrefix(this_prefix);
        if (NStr::EqualNocase(this_prefix, search)) {
            return *it;
        }
    }

    // NCBI_THROW (CCoreException, eNullPtr, "FindCommentRuleEx failed");

    return CConstRef<CComment_rule>();
}


const CComment_rule& CComment_set::FindCommentRule (const string& prefix) const
{
    auto rule = FindCommentRuleEx(prefix);
    if ( rule.Empty() ) {
        NCBI_THROW (CCoreException, eNullPtr, "FindCommentRule failed");
    } else {
        return *rule;
    }
}


static bool s_FieldRuleCompare (
    const CRef<CField_rule>& p1,
    const CRef<CField_rule>& p2
)

{
    return NStr::Compare(p1->GetField_name(), p2->GetField_name()) < 0;
}


static void s_InitializeStructuredCommentRules(void)
{
    CFastMutexGuard GUARD(s_StructuredCommentRulesMutex);
    if (s_StructuredCommentRulesInitialized) {
        return;
    }
    s_CommentRules.Reset(new CComment_set());
    string file = g_FindDataFile("validrules.prt");
  
    if ( !file.empty() ) {
        auto_ptr<CObjectIStream> in;
        in.reset(CObjectIStream::Open(file, eSerial_AsnText));
        string header = in->ReadFileHeader();
        in->Read(ObjectInfo(*s_CommentRules), CObjectIStream::eNoFileHeader);    
        if (getenv("NCBI_DEBUG")) {
            LOG_POST("Reading from " + file + " for structured comment rules.");
        }
    }
    if (!s_CommentRules->IsSet()) {
        if (getenv("NCBI_DEBUG")) {
            LOG_POST("Falling back on built-in data for structured comment rules");
        }
        size_t num_lines = sizeof (s_Defaultvalidrules) / sizeof (char *);     
        string all_rules = "";
        for (size_t i = 0; i < num_lines; i++) {
            all_rules += s_Defaultvalidrules[i];
        }
        CNcbiIstrstream istr(all_rules.c_str());
        istr >> MSerial_AsnText >> *s_CommentRules;
    }
    if (s_CommentRules->IsSet()) {
        NON_CONST_ITERATE(CComment_set::Tdata, it, s_CommentRules->Set()) {
            if (!(*it)->GetRequire_order() && (*it)->IsSetFields()) {
                CField_set& fields = (*it)->SetFields();
                fields.Set().sort(s_FieldRuleCompare);
            }
        }
    }       

    s_StructuredCommentRulesInitialized = true;
}


CConstRef<CComment_set> CComment_set::GetCommentRules()
{
    s_InitializeStructuredCommentRules();
    return CConstRef<CComment_set>(s_CommentRules.GetPointer());
}


vector<string> CComment_set::GetFieldNames(const string& prefix)
{
    vector<string> options;

    string prefix_to_use = CComment_rule::MakePrefixFromRoot(prefix);

    // look up mandatory and required field names from validator rules
    CConstRef<CComment_set> rules = CComment_set::GetCommentRules();

    if (rules) {
        try {
            CConstRef<CComment_rule> ruler = rules->FindCommentRuleEx(prefix_to_use);
            const CComment_rule& rule = *ruler;
            ITERATE(CComment_rule::TFields::Tdata, it, rule.GetFields().Get()) {
                options.push_back((*it)->GetField_name());
            }
        } catch (CException ) {
            // no rule for this prefix, can't list fields
        }
    }

    return options;
}


list<string> CComment_set::GetKeywords(const CUser_object& user)
{
    list<string> keywords;

    string prefix = CComment_rule::GetStructuredCommentPrefix (user);
    string prefix_to_use = CComment_rule::MakePrefixFromRoot(prefix);

    // look up mandatory and required field names from validator rules
    CConstRef<CComment_set> rules = CComment_set::GetCommentRules();

    if (rules) {
        try {
            CConstRef<CComment_rule> ruler = rules->FindCommentRuleEx(prefix_to_use);
            const CComment_rule& rule = *ruler;
            CComment_rule::TErrorList errors = rule.IsValid(user);
            if (errors.size() == 0) {
                string kywd = CComment_rule::KeywordForPrefix( prefix );
                NStr::Split(kywd, ";", keywords, NStr::fSplit_Tokenize);
            }
        } catch (CException& ) {
            // no rule for this prefix, can't list fields
        }
    }

    return keywords;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1726, CRC32: 31eca82f */
