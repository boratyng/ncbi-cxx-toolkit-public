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
 *   using specifications from the ASN data definition file
 *   'seqloc.asn'.
 */

#include <ncbi_pch.hpp>
#include <objects/seqloc/PDB_seq_id.hpp>
#include <objects/seqloc/PDB_mol_id.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::


// destructor
CPDB_seq_id::~CPDB_seq_id(void)
{
    return;
}


// comparison function
bool CPDB_seq_id::Match(const CPDB_seq_id& psip2) const
{
    if (IsSetChain() && psip2.IsSetChain()) {
        if (GetChain() != psip2.GetChain()) {
            return false;
        }
    }
    if (IsSetChain_id() && psip2.IsSetChain_id()) {
        if ( PCase().Compare(GetChain_id(), psip2.GetChain_id())) {
            return false;
        }
    }
    return
        PCase().Equals(GetMol(), psip2.GetMol());
}


int CPDB_seq_id::Compare(const CPDB_seq_id& psip2) const
{
    if (IsSetChain() && psip2.IsSetChain()) {
        if ( int diff = GetChain() - psip2.GetChain() ) {
            return diff;
        }
    }
    if (IsSetChain_id() && psip2.IsSetChain_id()) {
        if ( int diff = PCase().Compare(GetChain_id(), psip2.GetChain_id()) ) {
            return diff;
        }
    }
    return PCase().Compare(GetMol(), psip2.GetMol());
}


// format a FASTA style string
ostream& CPDB_seq_id::AsFastaString(ostream& s) const
{
    if (IsSetChain_id()) {
            return s << GetMol().Get() << '|' << GetChain_id();
    }
    // no Upcase per Ostell - Karl 7/2001 
    // Output "VB" when chain id is ASCII 124 ('|').
    // Output double upper case letter when chain is a lower case character.
    char chain = (char) GetChain();

    if (chain == '|') {
        return s << GetMol().Get() << "|VB";
    } else if ( islower((unsigned char) chain) != 0 ) {
        return s << GetMol().Get() << '|'
                 << (char) toupper((unsigned char) chain) << (char) toupper((unsigned char) chain);
    } else if ( chain == '\0' ) {
        return s << GetMol().Get() << "| ";
    } 
    return s << GetMol().Get() << '|' << chain; 
}


END_objects_SCOPE // namespace ncbi::objects::
END_NCBI_SCOPE
