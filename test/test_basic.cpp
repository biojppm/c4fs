#include <c4/fs/fs.hpp>
#include <c4/std/std.hpp>
#include <c4/substr.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <stdlib.h>
#include <string>
#include <thread>

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4996) // 'strncpy', fopen, etc: This function or variable may be unsafe
#elif defined(__clang__)
#elif defined(__GNUC__)
#endif

namespace c4 {
namespace fs {

struct ScopedTestFile
{
    char m_name[32];
    operator const char* () const { return m_name; }
    ScopedTestFile(const char* access="wb") : m_name()
    {
        tmpnam(m_name, sizeof(m_name), "scoped_file.XXXXXX.test");
        ::FILE *f = fopen(m_name, access);
        fclose(f);
    }
    ~ScopedTestFile()
    {
        c4::fs::rmfile(m_name);
    }
};

struct ScopedTestDir
{
    char m_name[32];
    operator const char* () const { return m_name; }
    ScopedTestDir() : m_name()
    {
        tmpnam(m_name, sizeof(m_name), "scoped_dir.XXXXXX.test");
        c4::fs::mkdir(m_name);
    }
    ~ScopedTestDir()
    {
        c4::fs::rmdir(m_name);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST_CASE("path_exists.file")
{
    std::string p;
    {
        auto file = ScopedTestFile();
        p.assign(file);
        CHECK(path_exists(file));
        CHECK(file_exists(file));
        CHECK_FALSE(dir_exists(file));
    }
    CHECK_FALSE(path_exists(p.c_str()));
    CHECK_FALSE(file_exists(p.c_str()));
    CHECK_FALSE(dir_exists(p.c_str()));
}

TEST_CASE("path_exists.dir")
{
    std::string p;
    {
        auto dir = ScopedTestDir();
        CHECK(path_exists(dir));
        CHECK_FALSE(file_exists(dir));
        CHECK(dir_exists(dir));
        p.assign(dir);
    }
    CHECK_FALSE(path_exists(p.c_str()));
    CHECK_FALSE(file_exists(p.c_str()));
    CHECK_FALSE(dir_exists(p.c_str()));
}

TEST_CASE("path_type.file")
{
    auto file = ScopedTestFile();
    CHECK_EQ(path_type(file), REGFILE);
    CHECK(is_file(file));
    CHECK(file_exists(file));
    CHECK_FALSE(is_dir(file));
}

TEST_CASE("path_type.dir")
{
    auto dir = ScopedTestDir();
    CHECK_EQ(path_type(dir), DIR);
    CHECK(is_dir(dir));
    CHECK(dir_exists(dir));
    CHECK(is_dir(dir));
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST_CASE("path_times")
{
    auto file = ScopedTestFile();
    auto t0 = times(file);
    CHECK_EQ(t0.creation, ctime(file));
    CHECK_EQ(t0.modification, mtime(file));
    CHECK_EQ(t0.access, atime(file));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    file_put_contents(file, csubstr("THE CONTENTS"));
    auto t1 = times(file);
    CHECK_EQ(t1.creation, ctime(file));
    CHECK_EQ(t1.modification, mtime(file));
    CHECK_EQ(t1.access, atime(file));
    CHECK_GT(t1.creation, t0.creation);
    CHECK_GT(t1.modification, t0.modification);
    // CHECK_GT(t1.access, t0.access); // not required by the system
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST_CASE("mkdir.basic")
{
    SUBCASE("mkdir") {
        CHECK_FALSE(dir_exists("c4fdx"));
        CHECK_FALSE(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        mkdir("c4fdx");
        CHECK(dir_exists("c4fdx"));
        CHECK_FALSE(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        mkdir("c4fdx/a");
        CHECK(dir_exists("c4fdx"));
        CHECK(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        mkdir("c4fdx/a/b");
        CHECK(dir_exists("c4fdx"));
        CHECK(dir_exists("c4fdx/a"));
        CHECK(dir_exists("c4fdx/a/b"));
    }
    SUBCASE("rmdir") {
        rmdir("c4fdx/a/b");
        CHECK(dir_exists("c4fdx"));
        CHECK(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        rmdir("c4fdx/a");
        CHECK(dir_exists("c4fdx"));
        CHECK_FALSE(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        rmdir("c4fdx");
        CHECK_FALSE(dir_exists("c4fdx"));
        CHECK_FALSE(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
    }
}

TEST_CASE("mkdirs.basic")
{
    SUBCASE("mkdirs") {
        CHECK_FALSE(dir_exists("c4fdx"));
        CHECK_FALSE(dir_exists("c4fdx/a"));
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        CHECK_FALSE(dir_exists("c4fdx/a/b/c"));
        char buf[32] = "c4fdx/a/b/c\0";
        mkdirs(buf);
        CHECK(dir_exists("c4fdx"));
        CHECK(dir_exists("c4fdx/a"));
        CHECK(dir_exists("c4fdx/a/b"));
        CHECK(dir_exists("c4fdx/a/b/c"));
    }
    SUBCASE("rmdir") {
        rmdir("c4fdx/a/b/c");
        CHECK_FALSE(dir_exists("c4fdx/a/b/c"));
        rmdir("c4fdx/a/b");
        CHECK_FALSE(dir_exists("c4fdx/a/b"));
        rmdir("c4fdx/a");
        CHECK_FALSE(dir_exists("c4fdx/a"));
        rmdir("c4fdx");
        CHECK_FALSE(dir_exists("c4fdx"));
    }
}

TEST_CASE("rmfile")
{
    SUBCASE("existing")
    {
        const char filename[] = "adslkjasdlkj";
        file_put_contents(filename, csubstr("THE CONTENTS"));
        CHECK(file_exists(filename));
        CHECK_EQ(rmfile(filename), 0);
        CHECK(!file_exists(filename));
    }
    SUBCASE("nonexisting")
    {
        const char filename[] = "adslkjasdlkj";
        CHECK(!file_exists(filename));
        CHECK_NE(rmfile(filename), 0);
    }
}

const char * _make_tree()
{
    auto fpcon = [](const char* path){
        file_put_contents(path, csubstr("THE CONTENTS"));
        CHECK(file_exists(path));
    };
    mkdir("c4fdx");
    fpcon("c4fdx/file1");
    fpcon("c4fdx/file2");
    mkdir("c4fdx/a");
    fpcon("c4fdx/a/file1");
    fpcon("c4fdx/a/file2");
    mkdir("c4fdx/a/1");
    fpcon("c4fdx/a/1/file1");
    fpcon("c4fdx/a/1/file2");
    mkdir("c4fdx/a/1/a");
    fpcon("c4fdx/a/1/a/file1");
    fpcon("c4fdx/a/1/a/file2");
    mkdir("c4fdx/a/1/b");
    fpcon("c4fdx/a/1/b/file1");
    fpcon("c4fdx/a/1/b/file2");
    mkdir("c4fdx/a/1/c");
    fpcon("c4fdx/a/1/c/file1");
    fpcon("c4fdx/a/1/c/file2");
    mkdir("c4fdx/a/2");
    fpcon("c4fdx/a/2/file1");
    fpcon("c4fdx/a/2/file2");
    mkdir("c4fdx/a/2/a");
    fpcon("c4fdx/a/2/a/file1");
    fpcon("c4fdx/a/2/a/file2");
    mkdir("c4fdx/a/2/b");
    fpcon("c4fdx/a/2/b/file1");
    fpcon("c4fdx/a/2/b/file2");
    mkdir("c4fdx/a/2/c");
    fpcon("c4fdx/a/2/c/file1");
    fpcon("c4fdx/a/2/c/file2");
    mkdir("c4fdx/b");
    fpcon("c4fdx/b/file1");
    fpcon("c4fdx/b/file2");
    mkdir("c4fdx/c");
    fpcon("c4fdx/c/file1");
    fpcon("c4fdx/c/file2");
    return "c4fdx";
}

TEST_CASE("rmtree")
{
    SUBCASE("existing")
    {
        auto treename = _make_tree();
        CHECK(dir_exists(treename));
        CHECK_EQ(rmtree(treename), 0);
        CHECK(!dir_exists(treename));
    }
    SUBCASE("nonexisting")
    {
        CHECK(!dir_exists("nonexisting"));
        CHECK_NE(rmtree("nonexisting"), 0);
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

u32 file_count = 0;
u32 dir_count = 0;
int entry_visitor(VisitedFile const& p)
{
    CHECK(path_exists(p.name));
    if(is_file(p.name))
        ++file_count;
    if(is_dir(p.name))
        ++dir_count;
    return 0;
}

TEST_CASE("walk_entries")
{
    const std::string cwd_orig = cwd<std::string>();
    constexpr const char dirname[] = "c4fdx";
    mkdir(dirname);
    file_put_contents("c4fdx/file0", csubstr("asdasdasd"));
    file_put_contents("c4fdx/file1", csubstr("asdasdasd"));
    SUBCASE("empty_name_buffer")
    {
        dir_count = file_count = 0;
        int ret = walk_entries(dirname, entry_visitor, nullptr, 0);
        CHECK_NE(ret, 0);
        CHECK_EQ(file_count, 0);
        CHECK_EQ(dir_count, 0);
    }
    SUBCASE("small_name_buffer")
    {
        char buf[sizeof(dirname) + 3]; // not enough for the children files
        dir_count = file_count = 0;
        int ret = walk_entries(dirname, entry_visitor, buf, sizeof(buf));
        CHECK(ret != 0);
        CHECK_EQ(file_count, 0);
        CHECK_EQ(dir_count, 0);
    }
    SUBCASE("vanilla")
    {
        char buf[100];
        dir_count = file_count = 0;
        int ret = walk_entries(dirname, entry_visitor, buf, sizeof(buf));
        CHECK(ret == 0);
        CHECK_EQ(file_count, 2);
        CHECK_EQ(dir_count, 0);
    }
    mkdir("c4fdx/dir");
    file_put_contents("c4fdx/dir/file2", csubstr("asdasdasd"));
    file_put_contents("c4fdx/dir/file3", csubstr("asdasdasd"));
    mkdir("c4fdx/dir2");
    file_put_contents("c4fdx/dir2/file4", csubstr("asdasdasd"));
    file_put_contents("c4fdx/dir2/file5", csubstr("asdasdasd"));
    SUBCASE("dont_descend_into_subdirs")
    {
        dir_count = file_count = 0;
        char buf[100];
        int ret = walk_entries(dirname, entry_visitor, buf, sizeof(buf));
        CHECK(ret == 0);
        CHECK_EQ(file_count, 2); // must not have changed
        CHECK_EQ(dir_count, 2); // but must see the new subdirs
    }
    CHECK_EQ(rmtree(dirname), 0);
    CHECK_EQ(cwd<std::string>(), cwd_orig);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int path_visitor(VisitedPath const& p)
{
    CHECK(path_exists(p.name));
    if(is_file(p.name))
        ++file_count;
    if(is_dir(p.name))
        ++dir_count;
    return 0;
}

TEST_CASE("walk_tree")
{
    SUBCASE("existing")
    {
        auto treename = _make_tree();
        CHECK(dir_exists(treename));
        int ret = walk_tree(treename, path_visitor);
        CHECK(ret == 0);
        CHECK_EQ(file_count, 26);
        CHECK_EQ(dir_count, 14); // but must see the new subdirs
        CHECK_EQ(rmtree(treename), 0);
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

csubstr test_contents = R"(
0
1
2
3
4
5
6
7
8
9
10
\0
)";

TEST_CASE("ScopedTmpFile.c_str")
{
    auto wfile = ScopedTmpFile(test_contents.str, test_contents.len);
    CHECK_EQ(file_size(wfile.m_name), test_contents.len);

    std::string out = file_get_contents<std::string>(wfile.m_name);
    CHECK_EQ(out.size(), test_contents.len);
    CHECK_EQ(to_csubstr(out), test_contents);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST_CASE("file_put_contents.basic")
{
    std::string filename_buf = tmpnam<std::string>();
    const char *filename = filename_buf.c_str();

    file_put_contents(filename, test_contents);
    CHECK_EQ(file_size(filename), test_contents.len);
    std::string cmp = file_get_contents<std::string>(filename);
    CHECK_EQ(to_csubstr(cmp), test_contents);
}

TEST_CASE("file_get_contents.std_string")
{
    auto wfile = ScopedTmpFile(test_contents.str, test_contents.len);
    std::string s = file_get_contents<std::string>(wfile.m_name);
    CHECK_EQ(to_csubstr(s), test_contents);
}

TEST_CASE("file_get_contents.std_vector_char")
{
    auto wfile = ScopedTmpFile(test_contents.str, test_contents.len);
    std::vector<char> s = file_get_contents<std::vector<char>>(wfile.m_name);
    CHECK_EQ(to_csubstr(s), test_contents);
}

} // namespace fs
} // namespace c4

#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#elif defined(__GNUC__)
#endif
