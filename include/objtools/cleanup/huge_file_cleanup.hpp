#ifndef HUGE_FILE_CLEANUP_HPP
#define HUGE_FILE_CLEANUP_HPP

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
 * Author:  Justin Foley
 * File Description:
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <objtools/edit/huge_asn_reader.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


/*
namespace edit {
    class CHugeFileProcess;
};

class NCBI_CLEANUP_EXPORT CHugeFileCleanup
{
public: 
    CHugeFileCleanup();
    virtual ~CHugeFileCleanup() = default;
private:
    unique_ptr<edit::CHugeFileProcess> m_Process;
};
*/

class NCBI_CLEANUP_EXPORT CCleanupHugeAsnReader :
    public edit::CHugeAsnReader
{   
public:
    CCleanupHugeAsnReader();
    virtual ~CCleanupHugeAsnReader() = default;
    using TParent = edit::CHugeAsnReader;

    void FlattenGenbankSet() override;
private:
    bool x_LooksLikeNucProtSet() const;
};

END_SCOPE(object);
END_NCBI_SCOPE

#endif  ///  HUGE_FILE_CLEANUP
