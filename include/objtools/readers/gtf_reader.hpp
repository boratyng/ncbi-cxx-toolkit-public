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
 * Author: Frank Ludwig
 *
 * File Description:
 *   BED file reader
 *
 */

#ifndef OBJTOOLS_READERS___GTF_READER__HPP
#define OBJTOOLS_READERS___GTF_READER__HPP

#include <corelib/ncbistd.hpp>
#include <objtools/readers/gff2_reader.hpp>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects) // namespace ncbi::objects::

//  ============================================================================
class CGtfAttributes
//  ============================================================================
{
public:
    using MultiValue = vector<string>;
    using MultiAttributes = map<string, MultiValue>;

    const MultiAttributes&
    Get() const
    {
        return mAttributes;
    };

    string
    ValueOf(
        const string& key) const
    {
        MultiValue values;
        GetValues(key, values);
        if (values.size() == 1) {
            return values.front();
        }
        return "";
    }

    bool
    HasValue(
        const string& key,
        const string& value = "") const
    {
        auto it = mAttributes.find(key);
        if (it == mAttributes.end()) {
            return false;
        }
        const auto& values = it->second;
        if (values.empty()) {
            return false;
        }
        if (value.empty()) {
            return true;
        }
        return (find(values.begin(), values.end(), value) != values.end());
    };

    void
    GetValues(
        const string& key,
        MultiValue& values) const
    {
        const MultiValue empty;
        values = empty;
        auto it = mAttributes.find(key);
        if (it != mAttributes.end()) {
            values = it->second;
        }
    };

    void
    AddValue(
        const string& key,
        const string& value)
    {
        auto kit = mAttributes.find(key);
        if (kit == mAttributes.end()) {
            kit = mAttributes.insert(make_pair(key, MultiValue())).first;
        }
        auto vit = find(kit->second.begin(), kit->second.end(), value);
        if (vit == kit->second.end()) {
            kit->second.push_back(value);
        }
    };

protected:
    MultiAttributes mAttributes;
};

//  ============================================================================
class CGtfReadRecord
//  ============================================================================
    : public CGff2Record
{
public:
    CGtfReadRecord(): CGff2Record() {};
    ~CGtfReadRecord() {};

    const CGtfAttributes&
    GtfAttributes() const 
    {
        return mAttributes;
    };

    string
    GeneKey() const
    {
        string geneId = mAttributes.ValueOf("gene_id");
        if (geneId.empty()) {
            cerr << "Unexpected: GTF feature without a gene_id." << endl;
        }
        return geneId;
    };

    string
    FeatureKey() const
    {
        static unsigned int tidCounter(1);
        if (Type() == "gene") {
            return GeneKey();
        }

        string transcriptId = mAttributes.ValueOf("transcript_id");
        if (transcriptId.empty()) {
            transcriptId = "t" + NStr::IntToString(tidCounter++);
        }
        return GeneKey() + "_" + transcriptId;
    }

protected:
    bool xAssignAttributesFromGff(
        const string&,
        const string& );

    CGtfAttributes mAttributes;
};

//  ----------------------------------------------------------------------------
class NCBI_XOBJREAD_EXPORT CGtfReader
//  ----------------------------------------------------------------------------
    : public CGff2Reader
{
public:
    enum EGtfFlags {
        fGenerateChildXrefs = 1<<8,
    };

    CGtfReader( 
        unsigned int =0, 
        const string& = "", 
        const string& = "",
        SeqIdResolver = CReadUtil::AsSeqId );

    virtual ~CGtfReader();
    
protected:
    virtual CGff2Record* x_CreateRecord() { return new CGtfReadRecord(); };    

    virtual bool xUpdateAnnotFeature(
        const CGff2Record&,
        CRef< CSeq_annot >,
        ILineErrorListener* =0);

    virtual bool x_UpdateAnnotCds(
        const CGtfReadRecord&,
        CRef< CSeq_annot > );

    virtual bool x_UpdateAnnotTranscript(
        const CGtfReadRecord&,
        CRef< CSeq_annot > );

    bool x_CreateFeatureId(
        const CGtfReadRecord&,
        const string&,
        CRef< CSeq_feat > );

    bool x_CreateFeatureLocation(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
    
    bool x_CreateGeneXrefs(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
    
    bool x_CreateMrnaXrefs(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
    
    bool x_CreateCdsXrefs(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
    
    bool x_MergeFeatureLocationSingleInterval(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
    
    bool x_MergeFeatureLocationMultiInterval(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );

    bool x_CreateParentGene(
        const CGtfReadRecord&,
        CRef< CSeq_annot > );
        
    bool x_MergeParentGene(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
            
    bool xFeatureSetQualifiersGene(
        const CGtfReadRecord& record,
        CRef<CSeq_feat>);

    bool xFeatureSetQualifiersRna(
        const CGtfReadRecord& record,
        CRef<CSeq_feat>);

    bool xFeatureSetQualifiersCds(
        const CGtfReadRecord& record,
        CRef<CSeq_feat>);

    bool x_CreateParentCds(
        const CGtfReadRecord&,
        CRef< CSeq_annot > );
        
    bool x_CreateParentMrna(
        const CGtfReadRecord&,
        CRef< CSeq_annot > );
        
    bool x_MergeParentCds(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );
            
    bool x_FeatureSetDataGene(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );

    virtual bool x_FeatureSetDataRna(
        const CGtfReadRecord&,
        CRef< CSeq_feat >,
        CSeqFeatData::ESubtype );

    bool x_FeatureSetDataMRNA(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );

    bool x_FeatureSetDataCDS(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );

    bool x_FeatureTrimQualifiers(
        const CGtfReadRecord&,
        CRef< CSeq_feat > );

protected:
    bool x_FindParentGene(
        const CGtfReadRecord&,
        CRef< CSeq_feat >& );

    bool x_FindParentCds(
        const CGtfReadRecord&,
        CRef< CSeq_feat >& );

    bool x_FindParentMrna(
        const CGtfReadRecord&,
        CRef< CSeq_feat >& );

    virtual bool xProcessQualifierSpecialCase(
        const string&,
        const CGtfAttributes::MultiValue&,
        CRef< CSeq_feat > );

    virtual void
        xFeatureAddQualifiers(
            const string& key,
            const CGtfAttributes::MultiValue& values,
            CRef<CSeq_feat> pFeature)
    {
        for (auto value: values) {
            pFeature->AddQualifier(key, value);
        }
    };

    bool x_CdsIsPartial(
        const CGtfReadRecord& );

    typedef map< string, CRef< CSeq_feat > > TIdToFeature;
    TIdToFeature m_GeneMap;
    TIdToFeature m_CdsMap;
    TIdToFeature m_MrnaMap;

    typedef map<string, string> TGeneIdToLocusTag;
    TGeneIdToLocusTag m_LocusTags;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif // OBJTOOLS_READERS___GTF_READER__HPP
