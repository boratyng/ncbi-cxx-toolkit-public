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
 * Author:  Viatcheslav Gorelenkov
 *
 */

#include <ncbi_pch.hpp>
#include <app/project_tree_builder/proj_datatool_generated_src.hpp>
#include <app/project_tree_builder/file_contents.hpp>
#include <app/project_tree_builder/proj_builder_app.hpp>


BEGIN_NCBI_SCOPE


CDataToolGeneratedSrc::CDataToolGeneratedSrc(void)
{
}


CDataToolGeneratedSrc::CDataToolGeneratedSrc(const string& source_file_path)
{
    LoadFrom(source_file_path, this);
}


CDataToolGeneratedSrc::CDataToolGeneratedSrc(const CDataToolGeneratedSrc& src)
{
    SetFrom(src);
}


CDataToolGeneratedSrc& 
CDataToolGeneratedSrc::operator= (const CDataToolGeneratedSrc& src)
{
    if(this != &src)
    {
        SetFrom(src);
    }
    return *this;
}


CDataToolGeneratedSrc::~CDataToolGeneratedSrc(void)
{
}


bool CDataToolGeneratedSrc::operator== (const CDataToolGeneratedSrc& src) const
{

    return  m_SourceFile        == src.m_SourceFile    &&
            m_SourceBaseDir     == src.m_SourceBaseDir &&
            m_ImportModules     == src.m_ImportModules ;
}


bool CDataToolGeneratedSrc::operator<  (const CDataToolGeneratedSrc& src) const
{
    if (m_SourceFile < src.m_SourceFile)
        return true;
    else if (m_SourceFile > src.m_SourceFile)
        return false;
    else {
        if (m_SourceBaseDir < src.m_SourceBaseDir)
            return true;
        else if (m_SourceBaseDir > src.m_SourceBaseDir)
            return false;
        else {
            return m_ImportModules < src.m_ImportModules;
        }
    }
}


void CDataToolGeneratedSrc::LoadFrom(const string&          source_file_path,
                                     CDataToolGeneratedSrc* src)
{
    src->Clear();

    string dir;
    string base;
    string ext;
    CDirEntry::SplitPath(source_file_path, &dir, &base, &ext);

    src->m_SourceBaseDir = dir;
    src->m_SourceFile    = base + ext;

    {{
        // module file
        string module_path = CDirEntry::ConcatPath(dir, 
                                                  base + ".module");
        if ( CDirEntry(module_path).Exists() ) {
            CSimpleMakeFileContents fc(module_path, eMakeType_Undefined);
            CSimpleMakeFileContents::TContents::const_iterator p = 
                fc.m_Contents.find("MODULE_IMPORT");
            if (p != fc.m_Contents.end()) {
                const list<string>& modules = p->second;
                ITERATE(list<string>, p, modules) {
                    // add ext from source file to all modules to import
                    const string& module = *p;
                    src->m_ImportModules.push_back
                                        (CDirEntry::NormalizePath(module + ext));
                }
            }
        } else {
            LOG_POST(Info << "Datatool module file not found: " + module_path);
        }
    }}

    {{
        // files file
        string files_path = CDirEntry::ConcatPath(dir, 
                                                  base + ".files");

        if ( CDirEntry(files_path).Exists() ) {

            CSimpleMakeFileContents fc(files_path, eMakeType_Undefined);

            // GENERATED_HPP
            CSimpleMakeFileContents::TContents::const_iterator p = 
                fc.m_Contents.find("GENERATED_HPP");
            if (p != fc.m_Contents.end())
                src->m_GeneratedHpp = p->second;

            // GENERATED_CPP
            p = fc.m_Contents.find("GENERATED_CPP");
            if (p != fc.m_Contents.end())
                src->m_GeneratedCpp = p->second;

            // GENERATED_HPP_LOCAL
            p = fc.m_Contents.find("GENERATED_HPP_LOCAL");
            if (p != fc.m_Contents.end())
                src->m_GeneratedHppLocal = p->second;

            // GENERATED_CPP_LOCAL
            p = fc.m_Contents.find("GENERATED_CPP_LOCAL");
            if (p != fc.m_Contents.end())
                src->m_GeneratedCppLocal = p->second;

            // SKIPPED_HPP
            p = fc.m_Contents.find("SKIPPED_HPP");
            if (p != fc.m_Contents.end())
                src->m_SkippedHpp = p->second;

            // SKIPPED_CPP
            p = fc.m_Contents.find("SKIPPED_CPP");
            if (p != fc.m_Contents.end())
                src->m_SkippedCpp = p->second;

            // SKIPPED_HPP_LOCAL
            p = fc.m_Contents.find("SKIPPED_HPP_LOCAL");
            if (p != fc.m_Contents.end())
                src->m_SkippedHppLocal = p->second;

            // SKIPPED_CPP_LOCAL
            p = fc.m_Contents.find("SKIPPED_CPP_LOCAL");
            if (p != fc.m_Contents.end())
                src->m_SkippedCppLocal = p->second;
        } else {
            LOG_POST(Info << "Datatool-generated file not found: " + files_path);
        }


    }}

}


bool CDataToolGeneratedSrc::IsEmpty(void) const
{
    return m_SourceFile.empty();
}


void CDataToolGeneratedSrc::Clear(void)
{
    m_SourceFile.erase();
    m_SourceBaseDir.erase();
    m_ImportModules.clear();

    m_GeneratedHpp.clear();
    m_GeneratedCpp.clear();
    m_GeneratedHppLocal.clear();
    m_GeneratedCppLocal.clear();
    m_SkippedHpp.clear();
    m_SkippedCpp.clear();
    m_SkippedHppLocal.clear();
    m_SkippedCppLocal.clear();
}


void CDataToolGeneratedSrc::SetFrom(const CDataToolGeneratedSrc& src)
{
    m_SourceFile        = src.m_SourceFile;
    m_SourceBaseDir     = src.m_SourceBaseDir;
    m_ImportModules     = src.m_ImportModules;

    m_GeneratedHpp      = src.m_GeneratedHpp;
    m_GeneratedCpp      = src.m_GeneratedCpp;
    m_GeneratedHppLocal = src.m_GeneratedHppLocal;
    m_GeneratedCppLocal = src.m_GeneratedCppLocal;
    m_SkippedHpp        = src.m_SkippedHpp;
    m_SkippedCpp        = src.m_SkippedCpp;
    m_SkippedHppLocal   = src.m_SkippedHppLocal;
    m_SkippedCppLocal   = src.m_SkippedCppLocal;
}




END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.8  2005/01/31 16:37:38  gouriano
 * Keep track of subproject types and propagate it down the project tree
 *
 * Revision 1.7  2004/12/06 18:12:20  gouriano
 * Improved diagnostics
 *
 * Revision 1.6  2004/11/17 19:54:30  gouriano
 * Ignore missing module file
 *
 * Revision 1.5  2004/05/21 21:41:41  gorelenk
 * Added PCH ncbi_pch.hpp
 *
 * Revision 1.4  2004/03/10 16:48:53  gorelenk
 * Implemented operators == and < of class CDataToolGeneratedSrc.
 *
 * Revision 1.3  2004/02/20 22:53:58  gorelenk
 * Added analysis of ASN projects depends.
 *
 * Revision 1.2  2004/02/13 23:07:38  gorelenk
 * Added data members-abstraction of datatool generated files.
 *
 * Revision 1.1  2004/01/30 20:44:22  gorelenk
 * Initial revision.
 *
 * ===========================================================================
 */
