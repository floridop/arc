// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32

#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>

#include <arc/Logger.h>

#include "FileCache.h"

namespace Arc {

  const std::string FileCache::CACHE_DATA_DIR = "data";
  const std::string FileCache::CACHE_JOB_DIR = "joblinks";
  const int FileCache::CACHE_DIR_LENGTH = 2;
  const int FileCache::CACHE_DIR_LEVELS = 1;
  const std::string FileCache::CACHE_LOCK_SUFFIX = ".lock";
  const std::string FileCache::CACHE_META_SUFFIX = ".meta";
  const int FileCache::CACHE_DEFAULT_AUTH_VALIDITY = 86400; // 24 h

  Logger FileCache::logger(Logger::getRootLogger(), "FileCache");

  FileCache::FileCache(std::string cache_path,
                       std::string id,
                       uid_t job_uid,
                       gid_t job_gid) {

    // make a vector of one item and call _init
    std::vector<std::string> caches;
    caches.push_back(cache_path);

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(std::vector<std::string> caches,
                       std::string id,
                       uid_t job_uid,
                       gid_t job_gid) {

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, id, job_uid, job_gid))
      _caches.clear();
  }

  bool FileCache::_init(std::vector<std::string> caches,
                        std::string id,
                        uid_t job_uid,
                        gid_t job_gid) {

    _id = id;
    _uid = job_uid;
    _gid = job_gid;

    // for each cache
    for (int i = 0; i < (int)caches.size(); i++) {
      std::string cache = caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No cache directory specified");
        return false;
      }
      std::string cache_link_path = "";
      if (cache.find(" ") != std::string::npos)
        cache_link_path = cache.substr(cache.find_last_of(" ") + 1, cache.length() - cache.find_last_of(" ") + 1);

      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length() - 1)
        cache_path = cache_path.substr(0, cache_path.length() - 1);
      if (cache_link_path.rfind("/") == cache_link_path.length() - 1)
        cache_link_path = cache_link_path.substr(0, cache_link_path.length() - 1);

      // create cache dir and subdirs
      if (!_cacheMkDir(cache_path + "/" + CACHE_DATA_DIR, true)) {
        logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path + "/" + CACHE_DATA_DIR);
        return false;
      }
      if (!_cacheMkDir(cache_path + "/" + CACHE_JOB_DIR, true)) {
        logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path + "/" + CACHE_JOB_DIR);
        return false;
      }
      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = cache_link_path;
      _caches.push_back(cache_params);
    }

    // our hostname and pid
    struct utsname buf;
    if (uname(&buf) != 0) {
      logger.msg(ERROR, "Cannot determine hostname from uname()");
      return false;
    }
    _hostname = buf.nodename;
    int pid_i = getpid();
    std::stringstream ss;
    ss << pid_i;
    ss >> _pid;
    return true;
  }


  FileCache::FileCache(const FileCache& cache) {

    // our hostname and pid
    struct utsname buf;
    if (uname(&buf) != 0) {
      logger.msg(ERROR, "Cannot determine hostname from uname()");
      return;
    }
    _hostname = buf.nodename;
    int pid_i = getpid();
    std::stringstream ss;
    ss << pid_i;
    ss >> _pid;

    _caches = cache._caches;
    _id = cache._id;
    _uid = cache._uid;
    _gid = cache._gid;
  }

  FileCache::~FileCache(void) {}

  bool FileCache::Start(std::string url, bool& available, bool& is_locked) {

    if (!(*this))
      return false;

    available = false;
    is_locked = false;
    std::string filename = File(url);
    std::string lock_file = _getLockFileName(url);

    // create directory structure if required, only readable by GM user
    if (!_cacheMkDir(lock_file.substr(0, lock_file.rfind("/")), false))
      return false;

    int lock_timeout = 86400; // one day timeout on lock TODO: make configurable?

    // locking mechanism:
    // - check if lock is there
    // - if not, create tmp file and check again
    // - if lock is still not there copy tmp file to cache lock file
    // - check pid inside lock file matches ours

    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == EACCES) {
        logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, strerror(errno));
        return false;
      }
      else if (errno != ENOENT) {
        // some other error occurred opening the lock file
        logger.msg(ERROR, "Error opening lock file %s in initial check: %s", lock_file, strerror(errno));
        return false;
      }
      // lock does not exist - create tmp file
      // ugly char manipulation to get mkstemp() to work...
      char tmpfile[256];
      tmpfile[0] = '\0';
      strcat(tmpfile, lock_file.c_str());
      strcat(tmpfile, ".XXXXXX");
      int h = mkstemp(tmpfile);
      if (h == -1) {
        logger.msg(ERROR, "Error creating file %s with mkstemp(): %s", tmpfile, strerror(errno));
        return false;
      }
      // write pid@hostname to the lock file
      char buf[_pid.length() + _hostname.length() + 2];
      sprintf(buf, "%s@%s", _pid.c_str(), _hostname.c_str());
      if (write(h, &buf, strlen(buf)) == -1) {
        logger.msg(ERROR, "Error writing to tmp lock file %s: %s", tmpfile, strerror(errno));
        close(h);
        // not much we can do if this doesn't work, but it is only a tmp file
        remove(tmpfile);
        return false;
      }
      if (close(h) != 0)
        // not critical as file will be removed after we are done
        logger.msg(WARNING, "Warning: closing tmp lock file %s failed", tmpfile);
      // check again if lock exists, in case creating the tmp file took some time
      err = stat(lock_file.c_str(), &fileStat);
      if (0 != err) {
        if (errno == ENOENT) {
          // ok, we can create lock
          if (rename(tmpfile, lock_file.c_str()) != 0) {
            logger.msg(ERROR, "Error renaming tmp file %s to lock file %s: %s", tmpfile, lock_file, strerror(errno));
            remove(tmpfile);
            return false;
          }
          // check it's really there
          err = stat(lock_file.c_str(), &fileStat);
          if (0 != err) {
            logger.msg(ERROR, "Error renaming lock file, even though rename() did not return an error");
            return false;
          }
          // check the pid inside the lock file, just in case...
          if (!_checkLock(url)) {
            is_locked = true;
            return false;
          }
        }
        else if (errno == EACCES) {
          logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, strerror(errno));
          return false;
        }
        else {
          // some other error occurred opening the lock file
          logger.msg(ERROR, "Error opening lock file we just renamed successfully %s: %s", lock_file, strerror(errno));
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "The file is currently locked with a valid lock");
        is_locked = true;
        return false;
      }
    }
    else {
      // the lock already exists, check if it has expired
      // look at modification time
      time_t mod_time = fileStat.st_mtime;
      time_t now = time(NULL);
      logger.msg(VERBOSE, "%li seconds since lock file was created", now - mod_time);

      if ((now - mod_time) > lock_timeout) {
        logger.msg(VERBOSE, "Timeout has expired, will remove lock file");
        // TODO: kill the process holding the lock, only if we know it was the original
        // process which created it
        if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
          return false;
        }
        // lock has expired and has been removed. Try to remove cache file and call Start() again
        if (remove(filename.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
          return false;
        }
        return Start(url, available, is_locked);
      }

      // lock is still valid, check if we own it
      FILE *pFile;
      char lock_info[100]; // should be long enough for a pid + hostname
      pFile = fopen((char*)lock_file.c_str(), "r");
      if (pFile == NULL) {
        // lock could have been released by another process, so call Start again
        if (errno == ENOENT) {
          logger.msg(VERBOSE, "Lock that recently existed has been deleted by another process, calling Start() again");
          return Start(url, available, is_locked);
        }
        logger.msg(ERROR, "Error opening valid and existing lock file %s: %s", lock_file, strerror(errno));
        return false;
      }
      if (fgets(lock_info, 100, pFile) == NULL) {
        logger.msg(ERROR, "Error reading valid and existing lock file %s: %s", lock_file, strerror(errno));
        fclose(pFile);
        return false;
      }
      fclose(pFile);

      std::string lock_info_s(lock_info);
      std::string::size_type index = lock_info_s.find("@", 0);
      if (index == std::string::npos) {
        logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
        return false;
      }

      if (lock_info_s.substr(index + 1) != _hostname) {
        logger.msg(VERBOSE, "Lock is owned by a different host");
        // TODO: here do ssh login and check
        is_locked = true;
        return false;
      }
      std::string lock_pid = lock_info_s.substr(0, index);
      if (lock_pid == _pid)
        // safer to wait until lock expires than use cached file or re-download
        logger.msg(WARNING, "Warning: This process already owns the lock");
      else {
        // check if the pid owning the lock is still running - if not we can claim the lock
        // this is not really portable... but no other way to do it
        std::string procdir("/proc/");
        procdir = procdir.append(lock_pid);
        if (stat(procdir.c_str(), &fileStat) != 0 && errno == ENOENT) {
          logger.msg(VERBOSE, "The process owning the lock is no longer running, will remove lock");
          if (remove(lock_file.c_str()) != 0) {
            logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
            return false;
          }
          // lock has been removed. try to delete cache file and call Start() again
          if (remove(filename.c_str()) != 0 && errno != ENOENT) {
            logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
            return false;
          }
          return Start(url, available, is_locked);
        }
      }

      logger.msg(VERBOSE, "The file is currently locked with a valid lock");
      is_locked = true;
      return false;
    }

    // if we get to here we have acquired the lock

    // create the meta file to store the URL, if it does not exist
    std::string meta_file = _getMetaFileName(url);
    err = stat(meta_file.c_str(), &fileStat);
    if (0 == err) {
      // check URL inside file for possible hash collisions
      FILE *pFile;
      char mystring[1024]; // should be long enough for a pid or url...
      pFile = fopen((char*)_getMetaFileName(url).c_str(), "r");
      if (pFile == NULL) {
        logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
        remove(lock_file.c_str());
        return false;
      }
      if (fgets(mystring, sizeof(mystring), pFile) == NULL) {
        logger.msg(ERROR, "Error reading valid and existing lock file %s: %s", lock_file, strerror(errno));
        fclose(pFile);
        return false;
      }
      fclose(pFile);

      std::string meta_str(mystring);
      // get the first line
      if (meta_str.find('\n') != std::string::npos)
        meta_str.resize(meta_str.find('\n'));

      std::string::size_type space_pos = meta_str.find(' ', 0);
      if (meta_str.substr(0, space_pos) != url) {
        logger.msg(ERROR, "Error: File %s is already cached at %s under a different URL: %s - this file will not be cached", url, filename, meta_str.substr(0, space_pos));
        remove(lock_file.c_str());
        return false;
      }
    }
    else if (errno == ENOENT) {
      // create new file
      FILE *pFile;
      pFile = fopen((char*)meta_file.c_str(), "w");
      if (pFile == NULL) {
        logger.msg(ERROR, "Failed to create info file %s: %s", meta_file, strerror(errno));
        remove(lock_file.c_str());
        return false;
      }
      fputs((char*)url.c_str(), pFile);
      fputs("\n", pFile);
      fclose(pFile);
      // make read/writeable only by GM user
      chmod(meta_file.c_str(), S_IRUSR | S_IWUSR);
    }
    else {
      logger.msg(ERROR, "Error looking up attributes of meta file %s: %s", meta_file, strerror(errno));
      remove(lock_file.c_str());
      return false;
    }
    // now check if the cache file is there already
    err = stat(filename.c_str(), &fileStat);
    if (0 == err)
      available = true;
    else if (errno != ENOENT)
      // this is ok, we will download again
      logger.msg(WARNING, "Warning: error looking up attributes of cached file: %s", strerror(errno));
    return true;
  }

  bool FileCache::Stop(std::string url) {

    if (!(*this))
      return false;

    // check the lock is ok to delete
    if (!_checkLock(url))
      return false;

    // delete the lock
    if (remove(_getLockFileName(url).c_str()) != 0) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
      return false;
    }
    return true;
  }

  bool FileCache::StopAndDelete(std::string url) {

    if (!(*this))
      return false;

    // check the lock is ok to delete, and if so, remove the file and the
    // associated lock
    if (!_checkLock(url))
      return false;

    // delete the cache file
    if (remove(File(url).c_str()) != 0 && errno != ENOENT) {
      logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
      return false;
    }

    // delete the meta file - not critical so don't fail on error
    if (remove(_getMetaFileName(url).c_str()) != 0)
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));

    // delete the lock
    if (remove(_getLockFileName(url).c_str()) != 0) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
      return false;
    }
    return true;
  }

  std::string FileCache::File(std::string url) {

    if (!(*this))
      return "";

    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);

    int index = 0;
    for (int level = 0; level < CACHE_DIR_LEVELS; level++) {
      hash.insert(index + CACHE_DIR_LENGTH, "/");
      // go to next slash position, add one since we just inserted a slash
      index += CACHE_DIR_LENGTH + 1;
    }
    return _caches.at(_chooseCache(hash)).cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
  }

  bool FileCache::Link(std::string link_path, std::string url) {

    if (!(*this))
      return false;

    // choose cache
    struct CacheParameters cache_params = _caches.at(_chooseCache(FileCacheHash::getHash(url)));

    // if _cache_link_path is '.' then copy instead, bypassing the hard-link
    if (cache_params.cache_link_path == ".")
      return Copy(link_path, url);

    // check the original file exists
    struct stat fileStat;
    if (stat(File(url).c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", File(url));
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", File(url), strerror(errno));
      return false;
    }

    std::string hard_link_path = cache_params.cache_path + "/" + CACHE_JOB_DIR + "/" + _id;

    // create per-job hard link dir if necessary, making the final dir readable only by the job user
    if (!_cacheMkDir(hard_link_path, true)) {
      logger.msg(ERROR, "Cannot create directory \"%s\" for per-job hard links", hard_link_path);
      return false;
    }
    if (chown(hard_link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Cannot change owner of %s", hard_link_path);
      return false;
    }
    if (chmod(hard_link_path.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Cannot change permissions of \"%s\" to 0700", hard_link_path);
      return false;
    }

    std::string filename = link_path.substr(link_path.rfind("/") + 1);
    std::string hard_link_file = hard_link_path + "/" + filename;
    std::string session_dir = link_path.substr(0, link_path.rfind("/"));

    // make the hard link
    if (link(File(url).c_str(), hard_link_file.c_str()) != 0) {
      logger.msg(ERROR, "Failed to create hard link from %s to %s: %s", hard_link_file, File(url), strerror(errno));
      return false;
    }
    // ensure the hard link is readable by all and owned by root (or GM user)
    // (to make cache file immutable but readable by all)
    if (chown(hard_link_file.c_str(), getuid(), getgid()) != 0) {
      logger.msg(ERROR, "Failed to change owner of hard link to %i: %s", getuid(), strerror(errno));
      return false;
    }
    if (chmod(hard_link_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
      logger.msg(ERROR, "Failed to change permissions of hard link to 0644: %s", strerror(errno));
      return false;
    }

    // make necessary dirs for the soft link
    // this probably should have already been done... somewhere...
    if (!_cacheMkDir(session_dir, true))
      return false;
    if (chown(session_dir.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of session dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    if (chmod(session_dir.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
      return false;
    }

    // make the soft link, changing the target if cache_link_path is defined
    if (!cache_params.cache_link_path.empty())
      hard_link_file = cache_params.cache_link_path + "/" + CACHE_JOB_DIR + "/" + _id + "/" + filename;
    if (symlink(hard_link_file.c_str(), link_path.c_str()) != 0) {
      logger.msg(ERROR, "Failed to create soft link: %s", strerror(errno));
      return false;
    }

    // change the owner of the soft link to the job user
    if (lchown(link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of session dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    return true;
  }

  bool FileCache::Copy(std::string dest_path, std::string url, bool executable) {

    if (!(*this))
      return false;

    // check the original file exists
    std::string cache_file = File(url);
    struct stat fileStat;
    if (stat(cache_file.c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
      return false;
    }

    // make necessary dirs for the copy
    // this probably should have already been done... somewhere...
    std::string dest_dir = dest_path.substr(0, dest_path.rfind("/"));
    if (!_cacheMkDir(dest_dir, true))
      return false;
    if (chown(dest_dir.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of destination dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    if (chmod(dest_dir.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
      return false;
    }

    // do the copy - taken directly from old datacache.cc
    char buf[65536];
    mode_t perm = S_IRUSR | S_IWUSR;
    if (executable)
      perm |= S_IXUSR;
    int fdest = open(dest_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, perm);
    if (fdest == -1) {
      logger.msg(ERROR, "Failed to create file %s for writing: %s", dest_path, strerror(errno));
      return false;
    }
    if (fchown(fdest, _uid, _gid) == -1) {
      logger.msg(ERROR, "Failed change ownership of destination file %s: %s", dest_path, strerror(errno));
      close(fdest);
      return false;
    }

    int fsource = open(cache_file.c_str(), O_RDONLY);
    if (fsource == -1) {
      close(fdest);
      logger.msg(ERROR, "Failed to open file %s for reading: %s", cache_file, strerror(errno));
      return false;
    }

    // source and dest opened ok - copy in chunks
    for (;;) {
      ssize_t lin = read(fsource, buf, sizeof(buf));
      if (lin == -1) {
        close(fdest);
        close(fsource);
        logger.msg(ERROR, "Failed to read file %s: %s", cache_file, strerror(errno));
        return false;
      }
      if (lin == 0)
        break;          // eof

      for (ssize_t lout = 0; lout < lin;) {
        ssize_t lwritten = write(fdest, buf + lout, lin - lout);
        if (lwritten == -1) {
          close(fdest);
          close(fsource);
          logger.msg(ERROR, "Failed to write file %s: %s", dest_path, strerror(errno));
          return false;
        }
        lout += lwritten;
      }
    }
    close(fdest);
    close(fsource);
    return true;
  }

  bool FileCache::Release() {

    // go through all caches and remove per-job dirs for our job id
    for (int i = 0; i < (int)_caches.size(); i++) {

      std::string job_dir = _caches.at(i).cache_path + "/" + CACHE_JOB_DIR + "/" + _id;

      // check if job dir exists
      DIR *dirp = opendir(job_dir.c_str());
      if (dirp == NULL) {
        if (errno == ENOENT)
          continue;
        logger.msg(ERROR, "Error opening per-job dir %s: %s", job_dir, strerror(errno));
        return false;
      }

      // list all files in the dir and delete them
      struct dirent *dp;
      errno = 0;
      while ((dp = readdir(dirp))) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
          continue;
        std::string to_delete = job_dir + "/" + dp->d_name;
        logger.msg(VERBOSE, "Removing %s", to_delete);
        if (remove(to_delete.c_str()) != 0) {
          logger.msg(ERROR, "Failed to remove hard link %s: %s", to_delete, strerror(errno));
          closedir(dirp);
          return false;
        }
      }
      closedir(dirp);

      // remove now-empty dir
      logger.msg(VERBOSE, "Removing %s", job_dir);
      if (rmdir(job_dir.c_str()) != 0) {
        logger.msg(ERROR, "Failed to remove cache per-job dir %s: %s", job_dir, strerror(errno));
        return false;
      }
    }
    return true;
  }

  bool FileCache::AddDN(std::string url, std::string DN, Time expiry_time) {

    if (DN.empty())
      return false;
    if (expiry_time == Time(0))
      expiry_time = Time(time(NULL) + CACHE_DEFAULT_AUTH_VALIDITY);

    // add DN to the meta file. If already there, renew the expiry time
    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    int err = stat(meta_file.c_str(), &fileStat);
    if (0 != err) {
      logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    FILE *pFile;
    char mystring[fileStat.st_size + 1];
    pFile = fopen(meta_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    // get the first line
    fgets(mystring, sizeof(mystring), pFile);

    // check for correct formatting and possible hash collisions between URLs
    std::string first_line(mystring);
    if (first_line.find('\n') == std::string::npos)
      first_line += '\n';
    std::string::size_type space_pos = first_line.rfind(' ');
    if (space_pos == std::string::npos)
      space_pos = first_line.length() - 1;

    if (first_line.substr(0, space_pos) != url) {
      logger.msg(ERROR, "Error: File %s is already cached at %s under a different URL: %s - will not add DN to cached list", url, File(url), first_line.substr(0, space_pos));
      fclose(pFile);
      return false;
    }

    // read in list of DNs
    std::vector<std::string> dnlist;
    dnlist.push_back(DN + ' ' + expiry_time.str(MDSTime) + '\n');

    char *res = fgets(mystring, sizeof(mystring), pFile);
    while (res) {
      std::string dnstring(mystring);
      space_pos = dnstring.rfind(' ');
      if (space_pos == std::string::npos) {
        logger.msg(WARNING, "Bad format detected in file %s, in line %s", meta_file, dnstring);
        continue;
      }
      // remove expired DNs (after some grace period)
      if (dnstring.substr(0, space_pos) != DN) {
        if (dnstring.find('\n') != std::string::npos)
          dnstring.resize(dnstring.find('\n'));
        Time exp_time(dnstring.substr(space_pos + 1));
        if (exp_time > Time(time(NULL) - CACHE_DEFAULT_AUTH_VALIDITY))
          dnlist.push_back(dnstring + '\n');
      }
      res = fgets(mystring, sizeof(mystring), pFile);
    }
    fclose(pFile);

    // write everything back to the file
    pFile = fopen(meta_file.c_str(), "w");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file for writing %s: %s", meta_file, strerror(errno));
      return false;
    }
    fputs((char*)first_line.c_str(), pFile);
    for (std::vector<std::string>::iterator i = dnlist.begin(); i != dnlist.end(); i++)
      fputs((char*)i->c_str(), pFile);
    fclose(pFile);
    return true;
  }

  bool FileCache::CheckDN(std::string url, std::string DN) {

    if (DN.empty())
      return false;

    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    int err = stat(meta_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno != ENOENT)
        logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    FILE *pFile;
    char mystring[fileStat.st_size + 1];
    pFile = fopen(meta_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    fgets(mystring, sizeof(mystring), pFile); // first line

    // read in list of DNs
    char *res = fgets(mystring, sizeof(mystring), pFile);
    while (res) {
      std::string dnstring(mystring);
      std::string::size_type space_pos = dnstring.rfind(' ');
      if (dnstring.substr(0, space_pos) == DN) {
        if (dnstring.find('\n') != std::string::npos)
          dnstring.resize(dnstring.find('\n'));
        std::string exp_time = dnstring.substr(space_pos + 1);
        if (Time(exp_time) > Time()) {
          logger.msg(VERBOSE, "DN %s is cached and is valid until %s for URL %s", DN, Time(exp_time).str(), url);
          fclose(pFile);
          return true;
        }
        else {
          logger.msg(VERBOSE, "DN %s is cached but has expired for URL %s", DN, url);
          fclose(pFile);
          return false;
        }
      }
      res = fgets(mystring, sizeof(mystring), pFile);
    }
    fclose(pFile);
    return false;
  }

  bool FileCache::CheckCreated(std::string url) {

    // check the cache file exists - if so we can get the creation date
    std::string cache_file = File(url);
    struct stat fileStat;
    return (stat(cache_file.c_str(), &fileStat) == 0) ? true : false;
  }

  Time FileCache::GetCreated(std::string url) {

    // check the cache file exists
    std::string cache_file = File(url);
    struct stat fileStat;
    if (stat(cache_file.c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
      return 0;
    }

    time_t ctime = fileStat.st_ctime;
    if (ctime <= 0)
      return Time(0);
    return Time(ctime);
  }

  bool FileCache::CheckValid(std::string url) {
    return (GetValid(url) != Time(0));
  }

  Time FileCache::GetValid(std::string url) {

    // open meta file and pick out expiry time if it exists

    FILE *pFile;
    char mystring[1024]; // should be long enough for a pid or url...
    pFile = fopen((char*)_getMetaFileName(url).c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
      return Time(0);
    }
    if (fgets(mystring, sizeof(mystring), pFile) == NULL) {
      logger.msg(ERROR, "Error reading meta file %s: %s", _getMetaFileName(url), strerror(errno));
      fclose(pFile);
      return Time(0);
    }
    fclose(pFile);

    std::string meta_str(mystring);
    // get the first line
    if (meta_str.find('\n') != std::string::npos)
      meta_str.resize(meta_str.find('\n'));

    // if the file contains only the url, we don't have an expiry time
    if (meta_str == url)
      return Time(0);

    // check sensible formatting - should be like "rls://rls1.ndgf.org/file1 20080101123456Z"
    if (meta_str.substr(0, url.length() + 1) != url + " ") {
      logger.msg(ERROR, "Mismatching url in file %s: %s Expected %s", _getMetaFileName(url), meta_str, url);
      return Time(0);
    }
    if (meta_str.length() != url.length() + 16) {
      logger.msg(ERROR, "Bad format in file %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }
    if (meta_str.substr(url.length(), 1) != " ") {
      logger.msg(ERROR, "Bad separator in file %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }
    if (meta_str.substr(url.length() + 1).length() != 15) {
      logger.msg(ERROR, "Bad value of expiry time in %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }

    // convert to Time object
    return Time(meta_str.substr(url.length() + 1));
    /*
       int exp_time;
       if(EOF == sscanf(meta_str.substr(url.length() + 1).c_str(), "%i", &exp_time) || exp_time < 0) {
       logger.msg(ERROR, "Error with converting time in file %s: %s", _getMetaFileName(url), meta_str);
       return 0;
       }
       return (time_t)exp_time;*/
  }

  bool FileCache::SetValid(std::string url, Time val) {

    std::string meta_file = _getMetaFileName(url);
    FILE *pFile;
    pFile = fopen((char*)meta_file.c_str(), "w");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    std::string file_data = url + " " + val.str(MDSTime);
    fputs((char*)file_data.c_str(), pFile);
    fclose(pFile);
    return true;
  }

  bool FileCache::operator==(const FileCache& a) {
    if (a._caches.size() != _caches.size())
      return false;
    for (int i = 0; i < (int)a._caches.size(); i++) {
      if (a._caches.at(i).cache_path != _caches.at(i).cache_path)
        return false;
      if (a._caches.at(i).cache_link_path != _caches.at(i).cache_link_path)
        return false;
    }
    return (
             a._id == _id &&
             a._uid == _uid &&
             a._gid == _gid
             );
  }
  bool FileCache::_checkLock(std::string url) {

    std::string filename = File(url);
    std::string lock_file = _getLockFileName(url);

    // check for existence of lock file
    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Lock file %s doesn't exist", lock_file);
      else
        logger.msg(ERROR, "Error listing lock file %s: %s", lock_file, strerror(errno));
      return false;
    }

    // check the lock file's pid and hostname matches ours
    FILE *pFile;
    char lock_info[100]; // should be long enough for a pid + hostname
    pFile = fopen((char*)lock_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening lock file %s: %s", lock_file, strerror(errno));
      return false;
    }
    if (fgets(lock_info, 100, pFile) == NULL) {
      logger.msg(ERROR, "Error reading lock file %s: %s", lock_file, strerror(errno));
      fclose(pFile);
      return false;
    }
    fclose(pFile);

    std::string lock_info_s(lock_info);
    std::string::size_type index = lock_info_s.find("@", 0);
    if (index == std::string::npos) {
      logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
      return false;
    }

    if (lock_info_s.substr(index + 1) != _hostname) {
      logger.msg(VERBOSE, "Lock is owned by a different host");
      // TODO: here do ssh login and check
      return false;
    }
    if (lock_info_s.substr(0, index) != _pid) {
      logger.msg(ERROR, "Another process owns the lock on file %s. Must go back to Start()", filename);
      return false;
    }
    return true;
  }

  std::string FileCache::_getLockFileName(std::string url) {
    return File(url) + CACHE_LOCK_SUFFIX;
  }

  std::string FileCache::_getMetaFileName(std::string url) {
    return File(url) + CACHE_META_SUFFIX;
  }

  bool FileCache::_cacheMkDir(std::string dir, bool all_read) {

    struct stat fileStat;
    int err = stat(dir.c_str(), &fileStat);
    if (0 != err) {
      logger.msg(VERBOSE, "Creating directory %s", dir);
      std::string::size_type slashpos = 0;
      do {
        slashpos = dir.find("/", slashpos + 1);
        std::string dirname = dir.substr(0, slashpos);
        // list dir to see if it exists (we can't tell the difference between
        // dir already exists and permission denied)
        struct stat statbuf;
        if (stat(dirname.c_str(), &statbuf) == 0)
          continue;

        // set perms based on all_read
        mode_t perm = S_IRWXU;
        if (all_read)
          perm |= S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
        if (mkdir(dirname.c_str(), perm) != 0)
          if (errno != EEXIST) {
            logger.msg(ERROR, "Error creating required dirs: %s", strerror(errno));
            return false;
          }
        // chmod to get around GM umask setting
        if (chmod(dirname.c_str(), perm) != 0) {
          logger.msg(ERROR, "Error changing permission of dir %s: %s", dirname, strerror(errno));
          return false;
        }
      } while (slashpos != std::string::npos);
    }
    return true;
  }

  int FileCache::_chooseCache(std::string hash) {
    // choose which cache to use
    // divide based on the first character of the hash, choosing the cache
    // based on the letter mod number of caches
    // this algorithm limits the number of caches to 16

    char firstletter = hash.at(0);
    int cacheno = firstletter % _caches.size();
    return cacheno;
  }

} // namespace Arc

#endif /*WIN32*/
