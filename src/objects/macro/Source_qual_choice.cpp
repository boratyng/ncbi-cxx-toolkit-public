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
 * Author:  J. Chen
 *
 * File Description:
 *   Get source qual value from BioSource
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'macro.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/macro/Source_qual_choice.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSource_qual_choice::~CSource_qual_choice(void)
{
}

bool CSource_qual_choice :: IsSubsrcQual(ESource_qual src_qual) const
{
   switch (src_qual) {
     case eSource_qual_cell_line:
     case eSource_qual_cell_type:
     case eSource_qual_chromosome:
     case eSource_qual_clone:
     case eSource_qual_clone_lib:
     case eSource_qual_collected_by:
     case eSource_qual_collection_date:
     case eSource_qual_country:
     case eSource_qual_dev_stage:
     case eSource_qual_endogenous_virus_name:
     case eSource_qual_environmental_sample:
     case eSource_qual_frequency:
     case eSource_qual_fwd_primer_name:
     case eSource_qual_fwd_primer_seq:
     case eSource_qual_genotype:
     case eSource_qual_germline:
     case eSource_qual_haplotype:
     case eSource_qual_identified_by:
     case eSource_qual_insertion_seq_name:
     case eSource_qual_isolation_source:
     case eSource_qual_lab_host:
     case eSource_qual_lat_lon:
     case eSource_qual_map:
     case eSource_qual_metagenomic:
     case eSource_qual_plasmid_name:
     case eSource_qual_plastid_name:
     case eSource_qual_pop_variant:
     case eSource_qual_rearranged:
     case eSource_qual_rev_primer_name:
     case eSource_qual_rev_primer_seq:
     case eSource_qual_segment:
     case eSource_qual_sex:
     case eSource_qual_subclone:
     case eSource_qual_subsource_note:
     case eSource_qual_tissue_lib :
     case eSource_qual_tissue_type:
     case eSource_qual_transgenic:
     case eSource_qual_transposon_name:
     case eSource_qual_mating_type:
     case eSource_qual_linkage_group:
     case eSource_qual_haplogroup:
     case eSource_qual_altitude:
        return true;
     default: return false;
   }
};

string CSource_qual_choice :: GetLimitedSourceQualFromBioSource(const CBioSource& biosrc, const CString_constraint& str_cons) const
{
   string str;
   switch (Which()) {
     case e_Location:
       {
          str = CBioSource::ENUM_METHOD_NAME(EGenome)()
             ->FindName((CBioSource::EGenome)biosrc.GetGenome(), false);
          if (str == "unknown") {
               str = kEmptyStr;
          }
          else if (str == "extrachrom") {
              str = "extrachromosomal";
          }
          if (!str_cons.Match (str)) {
              str = kEmptyStr;
          }
          break;
       }
     case e_Origin:
       {
          str = CBioSource::ENUM_METHOD_NAME(EOrigin)()
                     ->FindName((CBioSource::EOrigin)biosrc.GetOrigin(), false);
          if (!str_cons.Match(str)) {
            str = kEmptyStr;
          }
          break;
       }
     case e_Gcode:
       if (biosrc.IsSetGcode() && biosrc.GetGcode()) {
          str = NStr::IntToString(biosrc.GetGcode());
       }
       break;
    case e_Mgcode:
       if (biosrc.IsSetMgcode() && biosrc.GetMgcode()) {
              str = NStr::IntToString(biosrc.GetMgcode());
       }
       break;
    default: break;
   }
   return str;
};

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1747, CRC32: 9261c344 */
