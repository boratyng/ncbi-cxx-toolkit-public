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
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/misc/sequence_macros.hpp>
#include <objtools/discrepancy_report/hauto_disc_class.hpp>

BEGIN_NCBI_SCOPE

USING_NCBI_SCOPE;
USING_SCOPE(objects);
USING_SCOPE(DiscRepAutoNmSpc);

void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_E_ETC( CSeq_feat & arg0_raw )
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

  // m_NewSeqEntry.CheckSeqFeat( arg0 );  // no tests at the SeqFeat level

  if( efh ) {
    efh.Replace(arg0);
    arg0_raw.Assign( arg0 );
  }

} // end of x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_E_ETC

void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_ETC( CSeq_feat & arg0 )
{ // type Reference
    x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_E_ETC( arg0 );
} // end of x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_ETC

template< typename Tcontainer_ncbi_cref_cseq_feat_ >
void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_ETC( Tcontainer_ncbi_cref_cseq_feat_ & arg0 )
{ // type UniSequence
  NON_CONST_ITERATE( typename Tcontainer_ncbi_cref_cseq_feat_, iter, arg0 ) { 
    x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_E_ETC( **iter );
  }
} // end of x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_ETC

void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_E_data_ETC( CSeq_annot::C_Data & arg0 )
{ // type Choice
  switch( arg0.Which() ) {
  case CSeq_annot::C_Data::e_Ftable:
    x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_ETC( arg0.SetFtable() );
    break;
  default:
    break;
  }
} // end of x_LookAtSeqEntry_set_set_annot_E_E_data_ETC

void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_E_ETC( CSeq_annot & arg0 )
{ // type Sequence
  if( arg0.IsSetData() ) {
    x_LookAtSeqEntry_set_set_annot_E_E_data_ETC( arg0.SetData() );
  }
} // end of x_LookAtSeqEntry_set_set_annot_E_E_ETC

void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_E_ETC( CSeq_annot & arg0 )
{ // type Reference
    x_LookAtSeqEntry_set_set_annot_E_E_ETC( arg0 );
} // end of x_LookAtSeqEntry_set_set_annot_E_ETC

template< typename Tcontainer_ncbi_cref_cseq_annot_ >
void CAutoDiscClass::x_LookAtSeqEntry_set_set_annot_ETC( Tcontainer_ncbi_cref_cseq_annot_ & arg0 )
{ // type UniSequence
  NON_CONST_ITERATE( typename Tcontainer_ncbi_cref_cseq_annot_, iter, arg0 ) { 
    x_LookAtSeqEntry_set_set_annot_E_ETC( **iter );
  }
} // end of x_LookAtSeqEntry_set_set_annot_ETC

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext_map( CMap_ext & arg0 )
{ // type Reference
  if( arg0.IsSet() ) {
    x_LookAtSeqEntry_set_set_annot_E_E_data_ftable_ETC( arg0.Set() );
  }
} // end of x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext_map

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext( CSeq_ext & arg0 )
{ // type Choice
  switch( arg0.Which() ) {
  case CSeq_ext::e_Map:
    x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext_map( arg0.SetMap() );
    break;
  default:
    break;
  }
} // end of x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq_inst_inst_ext( CSeq_ext & arg0 )
{ // type Reference
    x_LookAtSeqEntry_seq_seq_inst_inst_ext_ext( arg0 );
} // end of x_LookAtSeqEntry_seq_seq_inst_inst_ext

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq_inst_inst( CSeq_inst & arg0 )
{ // type Sequence
  m_NewSeqEntry.CheckSeqInstMol( arg0, *m_LastArg_x_LookAtSeqEntry_seq_seq );
  if( arg0.IsSetExt() ) {
    x_LookAtSeqEntry_seq_seq_inst_inst_ext( arg0.SetExt() );
  }
} // end of x_LookAtSeqEntry_seq_seq_inst_inst

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq_inst( CSeq_inst & arg0 )
{ // type Reference
    x_LookAtSeqEntry_seq_seq_inst_inst( arg0 );
} // end of x_LookAtSeqEntry_seq_seq_inst

void CAutoDiscClass::x_LookAtSeqEntry_seq_seq( CBioseq & arg0 )
{ // type Sequence
  m_LastArg_x_LookAtSeqEntry_seq_seq = &arg0;

  m_NewSeqEntry.CheckBioseq( arg0 );
  if( arg0.IsSetAnnot() ) {
    x_LookAtSeqEntry_set_set_annot_ETC( arg0.SetAnnot() );
  }
  if( arg0.IsSetInst() ) {
    x_LookAtSeqEntry_seq_seq_inst( arg0.SetInst() );
  }

  m_LastArg_x_LookAtSeqEntry_seq_seq = NULL;
} // end of x_LookAtSeqEntry_seq_seq

void CAutoDiscClass::x_LookAtSeqEntry_seq( CBioseq & arg0 )
{ // type Reference
    x_LookAtSeqEntry_seq_seq( arg0 );
} // end of x_LookAtSeqEntry_seq

void CAutoDiscClass::x_LookAtSeqEntry_set_set_seq_set_E( CSeq_entry & arg0 )
{ // type Reference
    LookAtSeqEntry( arg0 );
} // end of x_LookAtSeqEntry_set_set_seq_set_E

template< typename Tcontainer_ncbi_cref_cseq_entry_ >
void CAutoDiscClass::x_LookAtSeqEntry_set_set_seq_set( Tcontainer_ncbi_cref_cseq_entry_ & arg0 )
{ // type UniSequence
  NON_CONST_ITERATE( typename Tcontainer_ncbi_cref_cseq_entry_, iter, arg0 ) { 
    x_LookAtSeqEntry_set_set_seq_set_E( **iter );
  }
} // end of x_LookAtSeqEntry_set_set_seq_set

void CAutoDiscClass::x_LookAtSeqEntry_set_set( CBioseq_set & arg0 )
{ // type Sequence
  m_NewSeqEntry.CheckBioseqSet( arg0 );
  if( arg0.IsSetAnnot() ) {
    x_LookAtSeqEntry_set_set_annot_ETC( arg0.SetAnnot() );
  }
  if( arg0.IsSetSeq_set() ) {
    x_LookAtSeqEntry_set_set_seq_set( arg0.SetSeq_set() );
  }
} // end of x_LookAtSeqEntry_set_set

void CAutoDiscClass::x_LookAtSeqEntry_set( CBioseq_set & arg0 )
{ // type Reference
    x_LookAtSeqEntry_set_set( arg0 );
} // end of x_LookAtSeqEntry_set

void CAutoDiscClass::LookAtSeqEntry( CSeq_entry & arg0 )
{ // type Choice
  switch( arg0.Which() ) {
  case CSeq_entry::e_Seq:
    x_LookAtSeqEntry_seq( arg0.SetSeq() );
    break;
  case CSeq_entry::e_Set:
    x_LookAtSeqEntry_set( arg0.SetSet() );
    break;
  default:
    break;
  }
} // end of LookAtSeqEntry

END_NCBI_SCOPE
