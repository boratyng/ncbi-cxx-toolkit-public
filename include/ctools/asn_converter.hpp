#ifndef CTOOLS___ASN_CONVERTER__HPP
#define CTOOLS___ASN_CONVERTER__HPP

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
* Authors:  Denis Vakatov, Aaron Ucko
*
* File Description:
*   Templates for converting ASN.1-based objects between NCBI's C and C++
*   in-memory layouts.
*
*/

#include <connect/ncbi_conn_stream.hpp>
#include <ctools/asn_connection.h>
#include <serial/objistrasnb.hpp>
#include <serial/objostrasnb.hpp>
#include <serial/serial.hpp>


/** @addtogroup CToolsASNConv
 *
 * @{
 */


BEGIN_NCBI_SCOPE

#define DECLARE_ASN_CONVERTER(TCpp, TC, name) \
CAsnConverter<TCpp, TC> name((AsnWriteFunc)TC##AsnWrite, (AsnReadFunc)TC##AsnRead)

template <typename TCpp, typename TC>
class CAsnConverter
{
public:
    typedef AsnWriteFunc FWrite;
    typedef AsnReadFunc  FRead;
    CAsnConverter(FWrite writer, FRead reader)
        : m_Write(writer), m_Read(reader) { }

    /// Creates and returns a new object if cpp_obj is null.
    TCpp* FromC(const TC* c_obj, TCpp* cpp_obj = 0,
                EAsnConn_Format format = eAsnConn_Binary);
    /// Always returns a new object, as that's how C readers work.
    TC*   ToC  (const TCpp& cpp_obj, EAsnConn_Format format = eAsnConn_Binary);

private:
    FWrite m_Write;
    FRead  m_Read;
};


/* @} */


///////////////////////////////////////////////////////////////////////////
// inline functions


inline
ESerialDataFormat MapAcfToSdf(EAsnConn_Format format) {
    switch (format) {
    case eAsnConn_Binary:  return eSerial_AsnBinary;
    case eAsnConn_Text:    return eSerial_AsnText;
    default:               _TROUBLE;
    }
    return eSerial_None;
}


template <typename TCpp, typename TC>
inline
TCpp* CAsnConverter<TCpp, TC>::FromC(const TC* c_obj, TCpp* cpp_obj,
                                     EAsnConn_Format format)
{
    if ( !c_obj ) {
        return 0;
    }

    CConn_MemoryStream conn_stream;

    AsnIoPtr aip = CreateAsnConn(conn_stream.GetCONN(), eAsnConn_Output,
                                 format);
    m_Write(const_cast<TC*>(c_obj), aip, 0);
    AsnIoFlush(aip);

    CRef<TCpp> cpp_ref(cpp_obj ? cpp_obj : new TCpp);
    auto_ptr<CObjectIStream> ois(CObjectIStream::Open
                                 (MapAcfToSdf(format), conn_stream));
    *ois >> *cpp_ref;
    return cpp_ref.Release();
}


template <typename TCpp, typename TC>
inline
TC* CAsnConverter<TCpp, TC>::ToC(const TCpp& cpp_obj, EAsnConn_Format format)
{
    CConn_MemoryStream conn_stream;

    auto_ptr<CObjectOStream> oos(CObjectOStream::Open
                                 (MapAcfToSdf(format), conn_stream));
    CObjectOStreamAsnBinary* bin_stream
        = dynamic_cast<CObjectOStreamAsnBinary*>(oos.get());
    if (bin_stream) {
        bin_stream->SetCStyleBigInt(true);
    }
    *oos << cpp_obj;
    oos->Flush();

    AsnIoPtr aip = CreateAsnConn(conn_stream.GetCONN(), eAsnConn_Input, format);
    return (TC*)m_Read(aip, 0);
}


END_NCBI_SCOPE


/*
 * ===========================================================================
 *
 * $Log$
 * Revision 1.9  2006/04/06 15:56:07  ucko
 * ToC: properly honor requests to use text mode, and ensure that Int8
 * fields get handled correctly in binary mode as well.
 *
 * Revision 1.8  2006/04/05 17:03:00  ucko
 * Add optional format arguments to FromC and ToC, but continue to
 * default to binary mode for efficiency.
 *
 * Revision 1.7  2006/03/30 18:15:20  lavr
 * Use CConn_MemoryStream directly
 *
 * Revision 1.6  2006/03/30 18:12:34  lavr
 * Adjust for lock-less MEMORY_Connector API
 *
 * Revision 1.5  2003/11/13 16:00:05  lavr
 * Guard macro changed; log moved to end
 *
 * Revision 1.4  2003/06/12 15:58:06  lavr
 * +#include <serial/serial.hpp>
 *
 * Revision 1.3  2003/05/28 14:53:51  lavr
 * Reduce the number of included headers
 *
 * Revision 1.2  2003/04/11 17:46:30  siyan
 * Added doxygen support
 * 
 * Revision 1.1  2002/08/08 18:18:01  ucko
 * Add central template class for converting ASN.1-based objects between
 * C and C++ representations.
 *
 *
 * ===========================================================================
 */

#endif /* CTOOLS___ASN_CONVERTER__HPP */
