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

/// @file ValidErrItem.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'valerr.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: ValidErrItem_.hpp


#ifndef OBJECTS_VALERR_VALIDERRITEM_HPP
#define OBJECTS_VALERR_VALIDERRITEM_HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbidiag.hpp>
#include <objects/seqset/Seq_entry.hpp>

// generated includes
#include <objects/valerr/ValidErrItem_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// ===========================  Internal error types  ==========================

#define ERR_CODE_BEGIN(x)  x##BEGIN
#define ERR_CODE_END(x) x##END

/*
    Validation errors can be saved as data objects.  So we must
    take care that these error code numbers do not change.
    Only add new codes at the ends of groups. (right before ERR_CODE_END(...)).
    Only add new groups of error codes at the end of enums, (right before eErr_Max).
    Do not change the initialization constants (e.g. = 1000 )
    eErr_Max must always be the last.
*/
enum EErrType {
    eErr_ALL = 0,
    eErr_UNKNOWN,

    ERR_CODE_BEGIN(SEQ_INST),
    eErr_SEQ_INST_ExtNotAllowed,
    eErr_SEQ_INST_ExtBadOrMissing,
    eErr_SEQ_INST_SeqDataNotFound,
    eErr_SEQ_INST_SeqDataNotAllowed,
    eErr_SEQ_INST_ReprInvalid,
    eErr_SEQ_INST_CircularProtein,
    eErr_SEQ_INST_DSProtein,
    eErr_SEQ_INST_MolNotSet,
    eErr_SEQ_INST_MolOther,
    eErr_SEQ_INST_FuzzyLen,
    eErr_SEQ_INST_InvalidLen,
    eErr_SEQ_INST_InvalidAlphabet,
    eErr_SEQ_INST_SeqDataLenWrong,
    eErr_SEQ_INST_SeqPortFail,
    eErr_SEQ_INST_InvalidResidue,
    eErr_SEQ_INST_StopInProtein,
    eErr_SEQ_INST_PartialInconsistent,
    eErr_SEQ_INST_ShortSeq,
    eErr_SEQ_INST_NoIdOnBioseq,
    eErr_SEQ_INST_BadDeltaSeq,
    eErr_SEQ_INST_LongHtgsSequence,
    eErr_SEQ_INST_LongLiteralSequence,
    eErr_SEQ_INST_SequenceExceeds350kbp,
    eErr_SEQ_INST_ConflictingIdsOnBioseq,
    eErr_SEQ_INST_MolNuclAcid,
    eErr_SEQ_INST_ConflictingBiomolTech,
    eErr_SEQ_INST_SeqIdNameHasSpace,
    eErr_SEQ_INST_IdOnMultipleBioseqs,
    eErr_SEQ_INST_DuplicateSegmentReferences,
    eErr_SEQ_INST_TrailingX,
    eErr_SEQ_INST_BadSeqIdFormat,
    eErr_SEQ_INST_PartsOutOfOrder,
    eErr_SEQ_INST_BadSecondaryAccn,
    eErr_SEQ_INST_ZeroGiNumber,
    eErr_SEQ_INST_RnaDnaConflict,
    eErr_SEQ_INST_HistoryGiCollision,
    eErr_SEQ_INST_GiWithoutAccession,
    eErr_SEQ_INST_MultipleAccessions,
    eErr_SEQ_INST_HistAssemblyMissing,
    eErr_SEQ_INST_TerminalNs,
    eErr_SEQ_INST_UnexpectedIdentifierChange,
    eErr_SEQ_INST_InternalNsInSeqLit,
    eErr_SEQ_INST_SeqLitGapLength0,
    eErr_SEQ_INST_TpaAssmeblyProblem,
    eErr_SEQ_INST_SeqLocLength,
    eErr_SEQ_INST_MissingGaps,
    eErr_SEQ_INST_CompleteTitleProblem,
    eErr_SEQ_INST_CompleteCircleProblem,
    eErr_SEQ_INST_BadHTGSeq,
    ERR_CODE_END(SEQ_INST),

    ERR_CODE_BEGIN(SEQ_DESCR) = 1000,
    eErr_SEQ_DESCR_BioSourceMissing,
    eErr_SEQ_DESCR_InvalidForType,
    eErr_SEQ_DESCR_FileOpenCollision,
    eErr_SEQ_DESCR_Unknown,
    eErr_SEQ_DESCR_NoPubFound,
    eErr_SEQ_DESCR_NoOrgFound,
    eErr_SEQ_DESCR_MultipleBioSources,
    eErr_SEQ_DESCR_NoMolInfoFound,
    eErr_SEQ_DESCR_BadCountryCode,
    eErr_SEQ_DESCR_NoTaxonID,
    eErr_SEQ_DESCR_InconsistentBioSources,
    eErr_SEQ_DESCR_MissingLineage,
    eErr_SEQ_DESCR_SerialInComment,
    eErr_SEQ_DESCR_BioSourceNeedsFocus,
    eErr_SEQ_DESCR_BadOrganelle,
    eErr_SEQ_DESCR_MultipleChromosomes,
    eErr_SEQ_DESCR_BadSubSource,
    eErr_SEQ_DESCR_BadOrgMod,
    eErr_SEQ_DESCR_InconsistentProteinTitle,
    eErr_SEQ_DESCR_Inconsistent,
    eErr_SEQ_DESCR_ObsoleteSourceLocation,
    eErr_SEQ_DESCR_ObsoleteSourceQual,
    eErr_SEQ_DESCR_StructuredSourceNote,
    eErr_SEQ_DESCR_MultipleTitles,
    eErr_SEQ_DESCR_Obsolete,
    eErr_SEQ_DESCR_UnnecessaryBioSourceFocus,
    eErr_SEQ_DESCR_RefGeneTrackingWithoutStatus,
    eErr_SEQ_DESCR_UnwantedCompleteFlag,
    eErr_SEQ_DESCR_CollidingPublications,
    eErr_SEQ_DESCR_TransgenicProblem,
    ERR_CODE_END(SEQ_DESCR),

    ERR_CODE_BEGIN(GENERIC) = 2000,
    eErr_GENERIC_NonAsciiAsn,
    eErr_GENERIC_Spell,
    eErr_GENERIC_AuthorListHasEtAl,
    eErr_GENERIC_MissingPubInfo,
    eErr_GENERIC_UnnecessaryPubEquiv,
    eErr_GENERIC_BadPageNumbering,
    eErr_GENERIC_MedlineEntryPub,
    ERR_CODE_END(GENERIC),

    ERR_CODE_BEGIN(SEQ_PKG) = 3000,
    eErr_SEQ_PKG_NoCdRegionPtr,
    eErr_SEQ_PKG_NucProtProblem,
    eErr_SEQ_PKG_SegSetProblem,
    eErr_SEQ_PKG_EmptySet,
    eErr_SEQ_PKG_NucProtNotSegSet,
    eErr_SEQ_PKG_SegSetNotParts,
    eErr_SEQ_PKG_SegSetMixedBioseqs,
    eErr_SEQ_PKG_PartsSetMixedBioseqs,
    eErr_SEQ_PKG_PartsSetHasSets,
    eErr_SEQ_PKG_FeaturePackagingProblem,
    eErr_SEQ_PKG_GenomicProductPackagingProblem,
    eErr_SEQ_PKG_InconsistentMolInfoBiomols,
    eErr_SEQ_PKG_GraphPackagingProblem,
    ERR_CODE_END(SEQ_PKG),

    ERR_CODE_BEGIN(SEQ_FEAT) = 4000,
    eErr_SEQ_FEAT_InvalidForType,
    eErr_SEQ_FEAT_PartialProblem,
    eErr_SEQ_FEAT_PartialsInconsistent,
    eErr_SEQ_FEAT_InvalidType,
    eErr_SEQ_FEAT_Range,
    eErr_SEQ_FEAT_MixedStrand,
    eErr_SEQ_FEAT_SeqLocOrder,
    eErr_SEQ_FEAT_CdTransFail,
    eErr_SEQ_FEAT_StartCodon,
    eErr_SEQ_FEAT_InternalStop,
    eErr_SEQ_FEAT_NoProtein,
    eErr_SEQ_FEAT_MisMatchAA,
    eErr_SEQ_FEAT_TransLen,
    eErr_SEQ_FEAT_NoStop,
    eErr_SEQ_FEAT_TranslExcept,
    eErr_SEQ_FEAT_NoProtRefFound,
    eErr_SEQ_FEAT_NotSpliceConsensus,
    eErr_SEQ_FEAT_OrfCdsHasProduct,
    eErr_SEQ_FEAT_GeneRefHasNoData,
    eErr_SEQ_FEAT_ExceptInconsistent,
    eErr_SEQ_FEAT_ProtRefHasNoData,
    eErr_SEQ_FEAT_GenCodeMismatch,
    eErr_SEQ_FEAT_RNAtype0,
    eErr_SEQ_FEAT_UnknownImpFeatKey,
    eErr_SEQ_FEAT_UnknownImpFeatQual,
    eErr_SEQ_FEAT_WrongQualOnImpFeat,
    eErr_SEQ_FEAT_MissingQualOnImpFeat,
    eErr_SEQ_FEAT_PsuedoCdsHasProduct,
    eErr_SEQ_FEAT_IllegalDbXref,
    eErr_SEQ_FEAT_FarLocation,
    eErr_SEQ_FEAT_DuplicateFeat,
    eErr_SEQ_FEAT_UnnecessaryGeneXref,
    eErr_SEQ_FEAT_TranslExceptPhase,
    eErr_SEQ_FEAT_TrnaCodonWrong,
    eErr_SEQ_FEAT_BadTrnaAA,
    eErr_SEQ_FEAT_BothStrands,
    eErr_SEQ_FEAT_CDSgeneRange,
    eErr_SEQ_FEAT_CDSmRNArange,
    eErr_SEQ_FEAT_OverlappingPeptideFeat,
    eErr_SEQ_FEAT_SerialInComment,
    eErr_SEQ_FEAT_MultipleCDSproducts,
    eErr_SEQ_FEAT_FocusOnBioSourceFeature,
    eErr_SEQ_FEAT_PeptideFeatOutOfFrame,
    eErr_SEQ_FEAT_InvalidQualifierValue,
    eErr_SEQ_FEAT_MultipleMRNAproducts,
    eErr_SEQ_FEAT_mRNAgeneRange,
    eErr_SEQ_FEAT_TranscriptLen,
    eErr_SEQ_FEAT_TranscriptMismatches,
    eErr_SEQ_FEAT_CDSproductPackagingProblem,
    eErr_SEQ_FEAT_DuplicateInterval,
    eErr_SEQ_FEAT_PolyAsiteNotPoint,
    eErr_SEQ_FEAT_ImpFeatBadLoc,
    eErr_SEQ_FEAT_LocOnSegmentedBioseq,
    eErr_SEQ_FEAT_UnnecessaryCitPubEquiv,
    eErr_SEQ_FEAT_ImpCDShasTranslation,
    eErr_SEQ_FEAT_ImpCDSnotPseudo,
    eErr_SEQ_FEAT_MissingMRNAproduct,
    eErr_SEQ_FEAT_AbuttingIntervals,
    eErr_SEQ_FEAT_CollidingGeneNames,
    eErr_SEQ_FEAT_CollidingLocusTags,
    eErr_SEQ_FEAT_MultiIntervalGene,
    eErr_SEQ_FEAT_FeatContentDup,
    eErr_SEQ_FEAT_BadProductSeqId,
    eErr_SEQ_FEAT_RnaProductMismatch,
    eErr_SEQ_FEAT_DifferntIdTypesInSeqLoc,
    eErr_SEQ_FEAT_MissingCDSproduct,
    eErr_SEQ_FEAT_MissingLocation,
    eErr_SEQ_FEAT_OnlyGeneXrefs,
    eErr_SEQ_FEAT_UTRdoesNotAbutCDS,
    eErr_SEQ_FEAT_MultipleCdsOnMrna,
    eErr_SEQ_FEAT_BadConflictFlag,
    eErr_SEQ_FEAT_ConflictFlagSet,
    eErr_SEQ_FEAT_LocusTagProblem,
    eErr_SEQ_FEAT_AltStartCodon,
    eErr_SEQ_FEAT_GenesInconsistent,
    eErr_SEQ_FEAT_DuplicateTranslExcept,
    eErr_SEQ_FEAT_TranslExceptAndRnaEditing,
    eErr_SEQ_FEAT_NoNameForProtein,
    eErr_SEQ_FEAT_TaxonDbxrefOnFeature,
    eErr_SEQ_FEAT_MultipleBioseqs,
    eErr_SEQ_FEAT_CDSmRNAmismatch,
    eErr_SEQ_FEAT_UnnecessaryException,
    eErr_SEQ_FEAT_LocusTagProductMismatch,
    eErr_SEQ_FEAT_MrnaTransFail,
    eErr_SEQ_FEAT_ImproperBondLocation,
    ERR_CODE_END(SEQ_FEAT),

    ERR_CODE_BEGIN(SEQ_ALIGN) = 5000,
    eErr_SEQ_ALIGN_SeqIdProblem,
    eErr_SEQ_ALIGN_StrandRev,
    eErr_SEQ_ALIGN_DensegLenStart,
    eErr_SEQ_ALIGN_StartMorethanBiolen,
    eErr_SEQ_ALIGN_EndMorethanBiolen,
    eErr_SEQ_ALIGN_LenMorethanBiolen,
    eErr_SEQ_ALIGN_SumLenStart,
    eErr_SEQ_ALIGN_SegsDimMismatch,
    eErr_SEQ_ALIGN_SegsNumsegMismatch,
    eErr_SEQ_ALIGN_SegsStartsMismatch,
    eErr_SEQ_ALIGN_SegsPresentMismatch,
    eErr_SEQ_ALIGN_SegsPresentStartsMismatch,
    eErr_SEQ_ALIGN_SegsPresentStrandsMismatch,
    eErr_SEQ_ALIGN_FastaLike,
    eErr_SEQ_ALIGN_SegmentGap,
    eErr_SEQ_ALIGN_SegsInvalidDim,
    eErr_SEQ_ALIGN_Segtype,
    eErr_SEQ_ALIGN_BlastAligns,
    ERR_CODE_END(SEQ_ALIGN),

    ERR_CODE_BEGIN(SEQ_GRAPH) = 6000,
    eErr_SEQ_GRAPH_GraphMin,
    eErr_SEQ_GRAPH_GraphMax,
    eErr_SEQ_GRAPH_GraphBelow,
    eErr_SEQ_GRAPH_GraphAbove,
    eErr_SEQ_GRAPH_GraphByteLen,
    eErr_SEQ_GRAPH_GraphOutOfOrder,
    eErr_SEQ_GRAPH_GraphBioseqLen,
    eErr_SEQ_GRAPH_GraphSeqLitLen,
    eErr_SEQ_GRAPH_GraphSeqLocLen,
    eErr_SEQ_GRAPH_GraphStartPhase,
    eErr_SEQ_GRAPH_GraphStopPhase,
    eErr_SEQ_GRAPH_GraphDiffNumber,
    eErr_SEQ_GRAPH_GraphACGTScore,
    eErr_SEQ_GRAPH_GraphNScore,
    eErr_SEQ_GRAPH_GraphGapScore,
    eErr_SEQ_GRAPH_GraphOverlap,
    ERR_CODE_END(SEQ_GRAPH),

    ERR_CODE_BEGIN(INTERNAL) = 7000,
    eErr_INTERNAL_Exception,
    ERR_CODE_END(INTERNAL),

    eErr_MAX
};

/////////////////////////////////////////////////////////////////////////////
class NCBI_VALERR_EXPORT CValidErrItem : public CValidErrItem_Base
{
    typedef CValidErrItem_Base Tparent;
public:

    // destructor
    CValidErrItem(void);
    ~CValidErrItem(void);

    // severity with proper type.
    EDiagSev                GetSeverity  (void) const;
    // Error code
    string                  GetErrCode  (void) const;
    static unsigned int     GetErrCount(void);
    // Error group (SEQ_FEAT, SEQ_INST etc.)
    const string&           GetErrGroup (void) const;
    // Verbose message
    string                  GetVerbose  (void) const;
    // Offending object
    const CSerialObject&    GetObject   (void) const;
    bool                    IsSetObject (void) const;

    // Convert Severity from enum to a string representation
    static const string&    ConvertSeverity(EDiagSev sev);
    static string           ConvertErrCode(unsigned int);

    bool IsSetContext(void) const;
    const CSeq_entry& GetContext(void) const;
    
private:
    friend class CValidError;

    // constructor
    CValidErrItem(EDiagSev             sev,       // severity
                  unsigned int         ec,        // error code
                  const string&        msg,       // message
                  const string&        obj_desc,  // object description
                  const CSerialObject& obj,       // offending object
                  const string&        acc);      // accession
    
    // constructor
    CValidErrItem(EDiagSev             sev,       // severity
                  unsigned int         ec,        // error code
                  const string&        msg,       // message
                  const string&        obj_desc,  // object description
                  const CSerialObject& obj,       // offending object
                  const CSeq_entry&    context,   // desc's context.
                  const string&        acc);      // accession

    // Prohibit default & copy constructor and assignment operator
    CValidErrItem(const CValidErrItem& value);
    CValidErrItem& operator=(const CValidErrItem& value);

    // member data values that are not serialized.
    CConstRef<CSerialObject>        m_Object;     // offending object
    CConstRef<CSeq_entry>           m_Ctx; // currently used for Seqdesc objects only

    static const string sm_Terse[];
    static const string sm_Verbose[];
};

/////////////////// CValidErrItem inline methods

// constructor
inline
CValidErrItem::CValidErrItem(void)
{
}


inline
EDiagSev CValidErrItem::GetSeverity(void) const
{
    // convert from internal integer to external enum type.
    return static_cast<EDiagSev>(GetSev());
}


inline
bool CValidErrItem::IsSetContext(void) const 
{ 
    return m_Ctx.NotEmpty(); 
}

inline
const CSeq_entry& CValidErrItem::GetContext(void) const 
{ 
    return *m_Ctx; 
}


/////////////////// end of CValidErrItem inline methods



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.4  2006/04/03 17:10:08  rsmith
* make Err values permanent. Move into objects/valerr
*
* Revision 1.3  2006/03/16 14:14:41  rsmith
* add IsSetObject()
*
* Revision 1.2  2006/02/08 16:30:31  rsmith
* fix export specs
*
* Revision 1.1  2006/02/07 18:35:41  rsmith
* initial checkin
*
*
* ===========================================================================
*/

#endif // OBJECTS_VALERR_VALIDERRITEM_HPP
/* Original file checksum: lines: 94, chars: 2634, CRC32: d01b90f9 */
