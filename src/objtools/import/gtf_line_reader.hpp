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

#ifndef GTF_LINE_READER__HPP
#define GTF_LINE_READER__HPP

#include <corelib/ncbifile.hpp>
#include <util/line_reader.hpp>

#include "gtf_import_data.hpp"

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE

//  ============================================================================
class CGtfLineReader: protected CStreamLineReader
//  ============================================================================
{
public:
    CGtfLineReader(
        CNcbiIstream& istr);
    virtual ~CGtfLineReader() {};

    virtual bool
    GetNextRecord(
        CGtfImportData&);

    unsigned int LineCount() const { return mLineNumber; };
    unsigned int RecordCount() const { return mRecordNumber; };

protected:
    void
    xSplitLine(
        const std::string&,
        std::vector<std::string>&);

    unsigned int mLineNumber;
    unsigned int mRecordNumber;
    std::string mColumnDelimiter;
    int mSplitFlags;
};

END_objects_SCOPE
END_NCBI_SCOPE

#endif
