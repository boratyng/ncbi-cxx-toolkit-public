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

/// @Gb_qual.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'seqfeat.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Gb_qual_.hpp


#ifndef OBJECTS_SEQFEAT_GB_QUAL_HPP
#define OBJECTS_SEQFEAT_GB_QUAL_HPP


// generated includes
#include <objects/seqfeat/Gb_qual_.hpp>

// generated classes

// other includes
#include <corelib/ncbistd.hpp>
#include <corelib/ncbistr.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <serial/iterator.hpp>

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class NCBI_SEQFEAT_EXPORT CGb_qual : public CGb_qual_Base
{
    typedef CGb_qual_Base Tparent;
public:
    // constructors
    CGb_qual(void);
    CGb_qual(const TQual& qual, const TVal& val);

    // destructor
    ~CGb_qual(void);
    
    int Compare(const CGb_qual& gbqual) const;

private:
    // Prohibit copy constructor and assignment operator
    CGb_qual(const CGb_qual& value);
    CGb_qual& operator=(const CGb_qual& value);

};

/////////////////// CGb_qual inline methods

// constructor
inline
CGb_qual::CGb_qual(void)
{
}

inline
CGb_qual::CGb_qual(const TQual& qual, const TVal& val)
{
    SetQual(qual);
    SetVal(val);
}


inline
int CGb_qual::Compare(const CGb_qual& gbqual) const
{
    if (GetQual() != gbqual.GetQual()) {
        return (GetQual() < gbqual.GetQual() ? -1 : 1);
    }
    if (GetVal() != gbqual.GetVal()) {
        return (GetVal() < gbqual.GetVal() ? -1 : 1);
    }
    return 0;
}


/////////////////// end of CGb_qual inline methods

/////////// inference prefix list
class NCBI_SEQFEAT_EXPORT CInferencePrefixList
{
public:
    // constructors
    CInferencePrefixList(void);

    // destructor
    ~CInferencePrefixList(void);
    
    static void GetPrefixAndRemainder (const string& inference, string& prefix, string& remainder);
    
private:
};

// =============================================================================
//                           Enumeration of GBQual types
// =============================================================================

class NCBI_SEQFEAT_EXPORT CGbqualType
{
public:
    enum EType {
        e_Bad = 0,
        e_Allele,
        e_Anticodon,
        e_Bio_material,
        e_Bound_moiety,
        e_Cell_line,
        e_Cell_type,
        e_Chromosome,
        e_Chloroplast,
        e_Chromoplast,
        e_Citation,
        e_Clone,
        e_Clone_lib,
        e_Codon,
        e_Codon_start,
        e_Collected_by,
        e_Collection_date,
        e_Compare,
        e_Cons_splice,
        e_Country,
        e_Cultivar,
        e_Culture_collection,
        e_Cyanelle,
        e_Cyt_map,
        e_Direction,
        e_EC_number,
        e_Ecotype,
        e_Environmental_sample,
        e_Db_xref,
        e_Dev_stage,
        e_Estimated_length,
        e_Evidence,
        e_Exception,
        e_Experiment,
        e_Focus,
        e_Frequency,
        e_Function,
        e_Gen_map,
        e_Gene,
        e_Gene_synonym,
        e_Gdb_xref,
        e_Germline,
        e_Haplotype,
        e_Host,
        e_Identified_by,
        e_Inference,
        e_Insertion_seq,
        e_Isolate,
        e_Isolation_source,
        e_Kinetoplast,
        e_Label,
        e_Lab_host,
        e_Lat_lon,
        e_Locus_tag,
        e_Macronuclear,
        e_Map,
        e_Mating_type,
        e_Metagenomic,
        e_Mitochondrion,
        e_Mobile_element,
        e_Mod_base,
        e_Mol_type,
        e_NcRNA_class,
        e_Note,
        e_Number,
        e_Old_locus_tag,
        e_Operon,
        e_Organelle,
        e_Organism,
        e_Partial,
        e_PCR_conditions,
        e_PCR_primers,
        e_Phenotype,
        e_Plasmid,
        e_Pop_variant,
        e_Product,
        e_Protein_id,
        e_Proviral,
        e_Pseudo,
        e_Rad_map,
        e_Rearranged,
        e_Replace,
        e_Ribosomal_slippage,
        e_Rpt_family,
        e_Rpt_type,
        e_Rpt_unit,
        e_Rpt_unit_range,
        e_Rpt_unit_seq,
        e_Satellite,
        e_Segment,
        e_Sequenced_mol,
        e_Serotype,
        e_Serovar,
        e_Sex,
        e_Site,
        e_Site_type,
        e_Specimen_voucher,
        e_Standard_name,
        e_Strain,
        e_Sub_clone,
        e_Sub_species,
        e_Sub_strain,
        e_Tag_peptide,
        e_Tissue_lib,
        e_Tissue_type,
        e_Transcript_id,
        e_Transgenic,
        e_Transl_except,
        e_Transl_table,
        e_Translation,
        e_Transposon,
        e_Trans_splicing,
        e_Usedin,
        e_Variety,
        e_Virion
    };

    // Conversions to enumerated type:
    static EType GetType(const string& qual);
    static EType GetType(const CGb_qual& qual);
    
    // Conversion from enumerated to string:
    static const string& GetString(EType gbqual);

private:
    CGbqualType(void);
    DECLARE_INTERNAL_ENUM_INFO(EType);
};


// =============================================================================
//                        Associating Features and GBQuals
// =============================================================================


class NCBI_SEQFEAT_EXPORT CFeatQualAssoc
{
public:
    typedef vector<CGbqualType::EType> TGBQualTypeVec;
    typedef map<CSeqFeatData::ESubtype, TGBQualTypeVec > TFeatQualMap;

    // Check to see is a certain gbqual is legal within the context of 
    // the specified feature
    static bool IsLegalGbqual(CSeqFeatData::ESubtype ftype,
                              CGbqualType::EType gbqual);

    // Retrieve the mandatory gbquals for a specific feature type.
    static const TGBQualTypeVec& GetMandatoryGbquals(CSeqFeatData::ESubtype ftype);

private:

    CFeatQualAssoc(void);
    void PoplulateLegalGbquals(void);
    void Associate(CSeqFeatData::ESubtype, CGbqualType::EType);
    void PopulateMandatoryGbquals(void);
    bool IsLegal(CSeqFeatData::ESubtype ftype, CGbqualType::EType gbqual);
    const TGBQualTypeVec& GetMandatoryQuals(CSeqFeatData::ESubtype ftype);
    
    static CFeatQualAssoc* Instance(void);

    static auto_ptr<CFeatQualAssoc> sm_Instance;
    // list of feature and their associated gbquals
    TFeatQualMap                    m_LegalGbquals;
    // list of features and their mandatory gbquals
    TFeatQualMap                    m_MandatoryGbquals;
};


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_SEQFEAT_GB_QUAL_HPP
/* Original file checksum: lines: 94, chars: 2540, CRC32: e99dce88 */
