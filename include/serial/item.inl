#if defined(ITEM__HPP)  &&  !defined(ITEM__INL)
#define ITEM__INL

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
* Author: Eugene Vasilchenko
*
* File Description:
*   !!! PUT YOUR DESCRIPTION HERE !!!
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.2  2000/10/03 17:22:32  vasilche
* Reduced header dependency.
* Reduced size of debug libraries on WorkShop by 3 times.
* Fixed tag allocation for parent classes.
* Fixed CObject allocation/deallocation in streams.
* Moved instantiation of several templates in separate source file.
*
* Revision 1.1  2000/09/18 20:00:02  vasilche
* Separated CVariantInfo and CMemberInfo.
* Implemented copy hooks.
* All hooks now are stored in CTypeInfo/CMemberInfo/CVariantInfo.
* Most type specific functions now are implemented via function pointers instead of virtual functions.
*
* ===========================================================================
*/

inline
const CMemberId& CItemInfo::GetId(void) const
{
    return m_Id;
}

inline
CMemberId& CItemInfo::GetId(void)
{
    return m_Id;
}

inline
TMemberIndex CItemInfo::GetIndex(void) const
{
    return m_Index;
}

inline
CItemInfo::TOffset CItemInfo::GetOffset(void) const
{
    return m_Offset;
}

inline
TTypeInfo CItemInfo::GetTypeInfo(void) const
{
    return m_Type.Get();
}

inline
TObjectPtr CItemInfo::GetItemPtr(TObjectPtr classPtr) const
{
    return Add(classPtr, GetOffset());
}

inline
TConstObjectPtr CItemInfo::GetItemPtr(TConstObjectPtr classPtr) const
{
    return Add(classPtr, GetOffset());
}

#endif /* def ITEM__HPP  &&  ndef ITEM__INL */
