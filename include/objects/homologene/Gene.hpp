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

#ifndef INTERNAL_HOMOLOGENE_GENE_HPP
#define INTERNAL_HOMOLOGENE_GENE_HPP


// generated includes
#include <objects/homologene/Gene_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CGene : public CGene_Base
{
    typedef CGene_Base Tparent;
public:
    // constructor
    CGene(void);
    // destructor
    ~CGene(void);

    // alias for old code support
    typedef TGene_links TLinks;
    bool IsSetLinks(void) const { return IsSetGene_links(); }
    bool CanGetLinks(void) const { return CanGetGene_links(); }
    void ResetLinks(void) { ResetGene_links(); }
    const TLinks& GetLinks(void) const { return GetGene_links(); }
    TLinks& SetLinks(void) { return SetGene_links(); }

private:
    // Prohibit copy constructor and assignment operator
    CGene(const CGene& value);
    CGene& operator=(const CGene& value);

};



/////////////////// CGene inline methods

// constructor
inline
CGene::CGene(void)
{
}


/////////////////// end of CGene inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.2  2005/09/01 20:09:27  vasilche
* Fixed compilation error.
*
* Revision 1.1  2005/09/01 17:46:09  dicuccio
* Initial revision - moved over from internal tree
*
* Revision 1.4  2004/03/17 17:15:57  lee
* Fixed typo
*
* Revision 1.3  2004/03/17 17:12:46  lee
* Added typedef for TLinks
*
* Revision 1.2  2004/03/16 20:47:56  lee
* Fixed typedef error (TLinks -> TGene_links)
*
* Revision 1.1  2004/03/16 20:21:32  lee
* Changed ASN specs (added protein-links to Gene, recip-best to Stats,
* version to HomoloGeneEntry)
*
*
* ===========================================================================
*/

#endif // INTERNAL_HOMOLOGENE_GENE_HPP
/* Original file checksum: lines: 93, chars: 2334, CRC32: d8674688 */
