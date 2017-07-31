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
 */

/// @file String_constraint.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'macro.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: String_constraint_.hpp


#ifndef OBJECTS_MACRO_STRING_CONSTRAINT_HPP
#define OBJECTS_MACRO_STRING_CONSTRAINT_HPP

// generated includes
#include <objects/macro/String_constraint_.hpp>
#include <serial/iterator.hpp>
#include <objects/seqfeat/Seq_feat.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class CString_constraint : public CString_constraint_Base
{
    typedef CString_constraint_Base Tparent;
public:
    // constructor
    CString_constraint(void);
    // destructor
    ~CString_constraint(void);

    // get all string type data from object
    template <class T>
    void GetStringsFromObject(const T& obj, vector <string>& strs) const
    {
       CTypesConstIterator it(CStdTypeInfo<string>::GetTypeInfo(),
                          CStdTypeInfo<utf8_string_type>::GetTypeInfo());
       for (it = ConstBegin(obj);  it;  ++it) {
          strs.push_back(*static_cast<const string*>(it.GetFoundPtr()));
       }
    }

    bool Match(const string& str) const;
    bool Empty() const;
    bool ReplaceStringConstraintPortionInString(string& val, const string& replace) const;

private:
    string m_digit_str, m_alpha_str;

    // Prohibit copy constructor and assignment operator
    CString_constraint(const CString_constraint& value);
    CString_constraint& operator=(const CString_constraint& value);

    bool x_DoesSingleStringMatchConstraint (const string& str) const;
    bool x_IsWeasel(const string& str) const;
    string x_SkipWeasel(const string& str) const;
    bool x_IsAllCaps(const string& str) const;
    bool x_IsAllLowerCase(const string& str) const;
    bool x_IsAllPunctuation(const string& str) const;
    bool x_IsSkippable(const char ch) const;
    bool x_IsAllSkippable(const string& str) const;
    // Checks whether the first letter of the first word is capitalized
    bool x_IsFirstCap(const string& str) const;
    // Checks whether the first letter of each word is capitalized
    bool x_IsFirstEachCap(const string& str) const;
        
    bool x_PartialCompare(const string& str, const string& pattern, char prev_char, size_t & match_len) const;
    bool x_AdvancedStringCompare(const string& str, 
                                const string& str_match, 
                                const char prev_char, 
                                size_t * ini_target_match_len = 0) const;
    bool x_AdvancedStringMatch(const string& str,const string& tmp_match) const;
    bool x_CaseNCompareEqual(string str1,
                               string str2,
                               size_t len1, bool case_sensitive) const;
    string x_StripUnimportantCharacters(const string& str,
                                     bool strip_space, bool strip_punct) const;
    bool x_IsWholeWordMatch(const string& start,
                              size_t found,
                              size_t match_len,
                              bool disallow_slash = false) const;
    bool x_DisallowCharacter(const char ch, bool disallow_slash) const;

    bool x_GetSpanFromHyphenInString(const string& str, 
                                     size_t hyphen, 
                                     string& first, 
                                     string& second) const;
    bool x_StringIsPositiveAllDigits(const string& str) const;
    bool x_IsStringInSpanInList (const string& str, const string& list) const;
    bool x_IsStringInSpan(const string& str, 
                          const string& first, 
                          const string& second) const;

    bool x_ReplaceContains(string& val, const string& replace) const;
};

/////////////////// CString_constraint inline methods

// constructor
inline
CString_constraint::CString_constraint(void)
{
  m_digit_str = "0123456789";
  m_alpha_str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
}


/////////////////// end of CString_constraint inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_MACRO_STRING_CONSTRAINT_HPP
/* Original file checksum: lines: 86, chars: 2538, CRC32: 9eb12277 */
