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
 *   using specifications from the data definition file
 *   'seq.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/seq/Seq_inst.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSeq_inst::~CSeq_inst(void)
{
}


typedef SStaticPair<CSeq_inst::EMol, string> TMolKey;

static const TMolKey mol_key_to_str[] = {
    { CSeq_inst::eMol_not_set, " " } ,
    { CSeq_inst::eMol_dna,     "DNA" } ,
    { CSeq_inst::eMol_rna,     "RNA" } ,
    { CSeq_inst::eMol_aa,      "protein" } ,
    { CSeq_inst::eMol_na,      "nucleotide" } ,
    { CSeq_inst::eMol_other,   "other" }
};


// dump the array into a map for faster searching
typedef CStaticPairArrayMap <CSeq_inst::EMol, string> TMolMap;
DEFINE_STATIC_ARRAY_MAP(TMolMap, sm_EMolKeys, mol_key_to_str);


string CSeq_inst::GetMoleculeClass(CSeq_inst::EMol mol)
{
    TMolMap::const_iterator g_iter = sm_EMolKeys.find(mol);
    if (g_iter != sm_EMolKeys.end()) {
        return g_iter->second;
    }
    return kEmptyStr;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 64, chars: 1872, CRC32: f553d25a */
