#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// These utilities are implemented using POSIX.
// But most used features are availble in MinGW
// and hence code should compile in windows environment.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glibmm.h>
#ifndef WIN32
#include <poll.h>
#include <sys/mman.h>
#endif

#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/User.h>
#include <arc/GUID.h>
#include <arc/FileAccess.h>

#include "FileUtils.h"

namespace Arc {

Glib::Mutex suid_lock;

static bool write_all(int h,const void* buf,size_t l) {
  for(;l>0;) {
    ssize_t ll = ::write(h,buf,l);
    if(ll == -1) {
      if(errno == EINTR) continue;
      return false;
    };
    buf = (const void*)(((const char*)buf)+ll);
    l-=ll;
  }
  return true;
}

static bool write_all(FileAccess& fa,const void* buf,size_t l) {
  for(;l>0;) {
    ssize_t ll = fa.write(buf,l);
    if(ll == -1) {
      if(fa.geterrno() == EINTR) continue;
      return false;
    };
    buf = (const void*)(((const char*)buf)+ll);
    l-=ll;
  }
  return true;
}

bool FileCopy(const std::string& source_path,const std::string& destination_path,uid_t uid,gid_t gid) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) return false;
    bool r = fa.copy(source_path,destination_path,S_IRUSR|S_IWUSR);
    errno = fa.geterrno();
    return r;
  };
  return FileCopy(source_path,destination_path);
}

bool FileCopy(const std::string& source_path,const std::string& destination_path) {
  struct stat st;
  int source_handle = ::open(source_path.c_str(),O_RDONLY,0);
  if(source_handle == -1) return false;
  if(::fstat(source_handle,&st) != 0) return false;
  int destination_handle = ::open(destination_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,st.st_mode);
  if(destination_handle == -1) {
    ::close(source_handle);
    return false;
  }
  bool r = FileCopy(source_handle,destination_handle);
  ::close(source_handle);
  ::close(destination_handle);
  return r;
}

bool FileCopy(const std::string& source_path,int destination_handle) {
  int source_handle = ::open(source_path.c_str(),O_RDONLY,0);
  if(source_handle == -1) return false;
  if(::ftruncate(destination_handle,0) != 0) {
    ::close(source_handle);
    return false;
  }
  bool r = FileCopy(source_handle,destination_handle);
  ::close(source_handle);
  return r;
}

bool FileCopy(int source_handle,const std::string& destination_path) {
  int destination_handle = ::open(destination_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0600);
  if(destination_handle == -1) return false;
  bool r = FileCopy(source_handle,destination_handle);
  ::close(destination_handle);
  return r;
}

#define FileCopyBigThreshold (50*1024*1024)
#define FileCopyBufSize (4*1024)

bool FileCopy(int source_handle,int destination_handle) {
  off_t source_size = lseek(source_handle,0,SEEK_END);
  if(source_size == (off_t)(-1)) return false;
  if(source_size == 0) return true;
#ifndef WIN32
  if(source_size <= FileCopyBigThreshold) {
    void* source_addr = mmap(NULL,source_size,PROT_READ,MAP_SHARED,source_handle,0);
    if(source_addr != (void *)(-1)) {
      bool r = write_all(destination_handle,source_addr,source_size);
      munmap(source_addr,source_size);
      return r;
    }
  }
#endif
  if(lseek(source_handle,0,SEEK_SET) != 0) return false;
  char* buf = new char[FileCopyBufSize];
  if(!buf) return false;
  bool r = true;
  for(;;) {
    ssize_t l = FileCopyBufSize;
    l=::read(source_handle,buf,l);
    if(l == 0) break; // less than expected
    if(l == -1) {
      if(errno == EINTR) continue;
      // EWOULDBLOCK
      r = false;
      break;
    }
    if(!write_all(destination_handle,buf,l)) {
      r = false;
      break;
    }
  }
  return r;
}

bool FileRead(const std::string& filename, std::list<std::string>& data, uid_t uid, gid_t gid) {
  data.clear();
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) {
      errno = fa.geterrno();
      return false;
    };
    if(!fa.open(filename,O_RDONLY,0)) {
      errno = fa.geterrno();
      return false;
    };
    char buf[1024];
    std::string line;
    for(;;) {
      ssize_t l = fa.read(buf,sizeof(buf)-1);
      if(l <= 0) break;
      buf[l] = 0; line += buf;
      for(;;) {
        std::string::size_type p = line.find('\r');
        data.push_back(line.substr(0,p));
        line.erase(0,p+1);
      }
    }
    fa.close();
    if(!line.empty()) data.push_back(line);
    return true;
  }
  std::ifstream is(filename.c_str());
  if (!is.good()) return false;
  std::string line;
  while (std::getline(is, line)) {
    data.push_back(line);
  }
  return true;
}

bool FileRead(const std::string& filename, std::string& data, uid_t uid, gid_t gid) {
  data.clear();
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) {
      errno = fa.geterrno();
      return false;
    };
    if(!fa.open(filename,O_RDONLY,0)) {
      errno = fa.geterrno();
      return false;
    };
    char buf[1024];
    for(;;) {
      ssize_t l = fa.read(buf,sizeof(buf));
      if(l <= 0) break;
      data += std::string(buf,l);
    }
    fa.close();
    return true;
  }
  int h = ::open(filename.c_str(),O_RDONLY);
  if(h == -1) return false;
  char buf[1024];
  for(;;) {
    ssize_t l = ::read(h,buf,sizeof(buf));
    if(l <= 0) break;
    data += std::string(buf,l);
  }
  ::close(h);
  return true;
}

bool FileCreate(const std::string& filename, const std::string& data, uid_t uid, gid_t gid, mode_t mode) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    // If somebody bother about changing uid/gid then probably safer mode is needed
    if(mode == 0) mode = S_IRUSR|S_IWUSR;
    if(!fa.setuid(uid,gid)) { errno = fa.geterrno(); return false; }
    if((!fa.remove(filename)) && (fa.geterrno() != ENOENT)) { errno = fa.geterrno(); return false; }
    if(!fa.open(filename,O_WRONLY|O_CREAT|O_EXCL,mode)) { errno = fa.geterrno(); return false; }
    if(!write_all(fa,data.c_str(),data.length())) { errno = fa.geterrno(); fa.close(); return false; }
    fa.close();
    return true;
  }
#ifndef WIN32
  if(mode == 0) mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
#else
  if(mode == 0) mode = S_IRUSR|S_IWUSR;
#endif
  if (remove(filename.c_str()) != 0 && errno != ENOENT) return false;
  int h = ::open(filename.c_str(),O_WRONLY|O_CREAT|O_EXCL,mode);
  if(h == -1) return false;
  if(!write_all(h,data.c_str(),data.length())) { ::close(h); return false; }
  ::close(h);
  return true;
}

// TODO: maybe by using open + fstat it would be possible to 
// make this functin less blocking
bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) { errno = fa.geterrno(); return false; }
    if(follow_symlinks) {
      if(!fa.stat(path,*st)) { errno = fa.geterrno(); return false; }
    } else {
      if(!fa.lstat(path,*st)) { errno = fa.geterrno(); return false; }
    }
    return true;
  };
  return FileStat(path,st,follow_symlinks);
}

bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks) {
  int r = -1;
  {
    if(follow_symlinks) {
      r = ::stat(path.c_str(),st);
    } else {
      r = ::lstat(path.c_str(),st);
    };
  };
  return (r == 0);
}

bool FileLink(const std::string& oldpath,const std::string& newpath,bool symbolic) {
  if(symbolic) {
    return (symlink(oldpath.c_str(),newpath.c_str()) == 0);
  } else {
    return (link(oldpath.c_str(),newpath.c_str()) == 0);
  }
}

bool FileLink(const std::string& oldpath,const std::string& newpath,uid_t uid,gid_t gid,bool symbolic) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) return false;
    if(symbolic) {
      if(!fa.softlink(oldpath,newpath)) { errno = fa.geterrno(); return false; }
    } else {
      if(!fa.link(oldpath,newpath)) { errno = fa.geterrno(); return false; }
    }
    return true;
  }
  return FileLink(oldpath,newpath,symbolic);
}

std::string FileReadLink(const std::string& path,uid_t uid,gid_t gid) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) return "";
    std::string linkpath;
    fa.readlink(path,linkpath);
    errno = fa.geterrno();
    return linkpath;
  }
  return FileReadLink(path);
}

std::string FileReadLink(const std::string& path) {
  class charbuf {
   private:
    char* v;
   public:
    charbuf(int size) {
      v = new char[size];
    };
    ~charbuf(void) {
      delete[] v;
    };
    char* str(void) {
      return v;
    };
    char& operator[](int n) {
      return v[n];
    };
  };
  const int bufsize = 1024;
  charbuf buf(bufsize);
  ssize_t l = readlink(path.c_str(),buf.str(),bufsize);
  if(l<0) {
    l = 0;
  } else if(l>bufsize) {
    l = bufsize; 
  }
  return std::string(buf.str(),l);
}

bool FileDelete(const std::string& path) {
  return (unlink(path.c_str()) == 0);
}

bool FileDelete(const std::string& path,uid_t uid,gid_t gid) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) { errno = fa.geterrno(); return false; }
    if(!fa.unlink(path)) { errno = fa.geterrno(); return false; }
    return true;
  };
  return FileDelete(path);
}

// TODO: find non-blocking way to create directory
bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    bool created = false;
    bool exists = false;
    FileAccess fa;
    if(!fa.setuid(uid,gid)) return false;
    if(with_parents) {
      created = fa.mkdirp(path,mode);
    } else {
      created = fa.mkdir(path,mode);
    }
    int err = fa.geterrno();
    exists = created;
    if(!created) {
      // Normally following should be done only if errno is EEXIST.
      // But some file systems may return different error even if 
      // directory exists. Lustre for example returns EACCESS
      // if user can't create directory even if directory already exists.
      // So doing stat in order to check if directory exists and that it 
      // is directory. That still does not solve problem with parent
      // directory without x access right.
      struct stat st;
      if(fa.stat(path,st)) {
        if(S_ISDIR(st.st_mode)) {
          exists = true;
        } else {
          err = EEXIST; // exists, but not a directory
        }
      }
    }
    if(!exists) {
      // Nothing we can do - escape
      errno = err;
      return false;
    }
    if(!created) {
      // Directory was created by another actor. 
      // There is no sense to apply permissions in that case.
      errno = EEXIST;
      return true;
    }
    if(fa.chmod(path,mode)) return true;
    errno = fa.geterrno();
    return false;
  }
  return DirCreate(path,mode,with_parents);
}

bool DirCreate(const std::string& path,mode_t mode,bool with_parents) {

  if(::mkdir(path.c_str(),mode) == 0) {
    if (::chmod(path.c_str(), mode) == 0) return true;
    else return false;
  }

  if(errno == ENOENT) {
    if(with_parents) {
      std::string ppath(path);
      if(!Glib::path_is_absolute(ppath)) {
        ppath=Glib::get_current_dir()+G_DIR_SEPARATOR_S+ppath;
      }
      std::string::size_type pos = ppath.rfind(G_DIR_SEPARATOR_S);
      if((pos != 0) && (pos != std::string::npos)) {
        ppath.resize(pos);
        if(!DirCreate(ppath,mode,true)) return false;
        if((::mkdir(path.c_str(),mode) == 0) && (chmod(path.c_str(), mode) == 0)) return true;
      }
    }
  }
  // Look above in previous DirCreate() for description of following
  struct stat st;
  if(::stat(path.c_str(),&st) != 0) return false;
  if(!S_ISDIR(st.st_mode)) return false;
  errno = EEXIST;
  return true;
}

bool DirDelete(const std::string& path,uid_t uid,gid_t gid) {
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) { errno = fa.geterrno(); return false; }
    if(!fa.rmdirr(path)) { errno = fa.geterrno(); return false; }
    return true;
  }
  return DirDelete(path);
}  

bool DirDelete(const std::string& path) {

  struct stat st;
  if (::stat(path.c_str(), &st) != 0 || ! S_ISDIR(st.st_mode))
    return false;
  try {
    Glib::Dir dir(path);
    std::string file_name;
    while ((file_name = dir.read_name()) != "") {
      std::string fullpath(path);
      fullpath += G_DIR_SEPARATOR_S + file_name;
      if (::lstat(fullpath.c_str(), &st) != 0) return false;
      if (S_ISDIR(st.st_mode)) {
        if (!DirDelete(fullpath.c_str())) {
          return false;
        }
      } else {
        if (::remove(fullpath.c_str()) != 0) {
          return false;
        }
      }
    } 
  }
  catch (Glib::FileError& e) {
    return false;
  }
  if (rmdir(path.c_str()) != 0) return false;
      
  return true;
}

bool TmpDirCreate(std::string& path) {
  std::string tmpdir(Glib::get_tmp_dir());
  bool result = false;
#ifdef HAVE_MKDTEMP
  char tmptemplate[] = "ARC-XXXXXX";
  tmpdir = Glib::build_filename(tmpdir, tmptemplate);
  char* tmpdirchar = mkdtemp(const_cast<char*>(tmpdir.c_str()));
  if (tmpdirchar) {
    path = tmpdirchar;
    result = true;
  }
#else
  // NOTE: not safe!
  std::string tmp("ARC-" + UUID());
  path = Glib::build_filename(tmpdir, tmp);
  result = DirCreate(path, 0700, true);
#endif
  return result;
}

bool TmpFileCreate(std::string& filename, const std::string& data, uid_t uid, gid_t gid, mode_t mode) {
  std::string tmpdir(Glib::get_tmp_dir());
  char tmptemplate[] = "ARC-XXXXXX";
  filename = Glib::build_filename(tmpdir, tmptemplate);
  if(mode == 0) mode = S_IRUSR|S_IWUSR;
  if((uid && (uid != getuid())) || (gid && (gid != getgid()))) {
    FileAccess fa;
    if(!fa.setuid(uid,gid)) { errno = fa.geterrno(); return false; }
    if(!fa.mkstemp(filename,mode)) { errno = fa.geterrno(); return false; }
    if(!write_all(fa,data.c_str(),data.length())) { errno = fa.geterrno(); fa.close(); return false; }
    return true;
  }
  int h = Glib::mkstemp(filename);
  if(h == -1) return false;
  if (::chmod(filename.c_str(), mode) != 0) {
    ::close(h);
    return false;
  }
  if(!write_all(h,data.c_str(),data.length())) {
    ::close(h);
    return false;
  };
  ::close(h);
  return true;
}


bool CanonicalDir(std::string& name, bool leading_slash) {
  std::string::size_type i,ii,n;
  char separator = G_DIR_SEPARATOR_S[0];
  ii=0; i=0;
  if(name[0] != separator) name=separator+name;
  for(;i<name.length();) {
    name[ii]=name[i];
    if(name[i] == separator) {
      n=i+1;
      if(n >= name.length()) {
        n=i; break;
      }
      else if(name[n] == '.') {
        n++;
        if(name[n] == '.') {
          n++;
          if((n >= name.length()) || (name[n] == separator)) {
            i=n;
            /* go 1 directory up */
            for(;;) {
              if(ii<=0) {
                 /* bad dir */
                return false;
              }
              ii--;
              if(name[ii] == separator) break;
            }
            ii--; i--;
          }
        }
        else if((n >= name.length()) || (name[n] == separator)) {
          i=n;
          ii--; i--;
        }
      }
      else if(name[n] == separator) {
        i=n;
        ii--; i--;
      }
    }
    n=i; i++; ii++;
  }
  if(leading_slash) {
    if((name[0] != separator) || (ii==0)) name=separator+name.substr(0,ii);
    else name=name.substr(0,ii);
  }
  else {
    if((name[0] != separator) || (ii==0)) name=name.substr(0,ii);
    else name=name.substr(1,ii-1);
  }
  return true;
}

} // namespace Arc

