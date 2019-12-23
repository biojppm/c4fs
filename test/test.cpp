#include <c4/fs/fs.hpp>
#include <gtest/gtest.h>
#include <stdlib.h>

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

TEST(mkdir, basic)
{
    EXPECT_FALSE(path_exists("c4fdx"));
    EXPECT_FALSE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    mkdir("c4fdx");
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_FALSE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    mkdir("c4fdx/a");
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_TRUE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    mkdir("c4fdx/a/b");
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_TRUE(path_exists("c4fdx/a"));
    EXPECT_TRUE(path_exists("c4fdx/a/b"));
    rmdir("c4fdx/a/b");
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_TRUE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    rmdir("c4fdx/a");
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_FALSE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    rmdir("c4fdx");
    EXPECT_FALSE(path_exists("c4fdx"));
    EXPECT_FALSE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
}

TEST(mkdirs, basic)
{
    EXPECT_FALSE(path_exists("c4fdx"));
    EXPECT_FALSE(path_exists("c4fdx/a"));
    EXPECT_FALSE(path_exists("c4fdx/a/b"));
    EXPECT_FALSE(path_exists("c4fdx/a/b/c"));
    char buf[32] = "c4fdx/a/b/c\0";
    mkdirs(buf);
    EXPECT_TRUE(path_exists("c4fdx"));
    EXPECT_TRUE(path_exists("c4fdx/a"));
    EXPECT_TRUE(path_exists("c4fdx/a/b"));
    EXPECT_TRUE(path_exists("c4fdx/a/b/c"));
    rmdir("c4fdx/a/b/c");
    rmdir("c4fdx/a/b");
    rmdir("c4fdx/a");
    rmdir("c4fdx");
}

TEST(path_exists, file)
{
    std::string p;
    {
        auto file = ScopedTestFile();
        p.assign(file);
        EXPECT_TRUE(path_exists(file));
    }
    EXPECT_FALSE(path_exists(p.c_str()));
}

TEST(path_exists, dir)
{
    std::string p;
    {
        auto dir = ScopedTestDir();
        EXPECT_TRUE(path_exists(dir));
        p.assign(dir);
    }
    EXPECT_FALSE(path_exists(p.c_str()));
}

TEST(path_type, file)
{
    auto file = ScopedTestFile();
    EXPECT_EQ(path_type(file), REGFILE);
    EXPECT_TRUE(is_file(file));
    EXPECT_FALSE(is_dir(file));
}

TEST(path_type, dir)
{
    auto file = ScopedTestDir();
    EXPECT_EQ(path_type(file), DIR);
    EXPECT_FALSE(is_file(file));
    EXPECT_TRUE(is_dir(file));
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

TEST(ScopedTmpFile, c_str)
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    FILE *rfile = fopen(wfile.m_name, "rb");

    fseek(rfile, 0, SEEK_END);
    size_t sz = ftell(rfile);
    rewind(rfile);
    EXPECT_EQ(sizeof(test_contents), sz+1);

    char cmp[2*sizeof(test_contents)] = {0};
    fread(cmp, 1, sz, rfile);
    EXPECT_STREQ(cmp, test_contents);
}

TEST(file_put_contents, basic)
{
    char filename[32];

    tmpnam(filename, sizeof(filename), "c4fpc.XXXXXX.test");

    file_put_contents(filename, test_contents, strlen(test_contents));

    auto rfile = fopen(filename, "rb");

    fseek(rfile, 0, SEEK_END);
    size_t sz = ftell(rfile);
    rewind(rfile);

    char cmp[2*sizeof(test_contents)] = {0};
    fread(cmp, 1, sz, rfile);
    EXPECT_STREQ(cmp, test_contents);
}

TEST(file_get_contents, basic)
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    char cmp[2*sizeof(test_contents)] = {0};
    size_t sz = file_get_contents(wfile.m_name, cmp, sizeof(cmp));
    EXPECT_EQ(sizeof(test_contents), sz+1);
    EXPECT_STREQ(cmp, test_contents);
}

TEST(file_get_contents, std_string)
{
    auto  wfile = ScopedTmpFile(test_contents, strlen(test_contents));
    std::string s;
    size_t sz = file_get_contents(wfile.m_name, &s);
    EXPECT_EQ(sizeof(test_contents), sz+1);
    EXPECT_STREQ(s.c_str(), test_contents);
}

} // namespace fs
} // namespace c4

#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#elif defined(__GNUC__)
#endif
