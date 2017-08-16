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
 *`
 * Author:  Jonathan Kans, Clifford Clausen, Aaron Ucko......
 *
 * File Description:
 *   Privae classes and definition for the validator
 *   .......
 *
 */

#ifndef VALIDATOR___VALIDATORP__HPP
#define VALIDATOR___VALIDATORP__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbi_autoinit.hpp>

#include <objmgr/scope.hpp>
#include <objmgr/feat_ci.hpp>  // for CMappedFeat
#include <objmgr/util/seq_loc_util.hpp>
#include <objects/seqset/Bioseq_set.hpp>
#include <objects/seq/GIBB_mol.hpp>
#include <util/strsearch.hpp>
#include <objects/misc/sequence_macros.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Std_seg.hpp>
#include <objects/seqalign/Packed_seg.hpp>
#include <objects/valid/Comment_set.hpp>
#include <objects/valid/Comment_rule.hpp>
#include <objects/taxon3/taxon3.hpp>

#include <objtools/validator/validator.hpp>
#include <objtools/validator/tax_validation_and_cleanup.hpp>
#include <objtools/validator/utilities.hpp>
#include <objtools/validator/feature_match.hpp>
#include <objtools/validator/gene_cache.hpp>
#include <objtools/validator/validerror_imp.hpp>
#include <objtools/validator/validerror_base.hpp>
#include <objtools/validator/validerror_feat.hpp>

#include <objtools/alnmgr/sparse_aln.hpp>

#include <objmgr/util/create_defline.hpp>

#include <objmgr/util/feature.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CSeq_entry;
class CCit_sub;
class CCit_art;
class CCit_gen;
class CSeq_feat;
class CBioseq;
class CSeqdesc;
class CSeq_annot;
class CTrna_ext;
class CProt_ref;
class CSeq_loc;
class CFeat_CI;
class CPub_set;
class CAuth_list;
class CTitle;
class CMolInfo;
class CUser_object;
class CSeqdesc_CI;
class CSeq_graph;
class CMappedGraph;
class CDense_diag;
class CDense_seg;
class CSeq_align_set;
class CPubdesc;
class CBioSource;
class COrg_ref;
class CByte_graph;
class CDelta_seq;
class CGene_ref;
class CCdregion;
class CRNA_ref;
class CImp_feat;
class CSeq_literal;
class CBioseq_Handle;
class CSeq_feat_Handle;
class CCountries;
class CInferencePrefixList;
class CComment_set;
class CTaxon3_reply;
class ITaxon3;
class CT3Error;

BEGIN_SCOPE(validator)

class CTaxValidationAndCleanup;
class CGeneCache;
class CValidError_base;
class CValidError_bioseq;

// =============================================================================
//                            Caching classes
// =============================================================================

// for convenience
typedef CValidator::CCache CCache;


// =============================================================================
//                         Specific validation classes
// =============================================================================




// ===========================  for handling PCR primer subtypes on BioSource ==

class CPCRSet
{
public:
    CPCRSet(size_t pos);
    virtual ~CPCRSet(void);

    string GetFwdName(void)            const { return m_FwdName; }
    string GetFwdSeq(void)             const { return m_FwdSeq; }
    string GetRevName(void)            const { return m_RevName; }
    string GetRevSeq(void)             const { return m_RevSeq; }
    size_t GetOrigPos(void)            const { return m_OrigPos; }

    void SetFwdName(string fwd_name) { m_FwdName = fwd_name; }
    void SetFwdSeq(string fwd_seq)   { m_FwdSeq = fwd_seq; }
    void SetRevName(string rev_name) { m_RevName = rev_name; }
    void SetRevSeq(string rev_seq)   { m_RevSeq = rev_seq; }

private:
    string m_FwdName;
    string m_FwdSeq;
    string m_RevName;
    string m_RevSeq;
    size_t m_OrigPos;
};

class CPCRSetList
{
public:
    CPCRSetList(void);
    ~CPCRSetList(void);

    void AddFwdName (string name);
    void AddRevName (string name);
    void AddFwdSeq (string name);
    void AddRevSeq (string name);

    bool AreSetsUnique(void);

private:
    vector <CPCRSet *> m_SetList;
};



END_SCOPE(validator)
END_SCOPE(objects)
END_NCBI_SCOPE

#endif  /* VALIDATOR___VALIDATORP__HPP */
