#ifndef _c4_FS_HPP_
#define _c4_FS_HPP_

#include <c4/error.hpp>
#include <c4/fs/export.hpp>
#include <stdio.h>
#include <string.h>

#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
struct dirent;
struct stat;
struct FTW;
#endif

#include "c4/c4_push.hpp"

namespace c4 {
namespace fs {


typedef enum {
    REGFILE, ///< regular file
    DIR,     ///< directory
    SYMLINK, ///< symbolic link
    PIPE,    ///< named pipe
    SOCK,    ///< socket
    OTHER,
    INVALID
} PathType_e;

PathType_e path_type(const char *pathname);

inline bool is_file(const char *pathname) { return path_type(pathname) == REGFILE; }
inline bool is_dir(const char *pathname) { return path_type(pathname) == DIR; }

/** true if a file or directory exists */
bool path_exists(const char *pathname);
/** true if the path exists and is a file */
bool file_exists(const char *pathname);
/** true if the path exists and is a directory */
bool dir_exists(const char *pathname);

/** convert a path to unix right-slash separators */
bool to_unix_sep(char *pathname, size_t sz);

/** check if a character in a pathname is an occurrence of a path separator */
bool is_sep(size_t char_pos, const char *pathname, size_t sz);


//-----------------------------------------------------------------------------

/** @name path times */

/** @{ */

struct path_times
{
    uint64_t creation, modification, access;
};

path_times times(const char *pathname);
/** get the creation time */
uint64_t ctime(const char *pathname);
/** get the modification time */
uint64_t mtime(const char *pathname);
/** get the access time */
uint64_t atime(const char *pathname);

/** @} */


//-----------------------------------------------------------------------------

/** @name creation and deletion */

/** @{ */
void mkdirs(char *pathname);
void mkdir(const char *pathname);
void rmdir(const char *pathname);

int rmfile(const char *filename);
int rmtree(const char *pathname);

/** @} */


//-----------------------------------------------------------------------------

/** @name working directory */

/** @{ */

/** get the current working directory. return null if the given buffer
 * is smaller than the required size */
char *cwd(char *buf, size_t sz);

template<class CharContainer>
char *cwd(CharContainer *v)
{
    if(v->empty())
        v->resize(32);
    while(cwd(&(*v)[0], v->size()) == nullptr)
        v->resize(v->size() * 2);
    v->resize(strlen(&(*v)[0]));
    return &(*v)[0];
}

template<class CharContainer>
CharContainer cwd()
{
    CharContainer c;
    cwd(&c);
    return c;
}

/** @} */


//-----------------------------------------------------------------------------

/** @name tmpnam
 * create a temporary name from a format. The format is scanned for
 * appearances of "XX"; each appearance of "XX" will be substituted by
 * an hexadecimal byte (ie 00...ff).
 *
 * @param fmt the format string
 * @param subchar the character used to form the pattern to be substituted.
 *        Eg, with subchar=='?', the pattern to be substituted will be "??".
 * @overload tmpnam
 */

/** @{ */

constexpr const char default_tmppat[] = "_c4fs_tmpname_XXXXXXXX.tmp";
constexpr const char default_tmpchar = 'X';

/** create a temporary name from a format.
 * output to a string, never writing beyond @p bufsz
 * @param the buffer - must be larger than @p fmt
 * @param the size of the buffer - must be higher than strlen(fmt) */
const char * tmpnam(char *buf, size_t bufsz, const char *fmt=default_tmppat, char subchar=default_tmpchar);

/** create a temporary name from a format.
 * a convenience wrapper for use with containers */
template<class CharContainer>
const char * tmpnam(CharContainer *buf, const char *fmt=default_tmppat, char subchar=default_tmpchar)
{
    size_t fmtsz = strlen(fmt);
    buf->resize(fmtsz + 1);
    tmpnam(&(*buf)[0], buf->size(), fmt, subchar);
    (*buf)[fmtsz] = '\0';
    buf->resize(fmtsz);
    return &(*buf)[0];
}

/** create a temporary name from a format.
 * a convenience wrapper for use with containers */
template<class CharContainer>
CharContainer tmpnam(const char *fmt=default_tmppat, char subchar=default_tmpchar)
{
    CharContainer c;
    tmpnam(&c, fmt, subchar);
    return c;
}

/** @} */


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** @name file contents */

/** @{ */

constexpr const char default_read_access[] = "rb";
constexpr const char default_write_access[] = "wb";

size_t file_size(const char *filename, const char* access=default_read_access);

size_t file_get_contents(const char *filename, char *buf, size_t sz, const char* access=default_read_access);

template<class CharContainer>
size_t file_get_contents(const char *filename, CharContainer *v, const char* access=default_read_access)
{
    size_t needed = file_get_contents(filename, v->empty() ? nullptr : &(*v)[0], v->size(), access);
    if(needed > v->size())
    {
        v->resize(needed);
        needed = file_get_contents(filename, v->empty() ? nullptr : &(*v)[0], v->size(), access);
        C4_CHECK(needed <= v->size());
    }
    v->resize(needed);
    return needed;
}

template<class CharContainer>
CharContainer file_get_contents(const char *filename, const char* access=default_read_access)
{
    CharContainer cc;
    file_get_contents<CharContainer>(filename, &cc, access);
    return cc;
}


void file_put_contents(const char *filename, const char *buf, size_t sz, const char* access=default_write_access);

template<class CharContainer>
void file_put_contents(const char *filename, CharContainer const& v, const char* access=default_write_access)
{
    const char *str = v.empty() ? "" : v.data();
    file_put_contents(filename, str, v.size(), access);
}

/** @} */


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct VisitedFile
{
    const char  *name;
    void        *user_data;
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    struct dirent *dirent_data;
#endif
};

struct VisitedPath
{
    const char  *name;
    void        *user_data;
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat const* stat_data;
    int                ftw_info;
    struct FTW  const* ftw_data;
#endif
};

using FileVisitor = int (*)(VisitedFile const& p);
using PathVisitor = int (*)(VisitedPath const& p);

/** order is NOT guaranteed. does NOT descend into subdirectories. */
int walk_entries(const char *pathname, FileVisitor fn, char *namebuf, size_t namebuf_size, void *user_data=nullptr);
/** order is NOT guaranteed */
int walk_tree(const char *pathname, PathVisitor fn, void *user_data=nullptr);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** open a writeable temporary file in the current working directory.
 * The dtor deletes the temporary file. */
struct ScopedTmpFile
{
    char m_name[64];
    ::FILE* m_file;
    bool m_delete;

public:

    const char* name() const { return m_name; }
    ::FILE* file() const { return m_file; }
    void do_delete(bool yes) { m_delete = yes; }

public:

    ~ScopedTmpFile();

    ScopedTmpFile(ScopedTmpFile const&) noexcept = delete;
    ScopedTmpFile& operator=(ScopedTmpFile const&) noexcept = delete;

    ScopedTmpFile(ScopedTmpFile && that) noexcept { _move(&that); }
    ScopedTmpFile& operator=(ScopedTmpFile && that) noexcept { _move(&that); return *this; }

    void _move(ScopedTmpFile *that);

public:

    explicit ScopedTmpFile(const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access=default_write_access, bool delete_after_use=true);
    explicit ScopedTmpFile(const char* contents_, size_t sz, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access=default_write_access, bool delete_after_use=true);

    template <class CharContainer>
    explicit ScopedTmpFile(CharContainer const& contents_, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char* access=default_write_access, bool delete_after_use=true)
        : ScopedTmpFile(contents_.data(), contents_.size(), name_pattern, access, delete_after_use)
    {
    }

public:

    const char* full_path(char *buf, size_t sz) const;

    template <class CharContainer>
    const char* full_path(CharContainer *v) const
    {
        if(v->empty())
            v->resize(16);
        while(full_path(&(*v)[0], v->size()) == nullptr)
            v->resize(v->size() * 2);
        v->resize(strlen(&(*v)[0]));
        return &(*v)[0];
    }

    template<class CharContainer>
    CharContainer full_path() const
    {
        CharContainer c;
        full_path(&c);
        return c;
    }

    template<class CharContainer>
    void contents(CharContainer *cont) const
    {
        file_get_contents(m_name, cont);
    }

    template<class CharContainer>
    CharContainer contents() const
    {
        CharContainer c;
        file_get_contents(m_name, &c);
        return c;
    }

};

} // namespace fs
} // namespace c4

#include "c4/c4_pop.hpp"

#endif /* _c4_FS_HPP_ */
