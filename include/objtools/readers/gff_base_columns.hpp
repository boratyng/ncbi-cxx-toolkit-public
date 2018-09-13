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
 * Author: Frank Ludwig
 *
 * File Description:
 *   GFF3 transient data structures
 *
 */

#ifndef OBJTOOLS_READERS___GFF_BASE_COLUMNS__HPP
#define OBJTOOLS_READERS___GFF_BASE_COLUMNS__HPP

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::

//  ----------------------------------------------------------------------------
class CGffBaseColumns
//  ----------------------------------------------------------------------------
{
public:
    typedef CCdregion::EFrame TFrame;

    CGffBaseColumns();

    CGffBaseColumns(
        const CGffBaseColumns& rhs);

    virtual ~CGffBaseColumns();

    const string& Id() const { 
        return m_strId; 
    };

    size_t SeqStart() const { 
        return m_uSeqStart; 
    };

    size_t SeqStop() const { 
        return m_uSeqStop; 
    };

    const string& Source() const {
        return m_strSource; 
    };

    const string& Type() const {
        return m_strType; 
    };

    double Score() const { 
        return IsSetScore() ? *m_pdScore : 0.0; 
    };

    ENa_strand Strand() const { 
        return IsSetStrand() ? *m_peStrand : eNa_strand_unknown; 
    };

    TFrame Phase() const {
        return IsSetPhase() ? *m_pePhase : CCdregion::eFrame_not_set; 
    };

    bool IsSetScore() const { 
        return m_pdScore != 0; 
    };
    bool IsSetStrand() const { 
        return m_peStrand != 0; 
    };
    bool IsSetPhase() const { 
        return m_pePhase != 0; 
    };

protected:
    string m_strId;
    size_t m_uSeqStart;
    size_t m_uSeqStop;
    string m_strSource;
    string m_strType;
    double* m_pdScore;
    ENa_strand* m_peStrand;
    TFrame* m_pePhase;
};


END_objects_SCOPE
END_NCBI_SCOPE

#endif // OBJTOOLS_READERS___GFF_BASE_COLUMNS__HPP
