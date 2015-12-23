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
 *   'seqfeat.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/seqfeat/Gb_qual.hpp>

// other includes
#include <serial/enumvalues.hpp>
#include <serial/serialimpl.hpp>
#include <util/xregexp/regexp.hpp>

// generated classes


BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CGb_qual::~CGb_qual(void)
{
}


static const char * const valid_inf_categories [] = {
    "EXISTENCE",
    "COORDINATES",
    "DESCRIPTION"
};

static const char * const valid_inf_prefixes [] = {
    "ab initio prediction",
    "nucleotide motif",
    "profile",
    "protein motif",
    "similar to AA sequence",
    "similar to DNA sequence",
    "similar to RNA sequence",
    "similar to RNA sequence, EST",
    "similar to RNA sequence, mRNA",
    "similar to RNA sequence, other RNA",
    "similar to sequence",
    "alignment"
};


void CGb_qual::ParseExperiment(const string& orig, string& category, string& experiment, string& doi)
{
    experiment = orig;
    category = "";
    doi = "";
    NStr::TruncateSpacesInPlace(experiment);

    for (unsigned int i = 0; i < sizeof (valid_inf_categories) / sizeof (char *); i++) {
        if (NStr::StartsWith (experiment, valid_inf_categories[i])) {
            category = valid_inf_categories[i];
            experiment = experiment.substr(category.length());
            NStr::TruncateSpacesInPlace(experiment);
            if (NStr::StartsWith(experiment, ":")) {
                experiment = experiment.substr(1);
            }
            NStr::TruncateSpacesInPlace(experiment);
            break;
        }
    }
    if (NStr::EndsWith(experiment, "]")) {
        size_t start_doi = NStr::Find(experiment, "[");

        if (start_doi != string::npos) {
            doi = experiment.substr(start_doi + 1);
            doi = doi.substr(0, doi.length() - 1);
            experiment = experiment.substr(0, start_doi);
        }
    }
}


string CGb_qual::BuildExperiment(const string& category, const string& experiment, const string& doi)
{
    string rval = "";
    if (!NStr::IsBlank(category)) {
        rval += category + ":";
    }
    rval += experiment;
    if (!NStr::IsBlank(doi)) {
        rval += "[" + doi + "]";
    }
    return rval;
}


bool CGb_qual::x_CleanupRptAndReplaceSeq(string& val)
{
    if (NStr::IsBlank(val)) {
        return false;
    }
    // do not clean if val contains non-sequence characters
    if (string::npos != val.find_first_not_of("ACGTUacgtu")) {
        return false;
    }
    string orig = val;
    NStr::ToLower(val);
    NStr::ReplaceInPlace(val, "u", "t");
    return !NStr::Equal(orig, val);
}


bool CGb_qual::CleanupRptUnitSeq(string& val)
{
    return x_CleanupRptAndReplaceSeq(val);
}


bool CGb_qual::CleanupReplace(string& val)
{
    return x_CleanupRptAndReplaceSeq(val);
}


// converts dashes to a pair of dots, unless dots are already present
// also makes no change if characters other than digits or dashes are found
bool CGb_qual::CleanupRptUnitRange(string& val)
{
    if (NStr::IsBlank(val)) {
        return false;
    }
    if (NStr::Find(val, ".") != string::npos) {
        return false;
    }
    if (NStr::Find(val, "-") == string::npos) {
        return false;
    }
    if (string::npos != val.find_first_not_of("0123456789-")) {
        return false;
    }
    NStr::ReplaceInPlace(val, "-", "..");
    return true;
}


// constructor
CInferencePrefixList::CInferencePrefixList(void)
{
}

//destructor
CInferencePrefixList::~CInferencePrefixList(void)
{
}


void CInferencePrefixList::GetPrefixAndRemainder (const string& inference, string& prefix, string& remainder)
{
    string category = "";
    prefix = "";
    remainder = "";
    string check = inference;

    for (unsigned int i = 0; i < sizeof (valid_inf_categories) / sizeof (char *); i++) {
        if (NStr::StartsWith (check, valid_inf_categories[i])) {
            category = valid_inf_categories[i];
            check = check.substr(category.length());
            NStr::TruncateSpacesInPlace(check);
            if (NStr::StartsWith(check, ":")) {
                check = check.substr(1);
            }
            if (NStr::StartsWith(check, " ")) {
                check = check.substr(1);
            }
            break;
        }
    }
    for (unsigned int i = 0; i < sizeof (valid_inf_prefixes) / sizeof (char *); i++) {
        if (NStr::StartsWith (check, valid_inf_prefixes[i], NStr::eNocase)) {
            prefix = valid_inf_prefixes[i];
        }
    }

    remainder = check.substr (prefix.length());
    NStr::TruncateSpacesInPlace (remainder);
}


static string s_LegalMobileElementStrings[] = {
    "transposon",
    "retrotransposon",
    "integron",
    "superintegron",
    "insertion sequence",
    "non-LTR retrotransposon",
    "P-element",
    "transposable element",
    "SINE",
    "MITE",
    "LINE",
    "other"
};


void CGb_qual::GetMobileElementValueElements(const string& val, string& element_type, string& element_name)
{
    element_type = "";
    element_name = "";
    for (size_t i = 0;
        i < sizeof(s_LegalMobileElementStrings) / sizeof(string);
        ++i) {
        if (NStr::StartsWith(val, s_LegalMobileElementStrings[i], NStr::eNocase)) {
            element_name = val.substr(s_LegalMobileElementStrings[i].length());
            if (!NStr::IsBlank(element_name) &&
                (!NStr::StartsWith(element_name, ":") || NStr::Equal(element_name, ":"))) {
                element_name = "";
            } else {
                element_type = s_LegalMobileElementStrings[i];
            }
            break;
        }
    }
}


bool CGb_qual::IsLegalMobileElementValue(const string& val)
{
    string element_type;
    string element_name;
    GetMobileElementValueElements(val, element_type, element_name);
    if (NStr::IsBlank(element_type)) {
        return false;
    } else if (NStr::Equal(element_type, "other") && NStr::IsBlank(element_name)) {
        return false;
    } else {
        return true;
    }
}


static string s_IllegalQualNameStrings[] = {
    "anticodon",
    "citation",
    "codon_start",
    "db_xref",
    "evidence",
    "exception",
    "gene",
    "note",
    "protein_id",
    "pseudo",
    "transcript_id",
    "translation",
    "transl_except",
    "transl_table"
};

bool CGb_qual::IsIllegalQualName(const string& val)
{
    for (size_t i = 0;
        i < sizeof(s_IllegalQualNameStrings) / sizeof(string);
        ++i) {
        if (NStr::EqualNocase(val, s_IllegalQualNameStrings[i])) {
            return true;
        }
    }
    return false;
}

static string s_FindInArray(const string &val, const char **arr)
{
    string result;
    for (unsigned int i = 0; arr[i][0] != '\0'; i++) 
        if (arr[i] == val)
        {
            result = val;
            break;
        }
    return result;
}

void CGb_qual::ParseInferenceString(string val, string &category, string &type_str, bool &is_same_species, string &database, 
                                    string &accession, string &program, string &version, string &acc_list)
{
    category.clear();
    static const char *categories[] = {"COORDINATES", "DESCRIPTION", "EXISTENCE", "\0"};
    for (unsigned int i = 0; categories[i][0] != '\0'; i++) 
    {
        if (NStr::StartsWith(val, categories[i], NStr::eNocase)) {
            category = categories[i];
            val = val.substr(strlen(categories[i]));
            NStr::TruncateSpacesInPlace(val);
            if (NStr::StartsWith(val, ":")) {
                val = val.substr(1);
                NStr::TruncateSpacesInPlace(val);
            }
            break;
        }
    }


    static const char *types[] = {
        "similar to sequence",
        "similar to protein",
        "similar to DNA",
        "similar to RNA",
        "similar to mRNA",
        "similar to EST",
        "similar to other RNA",
        "profile",
        "nucleotide motif",
        "protein motif",
        "ab initio prediction",
        "alignment",
        "\0"};

    type_str.clear();
    is_same_species = false;
    // start with 1 - first item is blank
    for (unsigned int i = 0; types[i][0] != '\0'; i++) 
    {
        if (NStr::StartsWith(val, types[i], NStr::eNocase)) 
        {
            type_str = types[i];
            val = val.substr(strlen(types[i]));
            NStr::TruncateSpacesInPlace(val);
            if (NStr::StartsWith(val, "(same species)", NStr::eNocase)) {
                is_same_species = true;
                val = val.substr(14);
                NStr::TruncateSpacesInPlace(val);
            }
	    if (NStr::StartsWith(val, ":")) {
                val = val.substr(1);
                NStr::TruncateSpacesInPlace(val);
            }
            break;
        }
    }

    // add type-dependent extra data
    if (NStr::StartsWith(type_str, "similar to ")) {

        static const char *choices[] = {
            "GenBank",
            "EMBL",
            "DDBJ",
            "INSD",
            "RefSeq",
            "UniProt",
            "Other",
            "\0"};
     
        NStr::TruncateSpacesInPlace(val);
        while (NStr::StartsWith(val, "|")) {
            val = val.substr(1);
            NStr::TruncateSpacesInPlace(val);
        }
        size_t pos = NStr::Find(val, ":");
        if (pos == string::npos) {
            database = s_FindInArray(val, choices);
            if (database.empty())
                accession = val;            
            else
                accession.clear();
        } else {
            string part1 = val.substr(0, pos);
            string part2 = val.substr(pos + 1);
            database = s_FindInArray(part1, choices);
            if (!database.empty())
            {
                accession = part2;            
            }
            else
            {
                if (NStr::IsBlank(part1)) 
                {
                    accession = part2;
                } else 
                {
                    accession = val;
                }
            }
        }
    } else if (NStr::EqualNocase(type_str, "profile") 
		         || NStr::EqualNocase(type_str, "nucleotide motif")
		         || NStr::EqualNocase(type_str, "protein motif")) {

        if (NStr::IsBlank (val)) {
            program.clear();
            version.clear();
        } else {
            size_t pos = NStr::Find(val, ":");
            if (pos == string::npos) {
                program = val;
                version.clear();
            } else {
                string part1 = val.substr(0, pos);
                string part2 = val.substr(pos + 1);
                program = part1;
                version = part2;
            }
        }
    } else if (NStr::EqualNocase(type_str, "ab initio prediction")) {

        if (NStr::IsBlank (val)) {
            program.clear();
            version.clear();
        } else {
            size_t pos = NStr::Find(val, ":");
            if (pos == string::npos) {
                program = val;
                version.clear();
            } else {
                string part1 = val.substr(0, pos);
                string part2 = val.substr(pos + 1);
                program = part1;
                version = part2;
            }
        }
    } else if (NStr::EqualNocase(type_str, "alignment")) {

        string acc_list_str;
        if (NStr::IsBlank (val)) {
            program.clear();
            version.clear();
        } else {
            size_t pos = NStr::Find(val, ":");
            if (pos == string::npos) {
                program = val;
                version.clear();
            } else {
                string part1 = val.substr(0, pos);
                string part2 = val.substr(pos + 1);
                program = part1;
                pos = NStr::Find(part2, ":");
                if (pos == string::npos) {
                    version = part2;
                    // set alignment list blank
                    acc_list.clear();
                } else {
                    string ver_str = part2.substr(0, pos);
                    acc_list_str = part2.substr(pos + 1);
                    version = ver_str;
                    // set alignment list
                    NStr::ReplaceInPlace(acc_list_str, ",", "\n");
                    acc_list = acc_list_str;
                }
            }
        }
    }
}

string CGb_qual::CleanupAndRepairInference( const string &orig_inference )
{
    string inference(orig_inference);
    if( inference.empty() ) {
        return inference;
    }


    CRegexpUtil colonFixer( inference );
    colonFixer.Replace( "[ ]+:", ":" );
    colonFixer.Replace( ":*:[ ]+", ": ");
    colonFixer.GetResult().swap( inference ); // swap is faster than assignment

    // check if missing space after a prefix
    // e.g. "COORDINATES:foo" should become "COORDINATES: foo"
    CRegexp spaceInserter("(COORDINATES|DESCRIPTION|EXISTENCE):[^ ]", CRegexp::fCompile_default);
    if( spaceInserter.IsMatch( inference ) ) {
        int location_just_beyond_match = spaceInserter.GetResults(0)[1];
        inference.insert( inference.begin() + location_just_beyond_match - 1, ' ' );
    }

    return inference;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 65, chars: 1885, CRC32: 3224da35 */
