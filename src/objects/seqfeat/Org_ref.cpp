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
 *   'seqfeat.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/Dbtag.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
COrg_ref::~COrg_ref(void)
{
}

// Appends a label to "label" based on content
void COrg_ref::GetLabel(string* label) const
{
    if (IsSetTaxname()) {
        *label += GetTaxname();
    } else if (IsSetCommon()) {
        *label += GetCommon();
    } else if (IsSetDb()) {
        GetDb().front()->GetLabel(label);
    }
}
    
static const string s_taxonName( "taxon" );

int
COrg_ref::GetTaxId() const
{
    const TDb& lDbTags = GetDb();
 
    for(TDb::const_iterator i = lDbTags.begin();
	i != lDbTags.end();
	++i) {
	if( i->GetPointer()
	    && i->GetObject().GetDb().compare(s_taxonName) == 0 ) {
	    const CObject_id& id = i->GetObject().GetTag();
	    if( id.IsId() )
		return id.GetId();
	}
    }
    return 0;
}

int
COrg_ref::SetTaxId( int tax_id )
{
    int old_id(0);

    TDb& lDbTags = SetDb();
    // Try to update existing tax id first
    for(TDb::iterator i = lDbTags.begin();
	i != lDbTags.end();
	++i) {
	if( i->GetPointer()
	    && i->GetObject().GetDb().compare(s_taxonName) == 0 ) {
	    CObject_id& id = i->GetObject().SetTag();
	    if( id.IsId() )
		old_id = id.GetId();
	    id.SetId() = tax_id;
	    return old_id;
	}
    }
    // Add new tag
    CRef< CDbtag > ref( new CDbtag() );
    ref->SetDb( s_taxonName );
    ref->SetTag().SetId( tax_id );
    SetDb().push_back( ref );

    return old_id;
}

bool COrg_ref::IsSetOrgMod(void) const
{
    return IsSetOrgname () && GetOrgname ().IsSetMod ();
}

bool COrg_ref::IsSetLineage(void) const
{
    return IsSetOrgname () && GetOrgname ().IsSetLineage ();
}

bool COrg_ref::IsSetGcode(void) const
{
    return IsSetOrgname () && GetOrgname ().IsSetGcode ();
}

bool COrg_ref::IsSetMgcode(void) const
{
    return IsSetOrgname () && GetOrgname ().IsSetMgcode ();
}

bool COrg_ref::IsSetDivision(void) const
{
    return IsSetOrgname () && GetOrgname ().IsSetDiv ();
}




END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 61, chars: 1882, CRC32: c3300cc2 */
