#ifndef BDBLOADER_HPP
#define BDBLOADER_HPP

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
*  ===========================================================================
*
*  Author: Christiam Camacho
*
*  File Description:
*   Data loader implementation that uses the blast databases
*
* ===========================================================================
*/

#include <objmgr/data_loader.hpp>
#include <objmgr/impl/data_source.hpp>
#include <readdb.h>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

/////////////////////////////////////////////////////////////////////////////////
//
// CBlastDbDataLoader
//   Data loader implementation that uses the blast databases.
//   Note: Only full bioseqs can be requested, not parts of a sequence.
//

class NCBI_XLOADER_BLASTDB_EXPORT CBlastDbDataLoader : public CDataLoader
{
public:

    /// Describes the type of blast database to use, equivalent to READDB_DB_* #defines
    enum EDbType {
        eNucleotide = 0,    ///< nucleotide database
        eProtein = 1,       ///< protein database
        eUnknown = 2        ///< protein is attempted first, then nucleotide
    };

    CBlastDbDataLoader(const string& loader_name = "BLASTDB", const string&
            dbname = "nr", const EDbType dbtype = eUnknown);
                    
    virtual ~CBlastDbDataLoader(void);
  
    virtual bool DropTSE(const CSeq_entry* sep) { return true; }
    virtual bool GetRecords(const CHandleRangeMap& hrmap, const EChoice choice);
  
    virtual inline void GC(void) {} 
    virtual void DebugDump(CDebugDumpContext ddc, unsigned int depth) const;
    
  private:

    const string m_dbname;      ///< blast database name
    EDbType m_dbtype;           ///< is this database protein or nucleotide?
    ReadDBFILEPtr m_rdfp;       ///< blast database handle
    CFastMutex *m_mutex;        ///< mutex to access the blast database

    typedef map<int, CRef<CBioseq> > TOid2Bioseq;
    TOid2Bioseq m_cache;
};

END_SCOPE(objects)
END_NCBI_SCOPE

/* ========================================================================== 
 *
 * $Log$
 * Revision 1.1  2003/08/06 16:15:17  jianye
 * Add BLAST DB loader.
 *
 * Revision 1.4  2003/05/19 21:11:46  camacho
 * Added caching
 *
 * Revision 1.3  2003/05/16 14:27:48  camacho
 * Proper use of namespaces
 *
 * Revision 1.2  2003/05/08 15:11:43  camacho
 * Changed prototype for GetRecords in base class
 *
 * Revision 1.1  2003/03/14 22:37:26  camacho
 * Initial revision
 *
 *
 * ========================================================================== */

#endif
