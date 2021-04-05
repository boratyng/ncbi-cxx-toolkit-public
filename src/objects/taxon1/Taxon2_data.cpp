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
 * Author:  Michael Domrachev
 *
 * File Description:
 *   Taxon2_data is returned to caller in various taxonomy-related lookups
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'taxon1.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/taxon1/Taxon2_data.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CTaxon2_data::~CTaxon2_data(void)
{
}

CTaxon2_data::TOrgProperties::iterator
CTaxon2_data::x_FindProperty( const string& name )
{
    NON_CONST_ITERATE( TOrgProperties, i, m_props ) {
	if( NStr::Equal( (*i)->GetDb(), name ) ) {
	    return i;
	}
    }
    return m_props.end();
}

CTaxon2_data::TOrgProperties::const_iterator
CTaxon2_data::x_FindPropertyConst( const string& name ) const
{
    ITERATE( TOrgProperties, i, m_props ) {
	if( NStr::Equal( (*i)->GetDb(), name ) ) {
	    return i;
	}
    }
    return m_props.end();
}

void 
CTaxon2_data::SetProperty( const string& name, const string& value )
{
    if( !name.empty() ) {
	TOrgProperties::iterator i = x_FindProperty( name );
	if( i != m_props.end() ) {
	    (*i)->SetTag().SetStr( value );
	} else {
	    CRef< CDbtag > pProp( new CDbtag );
	    pProp->SetDb( name );
	    pProp->SetTag().SetStr( value );
	    m_props.push_back( pProp );
	}
    }
}

void
CTaxon2_data::SetProperty( const string& name, int value )
{
    if( !name.empty() ) {
	TOrgProperties::iterator i = x_FindProperty( name );
	if( i != m_props.end() ) {
	    (*i)->SetTag().SetId( value );
	} else {
	    CRef< CDbtag > pProp( new CDbtag );
	    pProp->SetDb( name );
	    pProp->SetTag().SetId( value );
	    m_props.push_back( pProp );
	}
    }
}

void
CTaxon2_data::SetProperty( const string& name, bool value )
{
    if( !name.empty() ) {
	TOrgProperties::iterator i = x_FindProperty( name );
	if( i != m_props.end() ) {
	    (*i)->SetTag().SetId( value ? 1 : 0 );
	} else {
	    CRef< CDbtag > pProp( new CDbtag );
	    pProp->SetDb( name );
	    pProp->SetTag().SetId( value ? 1 : 0 );
	    m_props.push_back( pProp );
	}
    }
}

bool
CTaxon2_data::GetProperty( const string& name, string& value ) const
{
    if( !name.empty() ) {
	TOrgProperties::const_iterator i = x_FindPropertyConst( name );
	if( i != m_props.end() && (*i)->IsSetTag() ) {
	    switch( (*i)->GetTag().Which() ) {
	    case CObject_id::e_Str: value = (*i)->GetTag().GetStr(); return true;
	    case CObject_id::e_Id: value = NStr::IntToString( (*i)->GetTag().GetId() ); return true;
	    default: break; // unknown type
	    }
	}
    }
    return false;
}

bool
CTaxon2_data::GetProperty( const string& name, int& value ) const
{
    if( !name.empty() ) {
	TOrgProperties::const_iterator i = x_FindPropertyConst( name );
	if( i != m_props.end() && (*i)->IsSetTag() ) {
	    switch( (*i)->GetTag().Which() ) {
	    case CObject_id::e_Str: value = NStr::StringToInt( (*i)->GetTag().GetStr(), NStr::fConvErr_NoThrow ); return true;
	    case CObject_id::e_Id: value = (*i)->GetTag().GetId(); return true;
	    default: break; // unknown type
	    }
	}
    }
    return false;
}

bool
CTaxon2_data::GetProperty( const string& name, bool& value ) const
{
    if( !name.empty() ) {
	TOrgProperties::const_iterator i = x_FindPropertyConst( name );
	if( i != m_props.end() && (*i)->IsSetTag() ) {
	    switch( (*i)->GetTag().Which() ) {
	    case CObject_id::e_Str: value = NStr::StringToBool( (*i)->GetTag().GetStr() ); return true;
	    case CObject_id::e_Id: value = (*i)->GetTag().GetId() != 0; return true;
	    default: break; // unknown type
	    }
	}
    }
    return false;
}

void
CTaxon2_data::ResetProperty( const string& name )
{
    TOrgProperties::iterator i = x_FindProperty( name );
    while( i != m_props.end() ) {
	m_props.erase(i);
	i = x_FindProperty( name );
    }
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1728, CRC32: 2b99c14d */