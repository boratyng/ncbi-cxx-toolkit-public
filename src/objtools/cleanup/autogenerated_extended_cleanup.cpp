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
/// This file was generated by application DATATOOL
///
/// ATTENTION:
///   Don't edit or commit this file into SVN as this file will
///   be overridden (by DATATOOL) without warning!

#include <ncbi_pch.hpp>
#include "autogenerated_extended_cleanup.hpp"
#include "cleanup_utils.hpp"
#include "autogenerated_cleanup_extra.hpp"
#include <objects/misc/sequence_macros.hpp>

BEGIN_SCOPE(ncbi)
BEGIN_SCOPE(objects)


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupOrgName( COrgName & arg0 )
{ // type Sequence
  if( arg0.IsSetAttrib() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetAttrib() );
  }
  if( arg0.IsSetLineage() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetLineage() );
  }
  if( arg0.IsSetName() ) {
      auto& name = arg0.SetName();
      if (name.IsHybrid() && name.GetHybrid().IsSet()) {
          for (auto pOrgName : name.SetHybrid().Set()) {
              x_ExtendedCleanupOrgName(*pOrgName);
          }
      }
  }
} // end of x_ExtendedCleanupOrgName


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupOrgRef( COrg_ref & arg0 )
{ // type Sequence
  if( arg0.IsSetOrgname() ) {
    x_ExtendedCleanupOrgName( arg0.SetOrgname());
  }
} // end of x_ExtendedCleanupSeqFeat_xref_E_E_data_data_biosrc_biosrc_org_org_ETC


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupBioSource( CBioSource & arg0 )
{ // type Sequence
  m_NewCleanup.BioSourceEC( arg0 );
  if( arg0.IsSetOrg() ) {
    x_ExtendedCleanupOrgRef( arg0.SetOrg() );
  }
} // end of x_ExtendedCleanupSeqFeat_xref_E_E_data_data_biosrc_biosrc_ETC


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupGeneRef( CGene_ref & arg0 )
{ // type Sequence
  m_NewCleanup.x_RemoveRedundantComment( arg0, *m_LastArg_ExtendedCleanupSeqFeat );
  if( arg0.IsSetLocus_tag() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetLocus_tag() );
  }
  if( arg0.IsSetMaploc() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetMaploc() );
  }
} // end of x_ExtendedCleanupGeneRef


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupImpFeat( CImp_feat & arg0 )
{ // type Sequence
  if( arg0.IsSetKey() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetKey() );
  }
  if( arg0.IsSetLoc() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetLoc() );
  }
} // end of x_ExtendedCleanupSeqFeat_xref_E_E_data_data_imp_imp_ETC


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupProtRef( CProt_ref & arg0 )
{ // type Reference
    m_NewCleanup.x_RemoveProtDescThatDupsProtName( arg0 );
    m_NewCleanup.ProtRefEC( arg0 );
} // end of x_ExtendedCleanupProtRef

void CAutogeneratedExtendedCleanup::x_ExtendedCleanupPubDesc( CPubdesc & arg0 )
{ // type Sequence
  if( arg0.IsSetComment() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetComment() );
  }
} // end of x_ExtendedCleanupSeqFeat_xref_E_E_data_data_pub_pub_ETC


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupTxinit( CTxinit & arg0 )
{ // type Sequence
  if( arg0.IsSetGene() ) {
      for (auto pGeneRef : arg0.SetGene()) {
        x_ExtendedCleanupGeneRef(*pGeneRef);
      }
  }
  if( arg0.IsSetProtein() ) {
      for (auto pProtRef : arg0.SetProtein()) {
        x_ExtendedCleanupProtRef(*pProtRef);
      }
  }
  if( arg0.IsSetTxorg() ) {
    x_ExtendedCleanupOrgRef( arg0.SetTxorg() );
  }
}


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupSeqFeatData( CSeqFeatData & arg0 )
{ // type Choice
  switch( arg0.Which() ) {
  case CSeqFeatData::e_Biosrc:
    x_ExtendedCleanupBioSource( arg0.SetBiosrc() );
    break;
  case CSeqFeatData::e_Gene:
    x_ExtendedCleanupGeneRef( arg0.SetGene() );
    break;
  case CSeqFeatData::e_Imp:
    x_ExtendedCleanupImpFeat( arg0.SetImp() );
    break;
  case CSeqFeatData::e_Org:
    x_ExtendedCleanupOrgRef( arg0.SetOrg() );
    break;
  case CSeqFeatData::e_Prot:
    x_ExtendedCleanupProtRef( arg0.SetProt() );
    break;
  case CSeqFeatData::e_Pub:
    x_ExtendedCleanupPubDesc( arg0.SetPub() );
    break;
  case CSeqFeatData::e_Txinit:
    x_ExtendedCleanupTxinit( arg0.SetTxinit() );
    break;
  default:
    break;
  }
} // end of x_ExtendedCleanupSeqFeatData_data



void CAutogeneratedExtendedCleanup::x_ExtendedCleanupSeqFeatXref( CSeqFeatXref & arg0 )
{ // type Sequence
  if( arg0.IsSetData() ) {
    x_ExtendedCleanupSeqFeatData( arg0.SetData() );
  }
} // end of x_ExtendedCleanupSeqFeat_xref_E_E_ETC


void CAutogeneratedExtendedCleanup::ExtendedCleanupSeqFeat( CSeq_feat & arg0_raw )
{ // type Sequence
  CRef<CSeq_feat> raw_ref( &arg0_raw );
  CSeq_feat_EditHandle efh;

  CRef<CSeq_feat> new_feat;

  try {
    // Try to use an edit handle so we can update the object manager
    efh = CSeq_feat_EditHandle( m_Scope.GetSeq_featHandle( arg0_raw ) );
    new_feat.Reset( new CSeq_feat );
    new_feat->Assign( arg0_raw );
  } catch(...) {
    new_feat.Reset( &arg0_raw );
  }

  CSeq_feat &arg0 = *new_feat;

  m_LastArg_ExtendedCleanupSeqFeat = &arg0;

  m_NewCleanup.x_BondEC( arg0 );
  m_NewCleanup.x_tRNAEC( arg0 );
  m_NewCleanup.CdRegionEC( arg0 );
  m_NewCleanup.MoveDbxrefs( arg0 );
  m_NewCleanup.MoveStandardName( arg0 );
  m_NewCleanup.CreatePubFromFeat( arg0 );
  m_NewCleanup.ResynchProteinPartials( arg0 );
  m_NewCleanup.x_MoveSeqfeatOrgToSourceOrg( arg0 );
  if( arg0.IsSetData() ) {
    x_ExtendedCleanupSeqFeatData( arg0.SetData() );
  }
  if( arg0.IsSetExcept_text() ) {
    m_NewCleanup.x_ExceptTextEC(arg0.SetExcept_text());
  }
  if( arg0.IsSetTitle() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetTitle() );
  }
  if( arg0.IsSetXref() ) {
    for (auto pXref : arg0.SetXref()) {
        x_ExtendedCleanupSeqFeatXref(*pXref);
    }
  }

  m_LastArg_ExtendedCleanupSeqFeat = NULL;

  if( efh ) {
    efh.Replace(arg0);
    arg0_raw.Assign( arg0 );
  }

} // end of ExtendedCleanupSeqFeat


void CAutogeneratedExtendedCleanup::ExtendedCleanupSeqAnnotDescr(CAnnot_descr & arg0)
{ // type Reference
  if( arg0.IsSet() ) {
    for (auto pAnnotDesc : arg0.Set()) {
        if (pAnnotDesc->IsPub()) {
            x_ExtendedCleanupPubDesc(pAnnotDesc->SetPub());
        }
    }
  }
} // end of ExtendedCleanupSeqAnnot_E_desc_ETC

void CAutogeneratedExtendedCleanup::ExtendedCleanupSeqAnnot( CSeq_annot & arg0 )
{ // type Sequence
  m_NewCleanup.x_RemoveEmptyFeatures( arg0 );
  if( arg0.IsFtable() ) {
    for (auto pFeat : arg0.SetData().SetFtable()) {
        ExtendedCleanupSeqFeat(*pFeat);
    }
  }
  if( arg0.IsSetDesc() ) {
    ExtendedCleanupSeqAnnotDescr( arg0.SetDesc() );
  }
} // end of ExtendedCleanupSeqAnnot


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupGBBlock(CGB_block& arg0)
{ // type Sequence
  if( arg0.IsSetOrigin() ) {
    m_NewCleanup.x_TrimInternalSemicolonsMarkChanged( arg0.SetOrigin() );
  }
}

void CAutogeneratedExtendedCleanup::x_ExtendedCleanupSeqdesc( CSeqdesc & arg0 )
{ // type Choice
  m_NewCleanup.x_MoveSeqdescOrgToSourceOrg( arg0 );
  switch( arg0.Which() ) {
  case CSeqdesc::e_Genbank:
    x_ExtendedCleanupGBBlock( arg0.SetGenbank() );
    break;
  case CSeqdesc::e_Org:
    x_ExtendedCleanupOrgRef( arg0.SetOrg() );
    break;
  case CSeqdesc::e_Pub:
    x_ExtendedCleanupPubDesc( arg0.SetPub() );
    break;
  case CSeqdesc::e_Source:
    x_ExtendedCleanupBioSource( arg0.SetSource() );
    break;
  default:
    break;
  }
} // end of x_ExtendedCleanupBioseqSet_seq_set_E_E_seq_seq_descr_descr_E_E_ETC


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupSeqdescr( CSeq_descr & arg0 )
{ // type Reference
  m_NewCleanup.x_RemoveUnseenTitles(arg0);
  m_NewCleanup.x_MergeDupBioSources( arg0 );
  m_NewCleanup.x_RemoveEmptyUserObject( arg0 );
  m_NewCleanup.KeepLatestDateDesc( arg0 );
  m_NewCleanup.x_CleanupGenbankBlock( arg0 );
  m_NewCleanup.x_RemoveOldDescriptors( arg0 );
  m_NewCleanup.x_RemoveDupPubs( arg0 );
  m_NewCleanup.x_RemoveEmptyDescriptors( arg0 );
  if( arg0.IsSet() ) {
    for (auto pDesc : arg0.Set()) {
        x_ExtendedCleanupSeqdesc(*pDesc);
    }
  }
} // end of x_ExtendedCleanupSeqdescr


void CAutogeneratedExtendedCleanup::x_ExtendedCleanupBioseq_inst( CSeq_inst & seq_inst )
{ // type Reference
    if (seq_inst.IsSetExt() &&
        seq_inst.GetExt().IsMap()) {
        auto& map_ext = seq_inst.SetExt().SetMap();
        if (map_ext.IsSet()) {
            for (auto pFeat : map_ext.Set()) {
                ExtendedCleanupSeqFeat(*pFeat);
            }
        }
    }
} // end of x_ExtendedCleanupBioseq_inst

void CAutogeneratedExtendedCleanup::ExtendedCleanupBioseq( CBioseq & arg0 )
{ // type Sequence
  m_NewCleanup.x_ExtendProteinFeatureOnProteinSeq( arg0 ); // I think that protein features are not annotated on the bioseq, but I need to check.
  m_NewCleanup.MoveCitationQuals( arg0 );
  m_NewCleanup.CreateMissingMolInfo( arg0 );
  m_NewCleanup.x_ExtendSingleGeneOnMrna( arg0 );
  m_NewCleanup.x_RemoveOldFeatures( arg0 );
  m_NewCleanup.x_RemoveEmptyFeatureTables( arg0 );
  if( arg0.IsSetAnnot() ) {
    m_NewCleanup.x_MergeAdjacentFeatureTables( arg0 );
    x_ExtendedCleanupSeqAnnots( arg0.SetAnnot() );
  }
  if( arg0.IsSetDescr() ) {
    m_NewCleanup.x_FixStructuredCommentKeywords(arg0.SetDescr());
    m_NewCleanup.x_CleanupGenbankBlock( arg0 );
    m_NewCleanup.x_RescueMolInfo( arg0 );
    x_ExtendedCleanupSeqdescr( arg0.SetDescr() );
    m_NewCleanup.RemoveBadProteinTitle( arg0 );
  }
  if( arg0.IsSetInst() ) {
    x_ExtendedCleanupBioseq_inst( arg0.SetInst() );
  }
  m_NewCleanup.ResynchPeptidePartials( arg0 );
  m_NewCleanup.AddProteinTitles( arg0 );
  m_NewCleanup.x_ClearEmptyDescr( arg0 );
} // end of ExtendedCleanupBioseq


void CAutogeneratedExtendedCleanup::ExtendedCleanupBioseqSet( CBioseq_set & arg0 )
{ // type Sequence
  m_NewCleanup.x_BioseqSetEC( arg0 );
  m_NewCleanup.x_MoveCDSFromNucAnnotToSetAnnot( arg0 );
  m_NewCleanup.x_MovePopPhyMutPub( arg0 );
  m_NewCleanup.x_RemoveEmptyFeatureTables( arg0 );
  if( arg0.IsSetAnnot() ) {
    m_NewCleanup.x_MergeAdjacentFeatureTables( arg0 );
    x_ExtendedCleanupSeqAnnots(arg0.SetAnnot());
  }
  if( arg0.IsSetDescr() ) {
    m_NewCleanup.x_RemoveDupBioSource( arg0 ); // Only applies to nuc-prot sets
    m_NewCleanup.x_CleanupGenbankBlock( arg0 );
    x_ExtendedCleanupSeqdescr( arg0.SetDescr() );
  }
  if( arg0.IsSetSeq_set() ) {
    for (auto pEntry : arg0.SetSeq_set()) {
        ExtendedCleanupSeqEntry(*pEntry);
    }
  }
  m_NewCleanup.x_ClearEmptyDescr( arg0 );
  m_NewCleanup.x_SingleSeqSetToSeq( arg0 );
} // end of x_ExtendedCleanupSeqEntry_set_set_ETC


void CAutogeneratedExtendedCleanup::ExtendedCleanupSeqEntry( CSeq_entry & arg0 )
{ // type Choice
  switch( arg0.Which() ) {
  case CSeq_entry::e_Seq:
    ExtendedCleanupBioseq(arg0.SetSeq());
    break;
  case CSeq_entry::e_Set:
    if (!arg0.GetSet().GetParentEntry()) {
        arg0.ParentizeOneLevel();
    }
    ExtendedCleanupBioseqSet( arg0.SetSet() );
    break;
  default:
    break;
  }
  m_NewCleanup.x_SortSeqDescs( arg0 );
} // end of ExtendedCleanupSeqEntry

template<typename TSeqAnnotContainer>
void CAutogeneratedExtendedCleanup::x_ExtendedCleanupSeqAnnots(TSeqAnnotContainer& annots)
{
    for (auto pAnnot : annots) {
        ExtendedCleanupSeqAnnot(*pAnnot);
    }
}

void CAutogeneratedExtendedCleanup::ExtendedCleanupSeqSubmit( CSeq_submit & arg0 )
{ // type Sequence
  if(!arg0.IsSetData() ) {
      return;
  }
  auto& data = arg0.SetData();
  if (data.IsEntrys()) {
    for (auto pEntry : data.SetEntrys()) {
        ExtendedCleanupSeqEntry(*pEntry);
    }
  }
  else if (data.IsAnnots()) {
    for (auto pAnnot : data.SetAnnots()) {
        ExtendedCleanupSeqAnnot(*pAnnot);
    }
  }
} // end of ExtendedCleanupSeqSubmit

//LCOV_EXCL_STOP


END_SCOPE(objects)
END_SCOPE(ncbi)

