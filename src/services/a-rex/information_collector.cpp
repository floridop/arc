#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <glibmm.h>

#include <arc/ArcLocation.h>
#include <arc/Run.h>
#include <arc/wsrf/WSResourceProperties.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/FileUtils.h>

#include "ldif/LDIFtoXML.h"
#include "grid-manager/files/ControlFileHandling.h"
#include "job.h"
#include "arex.h"

namespace ARex {

void ARexService::InformationCollector(void) {
  thread_count_.RegisterThread();
  for(;;) {
    // Run information provider
    std::string xml_str;
    int r = -1;
    {
      std::string cmd;
      cmd=Arc::ArcLocation::GetDataDir()+"/CEinfo.pl --splitjobs --config "+config_.ConfigFile();
      std::string stdin_str;
      std::string stderr_str;
      Arc::Run run(cmd);
      run.AssignStdin(stdin_str);
      run.AssignStdout(xml_str);
      run.AssignStderr(stderr_str);
      logger_.msg(Arc::DEBUG,"Resource information provider: %s",cmd);
      if(!run.Start()) {
        if(thread_count_.WaitOrCancel(infoprovider_wakeup_period_*100)) break;
        continue; // try again
      };
      int failedstat = 0;
      bool infoproviderhastimedout = false;
      while(!run.Wait(infoprovider_wakeup_period_)) {
        if (!infoproviderhastimedout) {
          logger_.msg(Arc::WARNING,"Resource information provider timed out: %u seconds. Using heartbeat file from now on... Consider increasing infoproviders_timeout in arc.conf",
                    infoprovider_wakeup_period_);
          infoproviderhastimedout = true;
        }
        logger_.msg(Arc::DEBUG,"Resource information provider timed out: %u seconds. Checking heartbeat file...",
                    infoprovider_wakeup_period_);
        /* check freshness of heartbeat file. If no infoprovider created a new file during the run, performance is not acceptable,
        and can proceed to kill.
        */
        std::string heartbeatFileName;
        //heartbeatFileName="/tmp/infosys_heartbeat";
        heartbeatFileName=config_.ControlDir()+"/infosys_heartbeat";
        struct stat buf;
        bool statresult;
        statresult=Arc::FileStat(heartbeatFileName, &buf, false);
        if ( !statresult ) {
          failedstat++;
          if (failedstat == 1) {
            logger_.msg(Arc::WARNING,"Cannot stat %s. Are infoproviders running? This message will not be repeated.", heartbeatFileName);
          } else {
            logger_.msg(Arc::DEBUG,"Cannot stat %s. Are infoproviders running? It happened already %d times.", heartbeatFileName, failedstat);
          }
        } else {
          time_t now;
          time(&now);
          // kill infoprovider only if heartbeat has never been updated before timeout
          if (difftime(now, buf.st_mtime) > infoprovider_wakeup_period_) {
            logger_.msg(Arc::ERROR,"Checked time: %d | Heartbeat file stat: %d | %s has not beed touched before timeout (%d). \n The performance is too low, infoproviders will be killed. A-REX functionality is not ensured.", now, buf.st_mtime, heartbeatFileName, infoprovider_wakeup_period_);
            // abruptly kill the infoprovider
            run.Kill(1);
          } else {
            logger_.msg(Arc::DEBUG,"Found recent heartbeat file %s , waiting other %d seconds", heartbeatFileName, infoprovider_wakeup_period_);
          }
        }
      }
      // if out of the while loop, infoprovider has finished or was killed
      r = run.Result();
      if (r!=0) {
        logger_.msg(Arc::WARNING,"Resource information provider failed with exit status: %i\n%s",r,stderr_str);
      } else {
        logger_.msg(Arc::DEBUG,"Resource information provider log:\n%s",stderr_str);
      };
    };
    if (r!=0) {
      logger_.msg(Arc::WARNING,"No new informational document assigned");
    } else {
      logger_.msg(Arc::VERBOSE,"Obtained XML: %s",xml_str.substr(0,100));
      // Following code is suboptimal. Most of it should go away
      // and functionality to be moved to information providers.
      if(!xml_str.empty()) {
        // Currently glue states are lost. Counter of all jobs is lost too.
        infodoc_.Assign(xml_str);
        Arc::XMLNode root = infodoc_.Acquire();
        Arc::XMLNode all_jobs_count = root["Domains"]["AdminDomain"]["Services"]["ComputingService"]["AllJobs"];
        if((bool)all_jobs_count) {
          Arc::stringto((std::string)all_jobs_count,all_jobs_count_);
          all_jobs_count.Destroy(); // is not glue2 info
        };
        infodoc_.Release();
      } else {
        logger_.msg(Arc::ERROR,"Informational document is empty");
      };
    };
    if(thread_count_.WaitOrCancel(infoprovider_wakeup_period_*100)) break;
  };
  thread_count_.UnregisterThread();
}

bool ARexService::RegistrationCollector(Arc::XMLNode &doc) {

  
  logger_.msg(Arc::VERBOSE,"Passing service's information from collector to registrator");
  Arc::XMLNode empty(ns_, "RegEntry");
  empty.New(doc);

  doc.NewChild("SrcAdv");
  doc.NewChild("MetaSrcAdv");
  doc["SrcAdv"].NewChild("Type") = "org.nordugrid.execution.arex";
  doc["SrcAdv"].NewChild("EPR").NewChild("Address") = endpoint_;
  if(publishstaticinfo_){
    /** 
    This section uses SSPair to send Static
    Information of Arex to the ISIS.  
    **/
    Arc::XMLNode root = infodoc_.Acquire();
    Arc::XMLNode staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "HealthState";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingEndpoint"]["HealthState"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "Capability";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingEndpoint"]["Capability"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "OSFamily";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingManager"]
                                   ["ExecutionEnvironments"]["ExecutionEnvironment"]["OSFamily"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "Platform";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingManager"]
                                   ["ExecutionEnvironments"]["ExecutionEnvironment"]["Platform"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "PhysicalCPUs";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingManager"]
                                   ["ExecutionEnvironments"]["ExecutionEnvironment"]["PhysicalCPUs"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "CPUMultiplicity";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingManager"]
                                   ["ExecutionEnvironments"]["ExecutionEnvironment"]["CPUMultiplicity"];
    staticInfo = doc["SrcAdv"].NewChild("SSPair");
    staticInfo.NewChild("Name") = "CPUModel";
    staticInfo.NewChild("Value") = (std::string)root["Domains"]["AdminDomain"]["Services"]
                                   ["ComputingService"]["ComputingManager"]
                                   ["ExecutionEnvironments"]["ExecutionEnvironment"]["CPUModel"];

    std::string path = "Domains/AdminDomain/Services/ComputingService/ComputingManager/ApplicationEnvironments/ApplicationEnvironment";
    Arc::XMLNodeList AEs = root.Path(path);
    for(Arc::XMLNodeList::iterator AE = AEs.begin(); AE!=AEs.end(); AE++){

      staticInfo = doc["SrcAdv"].NewChild("SSPair");
      staticInfo.NewChild("Name") = (std::string)((*AE)["AppName"])+"-"+(std::string)((*AE)["AppVersion"]);
      staticInfo.NewChild("Value") = (std::string)((*AE)["ID"]);
    }
    logger.msg(Arc::VERBOSE, "Registered static information: \n doc: %s",(std::string)doc);
    infodoc_.Release();
  } else
    logger.msg(Arc::VERBOSE, "Information Registered without static attributes: \n doc: %s",(std::string)doc);
return true;
  //
  // TODO: filter information here.
  //Arc::XMLNode regdoc("<Service/>");
  //regdoc.New(doc);
  //doc.NewChild(root);
  //infodoc_.Release();

}

std::string ARexService::getID() {
  return "ARC:AREX";
}

class PrefixedFilePayload: public Arc::PayloadRawInterface {
 private:
  std::string prefix_;
  std::string postfix_;
  int handle_;
  void* addr_;
  off_t length_;
 public:
  PrefixedFilePayload(const std::string& prefix,const std::string& postfix,int handle) {
    prefix_ = prefix;
    postfix_ = postfix;
    handle_ = handle;
    addr_ = NULL;
    length_ = 0;
    if(handle != -1) {
      struct stat st;
      if(::fstat(handle,&st) == 0) {
        if(st.st_size > 0) {
          length_ = st.st_size;
          addr_ = ::mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,handle,0);
          if(!addr_) length_=0;
        };
      };
    };
  };
  ~PrefixedFilePayload(void) {
    if(addr_) ::munmap(addr_,length_);
    ::close(handle_);
  };
  virtual char operator[](Size_t pos) const {
    char* p = ((PrefixedFilePayload*)this)->Content(pos);
    if(!p) return 0;
    return *p;
  };
  virtual char* Content(Size_t pos) {
    if(pos < prefix_.length()) return (char*)(prefix_.c_str() + pos);
    pos -= prefix_.length();
    if(pos < length_) return ((char*)(addr_) + pos);
    pos -= length_; 
    if(pos < postfix_.length()) return (char*)(postfix_.c_str() + pos);
    return NULL;
  };
  virtual Size_t Size(void) const {
    return (prefix_.length() + length_ + postfix_.length());
  };
  virtual char* Insert(Size_t /* pos */ = 0,Size_t /* size */ = 0) {
    return NULL;
  };
  virtual char* Insert(const char* /* s */,Size_t /* pos */ = 0,Size_t /* size */ = -1) {
    return NULL;
  };
  virtual char* Buffer(unsigned int num = 0) {
    if(num == 0) return (char*)(prefix_.c_str());
    if(addr_) {
      if(num == 1) return (char*)addr_;
    } else {
      ++num;
    };
    if(num == 2) return (char*)(postfix_.c_str());
    return NULL;
  };
  virtual Size_t BufferSize(unsigned int num = 0) const {
    if(num == 0) return prefix_.length();
    if(addr_) {
      if(num == 1) return length_;
    } else {
      ++num;
    };
    if(num == 2) return postfix_.length();
    return 0;
  };
  virtual Size_t BufferPos(unsigned int num = 0) const {
    if(num == 0) return 0;
    if(addr_) {
      if(num == 1) return prefix_.length();
    } else {
      ++num;
    };
    if(num == 2) return (prefix_.length() + length_);
    return (prefix_.length() + length_ + postfix_.length());
  };
  virtual bool Truncate(Size_t /* size */) { return false; };
};

OptimizedInformationContainer::OptimizedInformationContainer(bool parse_xml) {
  handle_=-1;
  parse_xml_=parse_xml;
}

OptimizedInformationContainer::~OptimizedInformationContainer(void) {
  if(handle_ != -1) ::close(handle_);
  if(!filename_.empty()) ::unlink(filename_.c_str());
}

int OptimizedInformationContainer::OpenDocument(void) {
  int h = -1;
  olock_.lock();
  if(handle_ != -1) h = ::dup(handle_);
  olock_.unlock();
  return h;
}

Arc::MessagePayload* OptimizedInformationContainer::Process(Arc::SOAPEnvelope& in) {
  Arc::WSRF& wsrp = Arc::CreateWSRP(in);
  if(!wsrp) { delete &wsrp; return NULL; };
  try {
    Arc::WSRPGetResourcePropertyDocumentRequest* req =
         dynamic_cast<Arc::WSRPGetResourcePropertyDocumentRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    // Request for whole document
    std::string fake_str("<fake>fake</fake>");
    Arc::XMLNode xresp(fake_str);
    Arc::WSRPGetResourcePropertyDocumentResponse resp(xresp);
    std::string rest_str;
    resp.SOAP().GetDoc(rest_str);
    std::string::size_type p = rest_str.find(fake_str);
    if(p == std::string::npos) throw std::exception();
    PrefixedFilePayload* outpayload = new PrefixedFilePayload(rest_str.substr(0,p),rest_str.substr(p+fake_str.length()),OpenDocument());
    delete &wsrp;
    return outpayload;
  } catch(std::exception& e) { };
  delete &wsrp;
  if(!parse_xml_) return NULL; // No XML document available
  Arc::NS ns;
  Arc::SOAPEnvelope* out = InformationContainer::Process(in);
  if(!out) return NULL;
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);
  out->Swap(*outpayload);
  delete out;
  return outpayload;
}

void OptimizedInformationContainer::AssignFile(const std::string& filename) {
  olock_.lock();
  if(!filename_.empty()) ::unlink(filename_.c_str());
  if(handle_ != -1) ::close(handle_);
  filename_ = filename;
  handle_ = -1;
  if(!filename_.empty()) {
    handle_ = ::open(filename_.c_str(),O_RDONLY);
    if(parse_xml_) {
      lock_.lock();
      doc_.ReadFromFile(filename_);
      lock_.unlock();
      Arc::InformationContainer::Assign(doc_,false);
    };
  };
  olock_.unlock();
}

void OptimizedInformationContainer::Assign(const std::string& xml) {
  std::string filename;
  int h = Glib::file_open_tmp(filename);
  if(h == -1) {
    Arc::Logger::getRootLogger().msg(Arc::ERROR,"OptimizedInformationContainer failed to create temporary file");
    return;
  };
  Arc::Logger::getRootLogger().msg(Arc::VERBOSE,"OptimizedInformationContainer created temporary file: %s",filename);
  for(std::string::size_type p = 0;p<xml.length();++p) {
    ssize_t l = ::write(h,xml.c_str()+p,xml.length()-p);
    if(l == -1) {
      ::unlink(filename.c_str());
      ::close(h);
      Arc::Logger::getRootLogger().msg(Arc::ERROR,"OptimizedInformationContainer failed to store XML document to temporary file");
      return;
    };
    p+=l;
  };
  if(parse_xml_) {
    Arc::XMLNode newxml(xml);
    if(!newxml) {
      ::unlink(filename.c_str());
      ::close(h);
      Arc::Logger::getRootLogger().msg(Arc::ERROR,"OptimizedInformationContainer failed to parse XML");
      return;
    };
    // Here we have XML stored in file and parsed
    olock_.lock();
    if(!filename_.empty()) ::unlink(filename_.c_str());
    if(handle_ != -1) ::close(handle_);
    filename_ = filename;
    handle_ = h;
    lock_.lock();
    doc_.Swap(newxml);
    lock_.unlock();
    Arc::InformationContainer::Assign(doc_,false);
    olock_.unlock();
  } else {
    // Here we have XML stored in file
    olock_.lock();
    if(!filename_.empty()) ::unlink(filename_.c_str());
    if(handle_ != -1) ::close(handle_);
    filename_ = filename;
    handle_ = h;
    olock_.unlock();
  };
}

#define ESINFOFAULT(MSG) { \
  logger_.msg(Arc::ERROR, std::string("ES:GetResourceInfo: ")+(MSG)); \
  Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,(MSG)); \
  ESInternalResourceInfoFault(fault,(MSG)); \
  out.Destroy(); \
  return Arc::MCC_Status(Arc::STATUS_OK); \
}

#define ESFAULT(MSG) { \
  logger_.msg(Arc::ERROR, std::string("ES:QueryResourceInfo: ")+(MSG)); \
  Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,(MSG)); \
  ESInternalBaseFault(fault,(MSG)); \
  out.Destroy(); \
  return Arc::MCC_Status(Arc::STATUS_OK); \
}

// GetResourceInfo
//
// GetResourceInfoResponse
//   Services
//     glue:ComputingService 0-
//     glue:Service 0-  - not mentioned in specs
// InternalResourceInfoFault
// ResourceInfoNotFoundFault
// AccessControlFault
// InternalBaseFault
Arc::MCC_Status ARexService::ESGetResourceInfo(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  // WARNING. Suboptimal temporary solution.
  int h = infodoc_.OpenDocument();
  if(h == -1) ESINFOFAULT("Failed to open resource information file");
  ::lseek(h,0,SEEK_SET);
  struct stat st;
  if((::fstat(h,&st) != 0) || (st.st_size == 0)) {
    ::close(h);
    ESINFOFAULT("Failed to stat resource information file");
  };
  char* buf = (char*)::malloc(st.st_size+1);
  if(!buf) {
    ::close(h);
    ESINFOFAULT("Failed to allocate memory for resoure information");
  };
  off_t p = 0;
  for(;p<st.st_size;) {
    ssize_t l = ::read(h,buf+p,st.st_size-p);
    if(l == 0) break;
    if(l == -1) {
      if(errno != EAGAIN) {
        ::free(buf);
        ::close(h);
        ESINFOFAULT("Failed to read resource information file");
      };
    };
    p+=l;
  };
  buf[p] = 0;
  ::close(h);
  Arc::XMLNode doc(buf);
  ::free(buf); buf=NULL;
  if(!doc) {
    ESINFOFAULT("Failed to parse resource information document");
  };
  //Arc::NS glueNS("glue","http://schemas.ogf.org/glue/2009/03/spec_2.0_r1");
  Arc::XMLNode service = doc["Domains"]["AdminDomain"]["Services"]["ComputingService"];
  if(!service) {
    service = doc["Domains"]["AdminDomain"]["ComputingService"];
    //if(!service) {
    //  ESINFOFAULT("Missing ComputingService in resource information");
    //};
  };
  Arc::XMLNode manager = doc["Domains"]["AdminDomain"]["Services"]["Service"];
  if(!manager) {
    manager = doc["Domains"]["AdminDomain"]["Service"];
    //if(!manager) {
    //  ESINFOFAULT("Missing Service in resource information");
    //};
  };
  Arc::XMLNode services = out.NewChild("esrinfo:Services");
  for(;service;++service) {
    // TODO: use move instead of copy
    services.NewChild(service);
  }
  for(;manager;++manager) {
    // TODO: use move instead of copy
    services.NewChild(manager);
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

// QueryResourceInfo
//   QueryDialect
//     [XPATH 1.0|XPATH 2.0|XQUERY 1.0|SQL|SPARQL]
//   QueryExpression
//     any 0-1
//
// QueryResourceInfoResponse
//   QueryResourceInfoItem 0-
//     any 0-1
//     anyAttribute
// NotSupportedQueryDialectFault
// NotValidQueryStatementFault
// UnknownQueryFault
// AccessControlFault
// InternalBaseFault
Arc::MCC_Status ARexService::ESQueryResourceInfo(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  std::string dialect = (std::string)in["QueryDialect"];
  if(dialect.empty()) {
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
    ESNotSupportedQueryDialectFault(fault,"Query dialect not defined");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  if(dialect != "XPATH 1.0") {
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
    ESNotSupportedQueryDialectFault(fault,"Only XPATH 1.0 is supported");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  // It is not clearly defined how xpath string is put into QueryExpression
  Arc::XMLNode expression = in["QueryExpression"];
  if(expression.Size() > 0) expression = expression.Child(0);
  std::string xpath = (std::string)expression;
  if(xpath.empty()) {
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
    ESNotValidQueryStatementFault(fault,"Could not extract xpath query from request");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  // WARNING. Suboptimal temporary solution.
  int h = infodoc_.OpenDocument();
  if(h == -1) ESFAULT("Failed to open resource information file");
  ::lseek(h,0,SEEK_SET);
  struct stat st;
  if((::fstat(h,&st) != 0) || (st.st_size == 0)) {
    ::close(h);
    ESFAULT("Failed to stat resource information file");
  };
  char* buf = (char*)::malloc(st.st_size+1);
  if(!buf) {
    ::close(h);
    ESFAULT("Failed to allocate memory for resoure information");
  };
  off_t p = 0;
  for(;p<st.st_size;) {
    ssize_t l = ::read(h,buf+p,st.st_size-p);
    if(l == 0) break;
    if(l == -1) {
      if(errno != EAGAIN) {
        ::free(buf);
        ::close(h);
        ESFAULT("Failed to read resource information file");
      };
    };
    p+=l;
  };
  buf[p] = 0;
  ::close(h);
  Arc::XMLNode doc(buf);
  ::free(buf); buf=NULL;
  Arc::XMLNode rdoc;
  doc["Domains"]["AdminDomain"]["Services"].Move(rdoc);
  if((!doc) || (!rdoc)) {
    ESFAULT("Failed to parse resource information document");
  };

  Arc::NS glue_ns;
  glue_ns[rdoc.Prefix()] = rdoc.Namespace();
  rdoc.StripNamespace(-1); // to ignore namespace in XPathLookup
  //Arc::NS glueNS("glue","http://schemas.ogf.org/glue/2009/03/spec_2.0_r1");
  Arc::XMLNodeList ritems = rdoc.XPathLookup(xpath,Arc::NS());
  for(Arc::XMLNodeList::iterator ritem = ritems.begin(); ritem != ritems.end(); ++ritem) {
    Arc::XMLNode i = out.NewChild("esrinfo:QueryResourceInfoItem").NewChild(*ritem);
    i.Namespaces(glue_ns);
    i.Prefix(glue_ns.begin()->first,-1);
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

}

