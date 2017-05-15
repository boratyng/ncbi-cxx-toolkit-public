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
 * Authors:  Anton Perkov
 *
 */

#include <ncbi_pch.hpp>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#if NCBI_COMPILER_VERSION >= 310
#  include <cxxabi.h>
#endif
#include <stdio.h>
#include <dlfcn.h>

BEGIN_NCBI_SCOPE


// Call this function to get a backtrace.
class CStackTraceImpl
{
public:
    CStackTraceImpl(void);
    ~CStackTraceImpl(void);

    void Expand(CStackTrace::TStack& stack);

private:
    typedef void*               TStackFrame;
    typedef vector<TStackFrame> TStack;

    TStack m_Stack;
};


CStackTraceImpl::CStackTraceImpl(void)
{
    m_Stack.resize(CStackTrace::s_GetStackTraceMaxDepth());
}


CStackTraceImpl::~CStackTraceImpl(void)
{
}


void CStackTraceImpl::Expand(CStackTrace::TStack& stack)
{
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        CStackTrace::SStackFrameInfo info;
        unw_word_t offset;

        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0 && sym[0]) {
            info.func = sym;
            info.offs = offset;
#if NCBI_COMPILER_VERSION >= 310
            // use abi::__cxa_demangle
            size_t len = 0;
            char* buf = 0;
            int status = 0;
            buf = abi::__cxa_demangle(info.func.c_str(),
                                      buf, &len, &status);
            if ( !status ) {
                info.func = buf;
                free(buf);
            }
#endif
        } else {
            info.func = "???";
        }
        info.file = "???";

        unw_proc_info_t u;
        if (unw_get_proc_info(&cursor, &u) == 0) {
            info.addr = (void*)(u.start_ip+offset);
            if (info.addr) {
                Dl_info dlinfo;
                if (dladdr(info.addr, &dlinfo) != 0) {
                    info.module = dlinfo.dli_fname;
                }
            }
        }

        stack.push_back(info);
    }
}

    /*
      char** syms = backtrace_symbols(&m_Stack[0], (int)m_Stack.size());
    for (size_t i = 0;  i < m_Stack.size();  ++i) {
        string sym = syms[i];

        CStackTrace::SStackFrameInfo info;
        info.func = sym.empty() ? "???" : sym;
        info.file = "???";
        info.offs = 0;
        info.line = 0;

        string::size_type pos = sym.find_last_of("[");
        if (pos != string::npos) {
            string::size_type epos = sym.find_first_of("]", pos + 1);
            if (epos != string::npos) {
                info.addr = NStr::StringToPtr(sym.substr(pos + 1, epos - pos - 1));
            }
        }
        pos = sym.find_first_of("(");
        if (pos != string::npos) {
            info.module = sym.substr(0, pos);
            sym.erase(0, pos + 1);
        }

        pos = sym.find_first_of(")");
        if (pos != string::npos) {
            sym.erase(pos);
            pos = sym.find_last_of("+");
            if (pos != string::npos) {
                string sub = sym.substr(pos + 1, sym.length() - pos);
                info.func = sym.substr(0, pos);
                info.offs = NStr::StringToInt(sub, 0, 16);
            }
        }

        //
        // name demangling
        //
        if ( !info.func.empty()  &&  info.func[0] == '_') {
    */


END_NCBI_SCOPE
