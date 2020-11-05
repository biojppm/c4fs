#include <c4/fs/fs.hpp>
#include <c4/std/std.hpp>
#include <c4/substr.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <stdlib.h>
#include <string>

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
        delete_file(m_name);
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
    }
    CHECK_FALSE(path_exists(p.c_str()));
}

TEST_CASE("path_exists.dir")
{
    std::string p;
    {
        auto dir = ScopedTestDir();
        CHECK(path_exists(dir));
        p.assign(dir);
    }
    CHECK_FALSE(path_exists(p.c_str()));
}

TEST_CASE("path_type.file")
{
    auto file = ScopedTestFile();
    CHECK_EQ(path_type(file), REGFILE);
    CHECK(is_file(file));
    CHECK_FALSE(is_dir(file));
}

TEST_CASE("path_type.dir")
{
    auto file = ScopedTestDir();
    CHECK_EQ(path_type(file), DIR);
    CHECK_FALSE(is_file(file));
    CHECK(is_dir(file));
}

TEST_CASE("mkdir.basic")
{
    SUBCASE("mkdir") {
        CHECK_FALSE(path_exists("c4fdx"));
        CHECK_FALSE(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        mkdir("c4fdx");
        CHECK(path_exists("c4fdx"));
        CHECK_FALSE(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        mkdir("c4fdx/a");
        CHECK(path_exists("c4fdx"));
        CHECK(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        mkdir("c4fdx/a/b");
        CHECK(path_exists("c4fdx"));
        CHECK(path_exists("c4fdx/a"));
        CHECK(path_exists("c4fdx/a/b"));
    }
    SUBCASE("rmdir") {
        rmdir("c4fdx/a/b");
        CHECK(path_exists("c4fdx"));
        CHECK(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        rmdir("c4fdx/a");
        CHECK(path_exists("c4fdx"));
        CHECK_FALSE(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        rmdir("c4fdx");
        CHECK_FALSE(path_exists("c4fdx"));
        CHECK_FALSE(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
    }
}

TEST_CASE("mkdirs.basic")
{
    SUBCASE("mkdirs") {
        CHECK_FALSE(path_exists("c4fdx"));
        CHECK_FALSE(path_exists("c4fdx/a"));
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        CHECK_FALSE(path_exists("c4fdx/a/b/c"));
        char buf[32] = "c4fdx/a/b/c\0";
        mkdirs(buf);
        CHECK(path_exists("c4fdx"));
        CHECK(path_exists("c4fdx/a"));
        CHECK(path_exists("c4fdx/a/b"));
        CHECK(path_exists("c4fdx/a/b/c"));
    }
    SUBCASE("rmdir") {
        rmdir("c4fdx/a/b/c");
        CHECK_FALSE(path_exists("c4fdx/a/b/c"));
        rmdir("c4fdx/a/b");
        CHECK_FALSE(path_exists("c4fdx/a/b"));
        rmdir("c4fdx/a");
        CHECK_FALSE(path_exists("c4fdx/a"));
        rmdir("c4fdx");
        CHECK_FALSE(path_exists("c4fdx"));
    }
}

const char test_contents[] = R"(
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
\0)";

TEST_CASE("ScopedTmpFile.c_str")
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    FILE *rfile = fopen(wfile.m_name, "rb");

    fseek(rfile, 0, SEEK_END);
    size_t sz = static_cast<size_t>(ftell(rfile));
    rewind(rfile);
    CHECK_EQ(sizeof(test_contents), sz+1);

    char cmp[2*sizeof(test_contents)] = {0};
    size_t ret = fread(cmp, 1, sz, rfile);
    CHECK_EQ(ret+1, sizeof(test_contents));
    CHECK_EQ(to_csubstr(cmp), test_contents);
}

TEST_CASE("file_put_contents.basic")
{
    char filename[32];

    tmpnam(filename, sizeof(filename), "c4fpc.XXXXXX.test");

    file_put_contents(filename, test_contents, strlen(test_contents));

    auto rfile = fopen(filename, "rb");

    fseek(rfile, 0, SEEK_END);
    size_t sz = static_cast<size_t>(ftell(rfile));
    rewind(rfile);

    char cmp[2*sizeof(test_contents)] = {0};
    size_t ret = fread(cmp, 1, sz, rfile);
    CHECK_EQ(ret+1, sizeof(test_contents));
    CHECK_EQ(to_csubstr(cmp), test_contents);
}

TEST_CASE("file_get_contents.basic")
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    char cmp[2*sizeof(test_contents)] = {0};
    size_t sz = file_get_contents(wfile.m_name, cmp, sizeof(cmp));
    CHECK_EQ(sizeof(test_contents), sz+1);
    CHECK_EQ(to_csubstr(cmp), test_contents);
}

TEST_CASE("file_get_contents.std_string")
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    std::string s;
    size_t sz = file_get_contents(wfile.m_name, &s);
    CHECK_EQ(sizeof(test_contents), sz+1);
    CHECK_EQ(to_csubstr(s), test_contents);
}

} // namespace fs
} // namespace c4

#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#elif defined(__GNUC__)
#endif
