/* $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's offical duties as a United States Government employee and
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
 * Author: Ilya Dondoshansky
 *
 */

/** @file blast_traceback.c
 * Functions responsible for the traceback stage of the BLAST algorithm
 */

static char const rcsid[] = 
    "$Id$";

#include <algo/blast/core/blast_traceback.h>
#include <algo/blast/core/blast_util.h>
#include <algo/blast/core/link_hsps.h>
#include <algo/blast/core/blast_setup.h>
#include <algo/blast/core/blast_kappa.h>
#include "blast_psi_priv.h"

/** TRUE if c is between a and b; f between d and f.  Determines if the
 * coordinates are already in an HSP that has been evaluated. 
*/
#define CONTAINED_IN_HSP(a,b,c,d,e,f) (((a <= c && b >= c) && (d <= f && e >= f)) ? TRUE : FALSE)

/** Compares HSPs in an HSP array and deletes those whose ranges are completely
 * included in other HSP ranges. The HSPs must be sorted by e-value/score 
 * coming into this function. Null HSPs must be purged after this function is
 * called.
 * @param hsp_array Array of BlastHSP's [in]
 * @param hspcnt Size of the array [in]
 * @param is_ooframe Is this a search with out-of-frame gapping? [in]
 */
static void BLASTCheckHSPInclusion(BlastHSP* *hsp_array, Int4 hspcnt, 
                                   Boolean is_ooframe)
{
   Int4 index, index1;
   BlastHSP* hsp,* hsp1;
   
   for (index = 0; index < hspcnt; index++) {
      hsp = hsp_array[index];
      
      if (hsp == NULL)
         continue;
      
      for (index1 = 0; index1 < index; index1++) {
         hsp1 = hsp_array[index1];
         
         if (hsp1 == NULL)
            continue;
         
         if(is_ooframe) {
            if (SIGN(hsp1->query.frame) != SIGN(hsp->query.frame))
               continue;
         } else {
            if (hsp->context != hsp1->context)
               continue;
         }
         
         /* Check of the start point of this HSP */
         if (CONTAINED_IN_HSP(hsp1->query.offset, hsp1->query.end, 
                hsp->query.offset, hsp1->subject.offset, hsp1->subject.end, 
                hsp->subject.offset) == TRUE) {
            /* Check of the end point of this HSP */
            if (CONTAINED_IN_HSP(hsp1->query.offset, hsp1->query.end, 
                   hsp->query.end, hsp1->subject.offset, hsp1->subject.end, 
                   hsp->subject.end) == TRUE) {
               /* Now checking correct strand */
               if (SIGN(hsp1->query.frame) == SIGN(hsp->query.frame) &&
                   SIGN(hsp1->subject.frame) == SIGN(hsp->subject.frame)){
                  
                  /* If we come here through all these if-s - this
                     mean, that current HSP should be removed. */
                  
                  if(hsp_array[index] != NULL) {
                     hsp_array[index] = Blast_HSPFree(hsp_array[index]);
                     break;
                  }
               }
            }
         }
      }
   }
   return;
}

/* Window size used to scan HSP for highest score region, where gapped
 * extension starts. 
 */
#define HSP_MAX_WINDOW 11

/** Function to check that the highest scoring region in an HSP still gives a 
 * positive score. This value was originally calcualted by 
 * GetStartForGappedAlignment but it may have changed due to the introduction 
 * of ambiguity characters. Such a change can lead to 'strange' results from 
 * ALIGN. 
 * @param hsp An HSP structure [in]
 * @param query Query sequence buffer [in]
 * @param subject Subject sequence buffer [in]
 * @param sbp Scoring block containing matrix [in]
 * @return TRUE if region aroung starting offsets gives a positive score
*/
static Boolean
BLAST_CheckStartForGappedAlignment (BlastHSP* hsp, Uint1* query, 
                                    Uint1* subject, const BlastScoreBlk* sbp)
{
   Int4 index1, score, start, end, width;
   Uint1* query_var,* subject_var;
   Boolean positionBased = (sbp->posMatrix != NULL);
   
   width = MIN((hsp->query.gapped_start-hsp->query.offset), HSP_MAX_WINDOW/2);
   start = hsp->query.gapped_start - width;
   end = MIN(hsp->query.end, hsp->query.gapped_start + width);
   /* Assures that the start of subject is above zero. */
   if ((hsp->subject.gapped_start + start - hsp->query.gapped_start) < 0)
      start -= hsp->subject.gapped_start + (start - hsp->query.gapped_start);
   query_var = query + start;
   subject_var = subject + hsp->subject.gapped_start + 
      (start - hsp->query.gapped_start);
   score=0;
   for (index1=start; index1<end; index1++)
      {
         if (!positionBased)
	    score += sbp->matrix[*query_var][*subject_var];
         else
	    score += sbp->posMatrix[index1][*subject_var];
         query_var++; subject_var++;
      }
   if (score <= 0)
      return FALSE;
   
   return TRUE;
}

static Int4 GetPatternLengthFromBlastHSP(BlastHSP* hsp)
{
   return hsp->pattern_length;
}

static void SavePatternLengthInGapAlignStruct(Int4 length,
               BlastGapAlignStruct* gap_align)
{
   /* Kludge: save length in an output structure member, to avoid introducing 
      a new structure member. Probably should be changed??? */
   gap_align->query_stop = length;
}


#define MAX_FULL_TRANSLATION 2100

/** Translate the subject sequence into 6 frames, and create a mixed-frame 
 * sequnce if out-of-frame gapping will be used.
 * @param subject_blk Subject sequence structure [in]
 * @param gen_code_string Genetic code to use for translation [in]
 * @param translation_buffer_ptr Pointer to buffer to hold the 
 *                               translated sequence(s) [out]
 * @param frame_offsets_ptr Pointer to an array to hold offsets into the
 *                          translation buffer for each frame. Mixed-frame 
 *                          sequence is to be returned, if NULL. [in] [out]
 * @param partial_translation_ptr Should partial translations be performed later
 *                                for each HSP instead of a full 
 *                                translation? [out]
*/
static Int2
SetUpSubjectTranslation(BLAST_SequenceBlk* subject_blk, 
                        const Uint1* gen_code_string,
                        Uint1** translation_buffer_ptr, 
                        Int4** frame_offsets_ptr,
                        Boolean* partial_translation_ptr)
{
   Boolean partial_translation;
   Boolean is_ooframe = (frame_offsets_ptr == NULL);

   if (!gen_code_string)
      return -1;

   if (is_ooframe && subject_blk->oof_sequence) {
      /* If mixed-frame sequence is already available (two-sequences case),
         then no need to translate again */
      *partial_translation_ptr = FALSE;
      return 0;
   } 

   *partial_translation_ptr = partial_translation = 
      (subject_blk->length > MAX_FULL_TRANSLATION);
      
   if (!partial_translation) {
      if (is_ooframe) {
         BLAST_GetAllTranslations(subject_blk->sequence_start, 
            NCBI4NA_ENCODING, subject_blk->length, gen_code_string, 
            NULL, NULL, &subject_blk->oof_sequence);
      } else {
         BLAST_GetAllTranslations(subject_blk->sequence_start, 
            NCBI4NA_ENCODING, subject_blk->length, gen_code_string, 
            translation_buffer_ptr, frame_offsets_ptr, NULL);
      }
   }

   return 0;
}

/** Performs the translation and coordinates adjustment, if only part of the 
 * subject sequence is translated for gapped alignment. 
 * @param subject_blk Subject sequence structure [in]
 * @param hsp The HSP information [in]
 * @param is_ooframe Return a mixed-frame sequence if TRUE [in]
 * @param gen_code_string Database genetic code [in]
 * @param translation_buffer_ptr Pointer to buffer holding the translation [out]
 * @param subject_ptr Pointer to sequence to be passed to the gapped 
 *                    alignment [out]
 * @param subject_length_ptr Length of the translated sequence [in]
 * @param start_shift_ptr How far is the partial sequence shifted w.r.t. the 
 *                        full sequence. [out]
*/
static void 
GetPartialSubjectTranslation(BLAST_SequenceBlk* subject_blk, BlastHSP* hsp,
                             Boolean is_ooframe, const Uint1* gen_code_string, 
                             Uint1** translation_buffer_ptr,
                             Uint1** subject_ptr, Int4* subject_length_ptr,
                             Int4* start_shift_ptr)
{
   Int4 translation_length;
   Uint1* translation_buffer = *translation_buffer_ptr;
   Uint1* subject;
   Int4 start_shift;
   Int4 nucl_shift;

   sfree(translation_buffer);
   if (!is_ooframe) {
      start_shift = 
         MAX(0, 3*hsp->subject.offset - MAX_FULL_TRANSLATION);
      translation_length =
         MIN(3*hsp->subject.end + MAX_FULL_TRANSLATION, 
             subject_blk->length) - start_shift;
      if (hsp->subject.frame > 0) {
         nucl_shift = start_shift;
      } else {
         nucl_shift = subject_blk->length - start_shift 
            - translation_length;
      }
      GetPartialTranslation(subject_blk->sequence_start+nucl_shift, 
                            translation_length, hsp->subject.frame,
                            gen_code_string, &translation_buffer, 
                            subject_length_ptr, NULL);
      /* Below, the start_shift will be used for the protein
         coordinates, so need to divide it by 3 */
      start_shift /= CODON_LENGTH;
   } else {
      Int4 oof_start, oof_end;
      if (hsp->subject.frame > 0) {
         oof_start = 0;
         oof_end = subject_blk->length;
      } else {
         oof_start = subject_blk->length + 1;
         oof_end = 2*subject_blk->length + 1;
      }
      
      start_shift = 
         MAX(oof_start, hsp->subject.offset - MAX_FULL_TRANSLATION);
      translation_length =
         MIN(hsp->subject.end + MAX_FULL_TRANSLATION, 
             oof_end) - start_shift;
      if (hsp->subject.frame > 0) {
         nucl_shift = start_shift - oof_start;
      } else {
         nucl_shift = oof_end - start_shift - translation_length;
      }
      GetPartialTranslation(subject_blk->sequence_start+nucl_shift, 
                            translation_length, hsp->subject.frame, 
                            gen_code_string, NULL, 
                            subject_length_ptr, &translation_buffer);
   }
   hsp->subject.offset -= start_shift;
   hsp->subject.gapped_start -= start_shift;
   *translation_buffer_ptr = translation_buffer;
   *start_shift_ptr = start_shift;

   if (!is_ooframe) {
      subject = translation_buffer + 1;
   } else {
      subject = translation_buffer + CODON_LENGTH;
   }
   *subject_ptr = subject;
}

/** Check whether an HSP is already contain within another higher scoring HSP.
 * "Containment" is defined by the macro CONTAINED_IN_HSP.  
 * the implicit assumption here is that HSP's are sorted by score
 * The main goal of this routine is to eliminate double gapped extensions of HSP's.
 *
 * @param hsp_array Full Array of all HSP's found so far. [in]
 * @param hsp HSP to be compared to other HSP's [in]
 * @param max_index compare above HSP to all HSP's in hsp_array up to max_index [in]
 * @param is_ooframe true if out-of-frame gapped alignment (blastx and tblastn only). [in]
 */
static Boolean
HSPContainedInHSPCheck(BlastHSP** hsp_array, BlastHSP* hsp, Int4 max_index, Boolean is_ooframe)
{
      BlastHSP* hsp1;
      Boolean delete_hsp=FALSE;
      Boolean hsp_start_is_contained=FALSE;
      Boolean hsp_end_is_contained=FALSE;
      Int4 index;

      for (index=0; index<max_index; index++) {
         hsp_start_is_contained = FALSE;
         hsp_end_is_contained = FALSE;
         
         hsp1 = hsp_array[index];
         if (hsp1 == NULL)
            continue;
         
         if(is_ooframe) {
            if (SIGN(hsp1->query.frame) != SIGN(hsp->query.frame))
               continue;
         } else {
            if (hsp->context != hsp1->context)
               continue;
         }
         
         if (CONTAINED_IN_HSP(hsp1->query.offset, hsp1->query.end, hsp->query.offset, hsp1->subject.offset, hsp1->subject.end, hsp->subject.offset) == TRUE) {
            if (SIGN(hsp1->query.frame) == SIGN(hsp->query.frame) &&
                SIGN(hsp1->subject.frame) == SIGN(hsp->subject.frame))
               hsp_start_is_contained = TRUE;
         }
         if (CONTAINED_IN_HSP(hsp1->query.offset, hsp1->query.end, hsp->query.end, hsp1->subject.offset, hsp1->subject.end, hsp->subject.end) == TRUE) {
            if (SIGN(hsp1->query.frame) == SIGN(hsp->query.frame) &&
                SIGN(hsp1->subject.frame) == SIGN(hsp->subject.frame))
               hsp_end_is_contained = TRUE;
         }
         if (hsp_start_is_contained && hsp_end_is_contained && hsp->score <= hsp1->score) {
            delete_hsp = TRUE;
            break;
         }
      }

      return delete_hsp;
}

/** Sets some values that will be in the "score" block of the SeqAlign.
 * The values set here are expect value and number of identical matches.
 *
 * @param query_info information about query. [in]
 * @param query query sequence as a raw string [in]
 * @param hsp HPS to operate on [in][out]
 * @param subject database sequence as a raw string [in]
 * @param program_number which program [in]
 * @param sbp the scoring information [in]
 * @param scoring_params Parameters for how to score matches. [in]
 * @param hit_params Determines which scores to save, and whether to calculate 
 *                   e-values. [in]
 */
static Boolean
HSPSetScores(BlastQueryInfo* query_info, Uint1* query, 
   Uint1* subject, BlastHSP* hsp, 
   EBlastProgramType program_number, BlastScoreBlk* sbp,
   const BlastScoringParameters* score_params,
   const BlastHitSavingParameters* hit_params)
{

   Boolean keep = TRUE;
   Int4 align_length = 0;
   double scale_factor = 1.0;
   BlastScoringOptions *score_options = score_params->options;
   BlastHitSavingOptions *hit_options = hit_params->options;
 
   /* For RPS BLAST only, we'll need to divide Lambda by the scaling factor
      for the e-value calculations, because scores are scaled; for PSI-BLAST
      Lambda is already divided by scaling factor, so there is no need to do 
      it again. In all other programs, scaling factor is 1 anyway. */
   if (program_number == eBlastTypeRpsBlast ||
       program_number == eBlastTypeRpsTblastn)
      scale_factor = score_params->scale_factor;

   /* Calculate alignment length and number of identical letters. 
      Do not get the number of identities if the query is not available */
   if (query != NULL) {
      if (score_options->is_ooframe) {
         Blast_HSPGetOOFNumIdentities(query, subject, hsp, program_number,
                                      &hsp->num_ident, &align_length);
      } else {
         Blast_HSPGetNumIdentities(query, subject, hsp, 
                                   score_options->gapped_calculation, 
                                   &hsp->num_ident, &align_length);
      }
   }

   /* Check whether this HSP passes the percent identity and minimal hit 
      length criteria. */
   if ((hsp->num_ident * 100 < 
        align_length * hit_options->percent_identity) ||
       align_length < hit_options->min_hit_length) {
      keep = FALSE;
   }
            
   if (keep == TRUE) {
      Int4 context;
      /* For RPS tblastn query and subject are switched, so context for Karlin 
         block should be derived from subject frame. */
      if (program_number == eBlastTypeRpsTblastn)
         context = FrameToContext(hsp->subject.frame);
      else
         context = hsp->context;

      /* If sum statistics is not used, calcualte e-values here. */
      if (!hit_params->link_hsp_params) {
         
         Blast_KarlinBlk** kbp;
         if (score_options->gapped_calculation)
            kbp = sbp->kbp_gap;
         else
            kbp = sbp->kbp;
         
         if (hit_options->phi_align) {
            Blast_HSPPHIGetEvalue(hsp, sbp);
         } else {
            /* Divide lambda by the scaling factor, so e-value is 
               calculated correctly from a scaled score. Since score
               is an integer, adjusting score before the e-value 
               calculation would have lead to loss of precision.*/
            kbp[context]->Lambda /= scale_factor;
            hsp->evalue = 
               BLAST_KarlinStoE_simple(hsp->score, kbp[context],
                  query_info->eff_searchsp_array[context]);
            kbp[context]->Lambda *= scale_factor;
         }
         if (hsp->evalue > hit_options->expect_value) {
            /* put in for comp. based stats. */
            keep = FALSE;
         }
      }

      /* only one alignment considered for blast[np]. */
      /* This may be changed by LinkHsps for blastx or tblastn. */
      hsp->num = 1;
      if (hit_options->longest_intron > 0) {
         /* For uneven version of LinkHsps, the individual e-values
            need to be calculated for each HSP. */
         hsp->evalue = 
            BLAST_KarlinStoE_simple(hsp->score, sbp->kbp_gap[context],
               query_info->eff_searchsp_array[context]);
      }

      /* remove any scaling of the calculated score */
      hsp->score = (Int4) ((hsp->score+(0.5*score_params->scale_factor)) / 
			   score_params->scale_factor);
   }

   return keep;
}

/** Adjusts offset if out-of-frame and negative frame, or if partial sequence used for extension.
 * @param hsp The hit to work on [in][out]
 * @param subject_blk information about database sequence [in]
 * @param is_ooframe true if out-of-frame (blastx or tblastn) used. [in]
 * @param start_shift amount of database sequence not used for extension. [in]
*/
static void
HSPAdjustSubjectOffset(BlastHSP* hsp, BLAST_SequenceBlk* subject_blk, Boolean is_ooframe, Int4 start_shift)
{

            if (is_ooframe) {
               /* Adjust subject offsets for negative frames */
               if (hsp->subject.frame < 0) {
                  Int4 strand_start = subject_blk->length + 1;
                  hsp->subject.offset -= strand_start;
                  hsp->subject.end -= strand_start;
                  hsp->subject.gapped_start -= strand_start;
                  hsp->gap_info->start2 -= strand_start;
               }
            } 

            /* Adjust subject offsets if shifted (partial) sequence was used 
               for extension */
            if (start_shift > 0) {
               hsp->subject.offset += start_shift;
               hsp->subject.end += start_shift;
               hsp->subject.gapped_start += start_shift;
               if (hsp->gap_info)
                  hsp->gap_info->start2 += start_shift;
            }

            return;
}

/** Check whether an HSP is already contained within another higher scoring HSP.
 * This is done AFTER the gapped alignment has been performed
 *
 * @param hsp_array Full Array of all HSP's found so far. [in][out]
 * @param hsp HSP to be compared to other HSP's [in]
 * @param max_index compare above HSP to all HSP's in hsp_array up to max_index [in]
 */
static Boolean
HSPCheckForDegenerateAlignments(BlastHSP** hsp_array, BlastHSP* hsp, Int4 max_index)
{
            BlastHSP* hsp2;
            Boolean keep=TRUE;
            Int4 index;

            for (index=0; index<max_index; index++) {
               hsp2 = hsp_array[index];
               if (hsp2 == NULL)
                  continue;
               
               /* Check if both HSP's start or end on the same diagonal 
                  (and are on same strands). */
               if (((hsp->query.offset == hsp2->query.offset &&
                     hsp->subject.offset == hsp2->subject.offset) ||
                    (hsp->query.end == hsp2->query.end &&
                     hsp->subject.end == hsp2->subject.end))  &&
                   hsp->context == hsp2->context &&
                   hsp->subject.frame == hsp2->subject.frame) {
                  if (hsp2->score > hsp->score) {
                     keep = FALSE;
                     break;
                  } else {
                     hsp_array[index] = Blast_HSPFree(hsp2);
                  }
               }
            }

            return keep;
}

/*
    Comments in blast_traceback.h
 */
Int2
Blast_TracebackFromHSPList(EBlastProgramType program_number, BlastHSPList* hsp_list, 
   BLAST_SequenceBlk* query_blk, BLAST_SequenceBlk* subject_blk, 
   BlastQueryInfo* query_info_in,
   BlastGapAlignStruct* gap_align, BlastScoreBlk* sbp, 
   const BlastScoringParameters* score_params,
   const BlastExtensionOptions* ext_options,
   const BlastHitSavingParameters* hit_params,
   const Uint1* gen_code_string)
{
   Int4 index;
   BlastHSP* hsp;
   Uint1* query,* subject;
   Int4 query_length, query_length_orig, subject_length_orig;
   Int4 subject_length=0;
   BlastHSP** hsp_array;
   Int4 q_start, s_start;
   BlastHitSavingOptions* hit_options = hit_params->options;
   BlastScoringOptions* score_options = score_params->options;
   Int4 context_offset;
   Uint1* translation_buffer = NULL;
   Int4* frame_offsets = NULL;
   Boolean partial_translation = FALSE;
   const Boolean k_is_ooframe = score_options->is_ooframe;
   const Boolean kGreedyTraceback = (ext_options->eTbackExt == eGreedyTbck);
   const Boolean kTranslateSubject = 
      (program_number == eBlastTypeTblastn ||
       program_number == eBlastTypeRpsTblastn); 
   BlastQueryInfo* query_info = query_info_in;
   Int4 offsets[2];

   if (hsp_list->hspcnt == 0) {
      return 0;
   }

   hsp_array = hsp_list->hsp_array;

   if (kTranslateSubject) {
      if (!gen_code_string && program_number != eBlastTypeRpsTblastn)
         return -1;

      if (k_is_ooframe) {
         SetUpSubjectTranslation(subject_blk, gen_code_string,
                                 NULL, NULL, &partial_translation);
         subject = subject_blk->oof_sequence + CODON_LENGTH;
         /* Mixed-frame sequence spans all 6 frames, i.e. both strands
            of the nucleotide sequence. However its start will also be 
            shifted by 3.*/
         subject_length = 2*subject_blk->length - 1;
      } else if (program_number == eBlastTypeRpsTblastn) {
	 translation_buffer = subject_blk->sequence - 1;
	 frame_offsets = query_info_in->context_offsets;
	 /* Create a local BlastQueryInfo structure for this subject sequence
	    that has been switched with the query. */
	 query_info = BlastMemDup(query_info_in, sizeof(BlastQueryInfo));
	 query_info->first_context = query_info->last_context = 0;
	 query_info->num_queries = 1;
	 offsets[0] = 0;
	 offsets[1] = query_blk->length + 1;
	 query_info->context_offsets = offsets; 
      } else {
         SetUpSubjectTranslation(subject_blk, gen_code_string,
            &translation_buffer, &frame_offsets, &partial_translation);
         /* subject and subject_length will be set later, for each HSP. */
      }
   } else {
      /* Subject is not translated */
      subject = subject_blk->sequence;
      subject_length = subject_blk->length;
   }

   for (index=0; index < hsp_list->hspcnt; index++) {
      hsp = hsp_array[index];
      if (program_number == eBlastTypeBlastx || 
          program_number == eBlastTypeTblastx) {
         Int4 context = hsp->context - hsp->context % 3;
         context_offset = query_info->context_offsets[context];
         query_length_orig = 
            query_info->context_offsets[context+3] - context_offset - 1;
         if (k_is_ooframe) {
            query = query_blk->oof_sequence + CODON_LENGTH + context_offset;
            query_length = query_length_orig;
         } else {
            query = query_blk->sequence + 
               query_info->context_offsets[hsp->context];
            query_length = BLAST_GetQueryLength(query_info, hsp->context);
         }
      } else {
         query = query_blk->sequence + 
            query_info->context_offsets[hsp->context];
         query_length = query_length_orig = 
            BLAST_GetQueryLength(query_info, hsp->context);
      }

      subject_length_orig = subject_blk->length;
      /* For RPS tblastn the "subject" length should be the original 
	 nucleotide query length, which can be calculated from the 
	 context offsets in query_info_in. */
      if (program_number == eBlastTypeRpsTblastn) {
	 if (hsp->subject.frame > 0) {
	    subject_length_orig = 
	       query_info_in->context_offsets[NUM_FRAMES/2] - 1;
	 } else {
	    subject_length_orig = query_info_in->context_offsets[NUM_FRAMES] -
	       query_info_in->context_offsets[NUM_FRAMES/2] - 1;
	 }
      }

      /* preliminary RPS blast alignments have not had
         the composition-based correction applied yet, so
         we cannot reliably check whether an HSP is contained
         within another */

      if (program_number == eBlastTypeRpsBlast ||
          !HSPContainedInHSPCheck(hsp_array, hsp, index, k_is_ooframe)) {

         Int4 start_shift = 0;
         Int4 adjusted_s_length;
         Uint1* adjusted_subject;

         if (kTranslateSubject) {
            if (!k_is_ooframe && !partial_translation) {
               Int4 context = FrameToContext(hsp->subject.frame);
               subject = translation_buffer + frame_offsets[context] + 1;
               subject_length = 
                  frame_offsets[context+1] - frame_offsets[context] - 1;
            } else if (partial_translation) {
               GetPartialSubjectTranslation(subject_blk, hsp, k_is_ooframe,
                  gen_code_string, &translation_buffer, &subject,
                  &subject_length, &start_shift);
            }
         }

         if (!hit_options->phi_align && !k_is_ooframe && 
             (((hsp->query.gapped_start == 0 && 
                hsp->subject.gapped_start == 0) ||
               !BLAST_CheckStartForGappedAlignment(hsp, query, 
                   subject, sbp)))) {
            Int4 max_offset = 
               BlastGetStartForGappedAlignment(query, subject, sbp,
                  hsp->query.offset, hsp->query.length,
                  hsp->subject.offset, hsp->subject.length);
            q_start = max_offset;
            s_start = 
               (hsp->subject.offset - hsp->query.offset) + max_offset;
            hsp->query.gapped_start = q_start;
            hsp->subject.gapped_start = s_start;
         } else {
            if(k_is_ooframe) {
               /* Code below should be investigated for possible
                  optimization for OOF */
               s_start = hsp->subject.gapped_start;
               q_start = hsp->query.gapped_start;
               gap_align->subject_start = 0;
               gap_align->query_start = 0;
            } else {
               q_start = hsp->query.gapped_start;
               s_start = hsp->subject.gapped_start;
            }
         }
         
         adjusted_s_length = subject_length;
         adjusted_subject = subject;

        /* Perform the gapped extension with traceback */
         if (hit_options->phi_align) {
            Int4 pat_length = GetPatternLengthFromBlastHSP(hsp);
            SavePatternLengthInGapAlignStruct(pat_length, gap_align);
            PHIGappedAlignmentWithTraceback(query, subject, gap_align, 
               score_params, q_start, s_start, query_length, subject_length);
         } else {
            if (!kTranslateSubject) {
               AdjustSubjectRange(&s_start, &adjusted_s_length, q_start, 
                                  query_length, &start_shift);
               adjusted_subject = subject + start_shift;
            }
            if (kGreedyTraceback) {
               BLAST_GreedyGappedAlignment(query, adjusted_subject, 
                  query_length, adjusted_s_length, gap_align, 
                  score_params, q_start, s_start, FALSE, TRUE);
            } else {
               BLAST_GappedAlignmentWithTraceback(program_number, query, 
                  adjusted_subject, gap_align, score_params, q_start, s_start, 
                  query_length, adjusted_s_length);
            }
         }

         if (gap_align->score >= hit_params->cutoff_score) {
            Boolean keep=FALSE;
            Blast_HSPReset(gap_align->query_start, gap_align->query_stop,
                           gap_align->subject_start, gap_align->subject_stop, 
                           gap_align->score, &(gap_align->edit_block), hsp);

            /* FIXME not pretty, should be wrapped as function, done earlier or part of Blast_HSPReset. */
            if (hsp && hsp->gap_info) {
                   hsp->gap_info->frame1 = hsp->query.frame;
                   hsp->gap_info->frame2 = hsp->subject.frame;
                   hsp->gap_info->original_length1 = query_length_orig;
                   hsp->gap_info->original_length2 = subject_length_orig;
                   if (program_number == eBlastTypeBlastx)
                      hsp->gap_info->translate1 = TRUE;
                   if (program_number == eBlastTypeTblastn ||
                       program_number == eBlastTypeRpsTblastn)
                      hsp->gap_info->translate2 = TRUE;
            }

            if (kGreedyTraceback) {
               /* Low level greedy algorithm ignores ambiguities, so the score
                  needs to be reevaluated. */
               Blast_HSPReevaluateWithAmbiguities(hsp, query, adjusted_subject,
                  hit_options, score_params, query_info, sbp);
            }
            
            keep = HSPSetScores(query_info, query, adjusted_subject, hsp, 
                                program_number, sbp, score_params, hit_params);

            HSPAdjustSubjectOffset(hsp, subject_blk, k_is_ooframe, 
                                   start_shift);
            if (keep)
                keep = HSPCheckForDegenerateAlignments(hsp_array, hsp, index);

            if (!keep) {
               hsp_array[index] = Blast_HSPFree(hsp);
            }
         } else {
            /* Score is below threshold */
            gap_align->edit_block = GapEditBlockDelete(gap_align->edit_block);
            hsp_array[index] = Blast_HSPFree(hsp);
         }
      } else { 
         /* Contained within another HSP, delete. */
         hsp_array[index] = Blast_HSPFree(hsp);
      }
   } /* End loop on HSPs */

   if (program_number != eBlastTypeRpsTblastn) {
      if (translation_buffer) {
	 sfree(translation_buffer);
      }
      if (frame_offsets) {
	 sfree(frame_offsets);
      }
   }

   /* It's time to sort HSPs by e-value/score. */
   qsort(hsp_array, hsp_list->hspcnt, sizeof(BlastHSP*), 
	 Blast_HSPEvalueCompareCallback);
    
    /* Now try to detect simular alignments */

    BLASTCheckHSPInclusion(hsp_array, hsp_list->hspcnt, k_is_ooframe);
    Blast_HSPListPurgeNullHSPs(hsp_list);
    
    /* Relink and rereap the HSP list, if needed. */
    if (hit_params->link_hsp_params) {
       BLAST_LinkHsps(program_number, hsp_list, query_info, subject_blk,
                      sbp, hit_params->link_hsp_params, 
                      score_options->gapped_calculation);
       Blast_HSPListReapByEvalue(hsp_list, hit_options);
       /* Sort HSPs again by e-value/score. */
       qsort(hsp_array, hsp_list->hspcnt, sizeof(BlastHSP*), 
	 Blast_HSPEvalueCompareCallback);
    }
    
    /* Free the local query_info structure, if necessary (RPS tblastn only) */
    if (query_info != query_info_in)
       sfree(query_info);

    /* Remove extra HSPs if there is a user proveded limit on the number 
       of HSPs per database sequence */
    if (hit_options->hsp_num_max > 0 && 
        hit_options->hsp_num_max < hsp_list->hspcnt) {
       for (index=hit_options->hsp_num_max; index<hsp_list->hspcnt; ++index) {
          hsp_array[index] = Blast_HSPFree(hsp_array[index]);
       }
       hsp_list->hspcnt = hit_options->hsp_num_max;
    }

    return 0;
}

Uint1 Blast_TracebackGetEncoding(EBlastProgramType program_number) 
{
   Uint1 encoding;

   switch (program_number) {
   case eBlastTypeBlastn:
      encoding = BLASTNA_ENCODING;
      break;
   case eBlastTypeBlastp:
   case eBlastTypeRpsBlast:
      encoding = BLASTP_ENCODING;
      break;
   case eBlastTypeBlastx:
      encoding = BLASTP_ENCODING;
      break;
   case eBlastTypeTblastn:
   case eBlastTypeRpsTblastn:
      encoding = NCBI4NA_ENCODING;
      break;
   case eBlastTypeTblastx:
      encoding = NCBI4NA_ENCODING;
      break;
   default:
      encoding = ERROR_ENCODING;
      break;
   }
   return encoding;
}

/** Delete extra subject sequences hits, if after-traceback hit list size is
 * smaller than preliminary hit list size.
 * @param results All results after traceback, assumed already sorted by best
 *                e-value [in] [out]
 * @param hitlist_size Final hit list size [in]
 */
static void 
BlastPruneExtraHits(BlastHSPResults* results, Int4 hitlist_size)
{
   Int4 query_index, subject_index;
   BlastHitList* hit_list;

   for (query_index = 0; query_index < results->num_queries; ++query_index) {
      if (!(hit_list = results->hitlist_array[query_index]))
         continue;
      for (subject_index = hitlist_size;
           subject_index < hit_list->hsplist_count; ++subject_index) {
         hit_list->hsplist_array[subject_index] = 
            Blast_HSPListFree(hit_list->hsplist_array[subject_index]);
      }
      hit_list->hsplist_count = MIN(hit_list->hsplist_count, hitlist_size);
   }
}

Int2 BLAST_ComputeTraceback(EBlastProgramType program_number, BlastHSPStream* hsp_stream, 
        BLAST_SequenceBlk* query, BlastQueryInfo* query_info, 
        const BlastSeqSrc* seq_src, BlastGapAlignStruct* gap_align,
        BlastScoringParameters* score_params,
        const BlastExtensionParameters* ext_params,
        BlastHitSavingParameters* hit_params,
        BlastEffectiveLengthsParameters* eff_len_params,
        const BlastDatabaseOptions* db_options,
        const PSIBlastOptions* psi_options,
        BlastHSPResults** results_out)
{
   Int2 status = 0;
   BlastHSPResults* results = NULL;
   BlastHSPList* hsp_list = NULL;
   BlastScoreBlk* sbp;
   Uint1 encoding;
   GetSeqArg seq_arg;
   Uint1* gen_code_string = NULL;
 
   if (!query_info || !seq_src || !hsp_stream || !results_out) {
      return 0;
   }
   
   /* Set the raw X-dropoff value for the final gapped extension with 
      traceback */
   gap_align->gap_x_dropoff = ext_params->gap_x_dropoff_final;

   sbp = gap_align->sbp;
  
   if (db_options)
      gen_code_string = db_options->gen_code_string;
 
   encoding = Blast_TracebackGetEncoding(program_number);
   memset((void*) &seq_arg, 0, sizeof(seq_arg));

   Blast_HSPResultsInit(query_info->num_queries, &results);

   if (program_number == eBlastTypeBlastp && 
         (ext_params->options->compositionBasedStats == TRUE || 
            ext_params->options->eTbackExt == eSmithWatermanTbck)) {
         Kappa_RedoAlignmentCore(query, query_info, sbp, hsp_stream, seq_src, 
           score_params, ext_params, hit_params, psi_options, results); 
   } else {
      Boolean perform_traceback = 
         (score_params->options->gapped_calculation && 
          (ext_params->options->ePrelimGapExt != eGreedyWithTracebackExt) &&
          (ext_params->options->eTbackExt != eSkipTbck));
         
      while (BlastHSPStreamRead(hsp_stream, &hsp_list) 
             != kBlastHSPStream_Eof) {

	 /* Make sure the HSPs in this HSP list are sorted. */
	 Blast_HSPListCheckIfSorted(hsp_list);

         /* Perform traceback here, if necessary. */
         if (perform_traceback) {
            seq_arg.oid = hsp_list->oid;
            seq_arg.encoding = encoding;
            BlastSequenceBlkClean(seq_arg.seq);
            if (BLASTSeqSrcGetSequence(seq_src, (void*) &seq_arg) < 0)
               continue;
      
            if (BLASTSeqSrcGetTotLen(seq_src) == 0) {
               /* This is not a database search, so effective search spaces
                  need to be recalculated based on this subject sequence 
                  length */
               if ((status = BLAST_OneSubjectUpdateParameters(program_number, 
                                seq_arg.seq->length, score_params->options, 
                                query_info, sbp, ext_params, hit_params, 
                                NULL, eff_len_params)) != 0)
                  return status;
            }

            Blast_TracebackFromHSPList(program_number, hsp_list, query, 
               seq_arg.seq, query_info, gap_align, sbp, score_params, 
               ext_params->options, hit_params, gen_code_string);
            BLASTSeqSrcRetSequence(seq_src, (void*)&seq_arg);
         
            /* Recalculate the bit scores, as they might have changed. */
            Blast_HSPListGetBitScores(hsp_list, 
               score_params->options->gapped_calculation, sbp);
         }
         
         Blast_HSPResultsInsertHSPList(results, hsp_list, 
                                       hit_params->options->hitlist_size);
      }
   }

   /* Re-sort the hit lists according to their best e-values, because
      they could have changed. Only do this for a database search. */
   if (BLASTSeqSrcGetTotLen(seq_src) > 0)
      Blast_HSPResultsSortByEvalue(results);

   /* Eliminate extra hits from results, if preliminary hit list size is larger
      than the final hit list size */
   if (hit_params->options->hitlist_size < 
       hit_params->options->prelim_hitlist_size)
      BlastPruneExtraHits(results, hit_params->options->hitlist_size);

   BlastSequenceBlkFree(seq_arg.seq);

   *results_out = results;

   return status;
}

#define SWAP(a, b) {tmp = (a); (a) = (b); (b) = tmp; }

static void 
Blast_HSPRPSUpdate(BlastHSP *hsp)
{
   Int4 tmp;
   GapEditBlock *gap_info = hsp->gap_info;
   GapEditScript *esp;

   if (gap_info == NULL)
      return;

   SWAP(gap_info->start1, gap_info->start2);
   SWAP(gap_info->length1, gap_info->length2);
   SWAP(gap_info->original_length1, gap_info->original_length2);
   SWAP(gap_info->frame1, gap_info->frame2);
   SWAP(gap_info->translate1, gap_info->translate2);

   esp = gap_info->esp;
   while (esp != NULL) {
      if (esp->op_type == eGapAlignIns)
          esp->op_type = eGapAlignDel;
      else if (esp->op_type == eGapAlignDel)
          esp->op_type = eGapAlignIns;

      esp = esp->next;
   }
}

/** Switches back the query and subject in all HSPs in an HSP list; also
 * reassigns contexts to indicate query context, needed to pick correct
 * Karlin block later in the code.
 * @param program Program type: RPS or RPS tblastn [in]
 * @param hsplist List of HSPs [in] [out]
 */
static void 
Blast_HSPListRPSUpdate(EBlastProgramType program, BlastHSPList *hsplist)
{
   Int4 i;
   BlastHSP **hsp;
   BlastSeg tmp;

   hsp = hsplist->hsp_array;
   for (i = 0; i < hsplist->hspcnt; i++) {

      /* switch query and subject offsets (which are
         already in local coordinates) */

      tmp = hsp[i]->query;
      hsp[i]->query = hsp[i]->subject;
      hsp[i]->subject = tmp;

      /* Change the traceback information to reflect the
         query and subject sequences getting switched */

      Blast_HSPRPSUpdate(hsp[i]);

      /* If query was nucleotide, set context, because it is needed in order 
	 to pick correct Karlin block for calculating bit scores. */
      if (program == eBlastTypeRpsTblastn)
	 hsp[i]->context = FrameToContext(hsp[i]->query.frame);
   }
}

#define RPS_K_MULT 1.2

Int2 BLAST_RPSTraceback(EBlastProgramType program_number, 
        BlastHSPStream* hsp_stream, 
        BLAST_SequenceBlk* concat_db, BlastQueryInfo* concat_db_info, 
        BLAST_SequenceBlk* query, BlastQueryInfo* query_info, 
        BlastGapAlignStruct* gap_align,
        const BlastScoringParameters* score_params,
        const BlastExtensionParameters* ext_params,
        BlastHitSavingParameters* hit_params,
        const BlastDatabaseOptions* db_options,
        const double* karlin_k,
        BlastHSPResults** results_out)
{
   Int2 status = 0;
   BlastHSPList* hsp_list;
   BlastScoreBlk* sbp;
   Int4 **orig_pssm;
   BLAST_SequenceBlk one_db_seq;
   Int4 *db_seq_start;
   BlastHSPResults* results = NULL;
   
   if (!hsp_stream || !concat_db_info || !concat_db || !results_out) {
      return 0;
   }
   
   /* Set the raw X-dropoff value for the final gapped extension with 
      traceback */
   gap_align->gap_x_dropoff = ext_params->gap_x_dropoff_final;

   sbp = gap_align->sbp;
   orig_pssm = gap_align->sbp->posMatrix;

   Blast_HSPResultsInit(query_info->num_queries, &results);

   while (BlastHSPStreamRead(hsp_stream, &hsp_list) 
          != kBlastHSPStream_Eof) {
      if (!hsp_list)
         continue;

      /* Make sure the HSPs in this HSP list are sorted. */
      Blast_HSPListCheckIfSorted(hsp_list);

      /* If traceback should be skipped, just save the HSP list into the results
         structure. */
      if (ext_params->options->eTbackExt == eSkipTbck) {
         /* Save this HSP list in the results structure. */
         Blast_HSPResultsInsertHSPList(results, hsp_list, 
                                       hit_params->options->hitlist_size);
         continue;
      }

      /* pick out one of the sequences from the concatenated
         DB (given by the OID of this HSPList). The sequence
         size does not include the trailing NULL */
      
      db_seq_start = &concat_db_info->context_offsets[hsp_list->oid];
      memset(&one_db_seq, 0, sizeof(one_db_seq));
      one_db_seq.sequence = NULL;
      one_db_seq.length = db_seq_start[1] - db_seq_start[0] - 1;
      
      /* Update the statistics for this database sequence
         (if not a translated search) */
      
      if (program_number == eBlastTypeRpsTblastn) {
         sbp->posMatrix = orig_pssm + db_seq_start[0];
      } else {
         /* replace the PSSM and the Karlin values for this DB sequence. */
         sbp->posMatrix = 
            RPSCalculatePSSM(score_params->scale_factor,
                             query->length, query->sequence, one_db_seq.length,
                             orig_pssm + db_seq_start[0]);
         if (sbp->posMatrix == NULL)
            return -1;
         
         sbp->kbp_gap[0]->K = RPS_K_MULT * karlin_k[hsp_list->oid];
         sbp->kbp_gap[0]->logK = log(RPS_K_MULT * karlin_k[hsp_list->oid]);
      }

      /* compute the traceback information and calculate E values
         for all HSPs in the list */
      
      Blast_TracebackFromHSPList(program_number, hsp_list, &one_db_seq, 
         query, query_info, gap_align, sbp, score_params, 
         ext_params->options, hit_params, db_options->gen_code_string);

      if (program_number != eBlastTypeRpsTblastn)
         _PSIDeallocateMatrix((void**)sbp->posMatrix, one_db_seq.length+1);

      if (hsp_list->hspcnt == 0) {
         hsp_list = Blast_HSPListFree(hsp_list);
         continue;
      }

      /* Revert query and subject to their traditional meanings. 
         This involves switching the offsets around and reversing
         any traceback information */
      Blast_HSPListRPSUpdate(program_number, hsp_list);

      /* Calculate and fill the bit scores. This is the only time when 
         they are calculated. */
      Blast_HSPListGetBitScores(hsp_list, 
         score_params->options->gapped_calculation, sbp);

      /* Save this HSP list in the results structure. */
      Blast_HSPResultsInsertHSPList(results, hsp_list, 
                                    hit_params->options->hitlist_size);
   }

   /* The traceback calculated the E values, so it's safe
      to sort the results now */
   Blast_HSPResultsSortByEvalue(results);

   *results_out = results;

   gap_align->sbp->posMatrix = orig_pssm;
   return status;
}

Int2 
Blast_RunTracebackSearch(EBlastProgramType program, 
   BLAST_SequenceBlk* query, BlastQueryInfo* query_info, 
   const BlastSeqSrc* seq_src, const BlastScoringOptions* score_options,
   const BlastExtensionOptions* ext_options,
   const BlastHitSavingOptions* hit_options,
   const BlastEffectiveLengthsOptions* eff_len_options,
   const BlastDatabaseOptions* db_options, 
   const PSIBlastOptions* psi_options, BlastScoreBlk* sbp,
   BlastHSPStream* hsp_stream, BlastHSPResults** results)
{
   Int2 status = 0;
   BlastScoringParameters* score_params = NULL; /**< Scoring parameters */
   BlastExtensionParameters* ext_params = NULL; /**< Gapped extension 
                                                   parameters */
   BlastHitSavingParameters* hit_params = NULL; /**< Hit saving parameters*/
   BlastEffectiveLengthsParameters* eff_len_params = NULL; /**< Parameters
                                        for effective lengths calculations */
   BlastGapAlignStruct* gap_align = NULL; /**< Gapped alignment structure */

   status = 
      BLAST_GapAlignSetUp(program, seq_src, score_options, eff_len_options, 
         ext_options, hit_options, query_info, sbp, &score_params, 
         &ext_params, &hit_params, &eff_len_params, &gap_align);
   if (status)
      return status;

   /* Prohibit any subsequent writing to the HSP stream. */
   BlastHSPStreamClose(hsp_stream);

   status = 
      BLAST_ComputeTraceback(program, hsp_stream, query, query_info,
         seq_src, gap_align, score_params, ext_params, hit_params,
         eff_len_params, db_options, psi_options, results);

   /* Do not destruct score block here */
   gap_align->sbp = NULL;
   BLAST_GapAlignStructFree(gap_align);

   score_params = BlastScoringParametersFree(score_params);
   hit_params = BlastHitSavingParametersFree(hit_params);
   ext_params = BlastExtensionParametersFree(ext_params);
   eff_len_params = BlastEffectiveLengthsParametersFree(eff_len_params);
   return status;
}
