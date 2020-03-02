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
* Author:  Sergiy Gotvyanskyy, NCBI
*
* File Description:
*   Reader for feature tables
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>

#include <objects/general/Object_id.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/seqset/Bioseq_set.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_descr.hpp>

#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>

#include <objmgr/util/sequence.hpp>
#include <objmgr/object_manager.hpp>

#include <objtools/readers/readfeat.hpp>
#include <algo/sequence/orf.hpp>
#include <algo/align/prosplign/prosplign.hpp>

#include <objects/seqset/seqset_macros.hpp>
#include <objects/seq/seq_macros.hpp>

#include <algo/align/splign/compart_matching.hpp>
#include <algo/align/util/compartment_finder.hpp>
#include <objtools/readers/fasta.hpp>

#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqfeat/Genetic_code.hpp>

#include <objmgr/seq_annot_ci.hpp>

#include <objmgr/annot_selector.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objmgr/annot_ci.hpp>
#include <objtools/edit/gaps_edit.hpp>
#include <objtools/edit/cds_fix.hpp>
#include <objtools/edit/feattable_edit.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objmgr/util/feature.hpp>

#include "feature_table_reader.hpp"
#include <objtools/cleanup/cleanup.hpp>

#include "table2asn_context.hpp"
#include "visitors.hpp"
#include "utils.hpp"

#include <common/test_assert.h>  /* This header must go last */
#include <unordered_set>

BEGIN_NCBI_SCOPE

USING_SCOPE(objects);

namespace
{

    static string kAssemblyGap_feature = "assembly_gap";
    static string kGapType_qual = "gap_type";
    static string kLinkageEvidence_qual = "linkage_evidence";


    void MoveSomeDescr(CSeq_entry& dest, CBioseq& src)
    {
        CSeq_descr::Tdata::iterator it = src.SetDescr().Set().begin();

        while(it != src.SetDescr().Set().end())
        {
            switch ((**it).Which())
            {
            case CSeqdesc::e_User:
                if (CTable2AsnContext::IsDBLink(**it))
                {
                    dest.SetDescr().Set().push_back(*it);
                    src.SetDescr().Set().erase(it++);
                }
                else
                    it++;
                break;
            case CSeqdesc::e_Pub:
            case CSeqdesc::e_Source:
            case CSeqdesc::e_Create_date:
            case CSeqdesc::e_Update_date:
                {
                    dest.SetDescr().Set().push_back(*it);
                    src.SetDescr().Set().erase(it++);
                }
                break;
            default:
                it++;
            }
        }
    }

    const char mapids[] = {
        CSeqFeatData::e_Cdregion,
        CSeqFeatData::e_Rna,
        CSeqFeatData::e_Gene,
        CSeqFeatData::e_Org,
        CSeqFeatData::e_Prot,
        CSeqFeatData::e_Pub,              ///< publication applies to this seq
        CSeqFeatData::e_Seq,              ///< to annotate origin from another seq
        CSeqFeatData::e_Imp,
        CSeqFeatData::e_Region,           ///< named region (globin locus)
        CSeqFeatData::e_Comment,          ///< just a comment
        CSeqFeatData::e_Bond,
        CSeqFeatData::e_Site,
        CSeqFeatData::e_Rsite,            ///< restriction site  (for maps really)
        CSeqFeatData::e_User,             ///< user defined structure
        CSeqFeatData::e_Txinit,           ///< transcription initiation
        CSeqFeatData::e_Num,              ///< a numbering system
        CSeqFeatData::e_Psec_str,
        CSeqFeatData::e_Non_std_residue,  ///< non-standard residue here in seq
        CSeqFeatData::e_Het,              ///< cofactor, prosthetic grp, etc, bound to seq
        CSeqFeatData::e_Biosrc,
        CSeqFeatData::e_Clone,
        CSeqFeatData::e_Variation,
        CSeqFeatData::e_not_set      ///< No variant selected
    };

    struct SSeqAnnotCompare
    {
        static inline
        size_t mapwhich(CSeqFeatData::E_Choice c)
        {
            const char* m = mapids;
            if (c == CSeqFeatData::e_Gene)
                c = CSeqFeatData::e_Rna;

            return strchr(m, c)-m;
        }

        inline
        bool operator()(const CSeq_feat* left, const CSeq_feat* right) const
        {
            if (left->IsSetData() != right->IsSetData())
               return left < right;
            return mapwhich(left->GetData().Which()) < mapwhich(right->GetData().Which());
        }
    };

    void FindMaximumId(const CSeq_entry::TAnnot& annots, int& id)
    {
        ITERATE(CSeq_entry::TAnnot, annot_it, annots)
        {
            if (!(**annot_it).IsFtable()) continue;
            const CSeq_annot::TData::TFtable& ftable = (**annot_it).GetData().GetFtable();
            ITERATE(CSeq_annot::TData::TFtable, feature_it, ftable)
            {
                const CSeq_feat& feature = **feature_it;
                if (feature.IsSetId() && feature.GetId().IsLocal() && feature.GetId().GetLocal().IsId())
                {
                    int l = feature.GetId().GetLocal().GetId();
                    if (l >= id)
                        id = l + 1;
                }
            }
        }
    }

    void FindMaximumId(const CSeq_entry& entry, int& id)
    {
        if (entry.IsSetAnnot())
        {
            FindMaximumId(entry.GetAnnot(), id);
        }
        if (entry.IsSeq())
        {
        }
        else
        if (entry.IsSet())
        {
            ITERATE(CBioseq_set::TSeq_set, set_it, entry.GetSet().GetSeq_set())
            {
                FindMaximumId(**set_it, id);
            }
        }
    }

    void PostProcessFeatureTable(CSeq_entry& entry, CSeq_annot::TData::TFtable& ftable)
    {
        ftable.sort(SSeqAnnotCompare());
    }

    bool GetProteinName(string& protein_name, const CSeq_feat& cds)
    {
        if (cds.IsSetData())
        {
            if (cds.GetData().IsProt() &&
                cds.GetData().GetProt().IsSetName())
            {
                cds.GetData().GetProt().GetLabel(&protein_name);
                return true;
            }
        }

        if (cds.IsSetXref())
        {
            ITERATE(CSeq_feat_Base::TXref, xref_it, cds.GetXref())
            {
                if ((**xref_it).IsSetData())
                {
                    if ((**xref_it).GetData().IsProt() &&
                        (**xref_it).GetData().GetProt().IsSetName())
                    {
                        protein_name = (**xref_it).GetData().GetProt().GetName().front();
                        return true;
                    }
                }
            }
        }

        if ( (protein_name = cds.GetNamedQual("product")) != kEmptyStr)
        {
            return true;
        }
        return false;
    }

    CRef<CSeq_id> GetNewProteinId(CScope& scope, const string& id_base)
    {
        int offset = 1;
        string id_label;
        CRef<CSeq_id> id(new CSeq_id());
        CBioseq_Handle b_found;
        do
        {
            id_label = id_base + "_" + NStr::NumericToString(offset);
            id->SetLocal().SetStr(id_label);
            b_found = scope.GetBioseqHandle(*id);
            offset++;
        } while (b_found);
        return id;
    }

    CRef<CSeq_id> GetNewProteinId(CSeq_entry_Handle seh, CBioseq_Handle bsh)
    {
        string id_base;
        CSeq_id_Handle hid;

        ITERATE(CBioseq_Handle::TId, it, bsh.GetId()) {
            if (!hid || !it->IsBetter(hid)) {
                hid = *it;
            }
        }

        if (hid) {
            hid.GetSeqId()->GetLabel(&id_base, CSeq_id::eContent);
        }

        return GetNewProteinId(seh.GetScope(), id_base);
    }

    string NewProteinName(const CSeq_feat& feature, bool make_hypotethic)
    {
        string protein_name;
        GetProteinName(protein_name, feature);


        if (protein_name.empty() && make_hypotethic)
        {
            protein_name = "hypothetical protein";
        }

        return protein_name;
    }

    CConstRef<CSeq_entry> LocateProtein(CConstRef<CSeq_entry> proteins, const CSeq_feat& feature)
    {
        if (proteins.NotEmpty() && feature.IsSetProduct())
        {
            const CSeq_id* id = feature.GetProduct().GetId();
            ITERATE(CSeq_entry::TSet::TSeq_set, it, proteins->GetSet().GetSeq_set())
            {
                ITERATE(CBioseq::TId, seq_it, (**it).GetSeq().GetId())
                {
                    if ((**seq_it).Compare(*id) == CSeq_id::e_YES)
                    {
                        return *it;
                    }
                }
            }
        }
        return CConstRef<CSeq_entry>();
    }


    //LCOV_EXCL_START
    CRef<CSeq_annot> FindORF(const CBioseq& bioseq)
    {
        if (bioseq.IsNa())
        {
            COrf::TLocVec orfs;
            CSeqVector  seq_vec(bioseq);
            COrf::FindOrfs(seq_vec, orfs);
            if (orfs.size()>0)
            {
                CRef<CSeq_id> seqid(new CSeq_id);
                seqid->Assign(*bioseq.GetId().begin()->GetPointerOrNull());
                COrf::TLocVec best;
                best.push_back(orfs.front());
                ITERATE(COrf::TLocVec, it, orfs)
                {
                    if ((**it).GetTotalRange().GetLength() >
                        best.front()->GetTotalRange().GetLength() )
                        best.front() = *it;
                }

                CRef<CSeq_annot> annot = COrf::MakeCDSAnnot(best, 1, seqid);
                return annot;
            }
        }
        return CRef<CSeq_annot>();
    }
    //LCOV_EXCL_STOP

    bool BioseqHasId(const CBioseq& seq, const CSeq_id* id)
    {
        if (id && seq.IsSetId())
        {
            ITERATE(CBioseq::TId, it, seq.GetId())
            {
                if (id->Compare(**it) == CSeq_id::e_YES)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void MergeSeqIds(CBioseq& bioseq, CBioseq::TId& seq_ids)
    {
        ITERATE(CBioseq::TId, it, seq_ids)
        {
            if (!BioseqHasId(bioseq, *it))
            {
                bioseq.SetId().push_back(*it);
            }
        }
    }

    CConstRef<CSeq_id> GetAccessionId(const CBioseq::TId& ids)
    {
        CConstRef<CSeq_id> best;
        ITERATE(CBioseq::TId, it, ids)
        {
            if ((**it).IsGenbank() ||
                 best.Empty())
              best = *it;
        }
        return best;
    }


    CSeq_feat& CreateOrSetFTable(CBioseq& bioseq)
    {
        CSeq_annot::C_Data::TFtable* ftable = 0;                    
        if (bioseq.IsSetAnnot())
        {
            NON_CONST_ITERATE(CBioseq::TAnnot, it, bioseq.SetAnnot())
            {
                if ((**it).IsFtable())
                {
                    ftable = &(**it).SetData().SetFtable();                    
                    break;
                }
            }
        }
        if (ftable == 0)
        {
            CRef<CSeq_annot> annot(new CSeq_annot);
            ftable = &annot->SetData().SetFtable();
            bioseq.SetAnnot().push_back(annot);
        }

        if (ftable->empty())
        {
            CRef<CSeq_feat> feat;
            feat.Reset(new CSeq_feat);
            ftable->push_back(feat);
            return *feat;
        }
        else
        {
            return *ftable->front();
        }
    }

    int GetGenomicCodeOfBioseq(const CBioseq& bioseq)
    {
        CConstRef<CSeqdesc> closest_biosource = bioseq.GetClosestDescriptor(CSeqdesc::e_Source);
        if (closest_biosource.Empty())
            return 0;

        const CBioSource & bsrc = closest_biosource->GetSource();
        return bsrc.GetGenCode();
    }

}

CFeatureTableReader::CFeatureTableReader(CTable2AsnContext& context) : m_local_id_counter(0), m_context(context)
{
}

CFeatureTableReader::~CFeatureTableReader()
{
}

void CFeatureTableReader::_ClearTrees()
{
    m_locus_to_gene.clear();
    m_protein_to_mrna.clear();
    m_transcript_to_mrna.clear();
    m_Feat_Tree.ReleaseOrNull();
}

CRef<feature::CFeatTree> CFeatureTableReader::_GetFeatTree()
{
    if (m_Feat_Tree.Empty())
    {
        m_Feat_Tree.Reset(new feature::CFeatTree());
        m_Feat_Tree->AddFeatures(CFeat_CI(m_scope->GetBioseqHandle(*m_bioseq)));
    }

    return m_Feat_Tree;
}


void CFeatureTableReader::_AddFeatures()
{
    for (auto annot : m_bioseq->GetAnnot())
    {
        if (!annot->IsFtable())
            continue;

        for (auto feat : annot->GetData().GetFtable())
        {
            if (!feat->CanGetData())
                continue;

            bool ismrna = feat->GetData().IsRna() && feat->GetData().GetRna().IsSetType() &&
                feat->GetData().GetRna().GetType() == CRNA_ref::eType_mRNA;
            bool isgene = feat->GetData().IsGene();

            if (feat->IsSetQual())
            for (auto qual : feat->GetQual())
            {
                if (!qual->CanGetQual() || !qual->CanGetVal())
                    continue;

                const string& name = qual->GetQual();
                const string& value = qual->GetVal();

                if (name == "transcript_id")
                {
                    if (ismrna)
                    {
                        m_transcript_to_mrna.insert(TFeatMap::value_type(value, feat));
                    }
                }
                else
                if (name == "protein_id")
                {
                    if (ismrna)
                        m_protein_to_mrna.insert(TFeatMap::value_type(value, feat));
                }
                else
                if (name == "locus_tag")
                {
                    if (isgene)
                        m_locus_to_gene.insert(TFeatMap::value_type(value, feat));
                }

            }
            if (isgene)
            {
                if (feat->GetData().GetGene().IsSetLocus_tag())
                {
                    m_locus_to_gene.insert(TFeatMap::value_type(feat->GetData().GetGene().GetLocus_tag(), feat));
                }
            }
        }
    }

}

CConstRef<CSeq_feat> CFeatureTableReader::_FindFeature(const objects::CSeq_feat& feature, bool gene)
{
    if (!feature.GetData().IsCdregion() || !feature.IsSetQual())
        return CConstRef<CSeq_feat>();

    CConstRef<CSeq_feat> feat;
    const string* qual = 0; 
    TFeatMap::const_iterator it;

    if (gene)
    {
        qual = &feature.GetNamedQual("locus_tag");
        if (!qual->empty())
        {
            if ((it = m_locus_to_gene.find(*qual)) != m_locus_to_gene.end())
                feat = it->second;
        }
    }
    else {
        qual = &feature.GetNamedQual("transcript_id");
        if (!qual->empty())
        {
            if ((it = m_transcript_to_mrna.find(*qual)) != m_transcript_to_mrna.end())
                feat = it->second;
            else
            {
                qual = &feature.GetNamedQual("protein_id");
                if (!qual->empty())
                {
                    if ((it = m_protein_to_mrna.find(*qual)) != m_protein_to_mrna.end())
                        feat = it->second;
                }
            }
        }        
    }
    return feat;

#if 0
    for (auto qual : feature.GetQual())
    {
        if (!qual->CanGetQual() || !qual->CanGetVal())
            continue;

        const string& name = qual->GetQual();
        const string& value = qual->GetVal();
        if (name == "product_id" || name == "transcript_id")
        {
            TFeatMap::iterator it = m_feat_map.lower_bound(value);
            TFeatMap::iterator it_end = m_feat_map.upper_bound(value);
            for (; it->first == value && it != it_end; ++it)
            {
                if (gene)
                {
                    if (it->second->GetData().IsGene())
                        return it->second;
                } else {
                    if (it->second->GetData().IsRna() && it->second->GetData().GetRna().GetType() == CRNA_ref::eType_mRNA)
                        return it->second;                   
                }
                return it->second;
            }
        }
    }
#endif
    return CConstRef<CSeq_feat>();
}

CConstRef<CSeq_feat> CFeatureTableReader::_FindFeature(const CFeat_id& id)
{
    CConstRef<CSeq_feat> feature;
    for(auto annot: m_bioseq->GetAnnot())
    {
        if (!annot->IsFtable()) continue;

        ITERATE(CSeq_annot::TData::TFtable, feat_it, annot->GetData().GetFtable())
        {
            if ((**feat_it).IsSetIds())
            {
                ITERATE(CSeq_feat::TIds, id_it, (**feat_it).GetIds())
                {
                    if ((**id_it).Equals(id))
                    {
                        return *feat_it;
                    }
                }
            }
            if ((**feat_it).IsSetId() && (**feat_it).GetId().Equals(id))
                return *feat_it;
        }
    }
    return CConstRef<CSeq_feat>();
}


CConstRef<CSeq_feat> CFeatureTableReader::_GetLinkedFeature(const CSeq_feat& cd_feature, bool gene)
{
    CConstRef<CSeq_feat> feature;
    for (auto xref_it: cd_feature.GetXref())
    {
        if (!xref_it->IsSetId())
        {
            if (gene && xref_it->IsSetData() && xref_it->GetData().IsGene() && xref_it->GetData().GetGene().IsSetLocus_tag())
            {
                TFeatMap::const_iterator it = m_locus_to_gene.find(xref_it->GetData().GetGene().GetLocus_tag());
                if (it != m_locus_to_gene.end())
                    return it->second;
            }
            continue;
        };

        feature = _FindFeature(xref_it->GetId());
        if (!feature.Empty() && feature->IsSetData())
        {
            if (gene)
            {
                if (feature->GetData().IsGene())
                    return feature;
            }
            else
            {
                if (feature->GetData().IsRna() &&
                    feature->GetData().GetRna().GetType() == CRNA_ref::eType_mRNA)
                    return feature;
            }
        }
    }

    feature = _FindFeature(cd_feature, gene);

    if (feature.Empty())
    {

        CMappedFeat cds(m_scope->GetSeq_featHandle(cd_feature));
        CMappedFeat other;
        if (gene)
            other = feature::GetBestGeneForCds(cds, _GetFeatTree());
        else
            other = feature::GetBestMrnaForCds(cds, _GetFeatTree());

        if (other)
            feature.Reset(&other.GetOriginalFeature());

    }
    return feature;
}


static void s_AppendProtRefInfo(CProt_ref& current_ref, const CProt_ref& other_ref)
{

    auto append_nonduplicated_item = [](list<string>& current_list, 
                                        const list<string>& other_list) 
    {
        unordered_set<string> current_set;
        for (const auto& item : current_list) {
            current_set.insert(item);
        }

        for (const auto& item : other_list) {
            if (current_set.find(item) == current_set.end()) {
                current_list.push_back(item);
            }
        }
    };

    if (other_ref.IsSetName()) {
        append_nonduplicated_item(current_ref.SetName(),
                                  other_ref.GetName());
    }

    if (other_ref.IsSetDesc()) {
        current_ref.SetDesc() = other_ref.GetDesc();
    } 

    if (other_ref.IsSetEc()) {
        append_nonduplicated_item(current_ref.SetEc(),
                                  other_ref.GetEc());
    }

    if (other_ref.IsSetActivity()) {
        append_nonduplicated_item(current_ref.SetActivity(),
                                  other_ref.GetActivity());
    }

    if (other_ref.IsSetDb()) {
        for (const auto pDBtag : other_ref.GetDb()) {
            current_ref.SetDb().push_back(pDBtag);
        }
    }

    if (current_ref.GetProcessed() == CProt_ref::eProcessed_not_set) {
        const auto processed = other_ref.GetProcessed();
        if (processed != CProt_ref::eProcessed_not_set) {
            current_ref.SetProcessed(processed);
        }
    }
}

static void s_SetProtRef(const CSeq_feat& cds,
                         CConstRef<CSeq_feat> pMrna,
                         CProt_ref& prot_ref) 
{
   const CProt_ref* pProtXref = cds.GetProtXref();
   if (pProtXref) {
        s_AppendProtRefInfo(prot_ref, *pProtXref);
   } 


   if (!prot_ref.IsSetName()) {
        const string& product_name = cds.GetNamedQual("product");
        if (product_name != kEmptyStr) {
            prot_ref.SetName().push_back(product_name);
        }
   }

   if (pMrna.Empty()) { // Nothing more we can do here
        return;
   }

   if (prot_ref.IsSetName()) {
        for (auto& prot_name : prot_ref.SetName()) {
            if (NStr::CompareNocase(prot_name, "hypothetical protein")==0) {
                if (pMrna->GetData().GetRna().IsSetExt() &&
                    pMrna->GetData().GetRna().GetExt().IsName()){
                    prot_name = pMrna->GetData().GetRna().GetExt().GetName();
                    break;
                }
            }
        }
   } // prot_ref.IsSetName() 
}




CRef<CSeq_entry> CFeatureTableReader::_TranslateProtein(CSeq_entry_Handle top_entry_h, const CBioseq& bioseq, CSeq_feat& cd_feature)
{

    if (sequence::IsPseudo(cd_feature, *m_scope))
        return CRef<CSeq_entry>();


    CConstRef<CSeq_entry> replacement = LocateProtein(m_replacement_protein, cd_feature);

    CConstRef<CSeq_feat> mrna = _GetLinkedFeature(cd_feature, false);
    CConstRef<CSeq_feat> gene = _GetLinkedFeature(cd_feature, true);

    CRef<CBioseq> protein;
    bool was_extended = false;
    if (replacement.Empty())
    {
        CBioseq_Handle bsh = m_scope->GetBioseqHandle(bioseq);
        was_extended = CCleanup::ExtendToStopIfShortAndNotPartial(cd_feature, bsh);

        protein = CSeqTranslator::TranslateToProtein(cd_feature, *m_scope);

        if (protein.Empty())
            return CRef<CSeq_entry>();
    }
    else
    {
        protein.Reset(new CBioseq());
        protein->Assign(replacement->GetSeq());
    }

    CRef<CSeq_entry> protein_entry(new CSeq_entry);
    protein_entry->SetSeq(*protein);

    CAutoAddDesc molinfo_desc(protein->SetDescr(), CSeqdesc::e_Molinfo);
    molinfo_desc.Set().SetMolinfo().SetBiomol(CMolInfo::eBiomol_peptide);
    molinfo_desc.Set().SetMolinfo().SetTech(CMolInfo::eTech_concept_trans);
    feature::AdjustProteinMolInfoToMatchCDS(molinfo_desc.Set().SetMolinfo(), cd_feature);

    string org_name;
    CTable2AsnContext::GetOrgName(org_name, *top_entry_h.GetCompleteObject());

    CTempString locustag;
    if (!gene.Empty() && gene->IsSetData() && gene->GetData().IsGene() && gene->GetData().GetGene().IsSetLocus_tag())
    {
        locustag = gene->GetData().GetGene().GetLocus_tag();
    }

    string base_name;
    CRef<CSeq_id> newid;
    CTempString qual_to_remove;

    if (protein->GetId().empty())
    {
        const string* protein_ids = 0;

        qual_to_remove = "protein_id";
        protein_ids = &cd_feature.GetNamedQual(qual_to_remove);

        if (protein_ids->empty())
        {
            qual_to_remove = "orig_protein_id";
            protein_ids = &cd_feature.GetNamedQual(qual_to_remove);
        }

        if (protein_ids->empty())
        {
            if (mrna.NotEmpty())
                protein_ids = &mrna->GetNamedQual("protein_id");
        }

        if (protein_ids->empty())
        {
            protein_ids = &cd_feature.GetNamedQual("product_id");
        }

        if (!protein_ids->empty())
        {
            CBioseq::TId new_ids;
            CSeq_id::ParseIDs(new_ids, *protein_ids, CSeq_id::fParse_ValidLocal | CSeq_id::fParse_PartialOK);

            MergeSeqIds(*protein, new_ids);
            cd_feature.RemoveQualifier(qual_to_remove);
        }
    }
    else {
        cd_feature.RemoveQualifier("protein_id");
        cd_feature.RemoveQualifier("orig_protein_id");
    }

    if (protein->GetId().empty())
    {
        if (base_name.empty() && !bioseq.GetId().empty())
        {
            bioseq.GetId().front()->GetLabel(&base_name, CSeq_id::eContent);
        }

        newid = GetNewProteinId(*m_scope, base_name);
        protein->SetId().push_back(newid);
    }

    CSeq_feat& prot_feat = CreateOrSetFTable(*protein);
    CProt_ref& prot_ref = prot_feat.SetData().SetProt();

    s_SetProtRef(cd_feature, mrna, prot_ref);
    if ((!prot_ref.IsSetName() ||
        prot_ref.GetName().empty()) &&
        m_context.m_use_hypothetic_protein) {
        prot_ref.SetName().push_back("hypothetical protein");
    }

    prot_feat.SetLocation().SetInt().SetFrom(0);
    prot_feat.SetLocation().SetInt().SetTo(protein->GetInst().GetLength() - 1);
    prot_feat.SetLocation().SetInt().SetId().Assign(*GetAccessionId(protein->GetId()));
    feature::CopyFeaturePartials(prot_feat, cd_feature);

    if (!cd_feature.IsSetProduct())
        cd_feature.SetProduct().SetWhole().Assign(*GetAccessionId(protein->GetId()));

    CBioseq_Handle protein_handle = m_scope->AddBioseq(*protein);

    AssignLocalIdIfEmpty(cd_feature, m_local_id_counter);
    if (gene.NotEmpty() && mrna.NotEmpty())
        cd_feature.SetXref().clear();

    if (gene.NotEmpty())
    {
        CSeq_feat& gene_feature = (CSeq_feat&)*gene;
        AssignLocalIdIfEmpty(gene_feature, m_local_id_counter);
        cd_feature.AddSeqFeatXref(gene_feature.GetId());
        gene_feature.AddSeqFeatXref(cd_feature.GetId());
    }

    if (mrna.NotEmpty())
    {
        CSeq_feat& mrna_feature = (CSeq_feat&)*mrna;

        AssignLocalIdIfEmpty(mrna_feature, m_local_id_counter);

        if (prot_ref.IsSetName() &&
            !prot_ref.GetName().empty())
        {
            auto& ext = mrna_feature.SetData().SetRna().SetExt();
            if (ext.Which() == CRNA_ref::C_Ext::e_not_set ||
                (ext.IsName() && ext.SetName().empty()))
                ext.SetName() = prot_ref.GetName().front();
        }
        mrna_feature.AddSeqFeatXref(cd_feature.GetId());
        cd_feature.AddSeqFeatXref(mrna_feature.GetId());
    }

#if 0
    if (!protein_name.empty())
    {
        //cd_feature.ResetProduct();
        cd_feature.SetProtXref().SetName().clear();
        cd_feature.SetProtXref().SetName().push_back(protein_name);
        cd_feature.RemoveQualifier("product");
    }
#endif


    if (was_extended)
    {
        if (!mrna.Empty() && mrna->IsSetLocation() && CCleanup::LocationMayBeExtendedToMatch(mrna->GetLocation(), cd_feature.GetLocation()))
            CCleanup::ExtendStopPosition((CSeq_feat&)*mrna, &cd_feature);
        if (!gene.Empty() && gene->IsSetLocation() && CCleanup::LocationMayBeExtendedToMatch(gene->GetLocation(), cd_feature.GetLocation()))
            CCleanup::ExtendStopPosition((CSeq_feat&)*gene, &cd_feature);
    }

    return protein_entry;
}

void CFeatureTableReader::MergeCDSFeatures(CSeq_entry& entry)
{
    m_local_id_counter = 0;
    FindMaximumId(entry, ++m_local_id_counter);
    _MergeCDSFeatures_impl(entry);
}

void CFeatureTableReader::_MergeCDSFeatures_impl(CSeq_entry& entry)
{
    if (entry.IsSeq() && !entry.GetSeq().IsSetInst() )
        return;

    switch (entry.Which())
    {
    case CSeq_entry::e_Seq:
        if (_CheckIfNeedConversion(entry))
        {
            _ConvertSeqIntoSeqSet(entry, true);
            _ParseCdregions(entry);
        }
        break;
    case CSeq_entry::e_Set:
        if (entry.GetSet().IsSetClass())
        {
            switch (entry.GetSet().GetClass())
            {
                case CBioseq_set::eClass_nuc_prot:
                case CBioseq_set::eClass_gen_prod_set:
                    //ParseCdregions(entry);
                    return;
                case CBioseq_set::eClass_genbank:
                case CBioseq_set::eClass_eco_set:
                    break;
                default:
                    return;
            }
        }
        NON_CONST_ITERATE(CBioseq_set_Base::TSeq_set, it, entry.SetSet().SetSeq_set())
        {
            _MergeCDSFeatures_impl(**it);
        }
        break;
    default:
        break;
    }
}

//LCOV_EXCL_START
void CFeatureTableReader::FindOpenReadingFrame(CSeq_entry& entry) const
{
    switch(entry.Which())
    {
    case CSeq_entry::e_Seq:
        {
        CRef<CSeq_annot> annot = FindORF(entry.SetSeq());
        if (annot.NotEmpty())
        {
            entry.SetSeq().SetAnnot().push_back(annot);
        }
        }
        break;
    case CSeq_entry::e_Set:
        NON_CONST_ITERATE(CSeq_entry::TSet::TSeq_set, it, entry.SetSet().SetSeq_set())
        {
            FindOpenReadingFrame(**it);
        }
        break;
    default:
        break;
    }
}
//LCOV_EXCL_STOP

void CFeatureTableReader::_MoveCdRegions(CSeq_entry_Handle entry_h, const CBioseq& bioseq, CSeq_annot::TData::TFtable& seq_ftable, CSeq_annot::TData::TFtable& set_ftable)
{
    // sort and number ids
    CSeq_entry& entry = *(CSeq_entry*)entry_h.GetEditHandle().GetSeq_entryCore().GetPointerOrNull();
    PostProcessFeatureTable(entry, seq_ftable);
    for (CSeq_annot::TData::TFtable::iterator feat_it = seq_ftable.begin(); seq_ftable.end() != feat_it;)
    {
        CRef<CSeq_feat> feature = (*feat_it);
        if (!feature->IsSetData())
        {
            ++feat_it;
            continue;
        }

        CSeqFeatData& data = feature->SetData();
        if (data.IsCdregion())
        {
            if (!data.GetCdregion().IsSetCode())
            {
                int code = GetGenomicCodeOfBioseq(bioseq);
                if (code == 0)
                    code = 1;

                data.SetCdregion().SetCode().SetId(code);
            }
            if (!data.GetCdregion().IsSetFrame())
            {
                if ((*feat_it)->IsSetExcept_text() && NStr::Find((*feat_it)->GetExcept_text(), "annotated by transcript or proteomic data") != NPOS) {
                    data.SetCdregion().SetFrame(CCdregion::eFrame_one);
                } else {
                    data.SetCdregion().SetFrame(CSeqTranslator::FindBestFrame(*feature, *m_scope));
                }
            }
            CCleanup::ParseCodeBreaks(*feature, *m_scope);
            CRef<CSeq_entry> protein = _TranslateProtein(entry_h, bioseq, *feature);
            if (protein.NotEmpty())
            {
                entry.SetSet().SetSeq_set().push_back(protein);
                // move the cdregion into protein and step iterator to next
                set_ftable.push_back(feature);
                feat_it = seq_ftable.erase(feat_it);
                continue; // avoid iterator increment
            }
        }
        ++feat_it;
    }
}

void CFeatureTableReader::_ParseCdregions(CSeq_entry& entry)
{
    if (!entry.IsSet() ||
        entry.GetSet().GetClass() != CBioseq_set::eClass_nuc_prot)
        return;

    m_scope.Reset(new CScope(*CObjectManager::GetInstance()));
    m_scope->AddDefaults();

    CSeq_entry_Handle entry_h = m_scope->AddTopLevelSeqEntry(entry);

    // Create empty annotation holding cdregion features
    CRef<CSeq_annot> set_annot(new CSeq_annot);
    CSeq_annot::TData::TFtable& set_ftable = set_annot->SetData().SetFtable();
    entry.SetSet().SetAnnot().push_back(set_annot);

    for (auto seq: entry.SetSet().SetSeq_set())
    {
        if (!(seq->IsSeq() &&
            seq->GetSeq().IsSetInst() &&
            seq->GetSeq().IsNa() &&
            seq->GetSeq().IsSetAnnot()))
            continue;

        m_bioseq.Reset(&seq->SetSeq());

        CRef<CSeq_annot> main_ftable;

        for (CBioseq::TAnnot::iterator annot_it = seq->SetSeq().SetAnnot().begin();
            seq->SetSeq().SetAnnot().end() != annot_it;)
        {
            CRef<CSeq_annot> seq_annot(*annot_it);

            if (seq_annot->IsFtable())
            {

                if (main_ftable.IsNull())
                    main_ftable = seq_annot;
                else {
                    main_ftable->SetData().SetFtable().merge(
                        seq_annot->SetData().SetFtable());
                    annot_it = seq->SetSeq().SetAnnot().erase(annot_it++);
                    continue;
                }
            }

            ++annot_it;
        }

        _AddFeatures();

        //copy sequence feature table to edit it
        auto seq_ftable = main_ftable->SetData().SetFtable();

        _MoveCdRegions(entry_h, seq->GetSeq(), seq_ftable, set_ftable);

        _ClearTrees();

        if (seq_ftable.empty()) {
            seq->SetSeq().SetAnnot().remove(main_ftable);
        } else {
            main_ftable->SetData().SetFtable() = move(seq_ftable);
        }

        if (seq->GetSeq().GetAnnot().empty())
        {
            seq->SetSeq().ResetAnnot();
        }
    }
    if (set_ftable.empty())
    {
        entry.SetSet().ResetAnnot();
    }

    m_scope->RemoveTopLevelSeqEntry(entry_h);
    if (false)
    {
        CNcbiOfstream debug_annot("annot.sqn");
        debug_annot << MSerial_AsnText
            << MSerial_VerifyNo
            << entry;
    }
}

CRef<CSeq_entry> CFeatureTableReader::ReadProtein(ILineReader& line_reader)
{
    int flags = 0;
    flags |= CFastaReader::fAddMods
          |  CFastaReader::fNoUserObjs
          |  CFastaReader::fAssumeProt;

    unique_ptr<CFastaReader> pReader(new CFastaReader(0, flags));

    CRef<CSeq_entry> result;
    CRef<CSerialObject> pep = pReader->ReadObject(line_reader, m_context.m_logger);

    if (pep.NotEmpty())
    {
        if (pep->GetThisTypeInfo()->IsType(CSeq_entry::GetTypeInfo()))
        {
            result = (CSeq_entry*)(pep.GetPointerOrNull());
            if (result->IsSetDescr())
            {
                if (result->GetDescr().Get().size() == 0)
                {
                    if (result->IsSeq())
                        result->SetSeq().ResetDescr();
                    else
                        result->SetSet().ResetDescr();
                }
            }
            if (result->IsSeq())
            {
                // convert into seqset
                CRef<CSeq_entry> set(new CSeq_entry);
                set->SetSet().SetSeq_set().push_back(result);
                result = set;
            }
        }
    }

    return result;
}

void CFeatureTableReader::AddProteins(const CSeq_entry& possible_proteins, CSeq_entry& entry)
{

    CScope scope(*CObjectManager::GetInstance());
    CSeq_entry_Handle tse = scope.AddTopLevelSeqEntry(entry);

    CBioseq_CI nuc_it(tse, CSeq_inst::eMol_na);
    {
        CSeq_entry_Handle h_entry = nuc_it->GetParentEntry(); 

        if (possible_proteins.IsSeq())
        {
            _AddProteinToSeqEntry(&possible_proteins, h_entry);
        }
        else
        {
            ITERATE(CBioseq_set::TSeq_set, it, possible_proteins.GetSet().GetSeq_set())
            {
                _AddProteinToSeqEntry(*it,  h_entry);
            }
        }
    }
}

bool CFeatureTableReader::_CheckIfNeedConversion(const CSeq_entry& entry) const
{
    if (entry.GetParentEntry() && 
        entry.GetParentEntry()->IsSet() &&
        entry.GetParentEntry()->GetSet().IsSetClass())
    {
        switch (entry.GetParentEntry()->GetSet().GetClass())
        {
        case CBioseq_set::eClass_nuc_prot:
            return false;
        default:
            break;
        }
    }

    ITERATE(CSeq_entry::TAnnot, annot_it, entry.GetAnnot())
    {
        if ((**annot_it).IsFtable())
        {
            ITERATE(CSeq_annot::C_Data::TFtable, feat_it, (**annot_it).GetData().GetFtable())
            {
                if((**feat_it).CanGetData())
                {
                    switch ((**feat_it).GetData().Which())
                    {
                    case CSeqFeatData::e_Cdregion:
                    //case CSeqFeatData::e_Gene:
                        return true;
                    default:
                        break;
                    }
                }
            }
        }
    }

    return false;
}

void CFeatureTableReader::_ConvertSeqIntoSeqSet(CSeq_entry& entry, bool nuc_prod_set) const
{
    if (entry.IsSeq())
    {
        CRef<CSeq_entry> newentry(new CSeq_entry);
        newentry->SetSeq(entry.SetSeq());
        CBioseq& bioseq = newentry->SetSeq();
        entry.SetSet().SetSeq_set().push_back(newentry);

        MoveSomeDescr(entry, bioseq);

        CAutoAddDesc molinfo_desc(bioseq.SetDescr(), CSeqdesc::e_Molinfo);

        if (!molinfo_desc.Set().SetMolinfo().IsSetBiomol())
            molinfo_desc.Set().SetMolinfo().SetBiomol(CMolInfo::eBiomol_genomic);
        //molinfo_desc.Set().SetMolinfo().SetTech(CMolInfo::eTech_concept_trans);


        if (bioseq.IsSetInst() &&
            bioseq.IsNa() &&
            bioseq.IsSetInst() &&
            !bioseq.GetInst().IsSetMol())
        {
            bioseq.SetInst().SetMol(CSeq_inst::eMol_dna);
        }
        entry.SetSet().SetClass(nuc_prod_set ? CBioseq_set::eClass_nuc_prot : CBioseq_set::eClass_gen_prod_set);
        entry.Parentize();
    }
}

void CFeatureTableReader::ConvertNucSetToSet(CRef<CSeq_entry>& entry) const
{
    if (entry->IsSet() && entry->GetSet().GetClass() == CBioseq_set::eClass_nuc_prot)
    {
        CRef<CSeq_entry> newentry(new CSeq_entry);
        newentry->SetSet().SetClass(CBioseq_set::eClass_genbank);
        newentry->SetSet().SetSeq_set().push_back(entry);
        entry = newentry;
        newentry.Reset();
        entry->Parentize();
    }
}

namespace {

void s_ExtendIntervalToEnd (CSeq_interval& ival, CBioseq_Handle bsh)
{
    if (ival.IsSetStrand() && ival.GetStrand() == eNa_strand_minus) {
        if (ival.GetFrom() > 3) {
            ival.SetFrom(ival.GetFrom() - 3);
        } else {
            ival.SetFrom(0);
        }
    } else {
        TSeqPos len = bsh.GetBioseqLength();
        if (ival.GetTo() < len - 4) {
            ival.SetTo(ival.GetTo() + 3);
        } else {
            ival.SetTo(len - 1);
        }
    }
}

bool SetMolinfoCompleteness (CMolInfo& mi, bool partial5, bool partial3)
{
    bool changed = false;
    CMolInfo::ECompleteness new_val;
    if ( partial5  &&  partial3 ) {
        new_val = CMolInfo::eCompleteness_no_ends;
    } else if ( partial5 ) {
        new_val = CMolInfo::eCompleteness_no_left;
    } else if ( partial3 ) {
        new_val = CMolInfo::eCompleteness_no_right;
    } else {
        new_val = CMolInfo::eCompleteness_complete;
    }
    if (!mi.IsSetCompleteness() || mi.GetCompleteness() != new_val) {
        mi.SetCompleteness(new_val);
        changed = true;
    }
    return changed;
}


void SetMolinfoForProtein (CSeq_descr& protein_descr, bool partial5, bool partial3)
{
    CAutoAddDesc pdesc(protein_descr, CSeqdesc::e_Molinfo);
    pdesc.Set().SetMolinfo().SetBiomol(CMolInfo::eBiomol_peptide);
    SetMolinfoCompleteness(pdesc.Set().SetMolinfo(), partial5, partial3);   
}

CRef<CSeq_feat> AddEmptyProteinFeatureToProtein (CRef<CSeq_entry> protein, bool partial5, bool partial3)
{
    CRef<CSeq_annot> ftable(NULL);
    NON_CONST_ITERATE(CSeq_entry::TAnnot, annot_it, protein->SetSeq().SetAnnot()) {
        if ((*annot_it)->IsFtable()) {
            ftable = *annot_it;
            break;
        }
    }
    if (!ftable) {
        ftable = new CSeq_annot();
        protein->SetSeq().SetAnnot().push_back(ftable);
    }
    
    CRef<CSeq_feat> prot_feat(NULL);
    NON_CONST_ITERATE(CSeq_annot::TData::TFtable, feat_it, ftable->SetData().SetFtable()) {
        if ((*feat_it)->IsSetData() && (*feat_it)->GetData().IsProt() && !(*feat_it)->GetData().GetProt().IsSetProcessed()) {
            prot_feat = *feat_it;
            break;
        }
    }
    if (!prot_feat) {
        prot_feat = new CSeq_feat();
        prot_feat->SetData().SetProt();
        ftable->SetData().SetFtable().push_back(prot_feat);
    }
    CRef<CSeq_id> prot_id(new CSeq_id());
    prot_id->Assign(*(protein->GetSeq().GetId().front()));
    prot_feat->SetLocation().SetInt().SetId(*prot_id);
    prot_feat->SetLocation().SetInt().SetFrom(0);
    prot_feat->SetLocation().SetInt().SetTo(protein->GetSeq().GetLength() - 1);
    prot_feat->SetLocation().SetPartialStart(partial5, eExtreme_Biological);
    prot_feat->SetLocation().SetPartialStop(partial3, eExtreme_Biological);
    if (partial5 || partial3) {
        prot_feat->SetPartial(true);
    } else {
        prot_feat->ResetPartial();
    }
    return prot_feat;
}

CRef<CSeq_feat> AddProteinFeatureToProtein (CRef<CSeq_entry> nuc, CConstRef<CSeq_loc> cds_loc, CRef<CSeq_entry> protein, bool partial5, bool partial3)
{
    CSourceModParser smp(CSourceModParser::eHandleBadMod_Ignore);
    // later - fix protein title by removing attributes used?
    CConstRef<CSeqdesc> title_desc = protein->GetSeq().GetClosestDescriptor(CSeqdesc::e_Title);
    if (title_desc) {
        string& title(const_cast<string&>(title_desc->GetTitle()));
        title = smp.ParseTitle(title, CConstRef<CSeq_id>(protein->GetSeq().GetFirstId()) );
        smp.ApplyAllMods(protein->SetSeq());
        smp.ApplyAllMods(nuc->SetSeq(), "", cds_loc);
    }

    return AddEmptyProteinFeatureToProtein(protein, partial5, partial3);
}

void AddSeqEntry(CSeq_entry_Handle m_SEH, CSeq_entry* m_Add)
{
    CSeq_entry_EditHandle eh = m_SEH.GetEditHandle();
    if (!eh.IsSet() && m_Add->IsSeq() && m_Add->GetSeq().IsAa()) {
        CBioseq_set_Handle nuc_parent = eh.GetParentBioseq_set();
        if (nuc_parent && nuc_parent.IsSetClass() && nuc_parent.GetClass() == CBioseq_set::eClass_nuc_prot) {
            eh = nuc_parent.GetParentEntry().GetEditHandle();
        }
    }
    if (!eh.IsSet()) {
        eh.ConvertSeqToSet();
        if (m_Add->IsSeq() && m_Add->GetSeq().IsAa()) {
            // if adding protein sequence and converting to nuc-prot set, 
            // move all descriptors on nucleotide sequence except molinfo and title to set
            eh.SetSet().SetClass(CBioseq_set::eClass_nuc_prot);
            CConstRef<CBioseq_set> set = eh.GetSet().GetCompleteBioseq_set();
            if (set && set->IsSetSeq_set()) {
                CConstRef<CSeq_entry> nuc = set->GetSeq_set().front();
                CSeq_entry_EditHandle neh = m_SEH.GetScope().GetSeq_entryEditHandle(*nuc);
                CBioseq_set::TDescr::Tdata::const_iterator it = nuc->GetDescr().Get().begin();
                while (it != nuc->GetDescr().Get().end()) {
                    if (!(*it)->IsMolinfo() && !(*it)->IsTitle()) {
                        CRef<CSeqdesc> copy(new CSeqdesc());
                        copy->Assign(**it);
                        eh.AddSeqdesc(*copy);
                        neh.RemoveSeqdesc(**it);
                        it = nuc->GetDescr().Get().begin();
                    } else {
                        ++it;
                    }
                }
            }
        }
    }
    

    CSeq_entry_EditHandle added = eh.AttachEntry(*m_Add);
    /*int m_index = */ eh.GetSet().GetSeq_entry_Index(added);
}

void AddFeature(CSeq_entry_Handle m_seh, CSeq_feat* m_Feat)
{
    if (m_Feat->IsSetData() && m_Feat->GetData().IsCdregion() && m_Feat->IsSetProduct()) {
        CBioseq_Handle bsh = m_seh.GetScope().GetBioseqHandle(m_Feat->GetProduct());
        if (bsh) {
            CBioseq_set_Handle nuc_parent = bsh.GetParentBioseq_set();
            if (nuc_parent && nuc_parent.IsSetClass() && nuc_parent.GetClass() == CBioseq_set::eClass_nuc_prot) {
                m_seh = nuc_parent.GetParentEntry();
            }
        }
    }
    CSeq_annot_Handle ftable;

    CSeq_annot_CI annot_ci(m_seh, CSeq_annot_CI::eSearch_entry);
    for (; annot_ci; ++annot_ci) {
        if ((*annot_ci).IsFtable()) {
            ftable = *annot_ci;
            break;
        }
    }

    CSeq_entry_EditHandle eh = m_seh.GetEditHandle();
    CSeq_feat_EditHandle  m_feh;
    CSeq_annot_EditHandle m_FTableCreated;

    if (!ftable) {
        CRef<CSeq_annot> new_annot(new CSeq_annot());
        ftable = m_FTableCreated = eh.AttachAnnot(*new_annot);
    }

    CSeq_annot_EditHandle aeh(ftable);
    m_feh = aeh.AddFeat(*m_Feat);
}


}

bool CFeatureTableReader::_AddProteinToSeqEntry(const CSeq_entry* protein, CSeq_entry_Handle seh)
{
    CRef<CSeq_entry> nuc_entry((CSeq_entry*)seh.GetEditHandle().GetCompleteSeq_entry().GetPointerOrNull());

    CProSplign prosplign(CProSplignScoring(), false, true, false, false);

    // bool rval = false;
    CBioseq_Handle bsh_match;
    bool id_match = false;

    // only add protein if we can match it to a nucleotide sequence via the ID,
    // or if there is only one nucleotide sequence

    ITERATE (CBioseq::TId, id_it, protein->GetSeq().GetId()) {
        bsh_match = seh.GetScope().GetBioseqHandle(**id_it);
        if (bsh_match) {
            id_match = true;
            break;
        }
    }
    if (!bsh_match) {
        // if there is only one nucleotide sequence, we will use that one
        int nuc_count = 0;
        for (CBioseq_CI b_iter(seh, CSeq_inst::eMol_na); b_iter ; ++b_iter ) {
            bsh_match = *b_iter;
            nuc_count++;
            if (nuc_count > 1) {
                break;
            }
        }

        if (nuc_count == 0) {
            //wxMessageBox(wxT("You must import nucleotide sequences before importing protein sequences"), wxT("Error"),
            //             wxOK | wxICON_ERROR, NULL);
            return false;
        } else if (nuc_count > 1) {
            //wxMessageBox(wxT("If you have more than one nucleotide sequence, each protein sequence must use the ID of the nucleotide sequence where the coding region is found."), wxT("Error"),
            //             wxOK | wxICON_ERROR, NULL);
            return false;
        }
    }
            
    CRef<CSeq_id> seq_id(new CSeq_id());
    seq_id->Assign(*(bsh_match.GetSeqId()));
    CRef<CSeq_loc> match_loc(new CSeq_loc(*seq_id, 0, bsh_match.GetBioseqLength() - 1));

    CRef<CSeq_entry> protein_entry(new CSeq_entry());
    protein_entry->Assign(*protein);
    if (id_match) {
        CRef<CSeq_id> product_id = GetNewProteinId(seh, bsh_match);
        protein_entry->SetSeq().ResetId();
        protein_entry->SetSeq().SetId().push_back(product_id);
    }

    CSeq_entry_Handle protein_h = seh.GetScope().AddTopLevelSeqEntry(*protein_entry);

    //time_t t1 = time(NULL);
    CRef<CSeq_align> alignment = prosplign.FindAlignment(seh.GetScope(), *protein_entry->GetSeq().GetId().front(), *match_loc,
                                                     CProSplignOutputOptions(CProSplignOutputOptions::ePassThrough));
    //time_t t2 = time(NULL);
    //time_t elapsed = t2 - t1;
    CRef<CSeq_loc> cds_loc(new CSeq_loc());
    bool found_start_codon = false;
    bool found_stop_codon = false;
    if (alignment && alignment->IsSetSegs() && alignment->GetSegs().IsSpliced()) {
        CRef<CSeq_id> seq_id (new CSeq_id());
        seq_id->Assign(*match_loc->GetId());
        ITERATE (CSpliced_seg::TExons, exon_it, alignment->GetSegs().GetSpliced().GetExons()) {
            CRef<CSeq_loc> exon(new CSeq_loc(*seq_id, 
                                                      (*exon_it)->GetGenomic_start(), 
                                                      (*exon_it)->GetGenomic_end()));                
            if ((*exon_it)->IsSetGenomic_strand()) {
                exon->SetStrand((*exon_it)->GetGenomic_strand());
            }
            cds_loc->SetMix().Set().push_back(exon);
        }
        ITERATE (CSpliced_seg::TModifiers, mod_it,
                 alignment->GetSegs().GetSpliced().GetModifiers()) {
            if ((*mod_it)->IsStart_codon_found()) {
                found_start_codon = (*mod_it)->GetStart_codon_found();
            }
            if ((*mod_it)->IsStop_codon_found()) {
                found_stop_codon = (*mod_it)->GetStop_codon_found();
            }
        }
        
    }
    if (!cds_loc->IsMix()) {
        //no exons, no match
        string label = "";        
        protein->GetSeq().GetId().front()->GetLabel(&label, CSeq_id::eContent);
        string error = "Unable to find coding region location for protein sequence " + label + ".";
        m_context.m_logger->PutError(*unique_ptr<CLineError>(
            CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
            error)));
        return false;
    } else {
        if (cds_loc->GetMix().Get().size() == 1) {
            CRef<CSeq_loc> exon = cds_loc->SetMix().Set().front();
            cds_loc->Assign(*exon);
        }
    }
    if (!found_start_codon) {
        cds_loc->SetPartialStart(true, eExtreme_Biological);
    }
    if (found_stop_codon) {
        // extend to cover stop codon        
        if (cds_loc->IsMix()) {
            s_ExtendIntervalToEnd(cds_loc->SetMix().Set().back()->SetInt(), bsh_match);
        } else {
            s_ExtendIntervalToEnd(cds_loc->SetInt(), bsh_match);
        }        
    } else {
        cds_loc->SetPartialStop(true, eExtreme_Biological);
    }

    // if we add the protein sequence, we'll do it in the new nuc-prot set
    seh.GetScope().RemoveTopLevelSeqEntry(protein_h);
    bool partial5 = cds_loc->IsPartialStart(eExtreme_Biological);
    bool partial3 = cds_loc->IsPartialStop(eExtreme_Biological);
    SetMolinfoForProtein(protein_entry->SetDescr(), partial5, partial3);
    CRef<CSeq_feat> protein_feat = AddProteinFeatureToProtein(nuc_entry, cds_loc, protein_entry, partial5, partial3);

    AddSeqEntry(bsh_match.GetParentEntry(), protein_entry);
    CRef<CSeq_feat> new_cds(new CSeq_feat());
    new_cds->SetLocation(*cds_loc);
    if (partial5 || partial3) {
        new_cds->SetPartial(true);
    }
    new_cds->SetData().SetCdregion();
    CRef<CSeq_id> product_id(new CSeq_id());
    product_id->Assign(*(protein_entry->GetSeq().GetId().front()));
    new_cds->SetProduct().SetWhole(*product_id);
    AddFeature(seh, new_cds);

    string org_name;
    CTable2AsnContext::GetOrgName(org_name, *seh.GetCompleteObject());

    string protein_name = NewProteinName(*protein_feat, m_context.m_use_hypothetic_protein);
    string title = protein_name;
    if (org_name.length() > 0)
    {
        title += " [";
        title += org_name;
        title += "]";
    }

    CAutoAddDesc title_desc(protein_entry->SetDescr(), CSeqdesc::e_Title);
    title_desc.Set().SetTitle() += title;

    return true;
}

void CFeatureTableReader::RemoveEmptyFtable(objects::CBioseq& bioseq)
{
    if (bioseq.IsSetAnnot())
    {
        for (CBioseq::TAnnot::iterator annot_it = bioseq.SetAnnot().begin(); annot_it != bioseq.SetAnnot().end(); ) // no ++
        {
            if ((**annot_it).IsFtable() && (**annot_it).GetData().GetFtable().empty())
            {
                annot_it = bioseq.SetAnnot().erase(annot_it);
            }
            else
                annot_it++;
        }

        if (bioseq.GetAnnot().empty())
        {
            bioseq.ResetAnnot();
        }
    }
}

CRef<CDelta_seq> CFeatureTableReader::MakeGap(objects::CBioseq& bioseq, const CSeq_feat& feature_gap)
{
    const string& sGT = feature_gap.GetNamedQual(kGapType_qual);

    TSeqPos gap_start(kInvalidSeqPos);
    TSeqPos gap_length(kInvalidSeqPos);

    CSeq_gap::EType gap_type = CSeq_gap::eType_unknown;
    set<int> evidences;

    if (!sGT.empty())
    {
        const CSeq_gap::SGapTypeInfo * gap_type_info = CSeq_gap::NameToGapTypeInfo(sGT);

        if (gap_type_info)
        {
            gap_type = gap_type_info->m_eType;

            const CEnumeratedTypeValues::TNameToValue&
                linkage_evidence_to_value_map = CLinkage_evidence::GetTypeInfo_enum_EType()->NameToValue();

            ITERATE(CSeq_feat::TQual, sLE_qual, feature_gap.GetQual()) // we support multiple linkage evidence qualifiers
            {
                const string& sLE_name = (**sLE_qual).GetQual();
                if (sLE_name != kLinkageEvidence_qual)
                    continue;

                CLinkage_evidence::EType evidence = (CLinkage_evidence::EType) - 1; //CLinkage_evidence::eType_unspecified;

                CEnumeratedTypeValues::TNameToValue::const_iterator it = linkage_evidence_to_value_map.find(CFastaReader::CanonicalizeString((**sLE_qual).GetVal()));
                if (it == linkage_evidence_to_value_map.end())
                {
                    m_context.m_logger->PutError(*unique_ptr<CLineError>(
                        CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                        string("Unrecognized linkage evidence ") + (**sLE_qual).GetVal())));
                    return CRef<CDelta_seq>(0);
                }
                else
                {
                    evidence = (CLinkage_evidence::EType)it->second;
                }

                switch (gap_type_info->m_eType)
                {
                    /// only the "unspecified" linkage-evidence is allowed
                case CSeq_gap::eLinkEvid_UnspecifiedOnly:
                    if (evidence != CLinkage_evidence::eType_unspecified)
                    {
                        m_context.m_logger->PutError(*unique_ptr<CLineError>(
                            CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                            string("Linkage evidence must not be specified for ") + sGT)));
                        return CRef<CDelta_seq>(0);
                    }
                    break;
                    /// no linkage-evidence is allowed
                case CSeq_gap::eLinkEvid_Forbidden:
                    if (evidence == CLinkage_evidence::eType_unspecified)
                    {
                        m_context.m_logger->PutError(*unique_ptr<CLineError>(
                            CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                            string("Linkage evidence must be specified for ") + sGT)));
                        return CRef<CDelta_seq>(0);
                    }
                    break;
                    /// any linkage-evidence is allowed, and at least one is required
                case CSeq_gap::eLinkEvid_Required:
                    break;
                default:
                    break;
                }
                if (evidence != -1)
                    evidences.insert(evidence);
            }
        }
        else
        {
            m_context.m_logger->PutError(*unique_ptr<CLineError>(
                CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                string("Unrecognized gap type ") + sGT)));
            return CRef<CDelta_seq>(0);
        }
    }

    if (feature_gap.IsSetLocation())
    {
        gap_start = feature_gap.GetLocation().GetStart(eExtreme_Positional);
        gap_length = feature_gap.GetLocation().GetStop(eExtreme_Positional);
        gap_length -= gap_start;
        gap_length++;
    }

    CGapsEditor gap_edit(gap_type, evidences, 0, 0);
    //return gap_edit.CreateGap((CBioseq&)*bsh.GetEditHandle().GetCompleteBioseq(), gap_start, gap_length);
    return gap_edit.CreateGap(bioseq, gap_start, gap_length);
}

void CFeatureTableReader::MakeGapsFromFeatures(CSeq_entry_Handle seh)
{
    for (CBioseq_CI bioseq_it(seh); bioseq_it; ++bioseq_it)
    {
        {
            SAnnotSelector annot_sel(CSeqFeatData::e_Imp);           
            for (CFeat_CI feature_it(*bioseq_it, annot_sel); feature_it; ) // no ++
            {
                if (feature_it->IsSetData() && feature_it->GetData().IsImp())
                {
                    const CImp_feat& imp = feature_it->GetData().GetImp();
                    if (imp.IsSetKey() && imp.GetKey() == kAssemblyGap_feature)
                    {
                        // removing feature
                        const CSeq_feat& feature_gap = feature_it->GetOriginalFeature();
                        CSeq_feat_EditHandle to_remove(*feature_it);
                        ++feature_it;
                        try
                        {
                            auto pBioseq = const_cast<CBioseq*>(bioseq_it->GetCompleteBioseq().GetPointer());
                            //CRef<CDelta_seq> gap = MakeGap(*bioseq_it, feature_gap);
                            CRef<CDelta_seq> gap = MakeGap(*pBioseq, feature_gap);
                            if (gap.Empty())
                            {
                                m_context.m_logger->PutError(*unique_ptr<CLineError>(
                                    CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                                    "Failed to convert feature gap into a gap")));
                            }
                            else
                            {
                                to_remove.Remove();
                            }
                        }
                        catch(const CException& ex)
                        {
                            m_context.m_logger->PutError(*unique_ptr<CLineError>(
                                CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                                ex.GetMsg())));
                        }
                        continue;
                    }
                }
                ++feature_it;
            };
        }

        CBioseq& bioseq = (CBioseq&)*bioseq_it->GetEditHandle().GetCompleteBioseq();
        RemoveEmptyFtable(bioseq);
    }
}


void CFeatureTableReader::MakeGapsFromFeatures(CSeq_entry& entry) {

    VisitAllBioseqs(entry, [&](CBioseq& bioseq) { MakeGapsFromFeatures(bioseq); });
}


void CFeatureTableReader::MakeGapsFromFeatures(CBioseq& bioseq)
{
    if (!bioseq.IsSetAnnot()) {
        return;
    }

    for (auto pAnnot : bioseq.SetAnnot()) {
        if (!pAnnot->IsSetData() ||
            (pAnnot->GetData().Which() != CSeq_annot::TData::e_Ftable)) {
            continue;
        } 
        // Annot is a feature table
        // Feature tables are lists of CRef<CSeq_feat>
        auto& ftable = pAnnot->SetData().SetFtable();
        auto fit = ftable.begin();
        while (fit != ftable.end()) {
            auto pSeqFeat = *fit;
            if (pSeqFeat->IsSetData() &&
                pSeqFeat->GetData().IsImp() &&
                pSeqFeat->GetData().GetImp().IsSetKey() &&
                pSeqFeat->GetData().GetImp().GetKey() == kAssemblyGap_feature) {
                
                try {
                    if (MakeGap(bioseq, *pSeqFeat)) {
                        fit = ftable.erase(fit);
                        continue;
                    }
                    m_context.m_logger->PutError(*unique_ptr<CLineError>(
                                CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                                "Failed to convert feature gap into a gap")));
                }
                catch(const CException& ex)
                {
                    m_context.m_logger->PutError(*unique_ptr<CLineError>(
                                CLineError::Create(ILineError::eProblem_GeneralParsingError, eDiag_Error, "", 0,
                                ex.GetMsg())));
                }

            }
            ++fit;
        }
    }

    RemoveEmptyFtable(bioseq);
}




void CFeatureTableReader::ChangeDeltaProteinToRawProtein(objects::CSeq_entry& entry)
{
    VisitAllBioseqs(entry, [](CBioseq& bioseq)
        {
            if (bioseq.IsAa() && bioseq.IsSetInst() && bioseq.GetInst().IsSetRepr())
            {
                CSeqTranslator::ChangeDeltaProteinToRawProtein(Ref(&bioseq));
            }
        }
    );

}


void CFeatureTableReader::MoveRegionsToProteins(CSeq_entry& seq_entry)
{

    if (!seq_entry.IsSet()) {
        return;
    }

    auto& bioseq_set = seq_entry.SetSet();

    _ASSERT(bioseq_set.IsSetClass());

    if (bioseq_set.GetClass() != CBioseq_set::eClass_nuc_prot) {
        if (bioseq_set.IsSetSeq_set()) {
            for (auto pEntry : bioseq_set.SetSeq_set()) {
                if (pEntry) {
                    MoveRegionsToProteins(*pEntry);
                }
            }
        }
        return;
    }

    if (!bioseq_set.IsSetAnnot()) { // coding regions appear in the set-level annotations
        return;
    }

    CScope* pScope = nullptr;


    _ASSERT(bioseq_set.IsSetSeq_set()); // should be a nuc-prot set

    // Gather region features
    list<CRef<CSeq_feat>> regions;
    for (auto pSubEntry : bioseq_set.SetSeq_set()) {
        _ASSERT(pSubEntry->IsSeq());
        auto& seq = pSubEntry->SetSeq();
        if (seq.IsNa()) {
            if (seq.IsSetAnnot()) {
                auto annot_it = seq.SetAnnot().begin();
                while (annot_it != seq.GetAnnot().end()) {
                    auto& annot = **annot_it;
                    if (annot.IsFtable()) {
                        auto& ftable = annot.SetData().SetFtable();
                        auto boundary = 
                            stable_partition(ftable.begin(), 
                                             ftable.end(),
                                             [](auto pFeat) {
                                                return pFeat->IsSetData() &&
                                                       pFeat->GetData().IsRegion();
                                             });

                        if (boundary != ftable.begin()) {
                            regions.splice(regions.end(), ftable, ftable.begin(), boundary);
                            if (ftable.empty()) {
                                annot_it = seq.SetAnnot().erase(annot_it);
                                continue;
                            }
                        }
                    }
                    ++annot_it;
                }
                if (regions.empty()) { // nothing to do here
                    return; 
                }
            }
        }
    }

    // Gather coding region annotations
    list<CRef<CSeq_feat>> cdregions;
    for (auto pAnnot : bioseq_set.GetAnnot()) {
        if (pAnnot->IsFtable()) {
            for (auto pFeat : pAnnot->GetData().GetFtable()) {
                if (pFeat->IsSetData() &&
                    pFeat->GetData().IsCdregion() &&
                    pFeat->IsSetProduct()) { 
                    cdregions.push_back(pFeat);
                }
            }
        }
    }
    if (cdregions.empty()) {
        return;
    }

    auto compareLocations = [](const CRef<CSeq_feat>& lhs,
                                const CRef<CSeq_feat>& rhs) {
        return (lhs->Compare(*rhs) < 0);
    };

    regions.sort(compareLocations);
    cdregions.sort(compareLocations);

    for (auto pRegion : regions) {
        auto cdregion_it = cdregions.begin();
        while (cdregion_it != cdregions.end()) {
           auto pCdregion = *cdregion_it; 
           if (sequence::Compare(
                       pCdregion->GetLocation(), 
                       pRegion->GetLocation(), 
                       pScope, 
                       sequence::fCompareOverlapping)
                   == sequence::eContains) {
             // cdregions.erase(cdregion_it);
             // Don't want to erase cdregions until I'm sure they are not needed
             // I need to ensure that sorting and mapping takes account of changes in 
             // strand
                CSeq_loc_Mapper mapper(*pCdregion, CSeq_loc_Mapper::eLocationToProduct, pScope);
                auto pMappedLoc = mapper.Map(pRegion->GetLocation());
                pMappedLoc->ResetStrand();
                if (pMappedLoc) {
                    pRegion->SetLocation().Assign(*pMappedLoc);
                }
                break;
           }
           ++cdregion_it;
        } 
    }


    CScope scope(*CObjectManager::GetInstance());
    CSeq_entry_Handle tse = scope.AddTopLevelSeqEntry(seq_entry);
    

    for (auto pRegion : regions) {
        const auto& loc = pRegion->GetLocation();
        CSeq_id_Handle id_handle;
        if (loc.IsWhole()) {
            id_handle = CSeq_id_Handle::GetHandle(loc.GetWhole());
        }   
        else 
        if (loc.IsInt()) {
            id_handle = CSeq_id_Handle::GetHandle(loc.GetInt().GetId());
        }
        else
        if (loc.IsPnt()) {
            id_handle = CSeq_id_Handle::GetHandle(loc.GetPnt().GetId());
        }
        else {
            continue;
        }

        auto protein_handle = scope.GetBioseqHandleFromTSE(id_handle, seq_entry);
        if (!protein_handle) {
            continue;
        }

        auto pAnnot = Ref(new CSeq_annot());
        pAnnot->SetData().SetFtable().push_back(pRegion);
        protein_handle.GetEditHandle().AttachAnnot(*pAnnot);
    }

    // start with the first region
    // Get it's range. Only consider cdregion features that contain that range

    // use sequence::Compare(loc1, loc2, scope, flags) to compare locations
    // after we've matched region to cdregion, create a CSeq_loc_Mapper object 
    // and map the region location onto the protein sequence. 
    // Finally, place the region onto the protein sequence annotations. 


    // I've removed the regions from the nucleotide sequence. 
    // If it's not possible to match them to a protein, place them in the set-level annotation
}


END_NCBI_SCOPE

