#ifndef UTIL_COMPRESS__TAR__HPP
#define UTIL_COMPRESS__TAR__HPP

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
 * Authors:  Vladimir Ivanov
 *           Anton Lavrentiev
 *
 * File Description:
 *   Tar archive API.
 *
 *   Supports subset of POSIX.1-1988 (ustar) format.
 *   Old GNU (POSIX 1003.1) and V7 formats are also supported partially.
 *   New archives are created using POSIX (genuine ustar) format, using
 *   GNU extensions for long names/links only when unavoidably.
 *   Can handle no exotics like sparse files, devices, etc,
 *   but just regular files, directories, and symbolic links.
 *
 */

#include <corelib/ncbifile.hpp>
#include <list>
#include <memory>
#include <utility>


/** @addtogroup Compression
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// ETarMode --
///
/// Permission bits as defined in tar
///

enum ETarModeBits {
    // Special mode bits
    fTarSetUID    = 04000,   // set UID on execution
    fTarSetGID    = 02000,   // set GID on execution
    fTarSticky    = 01000,   // reserved (sticky bit)
    // File permissions
    fTarURead     = 00400,   // read by owner
    fTarUWrite    = 00200,   // write by owner
    fTarUExecute  = 00100,   // execute/search by owner
    fTarGRead     = 00040,   // read by group
    fTarGWrite    = 00020,   // write by group
    fTarGExecute  = 00010,   // execute/search by group
    fTarORead     = 00004,   // read by other
    fTarOWrite    = 00002,   // write by other
    fTarOExecute  = 00001    // execute/search by other
};
typedef unsigned int TTarMode; // Bitwise OR of ETarModeBits


/////////////////////////////////////////////////////////////////////////////
///
/// CTarException --
///
/// Define exceptions generated by the API.
///
/// CTarException inherits its basic functionality from CCoreException
/// and defines additional error codes for tar archive operations.

class NCBI_XUTIL_EXPORT CTarException : public CCoreException
{
public:
    /// Error types that file operations can generate.
    enum EErrCode {
        eUnsupportedTarFormat,
        eUnsupportedEntryType,
        eNameTooLong,
        eChecksum,
        eBadName,
        eCreate,
        eOpen,
        eRead,
        eWrite,
        eBackup,
        eMemory,
        eRestoreAttrs
    };

    /// Translate from an error code value to its string representation.
    virtual const char* GetErrCodeString(void) const
    {
        switch (GetErrCode()) {
        case eUnsupportedTarFormat: return "eUnsupportedTarFormat";
        case eUnsupportedEntryType: return "eUnsupportedEntryType";
        case eNameTooLong:          return "eNameTooLong";
        case eChecksum:             return "eChecksum";
        case eBadName:              return "eBadName";
        case eCreate:               return "eCreate";
        case eOpen:                 return "eOpen";
        case eRead:                 return "eRead";
        case eWrite:                return "eWrite";
        case eBackup:               return "eBackup";
        case eMemory:               return "eMemory";
        case eRestoreAttrs:         return "eRestoreAttrs";
        default:                    return CException::GetErrCodeString();
        }
    }

    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CTarException, CCoreException);
};


//////////////////////////////////////////////////////////////////////////////
///
/// CTarEntryInfo class
///
/// Information about a tar archive entry.

class NCBI_XUTIL_EXPORT CTarEntryInfo
{
public:
    /// Which entry type.
    enum EType {
        eFile        = CDirEntry::eFile,    ///< Regular file
        eDir         = CDirEntry::eDir,     ///< Directory
        eLink        = CDirEntry::eLink,    ///< Symbolic link
        eUnknown     = CDirEntry::eUnknown, ///< Unknown type
        eGNULongName = eUnknown + 1,        ///< GNU long name
        eGNULongLink = eUnknown + 2         ///< GNU long link
    };

    // Constructor
    CTarEntryInfo()
        : m_Type(eUnknown)
    {
        memset(&m_Stat, 0, sizeof(m_Stat));
    }

    // No setters -- they are not needed for access by the user, and
    // thus are done directly from CTar for the sake of performance.

    // Getters only!
    const string& GetName(void)             const { return m_Name;          }
    EType         GetType(void)             const { return m_Type;          }
    Uint8         GetSize(void)             const { return m_Stat.st_size;  }
    TTarMode      GetMode(void)             const; // Raw mode as stored in tar
    void          GetMode(CDirEntry::TMode*            user_mode,
                          CDirEntry::TMode*            group_mode   = 0,
                          CDirEntry::TMode*            other_mode   = 0,
                          CDirEntry::TSpecialModeBits* special_bits = 0) const;
    int           GetUserId(void)           const { return m_Stat.st_uid;   }
    int           GetGroupId(void)          const { return m_Stat.st_gid;   }
    const string& GetLinkName(void)         const { return m_LinkName;      }
    const string& GetUserName(void)         const { return m_UserName;      }
    const string& GetGroupName(void)        const { return m_GroupName;     }
    time_t        GetModificationTime(void) const { return m_Stat.st_mtime; }

private:
    string       m_Name;       ///< Name of file
    EType        m_Type;       ///< Type
    string       m_UserName;   ///< User name
    string       m_GroupName;  ///< Group name (empty string for MSWin)
    string       m_LinkName;   ///< Name of linked file if type is eLink
    struct stat  m_Stat;       ///< Dir entry compatible info

    friend class CTar;
};


NCBI_XUTIL_EXPORT ostream& operator << (ostream&, const CTarEntryInfo&);


/// Forward declaration of tar header used internally
struct SHeader;


//////////////////////////////////////////////////////////////////////////////
///
/// CTar class
///
/// (Throws exceptions on most errors.)
/// Note that if a stream constructor was used then CTar can only perform
/// one pass over the archive.  This means that only one full action will
/// succeed (and it the action was to update (e.g. append) the archive, it
/// has to be explicitly followed by Close().  Before next action, you should
/// explicitly reset the stream position to the beginning of the archive
/// for read/update operations or to end of archive for append operations.

class NCBI_XUTIL_EXPORT CTar
{
public:
    /// General flags
    enum EFlags {
        // --- Extract/List/Test ---
        /// Ignore blocks of zeros in archive.
        /// Generally, 2 or more consecutive zero blocks indicate EOF.
        fIgnoreZeroBlocks   = (1<<1),

        // --- Extract/Append ---
        /// Follow symbolic links (instead of overwriting them)
        fFollowLinks        = (1<<2),

        // --- Extract ---
        /// Allow to overwrite existing entries with entries from the archive
        fOverwrite          = (1<<3),
        /// Update entries that are older than those already in the archive
        fUpdate             = (1<<4) | fOverwrite,
        /// Backup destinations if they exist (all entries including dirs)
        fBackup             = (1<<5) | fOverwrite,
        /// If destination entry exists, it must have the same type as source
        fEqualTypes         = (1<<6),
        /// Create extracted files with the same ownership
        fPreserveOwner      = (1<<7),
        /// Create extracted files with the same permissions
        fPreserveMode       = (1<<8),
        /// Preserve date/times for extracted files
        fPreserveTime       = (1<<9),
        /// Preserve all attributes
        fPreserveAll        = fPreserveOwner | fPreserveMode | fPreserveTime,

        // --- Update ---
        fUpdateExistingOnly = (1<<10),

        /// Default flags
        fDefault            = fOverwrite | fPreserveAll
    };
    typedef unsigned int TFlags;  ///< Bitwise OR of EFlags


    /// Constructors
    CTar(const string& file_name, size_t blocking_factor = 20);
    /// Stream version does not at all use stream positioning and so
    /// is safe on non-positioning-able streams, like magnetic tapes :-I
    CTar(CNcbiIos& stream, size_t blocking_factor = 20);

    /// Destructor (finalize the archive if currently open).
    /// @sa
    ///   Close
    virtual ~CTar();


    /// Define a list of entries.
    typedef list< CTarEntryInfo > TEntries;
    /// Define a list of files with sizes.
    typedef list< pair<string, Uint8> > TFiles;


    //------------------------------------------------------------------------
    // Main functions
    //------------------------------------------------------------------------

    /// Create a new empty archive.
    ///
    /// If a file with such name already exists it will be reset.
    /// @sa
    ///   Append
    void Create(void);

    /// Close the archive making sure all pending output is flushed.
    ///
    /// Normally, direct call of this method need _not_ intersperse
    /// successive archive manipulations by other methods, as they open
    /// and close the archive automatically as necessary.  Rather, this
    /// call is to make sure the archive is complete earlier than it
    /// usually be done in the destructor of the CTar object.
    /// @sa
    ///   ~CTar
    void Close(void);

    /// Append an entry at the end of the archive that already exists.
    ///
    /// Appended entry can be either a file, a directory, or a symbolic link.
    /// The name of the entry may not contain leading '..'.
    /// Leading slash in the absolute path will be removed.
    /// The names of all appended entries will be converted to Unix format
    /// (that is, to have forward slashes in the paths).
    /// All entries will be added at the end of the archive.
    /// @sa
    ///   Create, Update, SetBaseDir
    auto_ptr<TEntries> Append(const string& entry_name);

    /// Look for more recent copies, if available, of archive members,
    /// and place them at the end of the archive:
    ///
    /// if fUpdateExistingOnly is set in processing flags, only the
    /// existing archive entries (including directories) will be updated;
    /// that is, Update(".") won't recursively add "." if "." is not
    /// the archive member;  it will, however, do the recursive update
    /// should "." be found in the archive.
    ///
    /// if fUpdateExistingOnly is unset, the existing entries will be
    /// updated (if newer), and inexistent entries will be added to
    /// the archive; that is, Update(".") will recursively scan "."
    /// to update both existing entries (if newer files found),
    /// and add new entries for any files/directories, which are
    /// currently not in the archive.
    ///
    /// @sa
    ///   Append, SetBaseDir, SetFlags
    auto_ptr<TEntries> Update(const string& entry_name);

/*
    // Delete an entry from the archive (not for use on magnetic tapes :-)
    void Delete(const string& entry_name);

    // Find file system entries that differ from corresponding
    // entries already in the archive.
    auto_ptr<TEntries> Diff(const string& diff_dir);
*/

    /// Extract the entire archive (into either current directory or
    /// a directory otherwise specified by SetBaseDir()).
    ///
    /// Extract all archive entries, which names match pre-set mask.
    /// @sa SetMask, SetBaseDir
    auto_ptr<TEntries> Extract(void);

    /// Get information about all matching archive entries.
    ///
    /// @return
    ///   An array containing information on those archive entries
    ///   which names match pre-set mask.
    /// @sa SetMask
    auto_ptr<TEntries> List(void);

    /// Verify archive integrity.
    ///
    /// Simulate extracting files from the archive without actually
    /// creating them on disk.
    void Test(void);

    /// Return archive size as if input entries were put in it.
    /// Note that the return value is not exact but upper bound of
    /// what the archive size can be expected.  This call does not recurse
    /// in any subdirectries but relies solely upon the information as
    /// passed via parameter.
    ///
    /// The returned size includes all necessary alignments and padding.
    Uint8 EstimateArchiveSize(const TFiles& files);


    //------------------------------------------------------------------------
    // Utility functions
    //------------------------------------------------------------------------

    /// Get processing flags.
    TFlags GetFlags(void) const;

    /// Set processing flags.
    void   SetFlags(TFlags flags);

    /// Set name mask.
    ///
    /// Use this set of masks to process entries in archive.
    /// The masks apply to list/test/extract entries from the archive.
    /// If masks are not defined then all archive entries will be processed.
    /// @param mask
    ///   Set of masks.
    /// @param if_to_own
    ///   Flag to take ownership on the masks (delete on destruction).
    /// @param use_case
    ///   Whether to do a case sensitive (eCase = default),
    ///   or a case-insensitive (eNocase) match.
    /// @sa UnsetMask
    void SetMask(CMask *mask, EOwnership if_to_own = eNoOwnership,
                 NStr::ECase use_case = NStr::eCase);

    /// Unset name mask.
    ///
    /// Upon mask reset, all entries become subject to archive processing in
    /// list/test/extract operations.
    /// @sa SetMask
    void UnsetMask();

    /// Get base directory to use for files while extracting from/adding to
    /// the archive, and in the latter case used only for relative paths.
    /// @sa SetBaseDir
    const string& GetBaseDir(void) const;

    /// Set base directory to use for files while extracting from/adding to
    /// the archive, and in the latter case used only for relative paths.
    /// @sa GetBaseDir
    void SetBaseDir(const string& dir_name);

protected:
    /// Archive action
    enum EOpenMode {
        eNone = 0,
        eWO   = 1,
        eRO   = 2,
        eRW   = eRO | eWO
    };
    enum EAction {
        eUndefined =  eNone,
        eList      = (1 << 2) | eRO,
        eAppend    = (2 << 2) | eRW,
        eUpdate    = eList | eAppend,
        eExtract   = (4 << 2) | eRO,
        eTest      = eList | eExtract,
        eCreate    = (8 << 2) | eWO
    };
    /// IO completion code
    enum EStatus {
        eFailure = -1,
        eSuccess =  0,
        eZeroBlock,
        eEOF
    };

    // Common part of initialization.
    void x_Init(void);

    // Open/close the archive.
    auto_ptr<TEntries> x_Open(EAction action);
    void x_Close(void);

    // Flush the archive (writing an appropriate EOT if necessary).
    void x_Flush(void);

    // Backspace the archive.
    void x_Backspace(EAction action, size_t blocks);

    // Read information about next entry in the archive.
    EStatus x_ReadEntryInfo(CTarEntryInfo& info);

    // Pack either name or linkname into archive file header.
    bool x_PackName(SHeader* header, const CTarEntryInfo& info, bool link);

    // Write information about entry into the archive.
    void x_WriteEntryInfo(const string& name, const CTarEntryInfo& info);

    // Read the archive and do some "action".
    auto_ptr<TEntries> x_ReadAndProcess(EAction action, bool use_mask = true);

    // Process next entry from the archive.
    // If extract == FALSE, then just skip the entry without any processing.
    void x_ProcessEntry(const CTarEntryInfo& info, bool extract = false);

    // Extract an entry from the archive into the file system,
    // and return the entry size (if any) still remaining in the archive.
    streamsize x_ExtractEntry(const CTarEntryInfo& info);

    // Restore attributes of a specified entry.
    // If 'target' not specified, then CDirEntry will be constructed
    // from 'info'. In this case, 'info' should have correct name for
    // the destination dir entry.
    void x_RestoreAttrs(const CTarEntryInfo& info, CDirEntry* dst = 0);

    // Read/write specified number of bytes from/to the archive.
    const char* x_ReadArchive(size_t& n);
    void        x_WriteArchive(size_t n, const char* buffer = 0);

    // Check path and convert it to an archive name.
    string x_ToArchiveName(const string& path) const;

    // Append an entry to the archive.
    auto_ptr<TEntries> x_Append(const string&   name,
                                const TEntries* toc = 0);
    // Append a file/symlink entry to the archive.
    void x_AppendFile(const string& name, const CTarEntryInfo& info);

protected:
    string         m_FileName;       ///< Tar archive file name.
    CNcbiFstream*  m_FileStream;     ///< File stream of the archive.
    EOpenMode      m_OpenMode;       ///< What was it opened for.
    CNcbiIos*      m_Stream;         ///< Archive stream (used for all I/O).
    const size_t   m_BufferSize;     ///< Buffer size for I/O operations.
    size_t         m_BufferPos;      ///< Position within the record.
    char*          m_BufPtr;         ///< Page unaligned buffer pointer.
    char*          m_Buffer;         ///< I/O buffer (page-aligned).
    TFlags         m_Flags;          ///< Bitwise OR of flags.
    CMask*         m_Mask;           ///< Masks for list/test/extract.
    EOwnership     m_MaskOwned;      ///< Flag to take ownership for m_Mask.
    NStr::ECase    m_MaskUseCase;    ///< Flag for mask matching.
    bool           m_IsModified;     ///< True after at least one write.
    string         m_BaseDir;        ///< Base directory for relative paths.
};


//////////////////////////////////////////////////////////////////////////////
//
// Inline methods
//

inline
void CTar::Create(void)
{
    x_Open(eCreate);
}

inline
void CTar::Close(void)
{
    x_Flush();
    x_Close();
}

inline
auto_ptr<CTar::TEntries> CTar::Append(const string& name)
{
    x_Open(eAppend);
    return x_Append(name);
}

inline
auto_ptr<CTar::TEntries> CTar::Update(const string& name)
{
    auto_ptr<TEntries> toc = x_Open(eUpdate);
    return x_Append(name, toc.get());
}

inline
auto_ptr<CTar::TEntries> CTar::List(void)
{
    return x_Open(eList);
}

inline
void CTar::Test(void)
{
    x_Open(eTest);
}

inline
CTar::TFlags CTar::GetFlags(void) const
{
    return m_Flags;
}

inline
void CTar::SetFlags(TFlags flags)
{
    m_Flags = flags;
}

inline
void CTar::SetMask(CMask *mask, EOwnership if_to_own, NStr::ECase use_case)
{
    UnsetMask();
    m_Mask        = mask;
    m_MaskOwned   = if_to_own;
    m_MaskUseCase = use_case;
}

inline
void CTar::UnsetMask()
{
    if ( m_MaskOwned ) {
        delete m_Mask;
    }
    m_Mask = 0;
}

inline
const string& CTar::GetBaseDir(void) const
{
    return m_BaseDir;
}

inline
void CTar::SetBaseDir(const string& dir_name)
{
    m_BaseDir = CDirEntry::AddTrailingPathSeparator(dir_name);
}


END_NCBI_SCOPE


/* @} */


/*
 * ===========================================================================
 * $Log$
 * Revision 1.23  2006/07/13 17:54:28  lavr
 * Correction in stream use of CTar
 *
 * Revision 1.22  2006/03/03 18:24:24  lavr
 * Document Update() finely
 *
 * Revision 1.21  2006/03/03 16:58:58  lavr
 * +fUpdateExistingOnly
 *
 * Revision 1.20  2005/12/28 16:50:13  ucko
 * +<memory> for auto_ptr<>
 *
 * Revision 1.19  2005/12/02 05:49:32  lavr
 * Fix some comments
 *
 * Revision 1.18  2005/07/06 17:58:25  ivanov
 * Removed Extract() with dir parameter again
 *
 * Revision 1.17  2005/06/30 11:15:30  ivanov
 * Restore missed Extract(const string& dir)
 *
 * Revision 1.16  2005/06/29 19:04:45  lavr
 * Doc'ing
 *
 * Revision 1.15  2005/06/24 12:35:32  ivanov
 * Removed extra comma in the enum ETarModeBits enum declaration
 *
 * Revision 1.14  2005/06/22 21:07:10  lavr
 * Avoid buffer modification (which may lead to data corruption) while reading
 *
 * Revision 1.13  2005/06/22 20:03:34  lavr
 * Proper append/update implementation; Major actions got return values
 *
 * Revision 1.12  2005/06/13 18:27:20  lavr
 * Use enums for mode; define special bits' manipulations
 *
 * Revision 1.11  2005/06/01 19:58:57  lavr
 * Fix previous "fix" of getting page size
 * Move tar permission bits to the header; some cosmetics
 *
 * Revision 1.10  2005/05/30 15:27:37  lavr
 * Comments reviewed, proper blocking factor fully implemented, other patches
 *
 * Revision 1.9  2005/05/27 21:12:54  lavr
 * Revert to use of std::ios as a main I/O stream (instead of std::iostream)
 *
 * Revision 1.8  2005/05/27 13:55:44  lavr
 * Major revamp/redesign/fix/improvement/extension of this API
 *
 * Revision 1.7  2005/05/05 13:41:58  ivanov
 * Added const to parameters in private methods
 *
 * Revision 1.6  2005/05/05 12:32:33  ivanov
 * + CTar::Update()
 *
 * Revision 1.5  2005/04/27 13:52:58  ivanov
 * Added support for (re)storing permissions/owner/times
 *
 * Revision 1.4  2005/01/31 15:30:59  ivanov
 * Lines wrapped at 79th column
 *
 * Revision 1.3  2005/01/31 14:23:35  ivanov
 * Added class CTarEntryInfo to store information about TAR entry.
 * Added CTar methods:           Create, Append, List, Test.
 * Added CTar utility functions: GetFlags/SetFlags, SetMask/UnsetMask,
 *                               GetBaseDir/SetBaseDir.
 *
 * Revision 1.2  2004/12/14 17:55:48  ivanov
 * Added GNU tar long name support
 *
 * Revision 1.1  2004/12/02 17:46:14  ivanov
 * Initial draft revision
 *
 * ===========================================================================
 */

#endif  /* UTIL_COMPRESS__TAR__HPP */
