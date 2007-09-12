#ifndef __ARC_GM_JSDL_JOB_H__
#define __ARC_GM_JSDL_JOB_H__
#include <iostream>
#include <stdsoap2.h>

#include "../../jobs/job.h"

class jsdl__JobDefinition_USCOREType;
class jsdlPOSIX__POSIXApplication_USCOREType;
class JobUser;
class JobLocalDescription;
class FileData;

class JSDLJob {
 private:
  struct soap* sp_;
  jsdl__JobDefinition_USCOREType* job_;
  jsdlPOSIX__POSIXApplication_USCOREType* job_posix_;
  bool check(void);
  void set(std::istream& f);
  void set_posix(void);
  bool get_jobname(std::string& jobname);
  bool get_arguments(std::list<std::string>& arguments);
  bool get_execs(std::list<std::string>& execs);
  bool get_RTEs(std::list<std::string>& rtes);
  bool get_middlewares(std::list<std::string>& mws);
  bool get_acl(std::string& acl);
  bool get_gmlog(std::string& s);
  bool get_loggers(std::list<std::string>& urls);
  bool get_stdin(std::string& s);
  bool get_stdout(std::string& s);
  bool get_stderr(std::string& s);
  bool get_queue(std::string& s);
  bool get_notification(std::string& s);
  bool get_lifetime(int& l);
  bool get_fullaccess(bool& b);
  bool get_data(std::list<FileData>& inputdata,int& downloads,
                std::list<FileData>& outputdata,int& uploads);
  bool get_memory(int& memory);
  bool get_cputime(int& t);
  bool get_walltime(int& t);
  bool get_count(int& n);
  bool get_reruns(int& n);
  bool get_client_software(std::string& s);
  bool get_credentialserver(std::string& url);
  void print_to_grami(std::ostream &o);
 public:
  JSDLJob(void);
  JSDLJob(const char* str);
  JSDLJob(std::istream& f);
  JSDLJob(jsdl__JobDefinition_USCOREType* j);
  JSDLJob(const JSDLJob& j);
  ~JSDLJob(void);
  operator bool(void) { return ((job_!=NULL) && (job_posix_!=NULL)); };
  bool operator!(void) { return ((job_==NULL) || (job_posix_==NULL)); };
  bool parse(JobLocalDescription &job_desc,std::string* acl = NULL);
  bool set_execs(const std::string &session_dir);
  bool write_grami(const JobDescription &desc,const JobUser &user,const char *opt_add = NULL);
};

#endif
