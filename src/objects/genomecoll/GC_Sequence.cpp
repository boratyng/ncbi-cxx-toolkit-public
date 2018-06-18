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
 *   using the following specifications:
 *   'genome_collection.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/genomecoll/GC_Sequence.hpp>
#include <objects/genomecoll/GC_SequenceStats.hpp>
#include <objects/genomecoll/GC_AssemblyUnit.hpp>
#include <objects/genomecoll/GC_Assembly.hpp>
#include <objects/genomecoll/GC_Replicon.hpp>
#include <objects/genomecoll/GC_External_Seqid.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


// constructor
CGC_Sequence::CGC_Sequence(void)
    : m_Assembly(NULL)
    , m_AssemblyUnit(NULL)
    , m_Replicon(NULL)
    , m_ParentSequence(NULL)
    , m_ParentRel(CGC_TaggedSequences::eState_not_set)
    , m_SeqLength(0)
    , m_SeqLengthRetrieved(false)
{
}


// destructor
CGC_Sequence::~CGC_Sequence(void)
{
}


CConstRef<CGC_AssemblyUnit> CGC_Sequence::GetAssemblyUnit() const
{
    return CConstRef<CGC_AssemblyUnit>(m_AssemblyUnit);
}

CConstRef<CGC_Assembly> CGC_Sequence::GetFullAssembly() const
{
    return CConstRef<CGC_Assembly>(m_Assembly);
}

CConstRef<CGC_Replicon> CGC_Sequence::GetReplicon() const
{
    return CConstRef<CGC_Replicon>(m_Replicon);
}

CConstRef<CGC_Sequence> CGC_Sequence::GetParent() const
{
    return CConstRef<CGC_Sequence>(m_ParentSequence);
}


CGC_TaggedSequences::TState CGC_Sequence::GetParentRelation() const
{
    return m_ParentRel;
}


CConstRef<CGC_Sequence> CGC_Sequence::GetTopLevelParent() const
{
    CConstRef<CGC_Sequence> top(this);
    for ( ;  top  &&  top->GetParent();  top = top->GetParent()) {
    }
    return top;
}

CConstRef<CSeq_id> CGC_Sequence::GetSynonymSeq_id( CGC_TypedSeqId::E_Choice syn_type, CGC_SeqIdAlias::E_AliasTypes alias_type ) const
{
    CConstRef<CSeq_id> ret;
    if( !IsSetSeq_id_synonyms() )
        return ret;

    CConstRef<CGC_TypedSeqId> typed_seq_id;

    ITERATE( TSeq_id_synonyms, it_syn, GetSeq_id_synonyms() )
    {
        if( (*it_syn)->Which() == syn_type )
        {
            typed_seq_id = *it_syn;
            break;
        }
    }

    if( typed_seq_id.IsNull() )
        return ret;


    if( syn_type == CGC_TypedSeqId::e_Genbank || syn_type == CGC_TypedSeqId::e_Refseq )
    {
        CConstRef<CGC_SeqIdAlias> seq_id_alias;
        if( syn_type == CGC_TypedSeqId::e_Genbank )
            seq_id_alias = CConstRef<CGC_SeqIdAlias> ( &typed_seq_id->GetGenbank() );
        else
            seq_id_alias = CConstRef<CGC_SeqIdAlias>( &typed_seq_id->GetRefseq() );

        switch( alias_type )
        {
        case CGC_SeqIdAlias::e_None: 
            break;
        case CGC_SeqIdAlias::e_Public:
            if( seq_id_alias->IsSetPublic() )
                ret = CConstRef<CSeq_id>( &seq_id_alias->GetPublic() );
            break;
        case CGC_SeqIdAlias::e_Gpipe:
            if( seq_id_alias->IsSetGpipe() )
                ret = CConstRef<CSeq_id>( &seq_id_alias->GetGpipe() );
            break;
        case CGC_SeqIdAlias::e_Gi:
            if( seq_id_alias->IsSetGi() )
                ret = CConstRef<CSeq_id>( &seq_id_alias->GetGi() );
            break;
        default:
            NCBI_THROW(CException, eUnknown, "unhandled alias type");
        }
        
    }
    else if( syn_type == CGC_TypedSeqId::e_Private )
        ret = CConstRef<CSeq_id>( &typed_seq_id->GetPrivate() );

    else if( syn_type == CGC_TypedSeqId::e_External )
        ret = CConstRef<CSeq_id>( &typed_seq_id->GetExternal().GetId() );

    return ret;
}


CConstRef<CSeq_id> CGC_Sequence::GetSubmitterName( ) const
{
    CConstRef<CSeq_id> ret;
    
    if( !IsSetSeq_id_synonyms() )
        return ret;

    CConstRef<CGC_TypedSeqId> typed_seq_id;
    ITERATE( TSeq_id_synonyms, it_syn, GetSeq_id_synonyms() )
    {
        if( (*it_syn)->IsExternal() && 
            NStr::EqualNocase((*it_syn)->GetExternal().GetExternal(), "SUBMITTER") )
        {
            ret = &((*it_syn)->GetExternal().GetId());
            break;
        }
    }
    
    return ret;
    
}

bool CGC_Sequence::HasRole(int Role) const 
{
	if( !CanGetRoles() )
		return false;

	ITERATE(TRoles, roleiter, GetRoles()) {
		if(*roleiter == Role)
			return true;
	}

	return false;
}

string CGC_Sequence::GetChrName() const
{
    if (GetTopLevelParent()->GetReplicon() &&
        GetTopLevelParent()->GetReplicon()->IsSetName())
    {
        return GetTopLevelParent()->GetReplicon()->GetName();
    } else {
        return "Un";
    }
}

string CGC_Sequence::GetUnitDisplayName() const
{
    return GetAssemblyUnit()->GetDisplayName();
}


bool CGC_Sequence::IsOrganelle() const
{
    return GetChrName() == "MT" || GetChrName() == "Pltd";
}

bool CGC_Sequence::CanGetLength() const
{
    return ( x_GetLength() != 0);
}

TSeqPos CGC_Sequence::GetLength() const
{
    TSeqPos result = x_GetLength();
    if (result == 0) {
         NCBI_THROW(CUnassignedMember, eGet, "CGC_Sequence::GetLength(): Sequence length not set.");
    }
    return result;
}

TSeqPos CGC_Sequence::x_GetLength() const
{
    if ( ! m_SeqLengthRetrieved) {
        if (HasRole(eGC_SequenceRole_chromosome)) {
            if (CanGetStats() && GetStats().CanGetOrdered_scaf()) {
                ITERATE(list< CRef< CGC_Scaf_stats > >, stat_it, GetStats().GetOrdered_scaf()) {
                    if ( (*stat_it)->GetStats_category() == CGC_Scaf_stats::eStats_category_total_length)
                    {
                        m_SeqLength = static_cast<TSeqPos>((*stat_it)->GetValue());
                        break;
                    }
                }
            }
        } else {
            if (CanGetStats() && GetStats().CanGetAll_scaf()) {
                ITERATE(list< CRef< CGC_Scaf_stats > >, stat_it, GetStats().GetAll_scaf()) {
                    if ( (*stat_it)->GetStats_category() == CGC_Scaf_stats::eStats_category_total_length)
                    {
                        m_SeqLength = static_cast<TSeqPos>((*stat_it)->GetValue());
                        break;
                    }
                }
            }
        }
        m_SeqLengthRetrieved = true;
    }
    return m_SeqLength;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1758, CRC32: 7250bf25 */
