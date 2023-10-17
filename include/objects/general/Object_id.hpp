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
 *   'general.asn'.
 */

#ifndef OBJECTS_GENERAL_OBJECT_ID_HPP
#define OBJECTS_GENERAL_OBJECT_ID_HPP


// generated includes
#include <objects/general/Object_id_.hpp>

// generated classes

#include <serial/objhook.hpp>
#include <map>

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class NCBI_GENERAL_EXPORT CObject_id : public CObject_id_Base
{
    typedef CObject_id_Base Tparent;
public:
    // constructor
    CObject_id(void);
    // destructor
    ~CObject_id(void);

    // matching id? (int-string)
    bool Match(const CObject_id& oid2) const;
    // Set this id to me matching to the argument
    // return false if there is no matching id
    // @sa SetStrOrId()
    // @sa GetIdType()
    bool SetAsMatchingTo(const CObject_id& oid2);
    
    // identical ids?
    int Compare(const CObject_id& oid2) const;
    bool operator<(const CObject_id& id2) const;

    // Try to interpret the Object-id as integer.
    // Returns:
    //   e_not_set if the Object-id is not set
    //   e_Id if the Object-id is an integer or a string with valid integer in a canonical form
    //   e_Str if the Object-id as a string without valid integer
    // If the result is e_Id the integer id will be returned by Int8 value,
    // otherwise the value will be set to 0.
    // @sa SetStrOrId()
    // @sa SetAsMatchingTo()
    typedef Int8 TId8;
    E_Choice GetIdType(TId8& value) const;

    bool IsId8(void) const;
    TId8 GetId8(void) const;
    NCBI_WARN_UNUSED_RESULT bool GetId8(TId8& value) const;
    void SetId8(TId8 value);

    bool IsGi(void) const;
    TGi GetGi(void) const;
    NCBI_WARN_UNUSED_RESULT bool GetGi(TGi& value) const;
    void SetGi(TGi value);
#ifdef NCBI_STRICT_GI
    void SetGi(TIntId value);
    NCBI_WARN_UNUSED_RESULT bool GetGi(TIntId& value) const;
#endif

    // Set integer id part if the argument is a positive integer without leading zeros
    // otherwise set str part.
    // @sa SetAsMatchingTo()
    // @sa GetIdType()
    void SetStrOrId(CTempString str);

    // format contents into a stream
    ostream& AsString(ostream &s) const;
private:
    // Prohibit copy constructor & assignment operator
    CObject_id(const CObject_id&);
    CObject_id& operator= (const CObject_id&);
};



/////////////////// CObject_id inline methods

// constructor
inline
CObject_id::CObject_id(void)
{
}


inline
bool CObject_id::operator<(const CObject_id& id2) const
{
    return Compare(id2) < 0;
}


NCBI_WARN_UNUSED_RESULT
inline
bool CObject_id::GetId8(TId8& value) const
{
    return GetIdType(value) == e_Id;
}


inline
bool CObject_id::IsId8(void) const
{
    TId8 value;
    return GetId8(value);
}


NCBI_WARN_UNUSED_RESULT
inline
bool CObject_id::GetGi(TGi& value) const
{
#ifdef NCBI_INT8_GI
    TId8 id8;
    bool ret = GetId8(id8);
    if (ret) value = GI_FROM(TId8, id8);
    return ret;
#else
    if ( IsId() ) {
        value = GI_FROM(TId, GetId());
        return true;
    }
    return false;
#endif
}


inline
bool CObject_id::IsGi(void) const
{
#ifdef NCBI_INT8_GI
    return IsId8();
#else
    return IsId();
#endif
}


inline
TGi CObject_id::GetGi(void) const
{
#ifdef NCBI_INT8_GI
    return GI_FROM(TId8, GetId8());
#else
    return GI_FROM(TId, GetId());
#endif
}


inline
void CObject_id::SetGi(TGi value)
{
#ifdef NCBI_INT8_GI
    SetId8(GI_TO(TId8, value));
#else
    SetId(GI_TO(TId8, value));
#endif
}

#ifdef NCBI_STRICT_GI
inline
void CObject_id::SetGi(TIntId value)
{
    SetId8(value);
}

NCBI_WARN_UNUSED_RESULT
inline
bool CObject_id::GetGi(TIntId& value) const
{
#ifdef NCBI_INT8_GI
    return GetId8(value);
#else
    if ( IsId() ) {
        value = GetId();
        return true;
    }
    return false;
#endif
}
#endif


/////////////////// end of CObject_id inline methods

/////////////////////////////////////////////////////////////////////////////
// CReadSharedObjectIdHookBase
//   base class for read hooks for shared Object-id objects.
/////////////////////////////////////////////////////////////////////////////

class NCBI_GENERAL_EXPORT CReadSharedObjectIdHookBase
    : public CReadClassMemberHook
{
public:
    /// Returns shared version of Object-id with specified 'str' field value.
    /// Can be stored for later read-only use.
    CObject_id& GetSharedObject_id(const string& id);
    /// Returns shared version of Object-id with specified 'id' field value.
    /// Can be stored for later read-only use.
    CObject_id& GetSharedObject_id(int id);

    /// Returns shared version of argument Object-id.
    /// Can be stored for later read-only use.
    CObject_id& GetSharedObject_id(const CObject_id& oid) {
        return oid.IsStr()?
            GetSharedObject_id(oid.GetStr()):
            GetSharedObject_id(oid.GetId());
    }

    /// Reads Object-id and returns reference to its shared version.
    /// Can be stored for later read-only use.
    CObject_id& ReadSharedObject_id(CObjectIStream& in);
    
protected:
    CObject_id m_Temp;

private:
    typedef map<string, CRef<CObject_id> > TMapByStr;
    typedef map<int, CRef<CObject_id> > TMapByInt;
    TMapByStr m_MapByStr;
    TMapByInt m_MapByInt;
};


/////////////////////////////////////////////////////////////////////////////


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_GENERAL_OBJECT_ID_HPP