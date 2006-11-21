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
 */

/** @file blast_aalookup.c
 * Functions interacting with the protein BLAST lookup table.
 */

#include <algo/blast/core/blast_aalookup.h>
#include <algo/blast/core/lookup_util.h>
#include <algo/blast/core/blast_encoding.h>

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] =
    "$Id$";
#endif                          /* SKIP_DOXYGEN_PROCESSING */

/** Structure containing information needed for adding neighboring words. 
 */
typedef struct NeighborInfo {
    BlastAaLookupTable *lookup; /**< Lookup table */
    Uint1 *query_word;   /**< the word whose neighbors we are computing */
    Uint1 *subject_word; /**< the computed neighboring word */
    Int4 alphabet_size;  /**< number of letters in the alphabet */
    Int4 wordsize;       /**< number of residues in a word */
    Int4 charsize;       /**< number of bits in a residue */
    Int4 **matrix;       /**< the substitution matrix */
    Int4 *row_max;       /**< maximum possible score for each row of the matrix */
    Int4 *offset_list;   /**< list of offsets where the word occurs in the query */
    Int4 threshold;      /**< the score threshold for neighboring words */
    Int4 query_bias;     /**< bias all stored offsets for multiple queries */
} NeighborInfo;

/**
 * Index a query sequence; i.e. fill a lookup table with the offsets
 * of query words
 *
 * @param lookup the lookup table [in/modified]
 * @param matrix the substitution matrix [in]
 * @param query the query sequence [in]
 * @param query_bias number added to each offset put into lookup table
 *                      (ordinarily 0; a nonzero value allows a succession of
 *                      query sequences to update the same lookup table)
 * @param locations the list of ranges of query offsets to examine 
 *                  for indexing [in]
 */
static void s_AddNeighboringWords(BlastAaLookupTable * lookup, Int4 ** matrix,
                                  BLAST_SequenceBlk * query, Int4 query_bias,
                                  BlastSeqLoc * location);

/**
 * A position-specific version of AddNeighboringWords. Note that
 * only the score matrix matters for indexing purposes, so an
 * actual query sequence is unneccessary
 *
 * @param lookup the lookup table [in/modified]
 * @param matrix the substitution matrix [in]
 * @param query_bias number added to each offset put into lookup table
 *                      (ordinarily 0; a nonzero value allows a succession of
 *                      query sequences to update the same lookup table)
 * @param locations the list of ranges of query offsets to examine for indexing
 */
static void s_AddPSSMNeighboringWords(BlastAaLookupTable * lookup, 
                                      Int4 ** matrix, Int4 query_bias, 
                                      BlastSeqLoc * location);

/** Add neighboring words to the lookup table.
 * @param lookup Pointer to the lookup table.
 * @param matrix Pointer to the substitution matrix.
 * @param query Pointer to the query sequence.
 * @param offset_list list of offsets where the word occurs in the query
 * @param query_bias bias all stored offsets for multiple queries
 * @param row_max maximum possible score for each row of the matrix
 */
static void s_AddWordHits(BlastAaLookupTable * lookup,
                          Int4 ** matrix, Uint1 * query,
                          Int4 * offset_list, Int4 query_bias, 
                          Int4 * row_max);

/** Add neighboring words to the lookup table using NeighborInfo structure.
 * @param info Pointer to the NeighborInfo structure.
 * @param score The partial sum of the score.
 * @param current_pos The current offset.
 */
static void s_AddWordHitsCore(NeighborInfo * info, Int4 score, 
                              Int4 current_pos);

/** Add neighboring words to the lookup table in case of a position-specific 
 * matrix.
 * @param lookup Pointer to the lookup table.
 * @param matrix The position-specific matrix.
 * @param query_bias bias all stored offsets for multiple queries
 * @param row_max maximum possible score for each row of the matrix
 */
static void s_AddPSSMWordHits(BlastAaLookupTable * lookup,
                            Int4 ** matrix, Int4 query_bias, Int4 * row_max);

/** Add neighboring words to the lookup table in case of a position-specific 
 * matrix, using NeighborInfo structure.
 * @param info Pointer to the NeighborInfo structure.
 * @param score The partial sum of the score.
 * @param current_pos The current offset.
 */
static void s_AddPSSMWordHitsCore(NeighborInfo * info,
                             Int4 score, Int4 current_pos);


Int2 RPSLookupTableNew(const BlastRPSInfo * info, BlastRPSLookupTable * *lut)
{
    Int4 i;
    BlastRPSLookupFileHeader *lookup_header;
    BlastRPSProfileHeader *profile_header;
    BlastRPSLookupTable *lookup = *lut =
        (BlastRPSLookupTable *) calloc(1, sizeof(BlastRPSLookupTable));
    Int4 *pssm_start;
    Int4 num_pssm_rows;
    PV_ARRAY_TYPE *pv;

    ASSERT(info != NULL);

    /* Fill in the lookup table information. */

    lookup_header = info->lookup_header;
    if (lookup_header->magic_number != RPS_MAGIC_NUM &&
        lookup_header->magic_number != RPS_MAGIC_NUM_28)
        return -1;

    /* set the alphabet size. Use hardwired numbers, since we cannot rely on
       #define'd constants matching up to the sizes implicit in disk files */
    if (lookup_header->magic_number == RPS_MAGIC_NUM)
        lookup->alphabet_size = 26;
    else
        lookup->alphabet_size = 28;

    lookup->wordsize = BLAST_WORDSIZE_PROT;
    lookup->charsize = ilog2(lookup->alphabet_size) + 1;
    lookup->backbone_size = 1 << (lookup->wordsize * lookup->charsize);
    lookup->mask = lookup->backbone_size - 1;
    lookup->rps_backbone = (RPSBackboneCell *) ((Uint1 *) lookup_header +
                                                lookup_header->
                                                start_of_backbone);
    lookup->overflow =
        (Int4 *) ((Uint1 *) lookup_header + lookup_header->start_of_backbone +
                  (lookup->backbone_size + 1) * sizeof(RPSBackboneCell));
    lookup->overflow_size = lookup_header->overflow_hits;

    /* fill in the pv_array */

    pv = lookup->pv = (PV_ARRAY_TYPE *) calloc(
                            (lookup->backbone_size >> PV_ARRAY_BTS),
                            sizeof(PV_ARRAY_TYPE));

    for (i = 0; i < lookup->backbone_size; i++) {
        if (lookup->rps_backbone[i].num_used > 0) {
            PV_SET(pv, i, PV_ARRAY_BTS);
        }
    }

    /* Fill in the PSSM information */

    profile_header = info->profile_header;
    if (profile_header->magic_number != RPS_MAGIC_NUM &&
        profile_header->magic_number != RPS_MAGIC_NUM_28)
        return -2;

    lookup->rps_seq_offsets = profile_header->start_offsets;
    lookup->num_profiles = profile_header->num_profiles;
    num_pssm_rows = lookup->rps_seq_offsets[lookup->num_profiles];
    lookup->rps_pssm = (Int4 **) malloc((num_pssm_rows + 1) * sizeof(Int4 *));
    pssm_start = profile_header->start_offsets + lookup->num_profiles + 1;

    for (i = 0; i < num_pssm_rows + 1; i++) {
        lookup->rps_pssm[i] = pssm_start;
        pssm_start += lookup->alphabet_size;
    }

    /* divide the concatenated database into regions of size RPS_BUCKET_SIZE. 
       bucket_array will then be used to organize offsets retrieved from the
       lookup table in order to increase cache reuse */

    lookup->num_buckets = num_pssm_rows / RPS_BUCKET_SIZE + 1;
    lookup->bucket_array = (RPSBucket *) malloc(lookup->num_buckets *
                                                sizeof(RPSBucket));
    for (i = 0; i < lookup->num_buckets; i++) {
        RPSBucket *bucket = lookup->bucket_array + i;
        bucket->num_filled = 0;
        bucket->num_alloc = 1000;
        bucket->offset_pairs = (BlastOffsetPair *) malloc(bucket->num_alloc *
                                                          sizeof
                                                          (BlastOffsetPair));
    }

    return 0;
}

BlastRPSLookupTable *RPSLookupTableDestruct(BlastRPSLookupTable * lookup)
{
    /* The following will only free memory that was allocated by
       RPSLookupTableNew. */
    Int4 i;
    for (i = 0; i < lookup->num_buckets; i++)
        sfree(lookup->bucket_array[i].offset_pairs);
    sfree(lookup->bucket_array);

    sfree(lookup->rps_pssm);
    sfree(lookup->pv);
    sfree(lookup);
    return NULL;
}

Int4 BlastAaLookupTableNew(const LookupTableOptions * opt,
                           BlastAaLookupTable * *lut)
{
    Int4 i;
    BlastAaLookupTable *lookup = *lut =
        (BlastAaLookupTable *) calloc(1, sizeof(BlastAaLookupTable));

    ASSERT(lookup != NULL);

    lookup->charsize = ilog2(BLASTAA_SIZE) + 1;
    lookup->word_length = opt->word_size;

    for (i = 0; i < lookup->word_length; i++)
        lookup->backbone_size |= (BLASTAA_SIZE - 1) << (i * lookup->charsize);
    lookup->backbone_size++;

    lookup->mask = (1 << (opt->word_size * lookup->charsize)) - 1;
    lookup->alphabet_size = BLASTAA_SIZE;
    lookup->threshold = opt->threshold;
    lookup->thin_backbone =
        (Int4 **) calloc(lookup->backbone_size, sizeof(Int4 *));
    ASSERT(lookup->thin_backbone != NULL);

    lookup->overflow = NULL;
    return 0;
}


BlastAaLookupTable *BlastAaLookupTableDestruct(BlastAaLookupTable * lookup)
{
    sfree(lookup->thick_backbone);
    sfree(lookup->overflow);
    sfree(lookup->pv);
    sfree(lookup);
    return NULL;
}


Int4 BlastAaLookupFinalize(BlastAaLookupTable * lookup)
{
    Int4 i;
    Int4 overflow_cells_needed = 0;
    Int4 overflow_cursor = 0;
    Int4 longest_chain = 0;
    PV_ARRAY_TYPE *pv;
#ifdef LOOKUP_VERBOSE
    Int4 backbone_occupancy = 0;
    Int4 thick_backbone_occupancy = 0;
    Int4 num_overflows = 0;
#endif

/* allocate the new lookup table */
    lookup->thick_backbone = (AaLookupBackboneCell *)
        calloc(lookup->backbone_size, sizeof(AaLookupBackboneCell));
    ASSERT(lookup->thick_backbone != NULL);

    /* allocate the pv_array */
    pv = lookup->pv = (PV_ARRAY_TYPE *) calloc(
                                  (lookup->backbone_size >> PV_ARRAY_BTS) + 1,
                                  sizeof(PV_ARRAY_TYPE));
    ASSERT(pv != NULL);

    /* find out how many cells need the overflow array */
    for (i = 0; i < lookup->backbone_size; i++) {
        if (lookup->thin_backbone[i] != NULL) {
            if (lookup->thin_backbone[i][1] > AA_HITS_PER_CELL)
                overflow_cells_needed += lookup->thin_backbone[i][1];

            if (lookup->thin_backbone[i][1] > longest_chain)
                longest_chain = lookup->thin_backbone[i][1];
        }
    }

    lookup->longest_chain = longest_chain;

    /* allocate the overflow array */
    if (overflow_cells_needed > 0) {
        lookup->overflow =
            (Int4 *) calloc(overflow_cells_needed, sizeof(Int4));
        ASSERT(lookup->overflow != NULL);
    }

/* for each position in the lookup table backbone, */
    for (i = 0; i < lookup->backbone_size; i++) {
        /* if there are hits there, */
        if (lookup->thin_backbone[i] != NULL) {
            /* set the corresponding bit in the pv_array */
            PV_SET(pv, i, PV_ARRAY_BTS);
#ifdef LOOKUP_VERBOSE
            backbone_occupancy++;
#endif

            /* if there are three or fewer hits, */
            if ((lookup->thin_backbone[i])[1] <= AA_HITS_PER_CELL)
                /* copy them into the thick_backbone cell */
            {
                Int4 j;
#ifdef LOOKUP_VERBOSE
                thick_backbone_occupancy++;
#endif

                lookup->thick_backbone[i].num_used =
                    lookup->thin_backbone[i][1];

                for (j = 0; j < lookup->thin_backbone[i][1]; j++)
                    lookup->thick_backbone[i].payload.entries[j] =
                        lookup->thin_backbone[i][j + 2];
            } else
                /* more than three hits; copy to overflow array */
            {
                Int4 j;

#ifdef LOOKUP_VERBOSE
                num_overflows++;
#endif

                lookup->thick_backbone[i].num_used =
                    lookup->thin_backbone[i][1];
                lookup->thick_backbone[i].payload.overflow_cursor =
                    overflow_cursor;
                for (j = 0; j < lookup->thin_backbone[i][1]; j++) {
                    lookup->overflow[overflow_cursor] =
                        lookup->thin_backbone[i][j + 2];
                    overflow_cursor++;
                }
            }

            /* done with this chain- free it */
            sfree(lookup->thin_backbone[i]);
            lookup->thin_backbone[i] = NULL;
        }

        else
            /* no hits here */
        {
            lookup->thick_backbone[i].num_used = 0;
        }
    }                           /* end for */

    lookup->overflow_size = overflow_cursor;

/* done copying hit info- free the backbone */
    sfree(lookup->thin_backbone);
    lookup->thin_backbone = NULL;

#ifdef LOOKUP_VERBOSE
    printf("backbone size: %d\n", lookup->backbone_size);
    printf("backbone occupancy: %d (%f%%)\n", backbone_occupancy,
           100.0 * backbone_occupancy / lookup->backbone_size);
    printf("thick_backbone occupancy: %d (%f%%)\n",
           thick_backbone_occupancy,
           100.0 * thick_backbone_occupancy / lookup->backbone_size);
    printf("num_overflows: %d\n", num_overflows);
    printf("overflow size: %d\n", overflow_cells_needed);
    printf("longest chain: %d\n", longest_chain);
    printf("exact matches: %d\n", lookup->exact_matches);
    printf("neighbor matches: %d\n", lookup->neighbor_matches);
#endif

    return 0;
}

void BlastAaLookupIndexQuery(BlastAaLookupTable * lookup,
                             Int4 ** matrix,
                             BLAST_SequenceBlk * query,
                             BlastSeqLoc * location, 
                             Int4 query_bias)
{
    if (lookup->use_pssm) {
        s_AddPSSMNeighboringWords(lookup, matrix, query_bias, location);
    }
    else {
        ASSERT(query != NULL);
        s_AddNeighboringWords(lookup, matrix, query, query_bias, location);
    }
}

static void s_AddNeighboringWords(BlastAaLookupTable * lookup, Int4 ** matrix,
                                  BLAST_SequenceBlk * query, Int4 query_bias,
                                  BlastSeqLoc * location)
{
    Int4 i, j;
    Int4 **exact_backbone;
    Int4 row_max[BLASTAA_SIZE];

    ASSERT(lookup->alphabet_size <= BLASTAA_SIZE);

    /* Determine the maximum possible score for each row of the score matrix */

    for (i = 0; i < lookup->alphabet_size; i++) {
        row_max[i] = matrix[i][0];
        for (j = 1; j < lookup->alphabet_size; j++)
            row_max[i] = MAX(row_max[i], matrix[i][j]);
    }

    /* create an empty backbone */

    exact_backbone = (Int4 **) calloc(lookup->backbone_size, sizeof(Int4 *));

    /* find all the exact matches, grouping together all offsets of identical 
       query words. The query bias is not used here, since the next stage
       will need real offsets into the query sequence */

    BlastLookupIndexQueryExactMatches(exact_backbone, lookup->word_length,
                                      lookup->charsize, lookup->word_length,
                                      query, location);

    /* walk though the list of exact matches previously computed. Find
       neighboring words for entire lists at a time */

    for (i = 0; i < lookup->backbone_size; i++) {
        if (exact_backbone[i] != NULL) {
            s_AddWordHits(lookup, matrix, query->sequence,
                          exact_backbone[i], query_bias, row_max);
            sfree(exact_backbone[i]);
        }
    }

    sfree(exact_backbone);
}

static void s_AddWordHits(BlastAaLookupTable * lookup, Int4 ** matrix,
                        Uint1 * query, Int4 * offset_list,
                        Int4 query_bias, Int4 * row_max)
{
    Uint1 *w;
    Uint1 s[32];   /* larger than any possible wordsize */
    Int4 score;
    Int4 i;
    NeighborInfo info;

#ifdef LOOKUP_VERBOSE
    lookup->exact_matches += offset_list[1];
#endif

    /* All of the offsets in the list refer to the same query word. Thus,
       neighboring words only have to be found for the first offset in the
       list (since all other offsets would have the same neighbors) */

    w = query + offset_list[2];

    /* Compute the self-score of this word */

    score = matrix[w[0]][w[0]];
    for (i = 1; i < lookup->word_length; i++)
        score += matrix[w[i]][w[i]];

    /* If the self-score is above the threshold, then the neighboring
       computation will automatically add the word to the lookup table.
       Otherwise, either the score is too low or neighboring is not done at
       all, so that all of these exact matches must be explicitly added to
       the lookup table */

    if (lookup->threshold == 0 || score < lookup->threshold) {
        for (i = 0; i < offset_list[1]; i++) {
            BlastLookupAddWordHit(lookup->thin_backbone, lookup->word_length,
                                  lookup->charsize, w,
                                  query_bias + offset_list[i + 2]);
        }
    } else {
#ifdef LOOKUP_VERBOSE
        lookup->neighbor_matches -= offset_list[1];
#endif
    }

    /* check if neighboring words need to be found */

    if (lookup->threshold == 0)
        return;

    /* Set up the structure of information to be used during the recursion */

    info.lookup = lookup;
    info.query_word = w;
    info.subject_word = s;
    info.alphabet_size = lookup->alphabet_size;
    info.wordsize = lookup->word_length;
    info.charsize = lookup->charsize;
    info.matrix = matrix;
    info.row_max = row_max;
    info.offset_list = offset_list;
    info.threshold = lookup->threshold;
    info.query_bias = query_bias;

    /* compute the largest possible score that any neighboring word can have; 
       this maximum will gradually be replaced by exact scores as subject
       words are built up */

    score = row_max[w[0]];
    for (i = 1; i < lookup->word_length; i++)
        score += row_max[w[i]];

    s_AddWordHitsCore(&info, score, 0);
}

static void s_AddWordHitsCore(NeighborInfo * info, Int4 score, 
                              Int4 current_pos)
{
    Int4 alphabet_size = info->alphabet_size;
    Int4 threshold = info->threshold;
    Uint1 *query_word = info->query_word;
    Uint1 *subject_word = info->subject_word;
    Int4 *row;
    Int4 i;

    /* remove the maximum score of letters that align with the query letter
       at position 'current_pos'. Later code will align the entire alphabet
       with this letter, and compute the exact score each time. Also point to 
       the row of the score matrix corresponding to the query letter at
       current_pos */

    score -= info->row_max[query_word[current_pos]];
    row = info->matrix[query_word[current_pos]];

    if (current_pos == info->wordsize - 1) {

        /* The recursion has bottomed out, and we can produce complete
           subject words. Pass the entire alphabet through the last position
           in the subject word, then save the list of query offsets in all
           positions corresponding to subject words that yield a high enough
           score */

        Int4 *offset_list = info->offset_list;
        Int4 query_bias = info->query_bias;
        Int4 wordsize = info->wordsize;
        Int4 charsize = info->charsize;
        BlastAaLookupTable *lookup = info->lookup;
        Int4 j;

        for (i = 0; i < alphabet_size; i++) {
            if (score + row[i] >= threshold) {
                subject_word[current_pos] = i;
                for (j = 0; j < offset_list[1]; j++) {
                    BlastLookupAddWordHit(lookup->thin_backbone, wordsize,
                                          charsize, subject_word,
                                          query_bias + offset_list[j + 2]);
                }
#ifdef LOOKUP_VERBOSE
                lookup->neighbor_matches += offset_list[1];
#endif
            }
        }
        return;
    }

    /* Otherwise, pass the entire alphabet through position current_pos of
       the subject word, and recurse on all words that could possibly exceed
       the threshold later */

    for (i = 0; i < alphabet_size; i++) {
        if (score + row[i] >= threshold) {
            subject_word[current_pos] = i;
            s_AddWordHitsCore(info, score + row[i], current_pos + 1);
        }
    }
}

static void s_AddPSSMNeighboringWords(BlastAaLookupTable * lookup, 
                                      Int4 ** matrix, Int4 query_bias, 
                                      BlastSeqLoc * location)
{
    Int4 offset;
    Int4 i, j;
    BlastSeqLoc *loc;
    Int4 *row_max;
    Int4 wordsize = lookup->word_length;

    /* for PSSMs, we only have to track the maximum score of 'wordsize'
       matrix columns */

    row_max = (Int4 *) malloc(lookup->word_length * sizeof(Int4));
    ASSERT(row_max != NULL);

    for (loc = location; loc; loc = loc->next) {
        Int4 from = loc->ssr->left;
        Int4 to = loc->ssr->right - wordsize + 1;
        Int4 **row = matrix + from;

        /* prepare to start another run of adjacent query words. Find the
           maximum possible score for the first wordsize-1 rows of the PSSM */

        if (to >= from) {
            for (i = 0; i < wordsize - 1; i++) {
                row_max[i] = row[i][0];
                for (j = 1; j < lookup->alphabet_size; j++)
                    row_max[i] = MAX(row_max[i], row[i][j]);
            }
        }

        for (offset = from; offset <= to; offset++, row++) {
            /* find the maximum score of the next PSSM row */

            row_max[wordsize - 1] = row[wordsize - 1][0];
            for (i = 1; i < lookup->alphabet_size; i++)
                row_max[wordsize - 1] = MAX(row_max[wordsize - 1],
                                            row[wordsize - 1][i]);

            /* find all neighboring words */

            s_AddPSSMWordHits(lookup, row, offset + query_bias, row_max);

            /* shift the list of maximum scores over by one, to make room for 
               the next maximum in the next loop iteration */

            for (i = 0; i < wordsize - 1; i++)
                row_max[i] = row_max[i + 1];
        }
    }

    sfree(row_max);
}

static void s_AddPSSMWordHits(BlastAaLookupTable * lookup, Int4 ** matrix,
                              Int4 offset, Int4 * row_max)
{
    Uint1 s[32];   /* larger than any possible wordsize */
    Int4 score;
    Int4 i;
    NeighborInfo info;

    /* Set up the structure of information to be used during the recursion */

    info.lookup = lookup;
    info.query_word = NULL;
    info.subject_word = s;
    info.alphabet_size = lookup->alphabet_size;
    info.wordsize = lookup->word_length;
    info.charsize = lookup->charsize;
    info.matrix = matrix;
    info.row_max = row_max;
    info.offset_list = NULL;
    info.threshold = lookup->threshold;
    info.query_bias = offset;

    /* compute the largest possible score that any neighboring word can have; 
       this maximum will gradually be replaced by exact scores as subject
       words are built up */

    score = row_max[0];
    for (i = 1; i < lookup->word_length; i++)
        score += row_max[i];

    s_AddPSSMWordHitsCore(&info, score, 0);
}

static void s_AddPSSMWordHitsCore(NeighborInfo * info, Int4 score,
                                  Int4 current_pos)
{
    Int4 alphabet_size = info->alphabet_size;
    Int4 threshold = info->threshold;
    Uint1 *subject_word = info->subject_word;
    Int4 *row;
    Int4 i;

    /* remove the maximum score of letters that align with the query letter
       at position 'current_pos'. Later code will align the entire alphabet
       with this letter, and compute the exact score each time. Also point to 
       the row of the score matrix corresponding to the query letter at
       current_pos */

    score -= info->row_max[current_pos];
    row = info->matrix[current_pos];

    if (current_pos == info->wordsize - 1) {

        /* The recursion has bottomed out, and we can produce complete
           subject words. Pass the entire alphabet through the last position
           in the subject word, then save the query offset in all lookup
           table positions corresponding to subject words that yield a high
           enough score */

        Int4 offset = info->query_bias;
        Int4 wordsize = info->wordsize;
        Int4 charsize = info->charsize;
        BlastAaLookupTable *lookup = info->lookup;

        for (i = 0; i < alphabet_size; i++) {
            if (score + row[i] >= threshold) {
                subject_word[current_pos] = i;
                BlastLookupAddWordHit(lookup->thin_backbone, wordsize,
                                      charsize, subject_word, offset);
#ifdef LOOKUP_VERBOSE
                lookup->neighbor_matches++;
#endif
            }
        }
        return;
    }

    /* Otherwise, pass the entire alphabet through position current_pos of
       the subject word, and recurse on all words that could possibly exceed
       the threshold later */

    for (i = 0; i < alphabet_size; i++) {
        if (score + row[i] >= threshold) {
            subject_word[current_pos] = i;
            s_AddPSSMWordHitsCore(info, score + row[i], current_pos + 1);
        }
    }
}
