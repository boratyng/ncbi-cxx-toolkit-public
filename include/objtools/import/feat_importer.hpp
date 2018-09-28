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
* Author:  Frank Ludwig, NCBI
*
* File Description:
*   Test application for the CFormatGuess component
*
* ===========================================================================
*/

#ifndef FEAT_IMPORTER__HPP
#define FEAT_IMPORTER__HPP

#include <corelib/ncbifile.hpp>
#include <objects/seq/Seq_annot.hpp>

#include <objtools/import/feat_message_handler.hpp>
#include <objtools/import/id_resolver.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE

class CFeatLineReader;
class CFeatImportData;
class CFeatAnnotAssembler;

//  ============================================================================
class NCBI_XOBJIMPORT_EXPORT CFeatImporter
//  ============================================================================
{
public:
    enum FLAGS {
        fNormal = 0,
        fAllIdsAsLocal = (1 << 0),
        fNumericIdsAsLocal = (1 << 2),
        fReportProgress = (1 << 3),
    };

    static CFeatImporter*
    Get(
        const std::string&,
        unsigned int);

public:
    CFeatImporter(
        unsigned int);

    virtual ~CFeatImporter();

    virtual void
    ReadSeqAnnot(
        CNcbiIstream&,
        CSeq_annot&,
        CFeatMessageHandler&);

    void
    SetIdResolver(
        CIdResolver*);

protected:
    unsigned int mFlags;

    virtual CFeatLineReader*
    GetReader(
        CNcbiIstream&,
        CFeatMessageHandler&) { return nullptr; };

    virtual CFeatImportData*
    GetImportData(
        CFeatMessageHandler&) { return nullptr; };

    virtual CFeatAnnotAssembler*
    GetAnnotAssembler(
        CSeq_annot&,
        CFeatMessageHandler&) { return nullptr; };

    unique_ptr<CIdResolver> mpIdResolver; 
};

END_objects_SCOPE
END_NCBI_SCOPE

#endif
