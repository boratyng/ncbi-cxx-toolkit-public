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
 * Author:  Lewis Y. Geer
 *
 * File Description:
 *   Contains code for reading in spectrum data sets.
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the data definition file
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbi_limits.h>
#include <corelib/ncbistre.hpp>
#include <corelib/ncbistr.hpp>
#include <util/regexp.hpp>
#include <objects/omssa/MSSpectrum.hpp>


// generated includes
#include "SpectrumSet.hpp"

// added includes
#include "msms.hpp"

// generated classes
BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


/////////////////////////////////////////////////////////////////////////////
//
//  CSpectrumSet::
//



///
/// load multiple dta's in xml-like format
///
int CSpectrumSet::LoadMultDTA(std::istream& DTA, int Max)
{   
    CRef <CMSSpectrum> MySpectrum;
    int iIndex(-1); // the spectrum index
    int Count(0);  // total number of spectra
    string Line;
    //    double dummy;
    bool GotOne(false);  // has a spectrum been read?
    try {
        //       DTA.exceptions(ios_base::failbit | ios_base::badbit);
        do {
            do {
                getline(DTA, Line);
            } while (NStr::Compare(Line, 0, 4, "<dta") != 0 && DTA && !DTA.eof());
            if (!DTA || DTA.eof()) {
                if (GotOne) return 0;
                else return 1;
            }
//            GotOne = true;
            Count++;
            if (Max > 0 && Count > Max) return -1;  // too many

            MySpectrum = new CMSSpectrum;
            CRegexp RxpGetNum("\\sid\\s*=\\s*(\"(\\S+)\"|(\\S+)\b)");
            string Match;
            if ((Match = RxpGetNum.GetMatch(Line.c_str(), 0, 2)) != "" ||
                (Match = RxpGetNum.GetMatch(Line.c_str(), 0, 3)) != "") {
                MySpectrum->SetNumber(NStr::StringToInt(Match));
            } else {
                MySpectrum->SetNumber(iIndex);
                iIndex--;
            }

            CRegexp RxpGetName("\\sname\\s*=\\s*(\"(\\S+)\"|(\\S+)\b)");
            if ((Match = RxpGetName.GetMatch(Line.c_str(), 0, 2)) != "" ||
                (Match = RxpGetName.GetMatch(Line.c_str(), 0, 3)) != "") {
                MySpectrum->SetIds().push_back(Match);
            }

            if(!GetDTAHeader(DTA, MySpectrum)) return 1;
            getline(DTA, Line);
            getline(DTA, Line);

            while (NStr::Compare(Line, 0, 5, "</dta") != 0) {
                CNcbiIstrstream istr(Line.c_str());
                if (!GetDTABody(istr, MySpectrum)) break;
                GotOne = true;
                getline(DTA, Line);
            } 

            Set().push_back(MySpectrum);
        } while (DTA && !DTA.eof());

    if (!GotOne) return 1;
        
    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultDTA: " );
        throw;
    }
    return 0;
}

///
/// load multiple dta's separated by a blank line
///
int CSpectrumSet::LoadMultBlankLineDTA(std::istream& DTA, int Max)
{   
    CRef <CMSSpectrum> MySpectrum;
    int iIndex(0); // the spectrum index
    int Count(0);  // total number of spectra
    string Line;
    bool GotOne(false);  // has a spectrum been read?
    try {
//        DTA.exceptions(ios_base::failbit | ios_base::badbit);
        do {
//            GotOne = true;
            Count++;
            if (Max > 0 && Count > Max) return -1;  // too many

            MySpectrum = new CMSSpectrum;
            MySpectrum->SetNumber(iIndex);
            iIndex++;

            if (!GetDTAHeader(DTA, MySpectrum)) return 1;
            getline(DTA, Line);
            getline(DTA, Line);

            if (!DTA || DTA.eof()) {
                if (GotOne) return 0;
                else return 1;
            }

            while (Line != "") {
                CNcbiIstrstream istr(Line.c_str());
                if (!GetDTABody(istr, MySpectrum)) break;
                GotOne = true;
                getline(DTA, Line);
            } 

            Set().push_back(MySpectrum);
        } while (DTA && !DTA.eof());

        if (!GotOne) return 1;

    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultBlankLineDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultBlankLineDTA: " );
        throw;
    }

    return 0;
}


///
///  Read in the header of a DTA file
///
bool CSpectrumSet::GetDTAHeader(std::istream& DTA, CRef <CMSSpectrum>& MySpectrum)
{
    double dummy(0.0L);

    DTA >> dummy;
    if (dummy <= 0) return false;
    MySpectrum->SetPrecursormz(static_cast <int> ((dummy-1.00794)*MSSCALE));
    DTA >> dummy;
    if (dummy <= 0) return false;
    MySpectrum->SetCharge().push_back(static_cast <int> (dummy)); 
}


///
/// Read in the body of a dta file
///
bool CSpectrumSet::GetDTABody(std::istream& DTA, CRef <CMSSpectrum>& MySpectrum)
{
    double dummy(0.0L);

    DTA >> dummy;
    if (dummy <= 0) return false;
    MySpectrum->SetMz().push_back(static_cast <int> (dummy*MSSCALE));
    // attenuate the really big peaks
    DTA >> dummy;
    if (dummy <= 0) return false;
    if (dummy > kMax_UInt) dummy = kMax_UInt/MSSCALE;
    MySpectrum->SetAbundance().push_back(static_cast <int> (dummy*MSSCALE));

    return true;
}


///
/// load in a single dta file
///
int CSpectrumSet::LoadDTA(std::istream& DTA)
{   
    CRef <CMSSpectrum> MySpectrum;
    bool GotOne(false);  // has a spectrum been read?

    try {
//        DTA.exceptions(ios_base::failbit | ios_base::badbit);

        MySpectrum = new CMSSpectrum;
        MySpectrum->SetNumber(1);
        if(!GetDTAHeader(DTA, MySpectrum)) return 1;

        while (DTA) {
            if (!GetDTABody(DTA, MySpectrum)) break;
            GotOne = true;
        } 

        Set().push_back(MySpectrum);
    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadDTA: " );
        throw;
    }

    if (!GotOne) return 1;
    return 0;
}



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 1.11  2004/11/01 22:04:01  lewisg
 * c-term mods
 *
 * Revision 1.10  2004/10/20 22:24:48  lewisg
 * neutral mass bugfix, concatenate result and response
 *
 * Revision 1.9  2004/06/08 19:46:21  lewisg
 * input validation, additional user settable parameters
 *
 * Revision 1.8  2004/05/27 20:52:15  lewisg
 * better exception checking, use of AutoPtr, command line parsing
 *
 * Revision 1.7  2004/05/21 21:41:03  gorelenk
 * Added PCH ncbi_pch.hpp
 *
 * Revision 1.6  2004/03/30 19:36:59  lewisg
 * multiple mod code
 *
 * Revision 1.5  2004/03/16 20:18:54  gorelenk
 * Changed includes of private headers.
 *
 * Revision 1.4  2003/10/22 15:03:32  lewisg
 * limits and string compare changed for gcc 2.95 compatibility
 *
 * Revision 1.3  2003/10/21 21:20:57  lewisg
 * use strstream instead of sstream for gcc 2.95
 *
 * Revision 1.2  2003/10/21 21:12:16  lewisg
 * reorder headers
 *
 * Revision 1.1  2003/10/20 21:32:13  lewisg
 * ommsa toolkit version
 *
 *
 * ===========================================================================
 */
/* Original file checksum: lines: 56, chars: 1753, CRC32: bdc55e21 */
