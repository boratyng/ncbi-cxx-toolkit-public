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
 *   using specifications from the ASN data definition file
 *   'twebenv.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.1  2001/07/27 18:33:44  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */

#ifndef QUERY_SELECT_HPP
#define QUERY_SELECT_HPP


// generated includes
#include <Query_Select_.hpp>

// generated classes

class CQuery_Select : public CQuery_Select_Base
{
    typedef CQuery_Select_Base Tparent;
public:
    // constructor
    CQuery_Select(void);
    // destructor
    ~CQuery_Select(void);

private:
    // Prohibit copy constructor and assignment operator
    CQuery_Select(const CQuery_Select& value);
    CQuery_Select& operator=(const CQuery_Select& value);

};



/////////////////// CQuery_Select inline methods

// constructor
inline
CQuery_Select::CQuery_Select(void)
{
}


/////////////////// end of CQuery_Select inline methods



#endif // QUERY_SELECT_HPP
/* Original file checksum: lines: 82, chars: 2268, CRC32: 993c15ff */
