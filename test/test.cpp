#include <c4/fs/fs.hpp>
#include <gtest/gtest.h>
#include <stdlib.h>

namespace c4 {
namespace fs {

struct ScopedTestFile
{
    char m_name[128];
    operator const char* () const { return m_name; }
    ScopedTestFile() : m_name()
    {
        tmpnam("scoped_file.XXXXXX.test", m_name, sizeof(m_name));
        ::FILE *f = fopen(m_name, "w");
        fclose(f);
    }
    ~ScopedTestFile()
    {
        delete_file(m_name);
    }
};

struct ScopedTestDir
{
    char m_name[128];
    operator const char* () const { return m_name; }
    ScopedTestDir() : m_name()
    {
        tmpnam("scoped_dir.XXXXXX.test", m_name, sizeof(m_name));
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

} // namespace fs
} // namespace c4
