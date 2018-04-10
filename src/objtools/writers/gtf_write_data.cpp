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
 * Author:  Frank Ludwig
 *
 * File Description:
 *   GFF file reader
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>
#include <objects/seqfeat/Gene_ref.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/Code_break.hpp>
#include <objects/seqfeat/Genetic_code.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>

//#include <objmgr/util/sequence.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objmgr/feat_ci.hpp>
#include <objmgr/mapped_feat.hpp>
#include <objmgr/util/feature.hpp>

#include <objtools/writers/write_util.hpp>
#include <objtools/writers/gff2_write_data.hpp>
#include <objtools/writers/gtf_write_data.hpp>
#include <objmgr/util/seq_loc_util.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::

//  ----------------------------------------------------------------------------
string s_GtfDbtag( const CDbtag& dbtag )
//  ----------------------------------------------------------------------------
{
    string strGffTag;
    if ( dbtag.IsSetDb() ) {
        strGffTag += dbtag.GetDb();
        strGffTag += ":";
    }
    if ( dbtag.IsSetTag() ) {
        if ( dbtag.GetTag().IsId() ) {
            strGffTag += NStr::UIntToString( dbtag.GetTag().GetId() );
        }
        if ( dbtag.GetTag().IsStr() ) {
            strGffTag += dbtag.GetTag().GetStr();
        }
    }
    return CWriteUtil::UrlEncode(strGffTag);
}
        
//  ----------------------------------------------------------------------------
bool CGtfRecord::MakeChildRecord(
    const CGtfRecord& parent,
    const CSeq_interval& location,
    unsigned int uExonNumber )
//  ----------------------------------------------------------------------------
{
    if ( ! location.CanGetFrom() || ! location.CanGetTo() ) {
        return false;
    }
    m_strId = parent.m_strId;
    m_strSource = parent.m_strSource;
    m_strType = parent.m_strType;
    m_strGeneId = parent.GeneId();
    m_strTranscriptId = parent.TranscriptId();

    m_uSeqStart = location.GetFrom();
    m_uSeqStop = location.GetTo();
    if ( parent.m_pScore ) {
        m_pScore = new string( *(parent.m_pScore) );
    }
    if ( parent.m_peStrand ) {
        m_peStrand = new ENa_strand( *(parent.m_peStrand) );
    }

    m_Attributes.insert( parent.m_Attributes.begin(), parent.m_Attributes.end() );
    if ( 0 != uExonNumber ) {
        SetAttribute("exon_number", NStr::UIntToString(uExonNumber));
    }
    return true;
};

//  ----------------------------------------------------------------------------
bool CGtfRecord::x_AssignAttributes(
    const CMappedFeat& mapped_feature,
    unsigned int )
//  ----------------------------------------------------------------------------
{
    if ( ! x_AssignAttributesFromAsnCore( mapped_feature ) ) {
        return false;
    }
    if ( ! x_AssignAttributesFromAsnExtended( mapped_feature ) ) {
        return false;
    }
    return true;
}
    
//  ----------------------------------------------------------------------------
string CGtfRecord::StrAttributes() const
//  ----------------------------------------------------------------------------
{
    string strAttributes;
	strAttributes.reserve(256);
    CGtfRecord::TAttributes attrs;
    attrs.insert( Attributes().begin(), Attributes().end() );
    CGtfRecord::TAttrIt it;

    strAttributes += x_AttributeToString( "gene_id", GeneId() );
    if ( StrType() != "gene" ) {
        strAttributes += x_AttributeToString( "transcript_id", TranscriptId() );
    }

    for ( it = attrs.begin(); it != attrs.end(); ++it ) {
        string strKey = it->first;
        if ( NStr::StartsWith( strKey, "gff_" ) ) {
            continue;
        }
        if ( strKey == "exon_number" ) {
            continue;
        }

        strAttributes += x_AttributeToString( strKey, it->second.front() );
    }
    
    if ( ! m_bNoExonNumbers ) {
        it = attrs.find( "exon_number" );
        if ( it != attrs.end() ) {
            strAttributes += x_AttributeToString( "exon_number", it->second.front() );
        }
    }
    return strAttributes;
}

//  ----------------------------------------------------------------------------
string CGtfRecord::StrStructibutes() const
//  ----------------------------------------------------------------------------
{
    string strAttributes;
	strAttributes.reserve(256);
    CGtfRecord::TAttributes attrs;
    attrs.insert( Attributes().begin(), Attributes().end() );
    CGtfRecord::TAttrIt it;

    strAttributes += x_AttributeToString( "gene_id", GeneId() );
    if ( StrType() != "gene" ) {
        strAttributes += x_AttributeToString( "transcript_id", TranscriptId() );
    }

    it = attrs.find( "exon_number" );
    if ( it != attrs.end() ) {
        strAttributes += x_AttributeToString( "exon_number", it->second.front() );
    }
    return strAttributes;
}

//  ----------------------------------------------------------------------------
bool CGtfRecord::x_AssignAttributesFromAsnCore(
    const CMappedFeat& mapped_feature )
//  ----------------------------------------------------------------------------
{
    const CSeq_feat& feature = mapped_feature.GetOriginalFeature();

    if ( feature.CanGetQual() ) {
        const vector< CRef< CGb_qual > >& quals = feature.GetQual();
        vector< CRef< CGb_qual > >::const_iterator it = quals.begin();
        for ( /*NOOP*/; it != quals.end(); ++it ) {
            if ( (*it)->CanGetQual() && (*it)->CanGetVal() ) {
                if ( (*it)->GetQual() == "ID" ) {
                    continue;
                }
                if ( (*it)->GetQual() == "Parent" ) {
                    continue;
                }
                if ( (*it)->GetQual() == "gff_type" ) {
                    continue;
                }
                if ( (*it)->GetQual() == "transcript_id" ) {
                    continue;
                }
                if ( (*it)->GetQual() == "gene_id" ) {
                    continue;
                }
                SetAttribute((*it)->GetQual(), (*it)->GetVal());
            }
        }
    }

    return true;
}

//  ----------------------------------------------------------------------------
bool CGtfRecord::x_AssignAttributesFromAsnExtended(
    const CMappedFeat& mf )
//  ----------------------------------------------------------------------------
{
    const CSeq_feat& feature = mf.GetOriginalFeature();

    string strDbxref = x_FeatureToDbxref(mf);
    if ( ! strDbxref.empty() ) {
        SetAttribute("db_xref", strDbxref);
    }

    string strNote = x_FeatureToNote(mf);
    if ( ! strNote.empty() ) {
        SetAttribute("note", strNote);
    }
    
    if ( x_FeatureToPseudo(mf) ) {    
        SetAttribute("pseudo", "");
    }

    if ( x_FeatureToPartial(mf) ) {    
        SetAttribute("partial", "");
    }

    switch ( feature.GetData().GetSubtype() ) {
    default:
		if (mf.GetFeatType() == CSeqFeatData::e_Rna) {
            m_strGeneId = x_MrnaToGeneId(mf);
            m_strTranscriptId = x_MrnaToTranscriptId(mf);

            string strProduct = x_MrnaToProduct(mf);
            if ( ! strProduct.empty() ) {
                SetAttribute("product", strProduct); 
            }
		}
        break;

	case CSeq_feat::TData::eSubtype_C_region:
	case CSeq_feat::TData::eSubtype_D_segment:
	case CSeq_feat::TData::eSubtype_J_segment:
	case CSeq_feat::TData::eSubtype_V_segment: {
        m_strGeneId = x_MrnaToGeneId(mf);
        m_strTranscriptId = x_MrnaToTranscriptId(mf);
		break;
	}
    case CSeq_feat::TData::eSubtype_cdregion: {

            m_strGeneId = x_CdsToGeneId(mf);
            m_strTranscriptId = x_CdsToTranscriptId(mf);

            string strProteinId = x_CdsToProteinId(mf);
            if ( ! strProteinId.empty() ) {
                SetAttribute("protein_id", strProteinId);
            }

            if ( x_CdsToRibosomalSlippage(mf) ) {
                SetAttribute("ribosomal_slippage", "");
            }

            string strProduct = x_CdsToProduct(mf);
            if ( ! strProduct.empty() ) {
                SetAttribute("product", strProduct); 
            }

            string strCode = x_CdsToCode(mf);
            if ( ! strCode.empty() ) {
                SetAttribute("transl_table", strCode);
            }
         }
        break;

    case CSeq_feat::TData::eSubtype_gene: {

            m_strGeneId = x_GeneToGeneId(mf);

            string strGeneSyn = x_GeneToGeneSyn(mf);
            if ( ! strGeneSyn.empty() ) {
                SetAttribute("gene_synonym", strGeneSyn);
            }                             
        }
        break;
    }
    return true;
}

//  ----------------------------------------------------------------------------
void CGtfRecord::SetCdsPhase(
    const list<CRef<CSeq_interval> > & cdsLocs,
    ENa_strand eStrand )
//  ----------------------------------------------------------------------------
{
    if ( cdsLocs.empty() ) {
        return;
    }
    if ( ! m_puPhase ) {
        m_puPhase = new unsigned int( 0 );
    }
    if ( eStrand == eNa_strand_minus ) {
        unsigned int uTopSize = 0;
        list<CRef<CSeq_interval> >::const_iterator it = cdsLocs.begin();
        for ( ; it != cdsLocs.end(); ++it ) {
            if ( (*it)->CanGetFrom()  && (*it)->GetFrom() > m_uSeqStop ) {
                uTopSize += (*it)->GetLength();
            }
        }
        *m_puPhase = (3 - (uTopSize % 3) ) % 3;
    }
    else {
        unsigned int uBottomSize = 0;
        list<CRef<CSeq_interval> >::const_iterator it = cdsLocs.begin();
        for ( ; it != cdsLocs.end(); ++it ) {
            if ( (*it)->CanGetFrom()  &&  (*it)->GetTo() < m_uSeqStart ) {
                uBottomSize += (*it)->GetLength();
            }
        }
        *m_puPhase = (3 - (uBottomSize % 3) ) % 3;
    }
}

//  ============================================================================
string CGtfRecord::x_GenericGeneId(
    const CMappedFeat& mapped_feat )
//  ============================================================================
{
    static unsigned int uId = 1;
    string strGeneId = string( "unknown_gene_" ) + 
        NStr::UIntToString( uId );
    if ( mapped_feat.GetData().GetSubtype() == CSeq_feat::TData::eSubtype_gene ) {
        uId++;
    }
    return strGeneId;
}

//  ============================================================================
string CGtfRecord::x_GenericTranscriptId(
    const CMappedFeat& mapped_feat )
//  ============================================================================
{
    static unsigned int uId = 1;
    string strTranscriptId = string("unknown_transcript_") + 
        NStr::UIntToString(uId++);
    return strTranscriptId;
}

//  ============================================================================
CMappedFeat CGtfRecord::x_CdsFeatureToMrnaParent(
    const CMappedFeat& mapped_feat )
//  ============================================================================
{
    return feature::GetBestMrnaForCds( mapped_feat, &FeatTree() );
}

//  ============================================================================
CMappedFeat CGtfRecord::x_CdsFeatureToGeneParent(
    const CMappedFeat& mapped_feat )
//  ============================================================================
{
    return feature::GetBestGeneForCds( mapped_feat, &FeatTree() );
}

//  ============================================================================
CMappedFeat CGtfRecord::x_MrnaFeatureToGeneParent(
    const CMappedFeat& mf)
//  ============================================================================
{
	if (mf.GetFeatSubtype() == CSeqFeatData::eSubtype_mRNA) {
		return feature::GetBestGeneForMrna(mf, &FeatTree());
	}
	return feature::GetBestGeneForFeat(mf, &FeatTree());
}

//  ============================================================================
string CGtfRecord::x_GeneToGeneId(
    const CMappedFeat& mf )
//  ============================================================================
{
    typedef string GENE_ID;
    typedef map<CMappedFeat, GENE_ID> GENE_MAP;
    typedef list<GENE_ID> GENE_IDS;

    static GENE_IDS usedGeneIds;
    static GENE_MAP geneMap;

    GENE_MAP::const_iterator geneIt = geneMap.find(mf);
    if (geneMap.end() != geneIt) {
        return geneIt->second;
    }

    GENE_ID geneId;
    const CGene_ref& gene = mf.GetData().GetGene();

    geneId = mf.GetNamedQual("gene_id");
    if (geneId.empty()  &&  gene.IsSetLocus_tag()) {
        geneId = gene.GetLocus_tag();
    }
    if (geneId.empty() &&  gene.IsSetLocus()) {
        geneId = gene.GetLocus();
    }
    if (geneId.empty() &&  gene.IsSetSyn() ) {
        geneId = gene.GetSyn().front();
    }
    if (geneId.empty()) {
        geneId = x_GenericGeneId(mf); 
        //we know the ID is going to be unique if we get it this way
        // not point in further checking
        usedGeneIds.push_back(geneId);
        geneMap[mf] = geneId;
        return geneId;
    }
    list<GENE_ID>::const_iterator cit = find(
        usedGeneIds.begin(), usedGeneIds.end(), geneId);
    if (usedGeneIds.end() == cit) {
        usedGeneIds.push_back(geneId);
        geneMap[mf] = geneId;
        return geneId;
    }
    unsigned int suffix = 1;
    geneId += "_";
    while (true) {
        GENE_ID qualifiedGeneId = geneId + NStr::UIntToString(suffix);
        cit = find(usedGeneIds.begin(), usedGeneIds.end(), qualifiedGeneId);
        if (usedGeneIds.end() == cit) {
            usedGeneIds.push_back(qualifiedGeneId);
            geneMap[mf] = qualifiedGeneId;
            return qualifiedGeneId;
        }
        ++suffix;
    }
}                        

//  ============================================================================
string CGtfRecord::x_GeneToGeneSyn(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    const CGene_ref& gene = mapped_feature.GetData().GetGene();
    if ( ! gene.IsSetSyn() ) {
        return "";
    }
    if ( gene.IsSetLocus_tag() ) {
        return gene.GetSyn().front();
    }
    CGene_ref::TSyn::const_iterator it = gene.GetSyn().begin();
    ++it;
    if ( it != gene.GetSyn().end() ) {
        return *it;
    }
    return "";
}

//  =============================================================================
string CGtfRecord::x_MrnaToGeneId(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    try {
        CMappedFeat gene = x_MrnaFeatureToGeneParent( mapped_feature );
        return x_GeneToGeneId( gene );
    }
    catch(...) {
        return "";
    } 
}

//  =============================================================================
string CGtfRecord::x_MrnaToTranscriptId(
    const CMappedFeat& mf)
//  ============================================================================
{
    typedef string RNA_ID;
    typedef map<CMappedFeat, RNA_ID> RNA_MAP;
    typedef list<RNA_ID> RNA_IDS;

    static RNA_MAP rnaMap;
    RNA_MAP::const_iterator rnaIt = rnaMap.find(mf);
    if (rnaMap.end() != rnaIt) {
        return rnaIt->second;
    }

    static RNA_IDS usedRnaIds;
    RNA_ID rnaId;

    // qualifier on feature first
    rnaId = mf.GetNamedQual("transcript_id");

    //then product if available
    if (rnaId.empty()  &&  mf.IsSetProduct()) {
        if (!CWriteUtil::GetBestId(mf.GetProductId(), mf.GetScope(), rnaId)) {
            rnaId.clear();
        }
    }

    //then orig_transcript_id if available
    if (rnaId.empty()) {
        rnaId = mf.GetNamedQual("orig_transcript_id");
    }

    //then gene/locus-tag
    //then gene/locus
    if (rnaId.empty()) {
        try {
            CMappedFeat gf = x_MrnaFeatureToGeneParent(mf);
            if (gf) {
                const CGene_ref& gRef = gf.GetData().GetGene();
                if (gRef.IsSetLocus_tag()) {
                    rnaId = gRef.GetLocus_tag();
                }
                else if (gRef.IsSetLocus()) {
                    rnaId = gRef.GetLocus();
                }
            }
        }
        catch(...){}
    }

    //generic ID as last resort
    if (rnaId.empty()) {
        rnaId = x_GenericTranscriptId(mf);
        //we know the ID is going to be unique if we get it this way
        // not point in further checking
        usedRnaIds.push_back(rnaId);
        rnaMap[mf] = rnaId;
        return rnaId;
    }

    //uniquify the ID we came up with
    RNA_IDS::const_iterator cit = find(
        usedRnaIds.begin(), usedRnaIds.end(), rnaId);
    if (usedRnaIds.end() == cit) {
        usedRnaIds.push_back(rnaId);
        rnaMap[mf] = rnaId;
        return rnaId;
    }     
    unsigned int suffix = 1;
    rnaId += "_";
    while (true) {
        RNA_ID qualifiedId = rnaId + NStr::UIntToString(suffix);   
        cit = find(usedRnaIds.begin(), usedRnaIds.end(), qualifiedId);
        if (usedRnaIds.end() == cit) {
            usedRnaIds.push_back(qualifiedId);
            rnaMap[mf] = qualifiedId;
            return qualifiedId;
        }
        ++suffix;
    }   
}
    
//  =============================================================================
string CGtfRecord::x_CdsToGeneId(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    CMappedFeat gene = x_CdsFeatureToGeneParent( mapped_feature );
	if (!gene) {
		return "";
	}
    return x_GeneToGeneId( gene );
}

//  =============================================================================
string CGtfRecord::x_CdsToTranscriptId(
    const CMappedFeat& mf )
//  ============================================================================
{
    CMappedFeat mrna = x_CdsFeatureToMrnaParent(mf);
	if (!mrna) {
		return x_GenericTranscriptId(mf);
	}
    return x_MrnaToTranscriptId(mrna);
}

//  ============================================================================
string CGtfRecord::x_CdsToProteinId(
    const CMappedFeat& mf )
//  ============================================================================
{
    string protein_id = mf.GetNamedQual("protein_id");
    if (!protein_id.empty()) {
        return protein_id;
    }
    if ( mf.IsSetProduct() ) {
        string product;
        if (CWriteUtil::GetBestId(mf.GetProductId(), mf.GetScope(), product)) {
            return product;
        }
        return mf.GetProduct().GetId()->GetSeqIdString( true );
    }
    return "";
}

//  ============================================================================
bool CGtfRecord::x_CdsToRibosomalSlippage(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( mapped_feature.IsSetExcept_text() ) {
        if ( mapped_feature.GetExcept_text() == "ribosomal slippage" ) {
            return true;
        }
    }
    return false;
}
    
//  ============================================================================
string CGtfRecord::x_CdsToProduct(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( ! mapped_feature.IsSetXref() ) {
        return "";
    }
    const vector< CRef< CSeqFeatXref > > xref = mapped_feature.GetXref();
    vector< CRef< CSeqFeatXref > >::const_iterator it = xref.begin();
    for ( ; it != xref.end(); ++it ) {
        const CSeqFeatXref& ref = **it;
        if ( ref.IsSetData() && ref.GetData().IsProt() && 
            ref.GetData().GetProt().IsSetName() ) 
        {
            return ref.GetData().GetProt().GetName().front();
        }
    }
    return "";
}
    
//  ============================================================================
string CGtfRecord::x_CdsToCode(
    const CMappedFeat& mf)
//  ============================================================================
{
    const CCdregion& cdr = mf.GetData().GetCdregion();
    if ( cdr.IsSetCode() ) {
        return NStr::IntToString( cdr.GetCode().GetId() );
    }
    return "";
}

//  ============================================================================
string CGtfRecord::x_MrnaToProduct(
    const CMappedFeat& mf)
//  ============================================================================
{
    string product = mf.GetNamedQual("product");
    if (!product.empty()) {
        return product;
    }
    const CRNA_ref& rna = mf.GetData().GetRna();
    if ( rna.IsSetExt() && rna.GetExt().IsName() ) {
        return rna.GetExt().GetName();
    }
    return "";
}

//  ============================================================================
string CGtfRecord::x_FeatureToDbxref(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( mapped_feature.IsSetDbxref() ) {
        const CSeq_feat::TDbxref& dbxrefs = mapped_feature.GetDbxref();
        if ( dbxrefs.size() > 0 ) {
            string value = s_GtfDbtag( *dbxrefs[ 0 ] );
            for ( size_t i=1; i < dbxrefs.size(); ++i ) {
                value += ",";
                value += s_GtfDbtag( *dbxrefs[ i ] );
            }
            return value;
        }
    }
    return "";
}

//  ============================================================================
string CGtfRecord::x_FeatureToNote(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( mapped_feature.IsSetComment() ) {
        return mapped_feature.GetComment();
    }
    return "";
}

//  ============================================================================
bool CGtfRecord::x_FeatureToPseudo(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( mapped_feature.IsSetPseudo()  &&  mapped_feature.GetPseudo() ) {
        return true;
    }
    return false;
}

//  ============================================================================
bool CGtfRecord::x_FeatureToPartial(
    const CMappedFeat& mapped_feature )
//  ============================================================================
{
    if ( mapped_feature.IsSetPartial()  &&  mapped_feature.GetPartial() ) {
        return true;
    }
    return false;
}

//  ============================================================================
string CGtfRecord::x_AttributeToString(
    const string& strKey,
    const string& strValue )
//  ============================================================================
{
    string str( strKey );
    str += " \"";
    str += strValue;
    str += "\"; ";
    return str;
}
    
END_objects_SCOPE
END_NCBI_SCOPE
