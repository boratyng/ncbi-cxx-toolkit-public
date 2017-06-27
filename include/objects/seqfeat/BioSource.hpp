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
 *   'seqfeat.asn'.
 */

#ifndef OBJECTS_SEQFEAT_BIOSOURCE_HPP
#define OBJECTS_SEQFEAT_BIOSOURCE_HPP


// generated includes
#include <objects/seqfeat/BioSource_.hpp>
#include <objects/seqfeat/SubSource.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class COrgName;
class CFieldDiff;
class CSubSource;

typedef vector< CRef<CFieldDiff> > TFieldDiffList;


class NCBI_SEQFEAT_EXPORT CBioSource : public CBioSource_Base
{
    typedef CBioSource_Base Tparent;
public:
    // constructor
    CBioSource(void);
    // destructor
    ~CBioSource(void);

    // Get the appropriate gene code from this BioSource.
    int GetGenCode(int def = 1) const;

    // function for getting genome value from organelle name
    static CBioSource::EGenome GetGenomeByOrganelle (const string& organelle, NStr::ECase use_case = NStr::eCase, bool starts_with = false);

    // function for getting organelle name from genome value
    static string GetOrganelleByGenome (unsigned int genome);

    // function for getting origin value from origin name
    static CBioSource::EOrigin GetOriginByString (const string& origin, NStr::ECase use_case = NStr::eCase, bool starts_with = false);

    // function for getting organelle name from genome value
    static string GetStringFromOrigin (unsigned int origin);


    // shortcut access to selected Org-ref and OrgName methods
    bool IsSetTaxname(void) const;
    const string& GetTaxname(void) const;

    bool IsSetCommon(void) const;
    const string& GetCommon(void) const;

    bool IsSetLineage(void) const;
    const string& GetLineage(void) const;

    bool IsSetGcode(void) const;
    int GetGcode(void) const;

    bool IsSetMgcode(void) const;
    int GetMgcode(void) const;

    bool IsSetPgcode(void) const;
    int GetPgcode(void) const;

    bool IsSetDivision(void) const;
    const string& GetDivision(void) const;

    bool IsSetOrgname(void) const;
    const COrgName& GetOrgname(void) const;

    bool IsSetOrgMod(void) const;

    // for GenColl
    string GetRepliconName (void) const;
    string GetBioprojectType (void) const;
    string GetBioprojectLocation (void) const;

    // for Taxonomy
    void SetDisableStrainForwarding(bool val);
    bool GetDisableStrainForwarding() const;

    // for BioSample
    void UpdateWithBioSample(const CBioSource& biosample, bool force, bool is_local_copy = false);
    static bool ShouldIgnoreConflict(const string& label, string src_val, string sample_val, bool is_local_copy = false);

    TFieldDiffList GetBiosampleDiffs(const CBioSource& biosample, bool is_local_copy = false) const;
    bool BiosampleDiffsOkForUpdate(const TFieldDiffList& diffs) const;

    typedef pair<string, string> TNameVal;
    typedef vector<TNameVal> TNameValList;

    TNameValList GetNameValPairs() const;

    static bool IsStopWord(const string& value);

    void AutoFix();
    void RemoveCultureNotes(bool is_species_level = true);
    bool RemoveLineageSourceNotes();

    bool RemoveSubSource(int subtype);
    bool RemoveOrgMod(int subtype);

    //If taxname starts with uncultured, set environmental-sample to true
    //If metagenomic, set environmental_sample
    //    Add environmental_sample to BioSource if BioSource.org.orgname.div == "ENV"
    //    Add metagenomic(and environmental_sample) if BioSource.org.orgname.lineage contains "metagenomes"
    //    Add metagenomic(and environmental_sample) if BioSource has / metagenome_source qualifier
    // returns true if change was made
    bool FixEnvironmentalSample();

    // Remove null terms from SubSource values and OrgMod values
    bool RemoveNullTerms();

    // do not allow sex qualifier if virus, bacteria, Archaea, or fungus
    static bool AllowSexQualifier(const string& lineage);
    bool AllowSexQualifier() const;

    // do not allow mating_type qualifier if animal, plant, or virus
    static bool AllowMatingTypeQualifier(const string& lineage);
    bool AllowMatingTypeQualifier() const;

    //Remove /sex qualifier from virus, bacteria, archaea, fungus organisms
    //Remove /mating_type qualifier from animal, plant, and virus organisms
    //Move /mating_type qualifier that is valid /sex qualifier word to /sex qualifier
    bool FixSexMatingTypeInconsistencies();

    //Remove qualifiers not appropriate for virus organisms from Virus organisms
    bool RemoveUnexpectedViralQualifiers();

    bool FixGenomeForQualifiers();

    static bool IsViral(const string& lineage);
    bool IsViral() const;

    bool HasSubtype(CSubSource::TSubtype subtype) const;

    CRef<CBioSource> MakeCommon( const CBioSource& other) const;
    CRef<CBioSource> MakeCommonExceptOrg(const CBioSource& other) const;

private:
    // Prohibit copy constructor and assignment operator
    CBioSource(const CBioSource& value);
    CBioSource& operator=(const CBioSource& value);

    void x_ClearCoordinatedBioSampleSubSources();
    void x_ClearCoordinatedBioSampleOrgMods();
    TNameValList x_GetOrgModNameValPairs() const;
    TNameValList x_GetSubtypeNameValPairs() const;
    void x_RemoveNameElementDiffs(const CBioSource& biosample, TFieldDiffList& diff_list) const;
    bool x_ShouldIgnoreNoteForBiosample() const;

    // for handling StopWords from BioSample
    static void x_InitStopWords(void);

    static void x_RemoveStopWords(COrg_ref& org_ref);
};



/////////////////// CBioSource inline methods

// constructor
inline
CBioSource::CBioSource(void)
{
}


/////////////////// end of CBioSource inline methods


// =============================================================================
//      For representing differences between BioSample and BioSource
// =============================================================================

class NCBI_SEQFEAT_EXPORT CFieldDiff : public CObject
{
public:
    CFieldDiff() {};
    CFieldDiff(string field_name, string src_val, string sample_val) :
        m_FieldName(field_name), m_SrcVal(src_val), m_SampleVal(sample_val)
        {};

    ~CFieldDiff(void) {};

    const string& GetFieldName() const { return m_FieldName; };
    const string& GetSrcVal() const { return m_SrcVal; };
    const string& GetSampleVal() const { return m_SampleVal; };

private:
    string m_FieldName;
    string m_SrcVal;
    string m_SampleVal;
};





END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_SEQFEAT_BIOSOURCE_HPP
/* Original file checksum: lines: 93, chars: 2400, CRC32: 29efac3b */
