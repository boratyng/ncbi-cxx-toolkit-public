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
#include <objects/seqfeat/Variation_ref.hpp>
#include <objects/seq/Seq_literal.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CVariation_ref::~CVariation_ref(void)
{
}


void CVariation_ref::PostRead()
{
    // population-data: ignore; deprecated, drop
    if (Tparent::IsSetPopulation_data()) {
        ERR_POST(Error
                 << "Variation-ref.population-data is deprecated and "
                 "will be ignored");
        Tparent::ResetPopulation_data();
    }

    // validated: move to VariantProperties.other-validation
    if (Tparent::IsSetValidated()) {
        if (SetVariant_prop().IsSetOther_validation()) {
            ERR_POST(Error
                     << "Both Variation-ref.validated and "
                     "Variation-ref.variant-properties.other-validation are "
                     "set; ignoring Variation-ref.validated");
        }
        else {
            SetVariant_prop().SetOther_validation(Tparent::GetValidated());
        }
        Tparent::ResetValidated();
    }

    // clinical-test: should be moved to Seq-feat.dbxref; no access here!
    // FIXME: is this used anywhere?
    if (Tparent::IsSetClinical_test()) {
        ERR_POST(Error
                 << "Variation-ref.clinical-test is deprecated and "
                 "will be ignored");
        Tparent::ResetClinical_test();
    }

    // allele-origin: move to Variant-properties
    if (Tparent::IsSetAllele_origin()) {
        if (SetVariant_prop().IsSetAllele_origin()) {
            ERR_POST(Error
                     << "Both Variation-ref.allele-origin and "
                     "Variation-ref.variant-properties.allele-origin are "
                     "set; ignoring Variation-ref.validated");
        }
        else {
            SetVariant_prop().SetAllele_origin(Tparent::GetAllele_origin());
        }
        Tparent::ResetAllele_origin();
    }

    // allele-state: move to Variant-properties
    if (Tparent::IsSetAllele_state()) {
        if (SetVariant_prop().IsSetAllele_state()) {
            ERR_POST(Error
                     << "Both Variation-ref.allele-state and "
                     "Variation-ref.variant-properties.allele-state are "
                     "set; ignoring Variation-ref.validated");
        }
        else {
            SetVariant_prop().SetAllele_state(Tparent::GetAllele_state());
        }
        Tparent::ResetAllele_state();
    }

    // allele-frequency: move to Variant-properties
    if (Tparent::IsSetAllele_frequency()) {
        if (SetVariant_prop().IsSetAllele_frequency()) {
            ERR_POST(Error
                     << "Both Variation-ref.allele-frequency and "
                     "Variation-ref.variant-properties.allele-frequency are "
                     "set; ignoring Variation-ref.validated");
        }
        else {
            SetVariant_prop().SetAllele_frequency(Tparent::GetAllele_frequency());
        }
        Tparent::ResetAllele_frequency();
    }

    // is-ancestral-allele: move to Variant-properties
    if (Tparent::IsSetIs_ancestral_allele()) {
        if (SetVariant_prop().IsSetIs_ancestral_allele()) {
            ERR_POST(Error
                     << "Both Variation-ref.is-ancestral-allele and "
                     "Variation-ref.variant-properties.is-ancestral-allele are "
                     "set; ignoring Variation-ref.validated");
        }
        else {
            SetVariant_prop().SetIs_ancestral_allele(Tparent::GetIs_ancestral_allele());
        }
        Tparent::ResetIs_ancestral_allele();
    }

    // pub: move to Seq-feat.dbxref; no access here
    // FIXME: do we need to do this?
    if (Tparent::IsSetPub()) {
        ERR_POST(Error
                 << "Variation-ref.pub is deprecated and "
                 "will be ignored");
        Tparent::ResetPub();
    }

    /**
    // location: deprecated, drop
    if (Tparent::IsSetLocation()) {
        ERR_POST(Error
                 << "Variation-ref.location is deprecated and "
                 "will be ignored");
        Tparent::ResetLocation();
    }
    **/

    /**
    // ext-locs: deprecated, drop
    if (Tparent::IsSetExt_locs()) {
        ERR_POST(Error
                 << "Variation-ref.ext-locs is deprecated and "
                 "will be ignored");
        Tparent::ResetExt_locs();
    }
    **/

    /**
    // ext: deprecated, drop
    if (Tparent::IsSetExt()) {
        ERR_POST(Error
                 << "Variation-ref.ext is deprecated and "
                 "will be ignored");
        Tparent::ResetExt();
    }
    **/
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetPopulation_data(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::IsSetPopulation_data(): "
               "unsupported deprecated API");
}


bool CVariation_ref::CanGetPopulation_data(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::CanGetPopulation_data(): "
               "unsupported deprecated API");
}


void CVariation_ref::ResetPopulation_data(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::ResetPopulation_data(): "
               "unsupported deprecated API");
}


const CVariation_ref::TPopulation_data& CVariation_ref::GetPopulation_data(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::GetPopulation_data(): "
               "unsupported deprecated API");
}


CVariation_ref::TPopulation_data& CVariation_ref::SetPopulation_data(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::SetPopulation_data(): "
               "unsupported deprecated API");
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetValidated(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().IsSetOther_validation();
    }
    return Tparent::IsSetValidated();
}


bool CVariation_ref::CanGetValidated(void) const
{
    if (CanGetVariant_prop()) {
        return GetVariant_prop().CanGetOther_validation();
    }
    return Tparent::CanGetValidated();
}


void CVariation_ref::ResetValidated(void)
{
    if (IsSetVariant_prop()) {
        SetVariant_prop().ResetOther_validation();
    }
    Tparent::ResetValidated();
}


CVariation_ref::TValidated CVariation_ref::GetValidated(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().GetOther_validation();
    }
    return Tparent::GetValidated();
}


void CVariation_ref::SetValidated(TValidated value)
{
    if (Tparent::IsSetValidated()) {
        Tparent::ResetValidated();
    }
    SetVariant_prop().SetOther_validation(value);
}


CVariation_ref::TValidated& CVariation_ref::SetValidated(void)
{
    if (Tparent::IsSetValidated()) {
        if (SetVariant_prop().IsSetOther_validation()) {
            ERR_POST(Error <<
                     "Dropping deprecated conflicting data: "
                     "Variation-ref.validated: "
                     "Variation-ref.variant-prop.other-validation set");
        }
        else {
            SetVariant_prop().SetOther_validation(Tparent::GetValidated());
        }

        Tparent::ResetValidated();
    }
    return SetVariant_prop().SetOther_validation();
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetClinical_test(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::IsSetClinical_test(): "
               "unsupported deprecated API");
}


bool CVariation_ref::CanGetClinical_test(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::CanGetClinical_test(): "
               "unsupported deprecated API");
}


void CVariation_ref::ResetClinical_test(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::ResetClinical_test(): "
               "unsupported deprecated API");
}


const CVariation_ref::TClinical_test& CVariation_ref::GetClinical_test(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::GetClinical_test(): "
               "unsupported deprecated API");
}


CVariation_ref::TClinical_test& CVariation_ref::SetClinical_test(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::SetClinical_test(): "
               "unsupported deprecated API");
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetAllele_origin(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().IsSetAllele_origin();
    }
    return Tparent::IsSetAllele_origin();
}


bool CVariation_ref::CanGetAllele_origin(void) const
{
    if (CanGetVariant_prop()) {
        return GetVariant_prop().CanGetAllele_origin();
    }
    return Tparent::CanGetAllele_origin();
}


void CVariation_ref::ResetAllele_origin(void)
{
    if (IsSetVariant_prop()) {
        SetVariant_prop().ResetAllele_origin();
    }
    Tparent::ResetAllele_origin();
}


CVariation_ref::TAllele_origin CVariation_ref::GetAllele_origin(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().GetAllele_origin();
    }
    return Tparent::GetAllele_origin();
}


void CVariation_ref::SetAllele_origin(TAllele_origin value)
{
    if (Tparent::IsSetAllele_origin()) {
        Tparent::ResetAllele_origin();
    }
    SetVariant_prop().SetAllele_origin(value);
}


CVariation_ref::TAllele_origin& CVariation_ref::SetAllele_origin(void)
{
    if (Tparent::IsSetAllele_origin()) {
        if (SetVariant_prop().IsSetAllele_origin()) {
            ERR_POST(Error <<
                     "Dropping deprecated conflicting data: "
                     "Variation-ref.allele-origin: "
                     "Variation-ref.variant-prop.allele-origin set");
        }
        else {
            SetVariant_prop().SetAllele_origin(Tparent::GetAllele_origin());
        }

        Tparent::ResetAllele_origin();
    }
    return SetVariant_prop().SetAllele_origin();
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetAllele_state(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().IsSetAllele_state();
    }
    return Tparent::IsSetAllele_state();
}


bool CVariation_ref::CanGetAllele_state(void) const
{
    if (CanGetVariant_prop()) {
        return GetVariant_prop().CanGetAllele_state();
    }
    return Tparent::CanGetAllele_state();
}


void CVariation_ref::ResetAllele_state(void)
{
    if (IsSetVariant_prop()) {
        SetVariant_prop().ResetAllele_state();
    }
    Tparent::ResetAllele_state();
}


CVariation_ref::TAllele_state CVariation_ref::GetAllele_state(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().GetAllele_state();
    }
    return Tparent::GetAllele_state();
}


void CVariation_ref::SetAllele_state(TAllele_state value)
{
    if (Tparent::IsSetAllele_state()) {
        Tparent::ResetAllele_state();
    }
    SetVariant_prop().SetAllele_state(value);
}


CVariation_ref::TAllele_state& CVariation_ref::SetAllele_state(void)
{
    if (Tparent::IsSetAllele_state()) {
        if (SetVariant_prop().IsSetAllele_state()) {
            ERR_POST(Error <<
                     "Dropping deprecated conflicting data: "
                     "Variation-ref.allele-state: "
                     "Variation-ref.variant-prop.allele-state set");
        }
        else {
            SetVariant_prop().SetAllele_state(Tparent::GetAllele_state());
        }

        Tparent::ResetAllele_state();
    }
    return SetVariant_prop().SetAllele_state();
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetAllele_frequency(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().IsSetAllele_frequency();
    }
    return Tparent::IsSetAllele_frequency();
}


bool CVariation_ref::CanGetAllele_frequency(void) const
{
    if (CanGetVariant_prop()) {
        return GetVariant_prop().CanGetAllele_frequency();
    }
    return Tparent::CanGetAllele_frequency();
}


void CVariation_ref::ResetAllele_frequency(void)
{
    if (IsSetVariant_prop()) {
        SetVariant_prop().ResetAllele_frequency();
    }
    Tparent::ResetAllele_frequency();
}


CVariation_ref::TAllele_frequency CVariation_ref::GetAllele_frequency(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().GetAllele_frequency();
    }
    return Tparent::GetAllele_frequency();
}


void CVariation_ref::SetAllele_frequency(TAllele_frequency value)
{
    if (Tparent::IsSetAllele_frequency()) {
        Tparent::ResetAllele_frequency();
    }
    SetVariant_prop().SetAllele_frequency(value);
}


CVariation_ref::TAllele_frequency& CVariation_ref::SetAllele_frequency(void)
{
    if (Tparent::IsSetAllele_frequency()) {
        if (SetVariant_prop().IsSetAllele_frequency()) {
            ERR_POST(Error <<
                     "Dropping deprecated conflicting data: "
                     "Variation-ref.allele-frequency: "
                     "Variation-ref.variant-prop.allele-frequency set");
        }
        else {
            SetVariant_prop().SetAllele_frequency(Tparent::GetAllele_frequency());
        }

        Tparent::ResetAllele_frequency();
    }
    return SetVariant_prop().SetAllele_frequency();
}

//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetIs_ancestral_allele(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().IsSetIs_ancestral_allele();
    }
    return Tparent::IsSetIs_ancestral_allele();
}


bool CVariation_ref::CanGetIs_ancestral_allele(void) const
{
    if (CanGetVariant_prop()) {
        return GetVariant_prop().CanGetIs_ancestral_allele();
    }
    return Tparent::CanGetIs_ancestral_allele();
}


void CVariation_ref::ResetIs_ancestral_allele(void)
{
    if (IsSetVariant_prop()) {
        SetVariant_prop().ResetIs_ancestral_allele();
    }
    Tparent::ResetIs_ancestral_allele();
}


CVariation_ref::TIs_ancestral_allele CVariation_ref::GetIs_ancestral_allele(void) const
{
    if (IsSetVariant_prop()) {
        return GetVariant_prop().GetIs_ancestral_allele();
    }
    return Tparent::GetIs_ancestral_allele();
}


void CVariation_ref::SetIs_ancestral_allele(TIs_ancestral_allele value)
{
    if (Tparent::IsSetIs_ancestral_allele()) {
        Tparent::ResetIs_ancestral_allele();
    }
    SetVariant_prop().SetIs_ancestral_allele(value);
}


CVariation_ref::TIs_ancestral_allele& CVariation_ref::SetIs_ancestral_allele(void)
{
    if (Tparent::IsSetIs_ancestral_allele()) {
        if (SetVariant_prop().IsSetOther_validation()) {
            ERR_POST(Error <<
                     "Dropping deprecated conflicting data: "
                     "Variation-ref.is-ancestral-allele: "
                     "Variation-ref.variant-prop.is-ancestral-allele set");
        }
        else {
            SetVariant_prop().SetIs_ancestral_allele(Tparent::GetIs_ancestral_allele());
        }
        Tparent::ResetIs_ancestral_allele();
    }
    return SetVariant_prop().SetIs_ancestral_allele();
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetPub(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::IsSetPub(): "
               "unsupported deprecated API");
}


bool CVariation_ref::CanGetPub(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::CanGetPub(): "
               "unsupported deprecated API");
}


void CVariation_ref::ResetPub(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::ResetPub(): "
               "unsupported deprecated API");
}


const CVariation_ref::TPub& CVariation_ref::GetPub(void) const
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::GetPub(): "
               "unsupported deprecated API");
}


void CVariation_ref::SetPub(TPub& /*value*/)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::SetPub(): "
               "unsupported deprecated API");
}


CVariation_ref::TPub& CVariation_ref::SetPub(void)
{
    NCBI_THROW(CException, eUnknown,
               "CVariation_ref::SetPub(): "
               "unsupported deprecated API");
}


void CVariation_ref::SetSNV(const CSeq_data& nucleotide,
                            const CRef<CDelta_item> offset) 
{
    auto& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_snv);
    inst.SetDelta().clear();

    if (offset.NotNull()) {
        inst.SetDelta().push_back(offset);
    }

    auto delta_item = Ref(new CDelta_item());
    auto& seq_literal = delta_item->SetSeq().SetLiteral();
    seq_literal.SetSeq_data().Assign(nucleotide);
    seq_literal.SetLength(1);
    inst.SetDelta().push_back(delta_item);
}


void CVariation_ref::SetMNP(const CSeq_data& nucleotide,
                            const TSeqPos length,
                            const CRef<CDelta_item> offset)
{
    auto& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_mnp);
    inst.SetDelta().clear();

    if (offset.NotNull()) {
        inst.SetDelta().push_back(offset);
     }
            
     auto delta_item = Ref(new CDelta_item());
     auto& seq_literal = delta_item->SetSeq().SetLiteral();
     seq_literal.SetSeq_data().Assign(nucleotide);
     seq_literal.SetLength(length);
     inst.SetDelta().push_back(delta_item);
}


void CVariation_ref::SetMissense(const CSeq_data& amino_acid) 
{
    auto& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_prot_missense);
    inst.SetDelta().clear();
    auto delta_item = Ref(new CDelta_item());
    delta_item->SetSeq().SetLiteral().SetSeq_data().Assign(amino_acid);
    delta_item->SetSeq().SetLiteral().SetLength() = 1;
    inst.SetDelta().push_back(delta_item);
}



void CVariation_ref::SetDuplication(const CRef<CDelta_item> start_offset,
                                    const CRef<CDelta_item> stop_offset) 
{
    auto& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_ins);
    inst.SetDelta().clear();
    if (start_offset.NotNull()) {
        inst.SetDelta().push_back(start_offset);
    }
    
    auto delta_item = Ref(new CDelta_item());
    delta_item->SetDuplication();
    inst.SetDelta().push_back(delta_item);
    if (stop_offset.NotNull()) {
        inst.SetDelta().push_back(stop_offset);
    }
}



void CVariation_ref::SetIdentity(CRef<CSeq_literal> seq_literal,
                                 const CRef<CDelta_item> start_offset,
                                 const CRef<CDelta_item> stop_offset)  
{
    auto& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_identity);
    if (seq_literal->IsSetSeq_data()) {
        inst.SetObservation(CVariation_inst::eObservation_asserted);
    }
    inst.SetDelta().clear();
    
    if (start_offset.NotNull()) {
        inst.SetDelta().push_back(start_offset);
    }
    
    auto delta_item = Ref(new CDelta_item());
    delta_item->SetSeq().SetLiteral(*seq_literal);
    inst.SetDelta().push_back(delta_item);
    
    if (stop_offset.NotNull()) {
        inst.SetDelta().push_back(stop_offset);
    }
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetLocation(void) const
{
    return Tparent::IsSetLocation();
}


bool CVariation_ref::CanGetLocation(void) const
{
    return Tparent::CanGetLocation();
}


void CVariation_ref::ResetLocation(void)
{
    Tparent::ResetLocation();
}


const CVariation_ref::TLocation& CVariation_ref::GetLocation(void) const
{
    return Tparent::GetLocation();
}


void CVariation_ref::SetLocation(TLocation& value)
{
    Tparent::SetLocation(value);
}


CVariation_ref::TLocation& CVariation_ref::SetLocation(void)
{
    return Tparent::SetLocation();
}


//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetExt_locs(void) const
{
    return Tparent::IsSetExt_locs();
}


bool CVariation_ref::CanGetExt_locs(void) const
{
    return Tparent::CanGetExt_locs();
}


void CVariation_ref::ResetExt_locs(void)
{
    Tparent::ResetExt_locs();
}


const CVariation_ref::TExt_locs& CVariation_ref::GetExt_locs(void) const
{
    return Tparent::GetExt_locs();
}


CVariation_ref::TExt_locs& CVariation_ref::SetExt_locs(void)
{
    return Tparent::SetExt_locs();
}

//////////////////////////////////////////////////////////////////////////////

bool CVariation_ref::IsSetExt(void) const
{
    return Tparent::IsSetExt();
}


bool CVariation_ref::CanGetExt(void) const
{
    return Tparent::CanGetExt();
}


void CVariation_ref::ResetExt(void)
{
    Tparent::ResetExt();
}


const CVariation_ref::TExt& CVariation_ref::GetExt(void) const
{
    return Tparent::GetExt();
}


void CVariation_ref::SetExt(TExt& value)
{
    Tparent::SetExt(value);
}


CVariation_ref::TExt& CVariation_ref::SetExt(void)
{
    return Tparent::SetExt();
}

/////////////////////////////////////////////////////////////////////////////


static void s_SetReplaces(CVariation_ref& ref,
                          const vector<string>& replaces,
                          CVariation_ref::ESeqType seq_type,
                          CVariation_inst::EType var_type)
{
    list< CRef<CDelta_item> > items;
    bool has_del = false;

    ITERATE (vector<string>, it, replaces) {
        string rep(*it);
        NStr::ToUpper(rep);
        NStr::TruncateSpacesInPlace(rep);

        if (rep.empty()  ||  rep == "-") {
            has_del = true;
        } else {
            CRef<CDelta_item> item(new CDelta_item);
            item->SetSeq().SetLiteral().SetLength(TSeqPos(rep.size()));
            if (seq_type == CVariation_ref::eSeqType_na) {
                item->SetSeq().SetLiteral().SetSeq_data().SetIupacna().Set(rep);
            } else {
                item->SetSeq().SetLiteral().SetSeq_data().SetIupacaa().Set(rep);
            }
            items.push_back(item);
        }
    }

    if (has_del  &&  items.size()) {
        ///
        /// both deletion and replaces
        /// therefore, we are a complex set
        ///
        CRef<CVariation_ref> sub;

        ref.SetData().SetSet().SetType
            (CVariation_ref::TData::TSet::eData_set_type_compound);
        
        /// deletion first
        sub.Reset(new CVariation_ref);
        sub->SetData().SetInstance().SetType(CVariation_inst::eType_del);
        sub->SetData().SetInstance().SetDelta().clear();
        ref.SetData().SetSet().SetVariations().push_back(sub);

        /// then the replaces
        sub.Reset(new CVariation_ref);
        sub->SetData().SetInstance().SetType(var_type);
        sub->SetData().SetInstance().SetDelta()
            .insert(sub->SetData().SetInstance().SetDelta().end(),
                    items.begin(), items.end());
        ref.SetData().SetSet().SetVariations().push_back(sub);
    }
    else if (has_del) {
        ref.SetData().SetInstance().SetDelta().clear();
    }
    else if (items.size()) {
        ref.SetData().SetInstance().SetType(var_type);
        ref.SetData().SetInstance().SetDelta()
            .insert(ref.SetData().SetInstance().SetDelta().end(),
                    items.begin(), items.end());
    }

    /**
    ITERATE (vector<string>, it, replaces) {
        string rep(*it);
        NStr::ToUpper(rep);

        CRef<CVariation_ref> ref(new CVariation_ref);
        CVariation_inst& inst = ref->SetData().SetInstance();
        inst.SetType(CVariation_inst::eType_snp);
        inst.SetDelta().clear();

        CRef<CDelta_item> item(new CDelta_item);
        item->SetSeq().SetLiteral().SetLength(rep.size());
        if (seq_type == eSeqType_na) {
            item->SetSeq().SetLiteral().SetSeq_data().SetIupacna().Set(rep);
        } else {
            item->SetSeq().SetLiteral().SetSeq_data().SetIupacaa().Set(rep);
        }
        inst.SetDelta().push_back(item);

        SetData().SetSet().SetVariations().push_back(ref);
    }
    SetData().SetSet().SetType(CVariation_ref::TData::TSet::eData_set_type_population);
    **/
}


void CVariation_ref::SetSNV(const vector<string>& replaces,
                            ESeqType seq_type)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    s_SetReplaces(*this, replaces, seq_type,
                  CVariation_inst::eType_snv);
}


bool CVariation_ref::IsSNV() const
{
    if (GetData().IsInstance()  &&
        GetData().GetInstance().IsSetType()  &&
        GetData().GetInstance().GetType() == CVariation_inst::eType_snv) {
        return true;
    }
    if (GetData().IsSet()) {
        ITERATE (TData::TSet::TVariations, it, GetData().GetSet().GetVariations()) {
            const CVariation_ref& ref = **it;
            if (ref.GetData().IsInstance()  &&
                ref.GetData().GetInstance().IsSetType()  &&
                ref.GetData().GetInstance().GetType() == CVariation_inst::eType_snv) {
                return true;
            }
        }
    }

    return false;
}


void CVariation_ref::SetMNP(const vector<string>& replaces,
                            ESeqType seq_type)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    s_SetReplaces(*this, replaces, seq_type,
                  CVariation_inst::eType_mnp);
}



bool CVariation_ref::IsMNP() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_mnp;
}



void CVariation_ref::SetDeletion()
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    inst.SetType(CVariation_inst::eType_del);
}

bool CVariation_ref::IsDeletion() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_del;
}



void CVariation_ref::SetInsertion(const string& sequence, ESeqType seq_type)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    inst.SetDelta().push_back(item);

    vector<string> replaces;
    replaces.push_back(sequence);
    s_SetReplaces(*this, replaces, seq_type,
                  CVariation_inst::eType_ins);
}

bool CVariation_ref::IsInsertion() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_ins;
}



void CVariation_ref::SetInsertion()
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_ins);

    CRef<CDelta_item> item(new CDelta_item);
    item->SetAction(CDelta_item::eAction_ins_before);
    inst.SetDelta().clear();
    inst.SetDelta().push_back(item);
}


void CVariation_ref::SetDeletionInsertion(const string& sequence,
                                          ESeqType seq_type)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    CRef<CDelta_item> item;

    item.Reset(new CDelta_item);
    item->SetAction(CDelta_item::eAction_del_at);
    inst.SetDelta().push_back(item);

    vector<string> replaces;
    replaces.push_back(sequence);
    s_SetReplaces(*this, replaces, seq_type,
                  CVariation_inst::eType_delins);
}

bool CVariation_ref::IsDeletionInsertion() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_delins;
}



void CVariation_ref::SetMicrosatellite(const string& nucleotide_seq,
                                       TSeqPos min_repeats,
                                       TSeqPos max_repeats)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    vector<string> replaces;
    replaces.push_back(nucleotide_seq);
    s_SetReplaces(*this, replaces, eSeqType_na,
                  CVariation_inst::eType_microsatellite);

    inst.SetDelta().front()->SetMultiplier(min_repeats);
    inst.SetDelta().front()
        ->SetMultiplier_fuzz().SetRange().SetMin(min_repeats);
    inst.SetDelta().front()
        ->SetMultiplier_fuzz().SetRange().SetMax(max_repeats);
}

bool CVariation_ref::IsMicrosatellite() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_microsatellite;
}



void CVariation_ref::SetMicrosatellite(const string& nucleotide_seq,
                                       const vector<TSeqPos>& observed_repeats)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetDelta().clear();

    vector<string> replaces;
    replaces.push_back(nucleotide_seq);
    s_SetReplaces(*this, replaces, eSeqType_na,
                  CVariation_inst::eType_microsatellite);

    inst.SetDelta().front()->SetMultiplier(observed_repeats.front());
    if (observed_repeats.size() > 1) {
        std::copy(observed_repeats.begin(),
                  observed_repeats.end(),
                  back_inserter(inst.SetDelta().front()
                                ->SetMultiplier_fuzz().SetAlt()));
    }
}


void CVariation_ref::SetCNV()
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_cnv);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    item->SetMultiplier_fuzz().SetLim(CInt_fuzz::eLim_unk);

    inst.SetDelta().push_back(item);
}

bool CVariation_ref::IsCNV() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_cnv;
}



void CVariation_ref::SetGain()
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_cnv);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    item->SetMultiplier_fuzz().SetLim(CInt_fuzz::eLim_gt);

    inst.SetDelta().push_back(item);
}

bool CVariation_ref::IsGain() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_cnv  &&
           GetData().GetInstance().IsSetDelta()  &&
           GetData().GetInstance().GetDelta().size()  &&
           GetData().GetInstance().GetDelta().front()->IsSetMultiplier_fuzz()  &&
           GetData().GetInstance().GetDelta().front()->GetMultiplier_fuzz().IsLim()  &&
           GetData().GetInstance().GetDelta().front()->GetMultiplier_fuzz().GetLim() == CInt_fuzz::eLim_gt;

}



void CVariation_ref::SetLoss()
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_cnv);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    item->SetMultiplier_fuzz().SetLim(CInt_fuzz::eLim_lt);

    inst.SetDelta().push_back(item);
}

bool CVariation_ref::IsLoss() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_cnv  &&
           GetData().GetInstance().IsSetDelta()  &&
           GetData().GetInstance().GetDelta().size()  &&
           GetData().GetInstance().GetDelta().front()->IsSetMultiplier_fuzz()  &&
           GetData().GetInstance().GetDelta().front()->GetMultiplier_fuzz().IsLim()  &&
           GetData().GetInstance().GetDelta().front()->GetMultiplier_fuzz().GetLim() == CInt_fuzz::eLim_lt;

}


void CVariation_ref::SetCNV(TSeqPos min_copy, TSeqPos max_copy)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_cnv);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    item->SetMultiplier_fuzz().SetRange().SetMin(min_copy);
    item->SetMultiplier_fuzz().SetRange().SetMax(max_copy);

    inst.SetDelta().push_back(item);
}


void CVariation_ref::SetCNV(const vector<TSeqPos>& observed_copies)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_cnv);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetThis();
    std::copy(observed_copies.begin(), observed_copies.end(),
              back_inserter(item->SetMultiplier_fuzz().SetAlt()));

    inst.SetDelta().push_back(item);
}

void CVariation_ref::SetInversion(const CSeq_loc& other_loc)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_inverted_copy);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetLoc().Assign(other_loc);
    inst.SetDelta().push_back(item);
}

bool CVariation_ref::IsInversion() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_inverted_copy;
}



void CVariation_ref::SetEversion(const CSeq_loc& other_loc)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_everted_copy);
    inst.SetDelta().clear();

    CRef<CDelta_item> item(new CDelta_item);
    item->SetSeq().SetLoc().Assign(other_loc);
    inst.SetDelta().push_back(item);
}

bool CVariation_ref::IsEversion() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_everted_copy;
}



/// The feature represents an eversion at the specified location
/// The provided location can be anywhere; a special case exists when the
/// provided location is on a different chromosome, in which case the
/// feature is considered a transchromosomal rearrangement
void CVariation_ref::SetTranslocation(const CSeq_loc& other_loc)
{
    CVariation_inst& inst = SetData().SetInstance();
    inst.SetType(CVariation_inst::eType_translocation);
    inst.SetDelta().clear();

    CRef<CDelta_item> item;
    item.Reset(new CDelta_item);
    item->SetAction(CDelta_item::eAction_del_at);
    inst.SetDelta().push_back(item);

    item.Reset(new CDelta_item);
    item->SetSeq().SetLoc().Assign(other_loc);
    inst.SetDelta().push_back(item);

}

bool CVariation_ref::IsTranslocation() const
{
    return GetData().IsInstance()  &&
           GetData().GetInstance().IsSetType()  &&
           GetData().GetInstance().GetType() == CVariation_inst::eType_translocation;
}



/// Establish a uniparental disomy mark-up
void CVariation_ref::SetUniparentalDisomy()
{
    SetData().SetUniparental_disomy();
}

bool CVariation_ref::IsUniparentalDisomy() const
{
    return GetData().IsUniparental_disomy();
}



/// Establish a complex undescribed variant
void CVariation_ref::SetComplex()
{
    SetData().SetComplex();
}

bool CVariation_ref::IsComplex() const
{
    return GetData().IsComplex();
}


void CVariation_ref::SetUnknown()
{
    SetData().SetUnknown();
}

bool CVariation_ref::IsUnknown() const
{
    return GetData().IsUnknown();
}


void CVariation_ref::SetOther()
{
    SetData().SetSet().SetType
        (CVariation_ref::TData::TSet::eData_set_type_other);
    SetData().SetSet().SetVariations();

}

bool CVariation_ref::IsOther() const
{
    return GetData().IsSet()  &&
        GetData().GetSet().GetType() ==
        CVariation_ref::TData::TSet::eData_set_type_other;
}



///
/// perform necessary validation
///
void CVariation_ref::Validate()
{
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1736, CRC32: 48a35aa2 */