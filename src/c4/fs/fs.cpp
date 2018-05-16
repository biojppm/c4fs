#include "c4/fs/fs.hpp"

#include <c4/substr.hpp>
#include <c4/to_str.hpp>

#ifdef C4_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#endif

#include <stdio.h>
#include <random>

namespace c4 {
namespace fs {

int _exec_stat(const char *pathname, struct stat *s)
{
#ifdef C4_WIN
    /* If path contains the location of a directory, it cannot contain
     * a trailing backslash. If it does, -1 will be returned and errno
     * will be set to ENOENT. */
    size_t len = strlen(pathname);
    C4_ASSERT(len > 0);
    C4_ASSERT(pathname[len] == '\0');
    C4_ASSERT(pathname[len-1] != '/' && pathname[len-1] != '\\');
    return ::stat(pathname, s);
#endif
#ifdef C4_POSIX
    return ::stat(pathname, s);
#endif
}

int _exec_fstat(int fd, struct stat *s)
{
#ifdef C4_POSIX
    return ::fstat(fd, s);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

PathType_e _path_type(struct stat *C4_RESTRICT s)
{
#if defined(C4_POSIX) || defined(C4_WIN)
#   if defined(C4_WIN)
#      define _c4is(what) (s->st_mode & _S_IF##what)
#   else
#      define _c4is(what) (S_IS##what(s->st_mode))
#   endif
    // https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
    if(_c4is(REG))
    {
        return REGFILE;
    }
    else if(_c4is(DIR))
    {
        return DIR;
    }
    else if(_c4is(LNK))
    {
        return SYMLINK;
    }
    else if(_c4is(FIFO))
    {
        return PIPE;
    }
    else if(_c4is(SOCK))
    {
        return SOCK;
    }
    else //if(_c4is(BLK) || _c4is(CHR))
    {
        return OTHER;
    }
#   undef _c4is
#else
    C4_NOT_IMPLEMENTED();
    return OTHER;
#endif

}

bool path_exists(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN)
    struct stat s;
    return _exec_stat(pathname, &s) == 0;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
}


PathType_e path_type(const char *pathname)
{
    struct stat s;
    C4_CHECK(_exec_stat(pathname, &s) == 0);
    return _path_type(&s);
}


path_times times(const char *pathname)
{
    path_times t;
#if defined(C4_POSIX) || defined(C4_WIN)
    struct stat s;
    _exec_stat(pathname, &s);
    t.creation = s.st_ctime;
    t.modification = s.st_mtime;
    t.access = s.st_atime;
#else
    C4_NOT_IMPLEMENTED();
#endif
    return t;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void rmdir(const char *dirname)
{
#if defined(C4_POSIX)
    ::rmdir(dirname);
#elif defined(C4_WIN) || defined(C4_XBOX)
    ::_rmdir(dirname);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

int _exec_mkdir(const char *dirname)
{
#if defined(C4_POSIX)
    return ::mkdir(dirname, 0755);
#elif defined(C4_WIN) || defined(C4_XBOX)
    return ::_mkdir(dirname);
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}

void mkdir(const char *dirname)
{
    _exec_mkdir(dirname);
}

void mkdirs(char *pathname)
{
    // add 1 to len because we know that the buffer has a null terminator
    c4::substr dir, buf = {pathname, 1 + strlen(pathname)};
    size_t start_pos = 0;
    while(buf.next_split('/', &start_pos, &dir))
    {
        char ctmp = buf.str[start_pos];
        buf.str[start_pos] = '\0';
        _exec_mkdir(buf.str);
        buf.str[start_pos] = ctmp;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

char *cwd(char *buf, size_t sz)
{
    C4_ASSERT(sz > 0);
#ifdef C4_POSIX
    return ::getcwd(buf, sz);
#else
    C4_NOT_IMPLEMENTED();
    return nullptr;
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void delete_file(const char *filename)
{
#ifdef C4_POSIX
    ::unlink(filename);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

void delete_path(const char *pathname, bool recursive)
{
#ifdef C4_POSIX
    if( ! recursive)
    {
        ::remove(pathname);
    }
    else
    {
        walk(pathname, [](VisitedPath const& C4_RESTRICT p) -> int {
                if(p.type == REGFILE || p.type == SYMLINK)
                {
                    delete_file(p.name);
                }
                return 0;
            });
        walk(pathname, [](VisitedPath const& C4_RESTRICT p) -> int {
                if(p.type == DIR)
                {
                    delete_path(p.name, /*recursive*/false);
                }
                return 0;
            });
    }
#else
    C4_NOT_IMPLEMENTED();
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int walk(const char *pathname, PathVisitor fn, void *user_data)
{
    C4_CHECK(is_dir(pathname));

#ifdef C4_POSIX
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    const char* argv[] = {pathname, nullptr};
    ::FTS *root = ::fts_open((char *const *)argv, fts_options, nullptr);
    C4_CHECK(root != nullptr);

    ::FTSENT *node;
    VisitedPath vp;
    vp.user_data = user_data;
    while(1)
    {
        node = fts_read(root);
        if( ! node) break;
        vp.name = node->fts_name;
        vp.type = _path_type(node->fts_statp);
        vp.node = node;
        int ret = fn(vp);
        if(ret != 0) break;
    }
    ::fts_close(root);
#else
    C4_NOT_IMPLEMENTED();
#endif
    return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char* tmpnam(const char *fmt_, char *buf, size_t bufsz)
{
    c4::csubstr fmt = to_csubstr(fmt_);
    C4_CHECK(bufsz > fmt.len);
    C4_CHECK(fmt.find("XXXXXX") != csubstr::npos);
    auto pos = fmt.find("XXXXXX");
    memcpy(buf, fmt.str, fmt.len);
    buf[fmt.len] = '\0';

    thread_local static std::random_device rand_eng;
    std::uniform_int_distribution<uint8_t> rand_dist;
    constexpr static const char hexchars[] = "01234567890abcdef";

    for(size_t i = 0; i < 3; ++i)
    {
        char * w = buf + (pos + 2*i);
        uint8_t num = rand_dist(rand_eng);
        w[0] = hexchars[(num >> 0) & 0xf];
        w[1] = hexchars[(num >> 4) & 0xf];
    }

    return buf;
}

} // namespace fs
} // namespace c4
