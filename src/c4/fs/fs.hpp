#ifndef _c4_FS_HPP_
#define _c4_FS_HPP_

#include <c4/platform.hpp>
#include <c4/error.hpp>
#include <stdio.h>
#include <string.h>

#ifdef C4_POSIX
typedef struct _ftsent FTSENT;
#endif


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

void mkdir(const char *pathname);
void rmdir(const char *pathname);
void mkdirs(char *pathname);


//-----------------------------------------------------------------------------

/** create a temporary name from a format. The format is scanned for
 * appearances of "XX"; each appearance of "XX" will be substituted by
 * an hexadecimal byte (ie 0x00...0xff). */
const char * tmpnam(const char *fmt, char *buf, size_t bufsz);

/** create a temporary name from a format. The format is scanned for
 * appearances of "XX"; each appearance of "XX" will be substituted by
 * an hexadecimal byte (ie 0x00...0xff). */
template< class CharContainer >
const char * tmpnam(const char *fmt, CharContainer *buf)
{
    buf->resize(strlen(fmt) + 1);
    return tmpnam(fmt, (*buf)[0], buf->size());
}


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

/** get the current working directory. return null if the buffer is smaller
 * than the required size */
char *cwd(char *buf, size_t sz);

template< class CharContainer >
char *cwd(CharContainer *v)
{
    if(v->empty()) v->resize(16);
    while( ! cwd(&(*v)[0], v->size()))
    {
        v->resize(v->size() * 2);
    }
    v->resize(strlen(v->data()));
    return &(*v)[0];
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

size_t file_get_contents(const char *filename,       char *buf, size_t sz, const char* access="rb");
void   file_put_contents(const char *filename, const char *buf, size_t sz, const char* access="wb");

template< class CharContainer >
size_t file_get_contents(const char *filename, CharContainer *v, const char* access="rb")
{
    ::FILE *fp = ::fopen(filename, access);
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fseek(fp, 0, SEEK_END);
    v->resize(::ftell(fp));
    ::rewind(fp);
    ::fread(&(*v)[0], 1, v->size(), fp);
    ::fclose(fp);
    return v->size();
}


template< class CharContainer >
inline void file_put_contents(const char *filename, CharContainer const& v, const char* access="wb")
{
    file_put_contents(filename, &v[0], v.size(), access);
}


//-----------------------------------------------------------------------------
/** open a writeable temporary file in the current working directory.
 * The dtor deletes the temporary file. */
struct ScopedTmpFile
{
    char m_name[32];
    ::FILE* m_file;
    bool m_delete;

    const char* name() const { return m_name; }
    ::FILE* file() const { return m_file; }
    void do_delete(bool yes) { m_delete = yes; }

    ~ScopedTmpFile()
    {
        ::fclose(m_file);
        if(m_delete) delete_file(m_name);
        m_file = nullptr;
    }

    ScopedTmpFile(const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access="wb")
    {
        tmpnam(name_pattern, m_name, sizeof(m_name));
        m_file = ::fopen(m_name, access);
        m_delete = true;
    }

    ScopedTmpFile(const char* contents, size_t sz, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char *access="wb")
        : ScopedTmpFile(name_pattern, access)
    {
        ::fwrite(contents, 1, sz, m_file);
        ::fflush(m_file);
    }

    template< class CharContainer >
    ScopedTmpFile(CharContainer const& contents, const char* name_pattern="c4_ScopedTmpFile.XXXXXX.tmp", const char* access="wb")
        : ScopedTmpFile(&contents[0], contents.size(), name_pattern, access)
    {
    }
};

} // namespace fs
} // namespace c4

#endif /* _c4_FS_HPP_ */