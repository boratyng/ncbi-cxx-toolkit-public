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
* Author:  Mati Shomrat, NCBI
*
* File Description:
*   shared utility functions
*
*/
#include <corelib/ncbistd.hpp>

#include <objects/general/Date.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/util/sequence.hpp>

#include "utils.hpp"


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


string ExpandTildes(const string& s, ETildeStyle style)
{
    if ( style == eTilde_tilde ) {
        return s;
    }

    SIZE_TYPE start = 0, tilde, length = s.length();
    string result;

    while ( (start < length)  &&  (tilde = s.find('~', start)) != NPOS ) {
        result += s.substr(start, tilde - start);
        char next = (tilde + 1) < length ? s[tilde + 1] : 0;
        switch ( style ) {
        case eTilde_space:
            if ( (tilde + 1 < length  &&  isdigit(next) )  ||
                 (tilde + 2 < length  &&  (next == ' '  ||  next == '(')  &&
                  isdigit(s[tilde + 2]))) {
                result += '~';
            } else {
                result += ' ';
            }
            start = tilde + 1;
            break;
            
        case eTilde_newline:
            if ( tilde + 1 < length  &&  s[tilde + 1] == '~' ) {
                result += '~';
                start = tilde + 2;
            } else {
                result += '\n';
                start = tilde + 1;
            }
            break;
            
        default: // just keep it, for lack of better ideas
            result += '~';
            start = tilde + 1;
            break;
        }
    }
    result += s.substr(start);
    return result;
}


void StripSpaces(string& str)
{
    if ( str.empty() ) {
        return;
    }

    string::iterator new_str = str.begin();
    NON_CONST_ITERATE(string, it, str) {
        *new_str++ = *it;
        if ( (*it == ' ')  ||  (*it == '\t')  ||  (*it == '(') ) {
            for (++it; *it == ' ' || *it == '\t'; ++it) continue;
            if (*it == ')' || *it == ',') {
                new_str--;
            }
        } else {
            it++;
        }
    }
    str.erase(new_str, str.end());
}


static bool s_IsWholeWord(const string& str, size_t pos, const string& word)
{
    // NB: To preserve the behavior of the C toolkit we only test on the left.
    // This was an old bug in the C toolkit that was never fixed and by now
    // has become the expected behavior.
    return (pos > 0) ?
        isspace(str[pos - 1])  ||  ispunct(str[pos - 1]) : true;
}


void JoinNoRedund(string& str1, const string& str2, const string& delim)
{
    if ( str2.empty() ) {
        return;
    }

    if ( str1.empty() ) {
        str1 = str2;
        return;
    }
    
    size_t pos = NPOS;
    for ( pos = NStr::FindNoCase(str1, str2);
          pos != NPOS  &&  !s_IsWholeWord(str1, pos, str2);
          pos += str2.length());

    if ( pos == NPOS  ||  !s_IsWholeWord(str1, pos, str2) ) {
        str1 += delim;
        str1 += str2;
    }
}


string JoinNoRedund(const list<string>& l, const string& delim)
{
    if ( l.empty() ) {
        return kEmptyStr;
    }

    string result = l.front();
    list<string>::const_iterator it = l.begin();
    while ( ++it != l.end() ) {
        JoinNoRedund(result, *it, delim);
    }

    return result;
}


// Validate the correct format of an accession string.
bool ValidateAccession(const string& acc)
{
    if ( acc.empty() ) {
        return false;
    }

    if ( acc.length() >= 16 ) {
        return false;
    }

    // first character must be uppercase letter
    if ( !(isalpha(acc[0])  &&  isupper(acc[0])) ) {
        return false;
    }

    size_t num_alpha   = 0,
           num_undersc = 0,
           num_digits  = 0;

    const char* ptr = acc.c_str();
    if ( NStr::StartsWith(acc, "NZ_") ) {
        ptr += 3;
    }
    for ( ; isalpha(*ptr); ++ptr, ++num_alpha );
    for ( ; *ptr == '_'; ++ptr, ++num_undersc );
    for ( ; isdigit(*ptr); ++ptr, ++num_digits );

    if ( (*ptr != '\0')  &&  (*ptr != ' ')  &&  (*ptr != '.') ) {
        return false;
    }

    switch ( num_undersc ) {
    case 0:
        {{
            if ( (num_alpha == 1  &&  num_digits == 5)  ||
                 (num_alpha == 2  &&  num_digits == 6)  ||
                 (num_alpha == 3  &&  num_digits == 5)  || 
                 (num_alpha == 4  &&  num_digits == 8) ) {
                return true;
            }
        }}
        break;

    case 1:
        {{
            // RefSeq accession
            if ( num_alpha != 2  ||
                 (num_digits != 6  && num_digits != 8) ) { 
                return false;
            }
            
            char first_letter = acc[0];
            char second_letter = acc[1];

            if ( first_letter == 'N' ) {
                if ( second_letter == 'C'  ||  second_letter == 'G'  ||
                     second_letter == 'M'  ||  second_letter == 'R'  ||
                     second_letter == 'P'  ||  second_letter == 'W'  ||
                     second_letter == 'T' ) {
                    return true;
                }
            } else if ( first_letter == 'X' ) {
                if ( second_letter == 'M'  ||  second_letter == 'R'  ||
                     second_letter == 'P' ) {
                    return true;
                }
            } else if ( first_letter == 'Z' ) {
                if ( second_letter == 'P' ) {
                    return true;
                }
            }
        }}
        break;

    default:
        return false;
    }

    return false;
}


void DateToString(const CDate& date, string& str,  bool is_cit_sub)
{
    static const string regular_format  = "%{%2D%|01%}-%{%3N%|JUN%}-%Y";
    static const string cit_sub_format = "%{%2D%|??%}-%{%3N%|???%}-%Y";

    const string& format = is_cit_sub ? cit_sub_format : regular_format;

    string date_str;
    date.GetDate(&date_str, format);
    NStr::ToUpper(date_str);

    str.append(date_str);
}


void GetDeltaSeqSummary
(const CBioseq& seq,
 CScope& scope,
 SDeltaSeqSummary& summary)
{
    _ASSERT(seq.CanGetInst());
    _ASSERT(seq.GetInst().CanGetRepr());
    _ASSERT(seq.GetInst().GetRepr() == CSeq_inst::eRepr_delta);
    _ASSERT(seq.GetInst().CanGetExt());
    _ASSERT(seq.GetInst().GetExt().IsDelta());

    SDeltaSeqSummary temp;

    const CDelta_ext::Tdata& segs = seq.GetInst().GetExt().GetDelta().Get();
    temp.num_segs = segs.size();
    
    
    size_t len = 0;

    CNcbiOstrstream text;

    CDelta_ext::Tdata::const_iterator curr = segs.begin();
    CDelta_ext::Tdata::const_iterator end = segs.end();
    CDelta_ext::Tdata::const_iterator next;
    for ( ; curr != end; curr = next ) {
        {{
            // set next to one after curr
            next = curr; ++next;
        }}
        size_t from = len + 1;
        switch ( (*curr)->Which() ) {
        case CDelta_seq::e_Loc:
            {{
                const CDelta_seq::TLoc& loc = (*curr)->GetLoc();
                if ( loc.IsNull() ) {  // gap
                    ++temp.num_gaps;
                    text << "* " << from << ' ' << len 
                         << " gap of unknown length~";
                } else {
                    size_t tlen = sequence::GetLength(loc, &scope);
                    len += tlen;
                    temp.residues += tlen;
                    text << "* " << from << " " << len << ": contig of " 
                        << tlen << " bp in length~";
                }
            }}  
            break;
        case CDelta_seq::e_Literal:
            {{
                const CDelta_seq::TLiteral& lit = (*curr)->GetLiteral();
                size_t lit_len = lit.CanGetLength() ? lit.GetLength() : 0;
                len += lit_len;
                if ( lit.CanGetSeq_data() ) {
                    temp.residues += lit_len;
                    while ( next != end  &&  (*next)->IsLiteral()  &&
                        (*next)->GetLiteral().CanGetSeq_data() ) {
                        const CDelta_seq::TLiteral& next_lit = (*next)->GetLiteral();
                        size_t next_len = next_lit.CanGetLength() ?
                            next_lit.GetLength() : 0;
                        lit_len += next_len;
                        len += next_len;
                        temp.residues += next_len;
                        ++next;
                    }
                    text << "* " << from << " " << len << ": contig of " 
                         << lit_len << " bp in length~";
                } else {
                    bool unk = false;
                    ++temp.num_gaps;
                    if ( lit.CanGetFuzz() ) {
                        const CSeq_literal::TFuzz& fuzz = lit.GetFuzz();
                        if ( fuzz.IsLim()  &&  
                             fuzz.GetLim() == CInt_fuzz::eLim_unk ) {
                            unk = true;
                            ++temp.num_faked_gaps;
                            if ( from > len ) {
                                text << "*                    gap of unknown length~";
                            } else {
                                text << "* " << from << " " << len 
                                     << ": gap of unknown length~";
                            }
                        }
                    }
                    if ( !unk ) {
                        text << "* " << from << " " << len << ": gap of "
                             << lit_len << " bp~";
                    }
                }
            }}
            break;

        default:
            break;
        }
    }
    summary = temp;
    summary.text = CNcbiOstrstreamToString(text);
}


const string& GetTechString(int tech)
{
    static const string concept_trans_str = "conceptual translation";
    static const string seq_pept_str = "direct peptide sequencing";
    static const string both_str = "conceptual translation with partial peptide sequencing";
    static const string seq_pept_overlap_str = "sequenced peptide, ordered by overlap";
    static const string seq_pept_homol_str = "sequenced peptide, ordered by homology";
    static const string concept_trans_a_str = "conceptual translation supplied by author";
    
    switch ( tech ) {
    case CMolInfo::eTech_concept_trans:
        return concept_trans_str;

    case CMolInfo::eTech_seq_pept :
        return seq_pept_str;

    case CMolInfo::eTech_both:
        return both_str;

    case CMolInfo::eTech_seq_pept_overlap:
        return seq_pept_overlap_str;

    case CMolInfo::eTech_seq_pept_homol:
        return seq_pept_homol_str;

    case CMolInfo::eTech_concept_trans_a:
        return concept_trans_a_str;

    default:
        return kEmptyStr;
    }

    return kEmptyStr;
}


bool s_IsModelEvidanceUop(const CUser_object& uo)
{
    return (uo.CanGetType()  &&  uo.GetType().IsStr()  &&
        uo.GetType().GetStr() == "ModelEvidence");
}


const CUser_object* s_FindModelEvidanceUop(const CUser_object& uo)
{
    if ( s_IsModelEvidanceUop(uo) ) {
        return &uo;
    }

    const CUser_object* temp = 0;
    ITERATE (CUser_object::TData, ufi, uo.GetData()) {
        const CUser_field& uf = **ufi;
        if ( !uf.CanGetData() ) {
            continue;
        }
        const CUser_field::TData& data = uf.GetData();

        switch ( data.Which() ) {
        case CUser_field::TData::e_Object:
            temp = s_FindModelEvidanceUop(data.GetObject());
            break;

        case CUser_field::TData::e_Objects:
            ITERATE (CUser_field::TData::TObjects, obj, data.GetObjects()) {
                temp = s_FindModelEvidanceUop(**obj);
                if ( temp != 0 ) {
                    break;
                }
            }
            break;

        default:
            break;
        }
        if ( temp != 0 ) {
            break;
        }
    }

    return temp;
}


bool s_GetModelEvidance(const CBioseq_Handle& bsh, SModelEvidance& me)
{
    const CUser_object* moduop = 0;
    bool result = false;

    for (CSeqdesc_CI it(bsh, CSeqdesc::e_User);  it;  ++it) {
        const CUser_object* modup = s_FindModelEvidanceUop(it->GetUser());
        if ( modup != 0 ) {
            result = true;
            const CUser_field* ufp = 0;
            if ( moduop->HasField("Contig Name") ) {
                ufp = &(moduop->GetField("Contig Name"));
                if ( ufp->CanGetData()  &&  ufp->GetData().IsStr() ) {
                    me.name = ufp->GetData().GetStr();
                }
            }
            if ( moduop->HasField("Method") ) {
                ufp = &(moduop->GetField("Method"));
                if ( ufp->CanGetData()  &&  ufp->GetData().IsStr() ) {
                    me.method = ufp->GetData().GetStr();
                }
            }
            if ( moduop->HasField("mRNA") ) {
                me.mrnaEv = true;
            }
            if ( moduop->HasField("EST") ) {
                me.estEv = true;
            }
        }
    }

    return result;
}


bool GetModelEvidance
(const CBioseq_Handle& bsh,
 CScope& scope,
 SModelEvidance& me)
{
    if ( s_GetModelEvidance(bsh, me) ) {
        return true;
    }

    if ( bsh.GetBioseq().IsAa() ) {
        const CBioseq* nuc =
            sequence::GetNucleotideParent(bsh.GetBioseq(), &scope);
        if ( nuc != 0 ) {
            return s_GetModelEvidance(scope.GetBioseqHandle(*nuc), me);
        }
    }

    return false;
}


END_SCOPE(objects)
END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.2  2004/02/11 16:57:34  shomrat
* added JoinNoRedund functions
*
* Revision 1.1  2003/12/17 20:25:01  shomrat
* Initial Revision (adapted from flat lib)
*
*
* ===========================================================================
*/
