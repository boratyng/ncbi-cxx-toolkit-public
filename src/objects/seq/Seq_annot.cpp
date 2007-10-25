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
 *   'seq.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/general/Date.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/error_codes.hpp>


#define NCBI_USE_ERRCODE_X   Objects_SeqAnnot

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSeq_annot::~CSeq_annot(void)
{
}


void CSeq_annot::AddName(const string &name)
{
    //NB: this used list::remove_if(), which is not portable to Windows
    TDesc::Tdata::iterator iter = SetDesc().Set().begin();
    for ( ;  iter != SetDesc().Set().end();  ) {
        if ((*iter)->IsName() ) {
            iter = SetDesc().Set().erase(iter);
        } else {
            ++iter;
        }
    }

    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetName(name);
    SetDesc().Set().push_back(desc);
}


void CSeq_annot::AddTitle(const string& title)
{
    LOG_POST_X(1, Warning
        << "CSeq_annot::AddTitle(): AddTitle() is deprecated, "
           "use SetTitle() instead");

    SetTitle(title);
}


void CSeq_annot::SetTitle(const string &title)
{
    TDesc::Tdata::iterator iter = SetDesc().Set().begin();
    for ( ;  iter != SetDesc().Set().end();  ) {
        if ((*iter)->IsTitle() ) {
            iter = SetDesc().Set().erase(iter);
        } else {
            ++iter;
        }
    }

    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetTitle(title);
    SetDesc().Set().push_back(desc);
}


void CSeq_annot::AddComment(const string &comment)
{
    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetComment(comment);
    SetDesc().Set().push_back(desc);
}


void CSeq_annot::SetCreateDate(const CTime& dt)
{
    CRef<CDate> date(new CDate(dt));
    SetCreateDate(*date);
}


void CSeq_annot::SetCreateDate(CDate& date)
{
    TDesc::Tdata::iterator iter = SetDesc().Set().begin();
    for ( ;  iter != SetDesc().Set().end();  ) {
        if ((*iter)->IsCreate_date() ) {
            iter = SetDesc().Set().erase(iter);
        } else {
            ++iter;
        }
    }

    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetCreate_date(date);
    SetDesc().Set().push_back(desc);
}


void CSeq_annot::SetUpdateDate(const CTime& dt)
{
    CRef<CDate> date(new CDate(dt));
    SetUpdateDate(*date);
}


void CSeq_annot::SetUpdateDate(CDate& date)
{
    TDesc::Tdata::iterator iter = SetDesc().Set().begin();
    for ( ;  iter != SetDesc().Set().end();  ) {
        if ((*iter)->IsUpdate_date() ) {
            iter = SetDesc().Set().erase(iter);
        } else {
            ++iter;
        }
    }

    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetUpdate_date(date);
    SetDesc().Set().push_back(desc);
}


void CSeq_annot::AddUserObject(CUser_object& obj)
{
    CRef<CAnnotdesc> desc(new CAnnotdesc());
    desc->SetUser(obj);
    SetDesc().Set().push_back(desc);
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#undef NCBI_USE_ERRCODE_X

/* Original file checksum: lines: 64, chars: 1875, CRC32: 377b3912 */
