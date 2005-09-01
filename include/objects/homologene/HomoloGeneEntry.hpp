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
 *   'homologene.asn'.
 */

#ifndef HOMOLOGENEENTRY_HPP
#define HOMOLOGENEENTRY_HPP


// generated includes
#include <objects/homologene/HomoloGeneEntry_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CHomoloGeneEntry : public CHomoloGeneEntry_Base
{
    typedef CHomoloGeneEntry_Base Tparent;
public:
    // constructor
    CHomoloGeneEntry(void);
    // destructor
    ~CHomoloGeneEntry(void);

private:
    // Prohibit copy constructor and assignment operator
    CHomoloGeneEntry(const CHomoloGeneEntry& value);
    CHomoloGeneEntry& operator=(const CHomoloGeneEntry& value);

public:
    // user added stuff
    typedef TGenes TCore_genes;

    bool IsSetCore_genes(void) const { return IsSetGenes(); }
    bool CanGetCore_genes(void) const { return CanGetGenes(); }
    void ResetCore_genes(void) { return ResetGenes(); }
    const TCore_genes& GetCore_genes(void) const { return GetGenes(); }
    TCore_genes& SetCore_genes(void) { return SetGenes(); }
};



/////////////////// CHomoloGeneEntry inline methods

// constructor
inline
CHomoloGeneEntry::CHomoloGeneEntry(void)
{
}


/////////////////// end of CHomoloGeneEntry inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2005/09/01 17:46:09  dicuccio
* Initial revision - moved over from internal tree
*
* Revision 1.1  2004/02/04 20:35:48  lee
* Added domains to asn spec.  Renamed "core-genes" to "genes" and created
* aliases so that old code won't break.
*
*
* ===========================================================================
*/

#endif // HOMOLOGENEENTRY_HPP
/* Original file checksum: lines: 93, chars: 2441, CRC32: 722077fa */
