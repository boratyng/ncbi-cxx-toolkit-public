/*
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
* Author:  Alexey Dobronadezhdin
*
* File Description:
*
* ===========================================================================
*/

#ifndef WGS_RAPAMS_H
#define WGS_RAPAMS_H

#include <memory>

#include <corelib/ncbiargs.hpp>
#include <objects/seqloc/Seq_id.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE
    class CDate_std;
END_objects_SCOPE
END_NCBI_SCOPE

USING_NCBI_SCOPE;
USING_SCOPE(objects);

namespace wgsparse
{

enum EUpdateMode
{
    eUpdateNew,
    eUpdatePartial,
    eUpdateAssembly,
    eUpdateScaffoldsNew,
    eUpdateFull,
    eUpdateScaffoldsUpd,
    eUpdateExtraContigs
};

enum ESource
{
    eNotSet,
    eDDBJ,
    eEMBL,
    eNCBI
};

enum EScaffoldType
{
    eRegularGenomic,
    eRegularChromosomal,
    eGenColGenomic,
    eTPAGenomic,
    eTPAChromosomal
};

enum ESortOrder
{
    eUnsorted,
    eSeqLenDesc,
    eSeqLenAsc,
    eById,
    eByAccession
};


enum EFixTech
{
    eNoFix = 0,
    eFixMolBiomol = 1 << 0,
    eFixBiomolMRNA = 1 << 1,
    eFixBiomolRRNA = 1 << 2,
    eFixBiomolNCRNA = 1 << 3
};

struct CParams_imp;

class CParams
{
public:
    bool IsTest() const;

    EUpdateMode GetUpdateMode() const;
    ESource GetSource() const;
    EScaffoldType GetScaffoldType() const;

    bool IsTpa() const;
    bool IsTsa() const;
    bool IsTls() const;
    bool IsWgs() const;
    bool IsChromosomal() const;
    bool EnforceNew() const;
    bool IsAccessionAssigned() const;
    bool IsDblinkOverride() const;
    bool IsSetSubmissionDate() const;
    bool IsVDBMode() const;
    bool IgnoreGeneralIds() const;
    bool IsReplaceDBName() const;
    bool IsSecondaryAccsAllowed() const;
    bool IsKeepRefs() const;
    bool IsAccessionsSortedInFile() const;
    bool IsUpdateScaffoldsMode() const;
    bool IsTaxonomyLookup() const;

    int GetFixTech() const;

    bool IsMasterInFile() const;
    const string& GetMasterFileName() const;

    const string& GetTpaKeyword() const;

    string GetIdPrefix() const;
    CSeq_id::E_Choice GetIdChoice() const;

    const string& GetScaffoldPrefix() const;

    char GetMajorAssemblyVersion() const;
    char GetMinorAssemblyVersion() const;
    int GetAssemblyVersion() const;

    const CDate_std& GetSubmissionDate() const;

    const list<string>& GetInputFiles() const;

    const string& GetProjPrefix() const;
    const string& GetProjAccStr() const;
    const string& GetProjAccVerStr() const;

    const string& GetAccession() const;

    ESortOrder GetSortOrder() const;

private:
    std::unique_ptr<CParams_imp> m_imp;

    friend const CParams& GetParams();
    friend bool SetParams(const CArgs& args);
    friend void SetScaffoldPrefix(const string& scaffold_prefix);

    CParams();
    CParams(const CParams&) = delete;
};

const CParams& GetParams();
bool SetParams(const CArgs& args);
void SetScaffoldPrefix(const string& scaffold_prefix);

}

#endif // WGS_RAPAMS_H