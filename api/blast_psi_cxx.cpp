static char const rcsid[] =
    "$Id$";
/* ===========================================================================
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
 * Author:  Christiam Camacho
 *
 */

/** @file blast_psi_cxx.cpp
 * Implementation of the C++ API for the PSI-BLAST PSSM generation engine.
 */

#include <ncbi_pch.hpp>
#include <sstream>

#include <algo/blast/api/blast_aux.hpp>
#include <algo/blast/api/blast_psi.hpp>
#include "blast_setup.hpp"

// Object includes
#include <objects/scoremat/Pssm.hpp>
#include <objects/scoremat/PssmParameters.hpp>
#include <objects/scoremat/PssmFinalData.hpp>
#include <objects/scoremat/PssmIntermediateData.hpp>
#include <objects/scoremat/PssmWithParameters.hpp>
#include <objects/scoremat/FormatRpsDbParameters.hpp>

// Core BLAST includes
#include <algo/blast/core/blast_options.h>
#include <algo/blast/core/blast_stat.h>
#include <algo/blast/core/blast_setup.h>
#include "../core/blast_psi_priv.h"

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)

CPssmEngine::CPssmEngine(IPssmInputData* input)
    : m_PssmInput(input)
{
    m_ScoreBlk.Reset(x_InitializeScoreBlock(m_PssmInput->GetQuery(),
                                            m_PssmInput->GetQueryLength()));
}

CPssmEngine::~CPssmEngine()
{
}

// This method is the core of this class. It delegates the extraction of
// information from the pairwise alignments into a multiple sequence alignment
// structure to its IPsiAlignmentProcessor strategy.
// Creating the PSSM is then delegated to the core PSSM engine.
// Afterwards the results are packaged in a scoremat (structure defined from
// the ASN.1 specification).
CRef<CPssmWithParameters>
CPssmEngine::Run()
{
    m_PssmInput->Process();

    // Note: currently there is no way for users to request diagnostics through
    // this interface
    PSIDiagnosticsRequest request;
    memset((void*) &request, 0, sizeof(request));
    request.frequency_ratios = true;

    CPSIMatrix pssm;
    CPSIDiagnosticsResponse diagnostics;
    int status = PSICreatePssmWithDiagnostics(m_PssmInput->GetData(),
                                              m_PssmInput->GetOptions(),
                                              m_ScoreBlk, 
                                              &request, 
                                              &pssm, 
                                              &diagnostics);
    if (status) {
        // FIXME: need to use core level perror-like facility
        ostringstream os;
        os << "Error code in PSSM engine: " << status;
        NCBI_THROW(CBlastException, eInternal, os.str());
    }

    // Convert core BLAST matrix structure into ASN.1 score matrix object
    CRef<CPssmWithParameters> retval(NULL);
    retval = x_PSIMatrix2Asn1(pssm, m_PssmInput->GetOptions(), diagnostics);

    return retval;
}

unsigned char*
CPssmEngine::x_GuardProteinQuery(const unsigned char* query,
                                 unsigned int query_length)
{
    ASSERT(query);

    unsigned char* retval = NULL;
    retval = (unsigned char*) malloc(sizeof(unsigned char)*(query_length + 2));
    if ( !retval ) {
        NCBI_THROW(CBlastException, eOutOfMemory, "Query with sentinels");
    }

    retval[0] = retval[query_length+1] = GetSentinelByte(BLASTP_ENCODING);
    memcpy((void*) &retval[1], (void*) query, query_length);
    return retval;
}

BlastQueryInfo*
CPssmEngine::x_InitializeQueryInfo(unsigned int query_length)
{
    BlastQueryInfo* retval = NULL;

    retval = (BlastQueryInfo*) calloc(1, sizeof(BlastQueryInfo));
    if ( !retval ) {
        NCBI_THROW(CBlastException, eOutOfMemory, "BlastQueryInfo");
    }

    retval->num_queries             = 1;
    retval->first_context           = 0;
    retval->last_context            = 0;
    retval->context_offsets         = (Int4*) calloc(2, sizeof(Int4));
    retval->context_offsets[1]      = query_length + 1;
    retval->eff_searchsp_array      = (Int8*) calloc(1, sizeof(Int8));
    retval->length_adjustments      = (Int4*) calloc(1, sizeof(Int4));
    retval->max_length              = query_length;

    return retval;
}

BlastScoreBlk*
CPssmEngine::x_InitializeScoreBlock(const unsigned char* query,
                                    unsigned int query_length)
{
    ASSERT(query);
    if (query_length == 0) {
        NCBI_THROW(CBlastException, eBadParameter, 
                   "Query length provided by PssmInput interface is 0");
    }

    // This program type will need to be changed when psi-tblastn is
    // implemented
    const EBlastProgramType kProgramType = eBlastTypeBlastp;
    short status = 0;

    AutoPtr<unsigned char, CDeleter<unsigned char> > guarded_query;
    guarded_query.reset(x_GuardProteinQuery(query, query_length));

    // Setup the scoring options
    CBlastScoringOptions opts;
    status = BlastScoringOptionsNew(kProgramType, &opts);
    if (status != 0) {
        NCBI_THROW(CBlastException, eOutOfMemory, "BlastScoringOptions");
    }
    opts->matrix = strdup("BLOSUM62"); // FIXME: shouldn't use hard coded value
    opts->matrix_path = strdup(FindMatrixPath(opts->matrix, true).c_str());

    // Setup the sequence block structure
    CBLAST_SequenceBlk query_blk;
    status = BlastSeqBlkNew(&query_blk);
    if (status != 0) {
        NCBI_THROW(CBlastException, eOutOfMemory, "BLAST_SequenceBlk");
    }
    
    // Populate the sequence block structure, transferring ownership of the
    // guarded protein sequence
    status = BlastSeqBlkSetSequence(query_blk, guarded_query.release(),
                                    query_length);
    if (status != 0) {
        // should never happen, previous function only performs assignments
        abort();    
    }

    // Setup the query info structure
    CBlastQueryInfo query_info(x_InitializeQueryInfo(query_length));

    BlastScoreBlk* retval = NULL;
    Blast_Message* errors = NULL;
    const double kScaleFactor = 1.0;
    const bool kIsPhiBlast = false;
    status = BlastSetup_GetScoreBlock(query_blk,
                                      query_info,
                                      opts,
                                      kProgramType,
                                      kIsPhiBlast,
                                      &retval,
                                      kScaleFactor,
                                      &errors);
    if (status != 0) {
        NCBI_THROW(CBlastException, eInternal, errors->message);
    }

    /*********************************************************************/
    // MUST INITIALIZE THIS: should be moved to setup code
    retval->kbp_ideal = Blast_KarlinBlkIdealCalc(retval);

    return retval;
}

CRef<CPssmWithParameters>
CPssmEngine::x_PSIMatrix2Asn1(const PSIMatrix* pssm,
                              const PSIBlastOptions* opts,
                              const PSIDiagnosticsResponse* diagnostics)
{
    ASSERT(pssm);

    CRef<CPssmWithParameters> retval(new CPssmWithParameters);

    // Record the parameters
    retval->SetParams().SetPseudocount(opts->pseudo_count);
    string mtx("BLOSUM62"); // FIXME
    mtx = NStr::ToUpper(mtx); // save the matrix name in all capital letters
    retval->SetParams().SetRpsdbparams().SetMatrixName(mtx);

    CPssm& asn1_pssm = retval->SetPssm();
    asn1_pssm.SetIsProtein(true);
    asn1_pssm.SetNumRows(pssm->nrows);
    asn1_pssm.SetNumColumns(pssm->ncols);
    asn1_pssm.SetByRow(false);

    asn1_pssm.SetFinalData().SetLambda(pssm->lambda);
    asn1_pssm.SetFinalData().SetKappa(pssm->kappa);
    asn1_pssm.SetFinalData().SetH(pssm->h);
    for (unsigned int i = 0; i < pssm->ncols; i++) {
        for (unsigned int j = 0; j < pssm->nrows; j++) {
            asn1_pssm.SetFinalData().SetScores().push_back(pssm->pssm[i][j]);
        }
    }

    /********** Collect information from diagnostics structure ************/
    if ( !diagnostics ) {
        return retval;
    }

    ASSERT(pssm->nrows == diagnostics->alphabet_size);
    ASSERT(pssm->ncols == diagnostics->query_length);

    if (diagnostics->information_content) {
        NCBI_THROW(CBlastException, eNotSupported, "Information content "
                   "cannot be stored in Score-matrix-parameters ASN.1");
    }

    if (diagnostics->residue_freqs) {
        CPssmIntermediateData::TResFreqsPerPos& res_freqs =
            asn1_pssm.SetIntermediateData().SetResFreqsPerPos();
        for (unsigned int i = 0; i < pssm->ncols; i++) {
            for (unsigned int j = 0; j < pssm->nrows; j++) {
                res_freqs.push_back(diagnostics->residue_freqs[i][j]);
            }
        }
    }
 
    if (diagnostics->weighted_residue_freqs) {
        CPssmIntermediateData::TWeightedResFreqsPerPos& wres_freqs =
            asn1_pssm.SetIntermediateData().SetWeightedResFreqsPerPos();
        for (unsigned int i = 0; i < pssm->ncols; i++) {
            for (unsigned int j = 0; j < pssm->nrows; j++) {
                wres_freqs.push_back(diagnostics->weighted_residue_freqs[i][j]);
            }
        }
    }

    if (diagnostics->frequency_ratios) {
        CPssmIntermediateData::TFreqRatios freq_ratios = 
            asn1_pssm.SetIntermediateData().SetFreqRatios();
        for (unsigned int i = 0; i < pssm->ncols; i++) {
            for (unsigned int j = 0; j < pssm->nrows; j++) {
                freq_ratios.push_back(diagnostics->frequency_ratios[i][j]);
            }
        }
    }

    if (diagnostics->gapless_column_weights) {
        NCBI_THROW(CBlastException, eNotSupported, "Gapless column weights "
                   "cannot be stored in Score-matrix-parameters ASN.1");
    }

    return retval;
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 1.14  2004/10/12 14:19:18  camacho
 * Update for scoremat.asn reorganization
 *
 * Revision 1.13  2004/10/08 20:20:11  camacho
 * Throw an exception is the query length is 0
 *
 * Revision 1.12  2004/10/06 18:21:08  camacho
 * Fixed contents of posFreqs field in scoremat structure
 *
 * Revision 1.11  2004/09/17 02:06:53  camacho
 * Renaming of diagnostics structure fields
 *
 * Revision 1.10  2004/08/05 18:55:09  camacho
 * Changes to use the most recent scoremat.asn specification
 *
 * Revision 1.9  2004/08/04 20:27:04  camacho
 * 1. Use of CPSIMatrix and CPSIDiagnosticsResponse classes instead of bare
 *    pointers to C structures.
 * 2. Completed population of CScore_matrix return value.
 * 3. Better error handling.
 *
 * Revision 1.8  2004/08/02 13:42:41  camacho
 * Fix warning
 *
 * Revision 1.7  2004/08/02 13:29:00  camacho
 * +Initialization of BlastScoreBlk and population of Score_matrix structure
 *
 * Revision 1.6  2004/07/29 17:54:29  camacho
 * Redesigned to use strategy pattern
 *
 * Revision 1.5  2004/06/21 12:53:05  camacho
 * Replace PSI_ALPHABET_SIZE for BLASTAA_SIZE
 *
 * Revision 1.4  2004/06/09 14:32:23  camacho
 * Added use_best_alignment option
 *
 * Revision 1.3  2004/05/28 17:42:02  camacho
 * Fix compiler warning
 *
 * Revision 1.2  2004/05/28 17:15:43  camacho
 * Fix NCBI {BEGIN,END}_SCOPE macro usage, remove warning
 *
 * Revision 1.1  2004/05/28 16:41:39  camacho
 * Initial revision
 *
 *
 * ===========================================================================
 */
