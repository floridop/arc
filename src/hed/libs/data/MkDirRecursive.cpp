#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "MkDirRecursive.h"

static int mkdir_force(const char* path,mode_t mode);

int mkdir_recursive(const std::string& base_path,const std::string& path,mode_t mode,uid_t uid,gid_t gid) {
  std::string name = base_path;
  if(path[0]!='/')
    name += '/';
  name += path;
  std::string::size_type name_start = base_path.length();
  std::string::size_type name_end   = name.length();
  /* go down */
  for(;;) {
    if((mkdir_force(name.substr(0,name_end).c_str(),mode) == 0) || (errno == EEXIST)) {
      if(errno != EEXIST) lchown(name.substr(0,name_end).c_str(),uid,gid);
      /* go up */
      for(;;) {
        if(name_end >= name.length()) { return 0; };
        name_end=name.find('/',name_end + 1);
        if(mkdir(name.substr(0,name_end).c_str(),mode) != 0) {
          if(errno == EEXIST) continue;
          return -1;
        };
        (void)chmod(name.substr(0,name_end).c_str(),mode);
        (void)lchown(name.substr(0,name_end).c_str(),uid,gid);
      };
    };
    /* if(errno == EEXIST) { free(name); errno=EEXIST; return -1; }; */
    if((name_end=name.rfind('/',name_end - 1)) == std::string::npos) break;
    if(name_end == name_start) break;
  };
  return -1;
}

int mkdir_force(const char* path,mode_t mode) {
  struct stat st;
  int r;
  if(stat(path,&st) != 0) {
    r=mkdir(path,mode);
    if(r==0) (void)chmod(path,mode);
    return r;
  };
  if(S_ISDIR(st.st_mode)) { /* simulate error */
    r=mkdir(path,mode);
    if(r==0) (void)chmod(path,mode);
    return r;
  };
  if(remove(path) != 0) return -1;
  r=mkdir(path,mode);
  if(r==0) (void)chmod(path,mode);
  return r;
}
