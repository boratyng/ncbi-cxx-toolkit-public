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
 *   using specifications from the data definition file
 *   'seqalign.asn'.
 */

#ifndef OBJECTS_SEQALIGN_SEQ_ALIGN_HPP
#define OBJECTS_SEQALIGN_SEQ_ALIGN_HPP


// generated includes
#include <objects/seqalign/Seq_align_.hpp>
#include <util/range.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CSeq_id;

class NCBI_SEQALIGN_EXPORT CSeq_align : public CSeq_align_Base
{
    typedef CSeq_align_Base Tparent;
public:
    // constructor
    CSeq_align(void);
    // destructor
    ~CSeq_align(void);

    // Validatiors
    TDim CheckNumRows(void)                   const;
    void Validate    (bool full_test = false) const;

    // GetSeqRange
    CRange<TSeqPos> GetSeqRange(TDim row) const;
    TSeqPos         GetSeqStart(TDim row) const;
    TSeqPos         GetSeqStop (TDim row) const;

    // Get strand (the first one if segments have different strands).
    ENa_strand      GetSeqStrand(TDim row) const;

    // Get seq-id (the first one if segments have different ids).
    // Throw exception if row is invalid.
    const CSeq_id&  GetSeq_id(TDim row) const;

    // Get score
    bool GetNamedScore(const string& id, int &score) const;
    bool GetNamedScore(const string& id, double &score) const;


    /// Reverse the segments' orientation
    /// NOTE: currently *only* works for dense-seg
    void Reverse(void);

    /// Swap the position of two rows in the alignment
    /// NOTE: currently *only* works for dense-seg & disc
    void SwapRows(TDim row1, TDim row2);

    // Create a Dense-seg from a Std-seg
    // Used by AlnMgr to handle nucl2prot alignments
    //

    // NOTE: Here we assume that the same rows on different segments
    // contain the same sequence. Without access to OM we can only check
    // if the ids are the same via SerialEquals, and we throw an exception
    // if not equal. Since the same sequence can be represented with a 
    // different type of seq-id, we provide an optional callback mechanism
    // to compare id1 and id2, and if both resolve to the same sequence 
    // and id2 is preferred, to SerialAssign it to id1. Otherwise, again,
    // an exception should be thrown.
    struct SSeqIdChooser : CObject
    {
        virtual void ChooseSeqId(CSeq_id& id1, const CSeq_id& id2) = 0;
    };
    CRef<CSeq_align> CreateDensegFromStdseg(SSeqIdChooser* SeqIdChooser = 0) const;

    // Create a Dense-seg with widths from Dense-seg of nucleotides
    // Used by AlnMgr to handle translated nucl2nucl alignments
    // IMPORTANT NOTE: Do *NOT* use for alignments containing proteins;
    //                 the code will not check for this
    CRef<CSeq_align> CreateTranslatedDensegFromNADenseg(void) const;

private:
    // Prohibit copy constructor and assignment operator
    CSeq_align(const CSeq_align& value);
    CSeq_align& operator=(const CSeq_align& value);

};



/////////////////// CSeq_align inline methods

// constructor
inline
CSeq_align::CSeq_align(void)
{
}


/////////////////// end of CSeq_align inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.14  2004/06/14 22:09:02  johnson
* Added GetSeqStrand method (analogous to GetSeq_id)
*
* Revision 1.13  2004/05/05 19:16:25  johnson
* Added SwapRows method for 'disc' seq-align / seq-align-set
*
* Revision 1.12  2004/04/27 19:17:13  johnson
* Added GetNamedScore helper function
*
* Revision 1.11  2004/04/19 17:27:22  grichenk
* Added GetSeq_id(TDim row)
*
* Revision 1.10  2004/03/15 17:42:29  todorov
* Derive SSeqIdChooser from CObject to avoid possible multiple inheritance in
* the client. Workshop has problems with it (missplaced vtable).
*
* Revision 1.9  2004/03/09 17:14:17  todorov
* changed the C-style callback to a functor
*
* Revision 1.8  2004/02/23 16:17:53  ucko
* Add forward declaration of CSeq_id.
*
* Revision 1.7  2004/02/23 15:30:55  todorov
* +TChooseSeqIdCallback to abstract resolving seq-ids in CreateDensegFromStdseg()
*
* Revision 1.6  2004/01/15 20:13:27  todorov
* -CheckNumSegs
*
* Revision 1.5  2003/12/16 22:54:14  todorov
* +CreateTranslatedDensegFromNADenseg
*
* Revision 1.4  2003/09/16 15:31:59  todorov
* Added validation methods. Added seq range methods
*
* Revision 1.3  2003/08/26 20:28:38  johnson
* added 'SwapRows' method
*
* Revision 1.2  2003/08/19 21:10:39  todorov
* +CreateDensegFromStdseg
*
* Revision 1.1  2003/08/13 18:11:35  johnson
* added 'Reverse' method
*
*
* ===========================================================================
*/

#endif // OBJECTS_SEQALIGN_SEQ_ALIGN_HPP
/* Original file checksum: lines: 93, chars: 2426, CRC32: 6ba198f0 */
