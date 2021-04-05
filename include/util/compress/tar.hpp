#ifndef UTIL_COMPRESS__TAR__HPP
#define UTIL_COMPRESS__TAR__HPP

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
 * Authors:  Vladimir Ivanov
 *           Anton Lavrentiev
 *
 * File Description:
 *   Tar archive API
 */

///  @file
///  Tar archive API.
///
///  Supports subsets of POSIX.1-1988 (ustar), POSIX 1003.1-2001 (posix), old
///  GNU (POSIX 1003.1), and V7 formats (all partially but reasonably).  New
///  archives are created using POSIX (genuine ustar) format, using GNU
///  extensions for long names/links only when unavoidable.  It cannot,
///  however, handle all the exotics like sparse files (except for GNU/1.0
///  sparse PAX extension) and contiguous files (yet still can work around both
///  of them gracefully, if needed), multivolume / incremental archives, etc.
///  but just regular files, devices (character or block), FIFOs, directories,
///  and limited links:  can extract both hard- and symlinks, but can store
///  symlinks only.  Also, this implementation is only minimally PAX(Portable
///  Archive eXchange)-aware for file extractions (and does not yet use any PAX
///  extensions to store the files).
///

#include <corelib/ncbifile.hpp>
#include <utility>


/** @addtogroup Compression
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// TTarMode --
///
/// Permission bits as defined in tar
///

enum ETarModeBits {
    // Special mode bits
    fTarSetUID   = 04000,       ///< set UID on execution
    fTarSetGID   = 02000,       ///< set GID on execution
    fTarSticky   = 01000,       ///< reserved (sticky bit)
    // File permissions
    fTarURead    = 00400,       ///< read by owner
    fTarUWrite   = 00200,       ///< write by owner
    fTarUExecute = 00100,       ///< execute/search by owner
    fTarGRead    = 00040,       ///< read by group
    fTarGWrite   = 00020,       ///< write by group
    fTarGExecute = 00010,       ///< execute/search by group
    fTarORead    = 00004,       ///< read by other
    fTarOWrite   = 00002,       ///< write by other
    fTarOExecute = 00001        ///< execute/search by other
};
typedef unsigned int TTarMode;  ///< Bitwise OR of ETarModeBits


/////////////////////////////////////////////////////////////////////////////
///
/// CTarException --
///
/// Define exceptions generated by the API.
/// Exception text may include detailed dump of a tar header (when appropriate)
/// if fDumpEntryHeaders is set in the archive flags.
///
/// CTarException inherits its basic functionality from CCoreException
/// and defines additional error codes for tar archive operations.
///
/// @sa
///   CTar::SetFlags

class NCBI_XUTIL_EXPORT CTarException : public CCoreException
{
public:
    /// Error types that file operations can generate.
    enum EErrCode {
        eUnsupportedTarFormat,
        eUnsupportedEntryType,
        eUnsupportedSource,
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
    virtual const char* GetErrCodeString(void) const override
    {
        switch (GetErrCode()) {
        case eUnsupportedTarFormat: return "eUnsupportedTarFormat";
        case eUnsupportedEntryType: return "eUnsupportedEntryType";
        case eUnsupportedSource:    return "eUnsupportedSource";
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
    /// Archive entry type.
    enum EType {
        eFile        = CDirEntry::eFile,         ///< Regular file
        eDir         = CDirEntry::eDir,          ///< Directory
        eSymLink     = CDirEntry::eSymLink,      ///< Symbolic link
        ePipe        = CDirEntry::ePipe,         ///< Pipe (FIFO)
        eCharDev     = CDirEntry::eCharSpecial,  ///< Character device
        eBlockDev    = CDirEntry::eBlockSpecial, ///< Block device
        eUnknown     = CDirEntry::eUnknown,      ///< Unknown type
        eHardLink,                               ///< Hard link
        eVolHeader,                              ///< Volume header
        ePAXHeader,                              ///< PAX extended header
        eSparseFile,                             ///< GNU/STAR sparse file 
        eGNULongName,                            ///< GNU long name
        eGNULongLink                             ///< GNU long link
    };

    /// Position type.
    enum EPos {
        ePos_Header,
        ePos_Data
    };

    // No setters -- they are not needed for access by the user, and thus are
    // done directly from CTar for the sake of performance and code clarity.

    // Getters only!
    EType         GetType(void)              const { return m_Type;      }
    const string& GetName(void)              const { return m_Name;      }
    const string& GetLinkName(void)          const { return m_LinkName;  }
    const string& GetUserName(void)          const { return m_UserName;  }
    const string& GetGroupName(void)         const { return m_GroupName; }
    time_t        GetModificationTime(void)  const
    { return m_Stat.orig.st_mtime; }
    CTime         GetModificationCTime(void) const
    { CTime mtime(m_Stat.orig.st_mtime);
      mtime.SetNanoSecond(m_Stat.mtime_nsec);
      return mtime;                }
    time_t        GetLastAccessTime(void)    const
    { return m_Stat.orig.st_atime; }
    CTime         GetLastAccessCTime(void)   const
    { CTime atime(m_Stat.orig.st_atime);
      atime.SetNanoSecond(m_Stat.atime_nsec);
      return atime;                }
    time_t        GetCreationTime(void)      const
    { return m_Stat.orig.st_ctime; }
    CTime         GetCreationCTime(void)     const
    { CTime ctime(m_Stat.orig.st_ctime);
      ctime.SetNanoSecond(m_Stat.ctime_nsec);
      return ctime;                }
    Uint8         GetSize(void)              const
    { return m_Stat.orig.st_size;  }
    TTarMode      GetMode(void)              const;// Raw mode as stored in tar
    void          GetMode(CDirEntry::TMode*            user_mode,
                          CDirEntry::TMode*            group_mode   = 0,
                          CDirEntry::TMode*            other_mode   = 0,
                          CDirEntry::TSpecialModeBits* special_bits = 0) const;
    unsigned int  GetMajor(void)             const;
    unsigned int  GetMinor(void)             const;
    unsigned int  GetUserId(void)            const
    { return m_Stat.orig.st_uid;   }
    unsigned int  GetGroupId(void)           const
    { return m_Stat.orig.st_gid;   }
    Uint8         GetPosition(EPos which)    const
    { return which == ePos_Header ? m_Pos : m_Pos + m_HeaderSize; }

    // Comparison operator.
    bool operator == (const CTarEntryInfo& info) const
    { return (m_Type       == info.m_Type                        &&
              m_Name       == info.m_Name                        &&
              m_LinkName   == info.m_LinkName                    &&
              m_UserName   == info.m_UserName                    &&
              m_GroupName  == info.m_GroupName                   &&
              m_HeaderSize == info.m_HeaderSize                  &&
              memcmp(&m_Stat,&info.m_Stat, sizeof(m_Stat)) == 0  &&
              m_Pos        == info.m_Pos ? true : false);         }

protected:
    // Constructor.
    CTarEntryInfo(Uint8 pos = 0)
        : m_Type(eUnknown), m_HeaderSize(0), m_Pos(pos)
    { memset(&m_Stat, 0, sizeof(m_Stat));                         }

    EType            m_Type;       ///< Type
    string           m_Name;       ///< Entry name
    string           m_LinkName;   ///< Link name if type is e{Sym|Hard}Link
    string           m_UserName;   ///< User name
    string           m_GroupName;  ///< Group name
    streamsize       m_HeaderSize; ///< Total size of all headers for the entry
    CDirEntry::SStat m_Stat;       ///< Direntry-compatible info
    Uint8            m_Pos;        ///< Entry (not data!) position in archive

    friend class CTar;             // Setter
};


/// User-creatable info for streaming into a tar.
/// Since the entry info is built largerly incomplete, all getters have been
/// disabled;  should some be needed they could be brought back by subclassing
/// and redeclaring the necessary one(s) in the public part of the new class.
class CTarUserEntryInfo : protected CTarEntryInfo
{
public:
    CTarUserEntryInfo(const string& name, Uint8 size)
    {
        m_Name              = name;
        m_Stat.orig.st_size = size;
    }

    friend class CTar;             // Accessor
};


/// Nice TOC(table of contents) printout.
NCBI_XUTIL_EXPORT ostream& operator << (ostream&, const CTarEntryInfo&);


/// Forward declaration of a tar header used internally.
struct STarHeader;


//////////////////////////////////////////////////////////////////////////////
///
/// CTar class
///
/// (Throws exceptions on most errors.)
/// Note that if stream constructor is used, then CTar can only perform one
/// pass over the archive.  This means that only one full action will succeed
/// (and if the action was to update -- e.g. append -- the archive, it has to
/// be explicitly followed by Close() when no more appends are expected).
/// Before the next read/update action, the stream position has to be reset
/// explicitly to the beginning of the archive, or it may also remain at the
/// end of the archive for a series of successive append operations.

class NCBI_XUTIL_EXPORT CTar
{
public:
    /// General flags
    enum EFlags {
        // --- Extract/List/Test ---
        /// Ignore blocks of zeros in archive.
        //  Generally, 2 or more consecutive zero blocks indicate EOT.
        fIgnoreZeroBlocks   = (1<<1),

        // --- Extract/Append/Update ---
        /// Follow symbolic links (instead of storing/extracting them)
        fFollowLinks        = (1<<2),

        // --- Extract --- (NB: fUpdate also applies to Update)
        /// Allow to overwrite destinations with entries from the archive
        fOverwrite          = (1<<3),
        /// Only update entries that are older than those already existing
        fUpdate             = (1<<4) | fOverwrite,
        /// Backup destinations if they exist (all entries including dirs)
        fBackup             = (1<<5) | fOverwrite,
        /// If destination entry exists, it must have the same type as source
        fEqualTypes         = (1<<6),
        /// Create extracted files with the original ownership
        fPreserveOwner      = (1<<7),
        /// Create extracted files with the original permissions
        fPreserveMode       = (1<<8),
        /// Preserve date/times for extracted files
        fPreserveTime       = (1<<9),
        /// Preserve all file attributes
        fPreserveAll        = fPreserveOwner | fPreserveMode | fPreserveTime,
        /// Preserve absolute path instead of stripping the leadind slash('/')
        fKeepAbsolutePath   = (1<<12),
        /// Do not extract PAX GNU/1.0 sparse files (treat 'em as unsupported)
        fSparseUnsupported  = (1<<13),

        // --- Extract/List ---
        /// Skip unsupported entries rather than make files out of them when
        /// extracting (the latter is the default behavior required by POSIX)
        fSkipUnsupported    = (1<<15),

        // --- Append ---
        /// Ignore unreadable files/dirs (still warn them, but don't stop)
        fIgnoreUnreadable   = (1<<17),
        /// Always use OldGNU headers for long names (default:only when needed)
        fLongNameSupplement = (1<<18),

        // --- Debugging ---
        fDumpEntryHeaders   = (1<<20),
        fSlowSkipWithRead   = (1<<21),

        // --- Miscellaneous ---
        /// Stream tar data through
        fStreamPipeThrough  = (1<<24),
        /// Do not trim tar file size after append/update
        fTarfileNoTruncate  = (1<<26),
        /// Suppress NCBI signatures in entry headers
        fStandardHeaderOnly = (1<<28),

        /// Default flags
        fDefault            = fOverwrite | fPreserveAll
    };
    typedef unsigned int TFlags;  ///< Bitwise OR of EFlags

    /// Mask type enumerator.
    /// @enum eExtractMask
    ///   CMask can select both inclusions and exclusions (in this order) of
    ///   fully-qualified archive entries for listing or extraction, so that
    ///   e.g. ".svn" does not match an entry like "a/.svn" for processing.
    /// @enum eExcludeMask
    ///   CMask can select both exclusions and inclusions (in this order) of
    ///   patterns of the archive entries for all operations (excepting eTest),
    ///   and so that ".svn" matches "a/b/c/.svn".
    enum EMaskType {
        eExtractMask = 0,  ///< exact for list or extract
        eExcludeMask       ///< pattern for all but test
    };

    /// Constructors
    CTar(const string& filename, size_t blocking_factor = 20);
    /// Stream version does not at all use stream positioning and so is safe on
    /// non-positionable streams, like pipes/sockets (or magnetic tapes :-I).
    CTar(CNcbiIos& stream, size_t blocking_factor = 20);

    /// Destructor (finalize the archive if currently open).
    /// @sa
    ///   Close
    virtual ~CTar();


    /// Define a list of entries.
    typedef list<CTarEntryInfo> TEntries;

    /// Define a list of files with sizes (directories and specials, such as
    /// devices, must be given with sizes of 0;  symlinks -- with the sizes
    /// of the names they are linking to).
    typedef pair<string, Uint8> TFile;
    typedef list<TFile>         TFiles;


    //------------------------------------------------------------------------
    // Main functions
    //------------------------------------------------------------------------

    /// Create a new empty archive.
    ///
    /// If a file with such a name already exists it will be overwritten.
    /// @sa
    ///   Append
    void Create(void);

    /// Close the archive making sure all pending output is flushed.
    ///
    /// Normally, direct call of this method need _not_ intersperse successive
    /// archive manipulations by other methods, as they open and close the
    /// archive automagically as needed.  Rather, this call is to make sure the
    /// archive is complete earlier than it otherwise usually be done
    /// automatically in the destructor of the CTar object.
    /// @sa
    ///   ~CTar
    void Close(void);

    /// Append an entry at the end of the archive that already exists.
    ///
    /// Appended entry can be either a file, a directory, a symbolic link,
    /// a device special file (block or character), or a FIFO special file,
    /// subject to any exclusions as set by SetMask() with eExcludeMask.
    /// The name is taken with respect to the base directory, if any set.
    ///
    /// Adding a directory results in all its files and subdirectories (subject
    //  for the exclusion mask) to get added: examine the return value to find
    /// out what has been added.
    ///
    /// Note that the final name of an entry may not contain embedded '..'.
    /// Leading slash in the absolute paths will be retained.  The names of
    /// all appended entries will be converted to Unix format (that is, to
    /// have only forward slashes in the paths, and drive letter, if any on
    /// MS-Windows, stripped).  All entries will be added at the logical end
    /// (not always EOF) of the archive, when appending to a non-empty one.
    ///
    /// @note Adding to a stream archive does not seek to the logical end of
    /// the archive but begins at the current position right away.
    ///
    /// @return
    ///   A list of entries appended.
    /// @sa
    ///   Create, Update, SetBaseDir, SetMask
    unique_ptr<TEntries> Append(const string& name);

    /// Append an entry from a stream (exactly entry.GetSize() bytes).
    /// @note
    ///   Name masks (if any set with SetMask()) are all ignored.
    /// @return
    ///   A list (containing this one entry) with full archive info filled in
    /// @sa
    ///   Append
    unique_ptr<TEntries> Append(const CTarUserEntryInfo& entry,
                                CNcbiIstream& is);

    /// Look whether more recent copies of the archive members are available in
    /// the file system, and if so, append them to the archive:
    ///
    /// - if fUpdate is set in processing flags, only the existing archive
    /// entries (including directories) will be updated;  that is, Update(".")
    /// won't recursively add "." if "." is not an archive member;  it will,
    /// however, do the recursive update should "." be found in the archive;
    ///
    /// - if fUpdate is unset, the existing entries will be updated (if their
    /// file system counterparts are newer), and nonexistent entries will be
    /// added to the archive;  that is, Update(".") will recursively scan "."
    /// to update both existing entries (if newer files found), and also add
    /// new entries for any files/directories, which are currently not in.
    ///
    /// @note Updating stream archive may (and most certainly will) cause
    /// zero-filled gaps in the archive (can be read with "ignore zeroes").
    ///
    /// @return
    ///   A list of entries that have been updated.
    /// @sa
    ///   Append, SetBaseDir, SetMask, SetFlags
    unique_ptr<TEntries> Update(const string& name);

    /// Extract the entire archive (into either current directory or a
    /// directory otherwise specified by SetBaseDir()).
    ///
    /// If the same-named files exist, they will be replaced (subject to
    /// fOverwrite) or backed up (fBackup), unless fUpdate is set, which would
    /// cause the replacement / backup only if the files are older than the
    /// archive entries.  Note that if fOverwrite is stripped, no matching
    /// files will be updated / backed up / overwritten, but skipped.
    ///
    /// Extract all archive entries, whose names match the pre-set mask.
    /// @note
    ///   Unlike Append(), extracting a matching directory does *not*
    ///   automatically extract all files within:  for them to be extracted,
    ///   they still must match the mask.  So if there is a directory "dir/"
    ///   stored in the archive, the extract mask can be "dir/*" for the
    ///   entire subtree to be extracted.  Note that "dir/" will only extract
    ///   the directory itself, and "dir" won't cause that directory to be
    ///   extracted at all (mismatch due to the trailing slash '/' missing).
    /// @return
    ///   A list of entries that have been actually extracted.
    /// @sa
    ///   SetMask, SetBaseDir, SetFlags
    unique_ptr<TEntries> Extract(void);

    /// Get information about all matching archive entries.
    ///
    /// @return
    ///   An array containing information on those archive entries, whose
    ///   names match the pre-set mask.
    /// @sa
    ///   SetMask
    unique_ptr<TEntries> List(void);

    /// Verify archive integrity.
    ///
    /// Read through the archive without actually extracting anything from it.
    /// Flag fDumpEntryHeaders causes most of archive headers to be dumped to
    /// the log (with eDiag_Info) as the Test() advances through the archive.
    /// @sa
    ///   SetFlags
    void Test(void);


    //------------------------------------------------------------------------
    // Utility functions
    //------------------------------------------------------------------------

    /// Get processing flags.
    TFlags GetFlags(void) const;

    /// Set processing flags.
    void   SetFlags(TFlags flags);

    /// Get current stream position.
    Uint8  GetCurrentPosition(void) const;

    /// Set name mask.
    ///
    /// The set of masks is used to process existing entries in the archive:
    /// both the extract and exclude masks apply to the list and extract
    /// operations, and only the exclude mask apply to the named append.
    /// If masks are not defined then all archive entries will be processed.
    ///
    /// @note Unset mask means wildcard processing (all entries match).
    ///
    /// @param mask
    ///   Set of masks (0 to unset the current set without setting a new one).
    /// @param own
    ///   Whether to take ownership on the mask (delete upon CTar destruction).
    /// @sa
    //    SetFlags
    void SetMask(CMask*      mask,
                 EOwnership  own   = eNoOwnership,
                 EMaskType   type  = eExtractMask,
                 NStr::ECase acase = NStr::eCase);

    /// Get base directory to use for files while extracting from/adding to
    /// the archive, and in the latter case used only for relative paths.
    /// @sa
    ///   SetBaseDir
    const string& GetBaseDir(void) const;

    /// Set base directory to use for files while extracting from/adding to
    /// the archive, and in the latter case used only for relative paths.
    /// @sa
    ///   GetBaseDir
    void          SetBaseDir(const string& dirname);

    /// Return archive size as if all specified input entries were put in it.
    /// Note that the return value is not the exact but the upper bound of
    /// what the archive size can be expected.  This call does not recurse
    /// into any subdirectories but relies solely upon the information as
    /// passed via the parameter.
    ///
    /// The returned size includes all necessary alignments and padding.
    /// @return
    ///   An upper estimate of archive size given that all specified files
    ///   were stored in it (the actual size may turn out to be smaller).
    static Uint8 EstimateArchiveSize(const TFiles& files,
                                     size_t blocking_factor = 20,
                                     const string& base_dir = kEmptyStr);


    //------------------------------------------------------------------------
    // Streaming
    //------------------------------------------------------------------------

    /// Iterate over the archive forward and return first (or next) entry.
    ///
    /// When using this method (possibly along with GetNextEntryData()), the
    /// archive stream (if any) must not be accessed outside the CTar API,
    /// because otherwise inconsistency in data may result.
    /// An application may call GetNextEntryData() to stream some or all of the
    /// data out of this entry, or it may call GetNextEntryInfo() again to skip
    /// to the next archive entry, etc.
    /// Note that the archive can contain multiple versions of the same entry
    /// (in case if an update was done on it), all of which but the last one
    /// are to be ignored.  This call traverses through all those entry
    /// versions, and sequentially exposes them to the application level.
    /// See test suite (in test/test_tar.cpp) for a usage example.
    /// @return
    ///   Pointer to next entry info in the archive or 0 if EOF encountered.
    /// @sa
    ///   CTarEntryInfo, GetNextEntryData
    const CTarEntryInfo* GetNextEntryInfo(void);

    /// Create and return an IReader, which can extract the current archive
    /// entry that has been previously returned via GetNextEntryInfo.
    ///
    /// The returned pointer is non-zero only if the current entry is a file
    /// (even of size 0).  The ownership of the pointer is passed to the caller
    /// (so it has to be explicitly deleted when no longer needed).
    /// The IReader may be used to read all or part of data out of the entry
    /// without affecting GetNextEntryInfo()'s ability to find any following
    /// entry in the archive.
    /// See test suite (in test/test_tar.cpp) for a usage example.
    /// @return
    ///   Pointer to IReader, or 0 if the current entry is not a file.
    /// @sa
    ///   GetNextEntryData, IReader, CRStream
    IReader*             GetNextEntryData(void);

    /// Create and return an IReader, which can extract contents of one named
    /// file (which can be requested by a name mask in the "name" parameter).
    ///
    /// The tar archive is deemed to be in the specified stream "is", properly
    /// positioned (either at the beginning of the archive, or at any
    /// CTarEntryInfo::GetPosition(ePos_Header)'s result possibly off-set
    /// with some fixed archive base position, e.g. if there is any preamble).
    /// The extraction is done at the first matching entry only, then stops.
    /// @note fStreamPipeThrough will be ignored if passed in flags.
    /// See test suite (in test/test_tar.cpp) for a usage example.
    /// @return
    ///   IReader interface to read the file contents with;  0 on error.
    /// @sa
    ///   CTarEntryInfo::GetPosition, Extract, SetMask, SetFlags,
    ///   GetNextEntryInfo, GetNextEntryData, IReader, CRStream
    static IReader* Extract(CNcbiIstream& is, const string& name,
                            TFlags flags = fSkipUnsupported);

protected:
    //------------------------------------------------------------------------
    // User-redefinable callback
    //------------------------------------------------------------------------

    /// Return false to skip the current entry when reading;
    /// the return code gets ignored when writing.
    ///
    /// Note that the callback can encounter multiple entries of the same file
    /// in case the archive has been updated (so only the last occurrence is
    /// the actual copy of the file when extracted).
    virtual bool Checkpoint(const CTarEntryInfo& /*current*/,
                            bool /*ifwrite: write==true, read==false*/)
    { return true; }

private:
    /// Archive open mode and action
    enum EOpenMode {
        eNone = 0,
        eWO   = 1,
        eRO   = 2,
        eRW   = eRO | eWO
    };
    enum EAction {
        eUndefined =  eNone,
        eList      = (1 << 2) | eRO,
        eAppend    = (1 << 3) | eRW,
        eUpdate    = eList | eAppend,
        eExtract   = (1 << 4) | eRO,
        eTest      = eList | eExtract,
        eCreate    = (1 << 5) | eWO,
        eInternal  = (1 << 6) | eRO
    };
    /// I/O completion code
    enum EStatus {
        eFailure = -1,
        eSuccess =  0,
        eContinue,
        eZeroBlock,
        eEOF
    };
    /// Mask storage
    struct SMask {
        CMask*      mask;
        NStr::ECase acase;
        EOwnership  owned;

        SMask(void)
            : mask(0), acase(NStr::eNocase), owned(eNoOwnership)
        { }
    };

    // Common part of initialization.
    void x_Init(void);

    // Open/close the archive.
    void x_Open(EAction action);
    void x_Close(bool truncate);  // NB: "truncate" effects file archives only

    // Flush the archive (w/EOT);  return "true" if it is okay to truncate
    bool x_Flush(bool nothrow = false);

    // Backspace and fast-forward the archive.
    void x_Backspace(EAction action);  // NB: m_ZeroBlockCount blocks back
    void x_Skip(Uint8 blocks);         // NB: Can do by either skip or read

    // Parse in extended entry information (PAX) for the current entry.
    EStatus x_ParsePAXData(const string& data);

    // Read information about current entry in the archive.
    EStatus x_ReadEntryInfo(bool dump, bool pax);

    // Pack current name or linkname into archive entry header.
    bool x_PackCurrentName(STarHeader* header, bool link);

    // Write information for current entry into the archive.
    void x_WriteEntryInfo(const string& name);

    // Read the archive and do the requested "action" on current entry.
    unique_ptr<TEntries> x_ReadAndProcess(EAction action);

    // Process current entry from the archive (the actual size passed in).
    // If action != eExtract, then just skip the entry without any processing.
    // Return true iff the entry was successfully extracted (ie with eExtract).
    bool x_ProcessEntry(EAction action, Uint8 size, const TEntries* done);

    // Extract current entry (archived size passed in) from the archive into
    // the file system, and update the size still remaining in the archive, if
    // any.  Return true if the extraction succeeded, false otherwise.
    bool x_ExtractEntry(Uint8& size, const CDirEntry* dst,
                        const CDirEntry* src);

    // Extract file data from the archive.
    void x_ExtractPlainFile (Uint8& size, const CDirEntry* dst);
    bool x_ExtractSparseFile(Uint8& size, const CDirEntry* dst,
                             bool dump = false);

    // Restore attributes of an entry in the file system.
    // If "path" is not specified, then the destination path will be
    // constructed from "info", and the base directory (if any).  Otherwise,
    // "path" will be used "as is", assuming it corresponds to "info".
    void x_RestoreAttrs(const CTarEntryInfo& info,
                        TFlags               what,
                        const CDirEntry*     path = 0,
                        TTarMode             perm = 0/*override*/) const;

    // Read a text string terminated with '\n'.
    string x_ReadLine(Uint8& size, const char*& data, size_t& nread);

    // Read/write specified number of bytes from/to the archive.
    const char* x_ReadArchive (size_t& n);
    void        x_WriteArchive(size_t  n, const char* buffer = 0);

    // Append an entry from the file system to the archive.
    unique_ptr<TEntries> x_Append(const string& name, const TEntries* toc = 0);

    // Append an entry from an istream to the archive.
    unique_ptr<TEntries> x_Append(const CTarUserEntryInfo& entry,
                                  CNcbiIstream& is);

    // Append data from an istream to the archive.
    void x_AppendStream(const string& name, CNcbiIstream& is);

    // Append a regular file to the archive.
    bool x_AppendFile(const string& file);

private:
    string        m_FileName;       ///< Tar archive file name (only if file)
    CNcbiFstream* m_FileStream;     ///< File stream of the archive (if file)
    CNcbiIos&     m_Stream;         ///< Archive stream (used for all I/O)
    size_t        m_ZeroBlockCount; ///< Zero blocks seen in between entries
    const size_t  m_BufferSize;     ///< Buffer(record) size for I/O operations
    size_t        m_BufferPos;      ///< Position within the record
    Uint8         m_StreamPos;      ///< Position in stream (0-based)
    char*         m_BufPtr;         ///< Page-unaligned buffer pointer
    char*         m_Buffer;         ///< I/O buffer (page-aligned)
    SMask         m_Mask[2];        ///< Entry masks for operations
    EOpenMode     m_OpenMode;       ///< What was it opened for
    bool          m_Modified;       ///< True after at least one write
    bool          m_Bad;            ///< True if a fatal output error occurred
    TFlags        m_Flags;          ///< Bitwise OR of flags
    string        m_BaseDir;        ///< Base directory for relative paths
    CTarEntryInfo m_Current;        ///< Current entry being processed

private:
    // Prohibit assignment and copy
    CTar& operator=(const CTar&);
    CTar(const CTar&);

    friend class CTarReader;
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
    x_Close(x_Flush());
}

inline
unique_ptr<CTar::TEntries> CTar::Append(const string& name)
{
    x_Open(eAppend);
    return x_Append(name);
}

inline
unique_ptr<CTar::TEntries> CTar::Append(const CTarUserEntryInfo& entry,
                                        CNcbiIstream& is)
{
    x_Open(eAppend);
    return x_Append(entry, is);
}

inline
unique_ptr<CTar::TEntries> CTar::Update(const string& name)
{
    x_Open(eUpdate);
    return x_Append(name, x_ReadAndProcess(eUpdate).get());
}

inline
unique_ptr<CTar::TEntries> CTar::List(void)
{
    x_Open(eList);
    return x_ReadAndProcess(eList);
}

inline
void CTar::Test(void)
{
    x_Open(eTest);
    x_ReadAndProcess(eTest);
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

inline Uint8 CTar::GetCurrentPosition(void) const
{
    return m_StreamPos;
}

inline
const string& CTar::GetBaseDir(void) const
{
    return m_BaseDir;
}


END_NCBI_SCOPE


/* @} */


#endif  /* UTIL_COMPRESS__TAR__HPP */