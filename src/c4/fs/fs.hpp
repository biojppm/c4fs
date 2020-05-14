#ifndef _c4_FS_HPP_
#define _c4_FS_HPP_

#include <c4/platform.hpp>
#include <c4/error.hpp>
#include <stdio.h>
#include <string.h>

#ifdef C4_POSIX
typedef struct _ftsent FTSENT;
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


//-----------------------------------------------------------------------------

bool path_exists(const char *pathname);
PathType_e path_type(const char *pathname);
inline bool is_file(const char *pathname) { return path_type(pathname) == REGFILE; }
inline bool is_dir(const char *pathname) { return path_type(pathname) == DIR; }
inline bool dir_exists(const char *pathname)
{
    return path_exists(pathname) && is_dir(pathname);
}

void mkdir(const char *pathname);
void rmdir(const char *pathname);
void mkdirs(char *pathname);

/** check if a character in a pathname is an occurrence of a path separator */
bool is_sep(size_t char_pos, const char *pathname, size_t sz);
bool to_unix_sep(char *pathname, size_t sz);


//-----------------------------------------------------------------------------

constexpr const char default_tmppat[] = "_c4fs_tmpname_XXXXXXXX.tmp";
constexpr const char default_tmpchar = 'X';

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

//@{
/** create a temporary name from a format.
 * output to a string, never writing with at most @p bufsz
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
//@}


//-----------------------------------------------------------------------------

struct path_times
{
    uint64_t creation, modification, access;
};

path_times times(const char *pathname);
inline uint64_t ctime(const char *pathname) { return times(pathname).creation; }
inline uint64_t mtime(const char *pathname) { return times(pathname).modification; }
inline uint64_t atime(const char *pathname) { return times(pathname).access; }


//-----------------------------------------------------------------------------

/** get the current working directory. return null if the given buffer
 * is smaller than the required size */
char *cwd(char *buf, size_t sz);

template<class CharContainer>
char *cwd(CharContainer *v)
{
    if(v->empty()) v->resize(16);
    while(cwd(&(*v)[0], v->size()) == nullptr)
    {
        v->resize(v->size() * 2);
    }
    v->resize(strlen(&(*v)[0]));
    return &(*v)[0];
}

template<class CharContainer>
CharContainer cwd()
{
    CharContainer c;
    c.resize(32);
    cwd(&c);
    return c;
}


//-----------------------------------------------------------------------------

void delete_file(const char *filename);
void delete_path(const char *pathname, bool recursive=false);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct VisitedPath
{
    const char  *name;
    PathType_e   type;
    void        *user_data;
#ifdef C4_POSIX
    FTSENT      *node;
#endif
};

using PathVisitor = int (*)(VisitedPath const& C4_RESTRICT p);

int walk(const char *pathname, PathVisitor fn, void *user_data=nullptr);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

size_t file_size(const char *filename);

void   file_put_contents(const char *filename, const char *buf, size_t sz, const char* access="wb");
size_t file_get_contents(const char *filename,       char *buf, size_t sz, const char* access="rb");

template<class CharContainer>
size_t file_get_contents(const char *filename, CharContainer *v)
{
    ::FILE *fp = ::fopen(filename, "rb");
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fseek(fp, 0, SEEK_END);
    long sz = ::ftell(fp);
    using size_type = typename CharContainer::size_type;
    v->resize(static_cast<size_type>(sz));
    if(sz)
    {
        ::rewind(fp);
        size_t ret = ::fread(&(*v)[0], 1, v->size(), fp);
        C4_CHECK(ret == (size_t)sz);
    }
    ::fclose(fp);
    return v->size();
}
template<class CharContainer>
CharContainer file_get_contents(const char *filename)
{
    CharContainer cc;
    file_get_contents(filename, &cc);
    return cc;
}


template<class CharContainer>
inline void file_put_contents(const char *filename, CharContainer const& v, const char* access="wb")
{
    file_put_contents(filename, v.empty() ? "" : &v[0], v.size(), access);
}


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

    ~ScopedTmpFile()
    {
        if(m_delete) delete_file(m_name);
        if( ! m_file) return;
        fclose(m_file);
        m_file = nullptr;
    }

    ScopedTmpFile(ScopedTmpFile const&) noexcept = delete;
    ScopedTmpFile& operator=(ScopedTmpFile const&) noexcept = delete;

    ScopedTmpFile(ScopedTmpFile && that) noexcept { _move(&that); }
    ScopedTmpFile& operator=(ScopedTmpFile && that) noexcept { _move(&that); return *this; }

    void _move(ScopedTmpFile *that)
    {
        memcpy(m_name, that->m_name, sizeof(m_name));
        memset(that->m_name, 0, sizeof(m_name));
        m_file = that->m_file;
        m_delete = that->m_delete;
        that->m_file = nullptr;
    }

public:

    explicit ScopedTmpFile(const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access="wb", bool delete_after_use=true)
    {
        C4_CHECK(strlen(name_pattern) < sizeof(m_name));
        tmpnam(m_name, sizeof(m_name), name_pattern);
        m_file = ::fopen(m_name, access);
        m_delete = delete_after_use;
    }

    explicit ScopedTmpFile(const char* contents_, size_t sz, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access="wb", bool delete_after_use=true)
        : ScopedTmpFile(name_pattern, access, delete_after_use)
    {
        ::fwrite(contents_, 1, sz, m_file);
        ::fflush(m_file);
    }

    template <class CharContainer>
    explicit ScopedTmpFile(CharContainer const& contents_, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char* access="wb", bool delete_after_use=true)
        : ScopedTmpFile(contents_.data(), contents_.size(), name_pattern, access, delete_after_use)
    {
    }

public:

    const char* full_path(char *buf, size_t sz) const
    {
        if(cwd(buf, sz) == nullptr) return nullptr;
        size_t cwdlen = strlen(buf);
        size_t namelen = strlen(m_name);
        if(sz < cwdlen + 1 + namelen) return nullptr;
        buf[cwdlen] = '/';
        memcpy(buf+cwdlen+1, m_name, namelen);
        buf[cwdlen + 1 + namelen] = '\0';
        return buf;
    }
    template <class CharContainer>
    const char* full_path(CharContainer *v) const
    {
        if(v->empty()) v->resize(16);
        while(full_path(&(*v)[0], v->size()) == nullptr)
        {
            v->resize(v->size() * 2);
        }
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
