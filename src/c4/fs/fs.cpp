#include "c4/fs/fs.hpp"

#include <c4/platform.hpp>
#include <c4/substr.hpp>
#include <c4/charconv.hpp>

#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ftw.h>
#include <dirent.h>
#endif

#include "c4/c4_push.hpp"

#ifdef C4_WIN
#   include <windef.h>
#   include <minwindef.h>
#   include <direct.h>
#   include <fileapi.h>
#   include <handleapi.h>
#   include <c4/memory_resource.hpp>
#endif

#include <random>


namespace c4 {
namespace fs {

namespace /*anon*/ {

/** @todo make this more complete */
bool _is_escape(size_t char_pos, const char *pathname, size_t sz)
{
    C4_UNUSED(sz);
    C4_ASSERT(char_pos < sz);
    const char c = pathname[char_pos];
#ifdef C4_WIN
    return c == '^';
#else
    return c == '\\';
#endif
}

int _exec_stat(const char *pathname, struct stat *s)
{
#ifdef C4_WIN
    /* If path contains the location of a directory, it cannot contain
     * a trailing backslash. If it does, -1 will be returned and errno
     * will be set to ENOENT. */
    auto len = strlen(pathname);
    C4_UNUSED(len);
    C4_ASSERT(len > 0);
    C4_ASSERT(pathname[len] == '\0');
    C4_ASSERT(pathname[len-1] != '/' && pathname[len-1] != '\\');
    return ::stat(pathname, s);
#elif defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    return ::stat(pathname, s);
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}

PathType_e _path_type(struct stat *C4_RESTRICT s)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
#   if defined(C4_WIN)
#      define _c4is(what) (s->st_mode & _S_IF##what)
#   else
#      define _c4is(what) (S_IS##what(s->st_mode))
#   endif
    // https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
    if(_c4is(REG))
        return REGFILE;
    else if(_c4is(DIR))
        return DIR;
#if !defined(C4_WIN)
    else if(_c4is(LNK))
        return SYMLINK;
    else if(_c4is(FIFO))
        return PIPE;
    else if(_c4is(SOCK))
        return SOCK;
#endif
    else //if(_c4is(BLK) || _c4is(CHR))
        return OTHER;
#   undef _c4is
#else
    C4_NOT_IMPLEMENTED();
    return OTHER;
#endif
}

int _exec_mkdir(const char *dirname)
{
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    return ::mkdir(dirname, 0755);
#elif defined(C4_WIN) || defined(C4_XBOX)
    return ::_mkdir(dirname);
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}

} // namespace /*anon*/


bool is_sep(size_t char_pos, const char *pathname, size_t sz)
{
    C4_ASSERT(char_pos < sz);
    const char c = pathname[char_pos];
    const char prev = char_pos > 0 ? pathname[char_pos - 1] : '\0';
#ifdef C4_WIN
    if(c != '/' || c == '\\')
        return false;
    if(prev)
        return ! _is_escape(char_pos-1, pathname, sz);
    return true;
#else
    if(c != '/')
        return false;
    if(prev)
        return ! _is_escape(char_pos-1, pathname, sz);
    return true;
#endif
}

bool to_unix_sep(char *pathname, size_t sz)
{
    bool changes = false;
    for(size_t i = 0; i < sz; ++i)
    {
        if(is_sep(i, pathname, sz))
        {
            pathname[i] = '/';
            changes = true;
        }
    }
    return changes;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool path_exists(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    return _exec_stat(pathname, &s) == 0;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
}


bool file_exists(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    auto ret = _exec_stat(pathname, &s);
    if(ret != 0)
        return false;
    auto type = _path_type(&s);
    return type == REGFILE || type == SYMLINK;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
}


bool dir_exists(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    auto ret = _exec_stat(pathname, &s);
    if(ret != 0)
        return false;
    auto type = _path_type(&s);
    return type == DIR;
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
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    _exec_stat(pathname, &s);
    using ttype = decltype(t.creation);
    t.creation = static_cast<ttype>(s.st_ctime);
    t.modification = static_cast<ttype>(s.st_mtime);
    t.access = static_cast<ttype>(s.st_atime);
#else
    C4_NOT_IMPLEMENTED();
#endif
    return t;
}

uint64_t ctime(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    _exec_stat(pathname, &s);
    return static_cast<uint64_t>(s.st_ctime);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

uint64_t mtime(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    _exec_stat(pathname, &s);
    return static_cast<uint64_t>(s.st_mtime);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

uint64_t atime(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN) || defined(C4_MACOS) || defined(C4_IOS)
    struct stat s;
    _exec_stat(pathname, &s);
    return static_cast<uint64_t>(s.st_atime);
#else
    C4_NOT_IMPLEMENTED();
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int rmdir(const char *dirname)
{
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    return ::rmdir(dirname);
#elif defined(C4_WIN) || defined(C4_XBOX)
    return ::_rmdir(dirname);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

int mkdir(const char *dirname)
{
    return _exec_mkdir(dirname);
}

void mkdirs(char *pathname)
{
    size_t sz = strlen(pathname);
    // add 1 to len because we know that the buffer has a null terminator
    c4::substr dir, buf = {pathname, 1 + sz};
    size_t start_pos = 0;
    while(buf.next_split('/', &start_pos, &dir))
    {
        if(dir.empty())
            continue;
        if(start_pos < sz)
        {
            char ctmp = buf.str[start_pos];
            buf.str[start_pos] = '\0';
            _exec_mkdir(buf.str);
            buf.str[start_pos] = ctmp;
        }
        else
        {
            _exec_mkdir(buf.str);
        }
    }
    C4_CHECK_MSG(dir_exists(pathname), "dir=%s", pathname);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int rmfile(const char *filename)
{
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS) || defined(C4_WIN)
    return ::unlink(filename);
#else
    C4_NOT_IMPLEMENTED();
    return 1;
#endif
}

#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
int _unlink_cb(const char *fpath, const struct stat *, int , struct FTW *)
{
    return ::remove(fpath);
}
#elif defined(C4_WIN)
int _rmtree_visitor(VisitedPath const& p)
{
    if((p.find_file_data && p.find_file_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || is_dir(p.name))
        return rmdir(p.name);
    else
        return rmfile(p.name);
}
#endif

int rmtree(const char *path)
{
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    return nftw(path, _unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
#elif defined(C4_WIN)
    if(!dir_exists(path))
        return ENOENT;
    return walk_tree(path, _rmtree_visitor);
#else
    C4_NOT_IMPLEMENTED();
    return 1;
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

char *cwd(char *buf, size_t sz)
{
    C4_ASSERT(sz > 0);
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    return ::getcwd(buf, sz);
#elif defined(C4_WIN)
    return ::getcwd(buf, (int)sz);
#else
    C4_NOT_IMPLEMENTED();
    return nullptr;
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool walk_entries(const char *pathname, FileVisitor fn, maybe_buf<char> *buf, void *user_data)
{
    C4_CHECK(is_dir(pathname));
    C4_CHECK((buf->buf == nullptr) == (buf->size == 0));
    csubstr base = to_csubstr(pathname);
    size_t base_size = (base.len + 1/*'/'*/) + 1/*'\0'*/;
    substr namebuf(buf->buf, buf->size);
    size_t maxlen = 0;
    C4_ASSERT(!namebuf.overlaps(base));
    buf->required_size = base_size;
    VisitedFile vp;
    vp.user_data = user_data;
    vp.name = namebuf.str;
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    if(buf->valid())
    {
        memcpy(namebuf.str, base.str, base.len);
        namebuf[base.len] = '/';
    }
    ::DIR *dir = opendir(pathname);
    C4_CHECK(dir);
    struct dirent *entry;
    while((entry = readdir(dir)) != nullptr)
    {
        if(strcmp(entry->d_name, ".") == 0)
            continue;
        if(strcmp(entry->d_name, "..") == 0)
            continue;
        csubstr entry_name = to_csubstr(entry->d_name);
        maxlen = entry_name.len > maxlen ? entry_name.len : maxlen;
        buf->required_size = base_size + maxlen + 1;
        if(buf->valid())
        {
            substr after_slash = namebuf.sub(base.len + 1);
            memcpy(after_slash.str, entry_name.str, entry_name.len);
            after_slash[entry_name.len] = '\0';
            vp.dirent_data = entry;
            if(fn(vp) != 0)
                break;
        }
    }
    closedir(dir);
#else
    base_size = base.len + 4;
    buf->required_size = base_size;
    if(!buf->valid())
    {
        // if the buffer cannot accomodate the base_size,
        // we cannot open the directory to iterate.
        // To be safer, let's require 64 characters for appending
        // the filenames.
        buf->required_size += 64u;
        return false;
    }
    memcpy(namebuf.str, base.str, base.len);
    memcpy(namebuf.str + base.len, "\\*\0", 3);
    WIN32_FIND_DATAA ffd = {};
    HANDLE hFindFile = FindFirstFileA(namebuf.str, &ffd);
    C4_CHECK(hFindFile != INVALID_HANDLE_VALUE);
    namebuf[base.len] = '/';
    vp.find_file_data = &ffd;
    while(FindNextFileA(hFindFile, &ffd))
    {
        if(!strcmp(ffd.cFileName, "."))
            continue;
        if(!strcmp(ffd.cFileName, ".."))
            continue;
        csubstr entry_name = to_csubstr(ffd.cFileName);
        maxlen = entry_name.len > maxlen ? entry_name.len : maxlen;
        buf->required_size = base_size + maxlen + 1;
        if(buf->valid())
        {
            substr after_slash = namebuf.sub(base.len + 1);
            memcpy(after_slash.str, entry_name.str, entry_name.len);
            after_slash[entry_name.len] = '\0';
            if(fn(vp) != 0)
                break;
        }
    }
    FindClose(hFindFile);
#endif
    return buf->valid();
}

PathVisitor wtf; // fixme
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
int _path_visitor_adapter(const char *name, const struct stat *stat_data, int ftw_info, struct FTW *ftw_data)
{
    VisitedPath vp;
    vp.name = name;
    vp.user_data = nullptr; // fixme
    vp.stat_data = stat_data;
    vp.ftw_info = ftw_info;
    vp.ftw_data = ftw_data;
    return wtf(vp);
}
#elif defined(C4_WIN)
int _walk_tree(PathVisitor fn, void *user_data, substr namebuf, size_t namelen)
{
    int exit_status = 0;
    VisitedPath vp;
    vp.user_data = user_data;
    vp.name = namebuf.str;
    if(namebuf.len < namelen + 4)
        return ENAMETOOLONG;
    WIN32_FIND_DATAA ffd = {};
    memcpy(namebuf.str + namelen, "\\*\0", 3);
    HANDLE hFindFile = FindFirstFileA(namebuf.str, &ffd);
    C4_CHECK(hFindFile != INVALID_HANDLE_VALUE);
    namebuf[namelen] = '/';
    substr filenamebuf = namebuf.sub(namelen + 1);
    vp.find_file_data = &ffd;
    while(FindNextFileA(hFindFile, &ffd))
    {
        if(!strcmp(ffd.cFileName, "."))
            continue;
        if(!strcmp(ffd.cFileName, ".."))
            continue;
        csubstr entry_name = to_csubstr(ffd.cFileName);
        if(filenamebuf.len < entry_name.len + 2)
        {
            exit_status = ENAMETOOLONG;
            break;
        }
        memcpy(filenamebuf.str, entry_name.str, entry_name.len);
        filenamebuf[entry_name.len] = '\0';
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            int ret = _walk_tree(fn, user_data, namebuf, namelen + 1 + entry_name.len);
            if(ret)
            {
                exit_status = ret;
                break;
            }
        }
        if(fn(vp) != 0)
            break;
    }
    FindClose(hFindFile);
    namebuf[namelen] = '\0';
    return exit_status;
}
#endif

int walk_tree(const char *pathname, PathVisitor fn, void *user_data)
{
    C4_CHECK(is_dir(pathname));
    wtf = fn; // fixme
#if defined(C4_POSIX) || defined(C4_MACOS) || defined(C4_IOS)
    (void)user_data;
    return nftw(pathname, _path_visitor_adapter, 64, FTW_PHYS);
#elif defined(C4_WIN)
    substr namebuf;
    namebuf.len = MAX_PATH;
    csubstr base = to_csubstr(pathname);
    if(namebuf.len < base.len + 1)
        namebuf.len = 2 * (base.len + 1);
    namebuf.str = (char*) c4::aalloc(namebuf.len, alignof(std::max_align_t));
    C4_ASSERT(!namebuf.overlaps(base));
    memcpy(namebuf.str, base.str, base.len);
    namebuf[base.len] = '\0';
    int exit_status = _walk_tree(fn, user_data, namebuf, base.len);
    while(exit_status == ENAMETOOLONG)
    {
        c4::afree(namebuf.str);
        namebuf.len *= 2;
        namebuf.str = (char*) c4::aalloc(namebuf.len, alignof(std::max_align_t));
        exit_status = _walk_tree(fn, user_data, namebuf, base.len);
    }
    if(!exit_status)
    {
        VisitedPath vp = {};
        vp.name = pathname;
        fn(vp);
    }
    c4::afree(namebuf.str);
    return exit_status;
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}


thread_local static EntryList _list_entries_workspace = {};
int _list_entries_visitor(VisitedFile const& vf)
{
    size_t namelen = strlen(vf.name);
    auto arena_prev = _list_entries_workspace.arena;
    auto names_prev = _list_entries_workspace.names;
    _list_entries_workspace.names.required_size += 1u;
    _list_entries_workspace.arena.required_size += namelen + 1u;
    if(_list_entries_workspace.valid())
    {
        char *dst = arena_prev.buf + arena_prev.required_size;
        memcpy(dst, vf.name, namelen);
        dst[namelen] = '\0';
        names_prev.buf[names_prev.required_size] = dst;
    }
    return 0;
}

bool list_entries(const char *pathname, EntryList *C4_RESTRICT entries, maybe_buf<char> *scratch)
{
    scratch->reset();
    entries->reset();
    _list_entries_workspace = *entries;
    if(!walk_entries(pathname, _list_entries_visitor, scratch))
        return false;
    scratch->required_size = scratch->size;
    *entries = _list_entries_workspace;
    return entries->valid() && scratch->valid();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char* tmpnam(char *buf_, size_t bufsz, const char *fmt_, char subchar)
{
    const char lookup[3] = {subchar, subchar, '\0'};
    c4::csubstr fmt = to_csubstr(fmt_);
    c4::substr buf(buf_, fmt.len+1);
    C4_CHECK(bufsz > fmt.len);
    C4_CHECK(fmt.find(lookup) != csubstr::npos);
    memcpy(buf_, fmt.str, fmt.len);
    buf_[fmt.len] = '\0';

    constexpr static const char hexchars[] = "01234567890abcdef";
    thread_local static std::random_device rand_eng;
    std::uniform_int_distribution<int> rand_dist(0, 255); // N4659 29.6.1.1 [rand.req.genl]/1e requires one of short, int, long, long long, unsigned short, unsigned int, unsigned long, or unsigned long long

    size_t pos = 0;
    while((pos = buf.find(lookup, pos)) != csubstr::npos)
    {
        int num = rand_dist(rand_eng);
        buf[pos++] = hexchars[ num       & 0xf];
        buf[pos++] = hexchars[(num >> 4) & 0xf];
    }

    return buf_;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

size_t file_size(const char *filename, const char *access)
{
    ::FILE *fp = ::fopen(filename, access);
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fseek(fp, 0, SEEK_END);
    size_t fs = static_cast<size_t>(::ftell(fp));
    ::rewind(fp);
    return fs;
}

size_t file_get_contents(const char *filename, char *buf, size_t sz, const char* access)
{
    ::FILE *fp = ::fopen(filename, access);
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fseek(fp, 0, SEEK_END);
    size_t fs = static_cast<size_t>(::ftell(fp));
    ::rewind(fp);
    if(fs <= sz && buf != nullptr)
    {
        size_t ret = ::fread(buf, 1, fs, fp);
        C4_CHECK(ret == fs);
    }
    ::fclose(fp);
    return fs;
}

void file_put_contents(const char *filename, const char *buf, size_t sz, const char* access)
{
    ::FILE *fp = ::fopen(filename, access);
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fwrite(buf, 1, sz, fp);
    ::fclose(fp);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

ScopedTmpFile::ScopedTmpFile(const char* name_pattern, const char *access, bool delete_after_use)
{
    C4_CHECK(strlen(name_pattern) < sizeof(m_name));
    tmpnam(m_name, sizeof(m_name), name_pattern);
    m_file = ::fopen(m_name, access);
    m_delete = delete_after_use;
}

ScopedTmpFile::ScopedTmpFile(const char* contents_, size_t sz, const char* name_pattern, const char *access, bool delete_after_use)
    : ScopedTmpFile(name_pattern, access, delete_after_use)
{
    ::fwrite(contents_, 1, sz, m_file);
    ::fflush(m_file);
}

ScopedTmpFile::~ScopedTmpFile()
{
    if(m_delete)
        rmfile(m_name);
    if( ! m_file)
        return;
    fclose(m_file);
    m_file = nullptr;
}

void ScopedTmpFile::_move(ScopedTmpFile *that)
{
    memcpy(m_name, that->m_name, sizeof(m_name));
    memset(that->m_name, 0, sizeof(m_name));
    m_file = that->m_file;
    m_delete = that->m_delete;
    that->m_file = nullptr;
}

const char* ScopedTmpFile::full_path(char *buf, size_t sz) const
{
    if(cwd(buf, sz) == nullptr)
        return nullptr;
    size_t cwdlen = strlen(buf);
    size_t namelen = strlen(m_name);
    if(sz < cwdlen + 1 + namelen)
        return nullptr;
    buf[cwdlen] = '/';
    memcpy(buf+cwdlen+1, m_name, namelen);
    buf[cwdlen + 1 + namelen] = '\0';
    return buf;
}

} // namespace fs
} // namespace c4

#include "c4/c4_pop.hpp"
