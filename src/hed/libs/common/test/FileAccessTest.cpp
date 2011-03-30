#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/FileUtils.h>
#include <arc/FileAccess.h>

class FileAccessTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileAccessTest);
  CPPUNIT_TEST(TestOpenWriteReadStat);
  CPPUNIT_TEST(TestCopy);
  CPPUNIT_TEST(TestDir);
  CPPUNIT_TEST(TestSeekAllocate);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestOpenWriteReadStat();
  void TestCopy();
  void TestDir();
  void TestSeekAllocate();

private:
  uid_t uid;
  gid_t gid;
  std::string testroot;
};


void FileAccessTest::setUp() {
  CPPUNIT_ASSERT(Arc::TmpDirCreate(testroot));
  CPPUNIT_ASSERT(!testroot.empty());
  if(getuid() == 0) {
    struct passwd* pwd = getpwnam("nobody");
    CPPUNIT_ASSERT(pwd);
    uid = pwd->pw_uid;
    gid = pwd->pw_gid;
    CPPUNIT_ASSERT_EQUAL(0,::chmod(testroot.c_str(),0777));
  } else {
    uid = getuid();
    gid = getgid();
  }
  Arc::FileAccess::testtune();
}

void FileAccessTest::tearDown() {
  Arc::DirDelete(testroot);
}

void FileAccessTest::TestOpenWriteReadStat() {
  Arc::FileAccess fa;
  std::string testfile = testroot+"/file1";
  std::string testdata = "test";
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(fa.open(testfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.close());
  struct stat st;
  CPPUNIT_ASSERT_EQUAL(0,::stat(testfile.c_str(),&st));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)st.st_size);
  CPPUNIT_ASSERT_EQUAL(uid,st.st_uid);
  CPPUNIT_ASSERT_EQUAL(gid,st.st_gid);
  CPPUNIT_ASSERT_EQUAL(0600,(int)(st.st_mode & 0777));
  CPPUNIT_ASSERT(fa.open(testfile,O_RDONLY,0));
  char buf[16];
  struct stat st2;
  CPPUNIT_ASSERT(fa.fstat(st2));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
  CPPUNIT_ASSERT(::memcmp(&st,&st2,sizeof(struct stat)) == 0);
  CPPUNIT_ASSERT(fa.stat(testfile,st2));
  CPPUNIT_ASSERT(::memcmp(&st,&st2,sizeof(struct stat)) == 0);
}

void FileAccessTest::TestCopy() {
  Arc::FileAccess fa;
  std::string testfile1 = testroot+"/copyfile1";
  std::string testfile2 = testroot+"/copyfile2";
  std::string testdata = "copytest";
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(fa.open(testfile1,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.close());
  CPPUNIT_ASSERT(fa.copy(testfile1,testfile2,0600));
  CPPUNIT_ASSERT(fa.open(testfile2,O_RDONLY,0));
  char buf[16];
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
}

void FileAccessTest::TestDir() {
  std::string testdir1 = testroot + "/dir1/dir2/dir3";
  std::string testdir2 = testroot + "/dir1/dir2/dir3/dir4";
  std::string testdir3 = testroot + "/dir1";
  Arc::FileAccess fa;
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(!fa.mkdir(testdir1,0700));
  CPPUNIT_ASSERT(fa.mkdirp(testdir1,0700));
  CPPUNIT_ASSERT(fa.mkdir(testdir2,0700));
  CPPUNIT_ASSERT(fa.opendir(testdir1));
  std::string name;
  while(true) {
    CPPUNIT_ASSERT(fa.readdir(name));
    if(name == ".") continue;
    if(name == "..") continue;
    break;
  }
  CPPUNIT_ASSERT(fa.closedir());
  CPPUNIT_ASSERT_EQUAL(testdir2.substr(testdir1.length()+1),name);
  CPPUNIT_ASSERT(!fa.rmdir(testdir3));
  CPPUNIT_ASSERT(fa.rmdir(testdir2));
  CPPUNIT_ASSERT(fa.rmdirr(testdir3));
}

void FileAccessTest::TestSeekAllocate() {
  Arc::FileAccess fa;
  std::string testfile = testroot+"/file3";
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(fa.open(testfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)4096,(int)fa.fallocate(4096));
  CPPUNIT_ASSERT_EQUAL((int)0,(int)fa.lseek(0,SEEK_SET));
  CPPUNIT_ASSERT_EQUAL((int)4096,(int)fa.lseek(0,SEEK_END));
  CPPUNIT_ASSERT(fa.close());
}

CPPUNIT_TEST_SUITE_REGISTRATION(FileAccessTest);
