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
 * File Description:
 *   This code is generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'twebenv.asn'.
 *
 * ATTENTION:
 *   Don't edit or check-in this file to the CVS as this file will
 *   be overridden (by DATATOOL) without warning!
 * ===========================================================================
 */

#ifndef NAMED_QUERY_BASE_HPP
#define NAMED_QUERY_BASE_HPP

// standard includes
#include <serial/serialbase.hpp>

// forward declarations
class CName;
class CQuery_Command;
class CTime;


// generated classes

class CNamed_Query_Base : public ncbi::CSerialObject
{
    typedef ncbi::CSerialObject Tparent;
public:
    // constructor
    CNamed_Query_Base(void);
    // destructor
    virtual ~CNamed_Query_Base(void);

    // type info
    DECLARE_INTERNAL_TYPE_INFO();

    // members' types
    typedef CName TName;
    typedef CTime TTime;
    typedef CQuery_Command TCommand;

    // members' getters
    // members' setters
    void ResetName(void);
    const CName& GetName(void) const;
    void SetName(CName& value);
    CName& SetName(void);

    void ResetTime(void);
    const CTime& GetTime(void) const;
    void SetTime(CTime& value);
    CTime& SetTime(void);

    void ResetCommand(void);
    const CQuery_Command& GetCommand(void) const;
    void SetCommand(CQuery_Command& value);
    CQuery_Command& SetCommand(void);

    // reset whole object
    virtual void Reset(void);


private:
    // Prohibit copy constructor and assignment operator
    CNamed_Query_Base(const CNamed_Query_Base&);
    CNamed_Query_Base& operator=(const CNamed_Query_Base&);

    // members' data
    ncbi::CRef< TName > m_Name;
    ncbi::CRef< TTime > m_Time;
    ncbi::CRef< TCommand > m_Command;
};






///////////////////////////////////////////////////////////
///////////////////// inline methods //////////////////////
///////////////////////////////////////////////////////////
inline
CNamed_Query_Base::TName& CNamed_Query_Base::SetName(void)
{
    return (*m_Name);
}

inline
CNamed_Query_Base::TTime& CNamed_Query_Base::SetTime(void)
{
    return (*m_Time);
}

inline
CNamed_Query_Base::TCommand& CNamed_Query_Base::SetCommand(void)
{
    return (*m_Command);
}

///////////////////////////////////////////////////////////
////////////////// end of inline methods //////////////////
///////////////////////////////////////////////////////////






#endif // NAMED_QUERY_BASE_HPP
