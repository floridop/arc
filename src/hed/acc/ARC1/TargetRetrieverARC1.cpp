#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>
#include <arc/message/MCC.h>

#include "AREXClient.h"
#include "TargetRetrieverARC1.h"

//Remove these after debugging
#include <iostream>
//End of debugging headers

namespace Arc {

  struct ThreadArg {
    TargetGenerator *mom;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    URL url;
    int targetType;
    int detailLevel;
  };

  ThreadArg* TargetRetrieverARC1::CreateThreadArg(TargetGenerator& mom,
						  int targetType,
						  int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->proxyPath = proxyPath;
    arg->certificatePath = certificatePath;
    arg->keyPath = keyPath;
    arg->caCertificatesDir = caCertificatesDir;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverARC1::logger(TargetRetriever::logger, "ARC1");

  TargetRetrieverARC1::TargetRetrieverARC1(Config *cfg)
    : TargetRetriever(cfg, "ARC1") {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  Plugin* TargetRetrieverARC1::Instance(Arc::PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new TargetRetrieverARC1((Arc::Config*)(*accarg));
  }

  void TargetRetrieverARC1::GetTargets(TargetGenerator& mom, int targetType,
				       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC1 initialized with %s service url: %s",
	       serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added) {
	ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
	if (!CreateThreadFunction(&InterrogateTarget, arg)) {
	  delete arg;
	  mom.RetrieverDone();
	}
      }
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      bool added = mom.AddIndexServer(url);
      if (added) {
	ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
	if (!CreateThreadFunction(&QueryIndex, arg)) {
	  delete arg;
	  mom.RetrieverDone();
	}
      }
    }
    else
      logger.msg(ERROR,
		 "TargetRetrieverARC1 initialized with unknown url type");
  }

  void TargetRetrieverARC1::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    // URL& url = thrarg->url;

    // TODO: ISIS

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;
    MCCConfig cfg;
    if (!thrarg->proxyPath.empty())
      cfg.AddProxy(thrarg->proxyPath);
    if (!thrarg->certificatePath.empty())
      cfg.AddCertificate(thrarg->certificatePath);
    if (!thrarg->keyPath.empty())
      cfg.AddPrivateKey(thrarg->keyPath);
    if (!thrarg->caCertificatesDir.empty())
      cfg.AddCADir(thrarg->caCertificatesDir);
    AREXClient ac(url, cfg);
    XMLNode ServerStatus;
    if (!ac.sstat(ServerStatus)) {
      delete thrarg;
      mom.RetrieverDone();
    }

    #define _XML_descend(X , sl)\
    {\
      std::list<std::string>::iterator si = sl.begin();\
      for (; si != sl.end() && X[*si]; X=X[*si], si++);\
      if (si != sl.end()) {\
        std::string path = ".";\
        for (std::list<std::string>::iterator si2 = sl.begin(); si2 != si; si2++){\
          path += "/" + *si2;\
        }\
        logger.msg(ERROR, "The node %s has no %s element.", path, *si);\
        delete thrarg;\
        mom.RetrieverDone();\
      }\
    }

    XMLNode GLUEService = ServerStatus;
    std::string tmpsa[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "Domains", "AdminDomain", "Services", "Service"};
    std::list<std::string> tmpsl(tmpsa, tmpsa+6);
    _XML_descend (GLUEService, tmpsl);
    XMLNode NUGCService = ServerStatus;
    std::string tmpsb[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "nordugrid"};
    std::list<std::string> tmpsk(tmpsb, tmpsb+3);
    _XML_descend (NUGCService, tmpsk);

    // std::cout << GLUEService << std::endl; //for dubugging

    ExecutionTarget target;

    target.GridFlavour = "ARC1";
    target.Cluster = thrarg->url;
    target.url = url;
    target.Interface = "BES";
    target.Implementor = "NorduGrid";
    target.ImplementationName = "A-REX";

    target.DomainName = url.Host();

    if (GLUEService["ComputingEndpoint"]["HealthState"]) {
      target.HealthState = (std::string) GLUEService["ComputingEndpoint"]["HealthState"];
    } else {
      logger.msg(WARNING, "The Service advertises no Health State.");
    }

    if (GLUEService["ComputingEndpoint"]["ID"]) {
      target.CEID = (std::string)GLUEService["ComputingEndpoint"]["ID"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its ID.");
    }

    if (GLUEService["Name"]) {
      target.CEName = (std::string)GLUEService["Name"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Name.");
    }

    if (GLUEService["Capability"]) {
      target.Capability = (std::string)GLUEService["Capability"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Capability.");
    }

    if (GLUEService["Type"]) {
      target.Type = (std::string)GLUEService["Type"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Type.");
    }

    if (GLUEService["QualityLevel"]) {
      target.QualityLevel = (std::string)GLUEService["QualityLevel"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Quality Level.");
    }

    if (GLUEService["ComputingEndpoint"]["Technology"]) {
      target.Technology = (std::string)GLUEService["ComputingEndpoint"]["Technology"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Technology.");
    }

    if (GLUEService["ComputingEndpoint"]["InterfaceName"]) {
      target.Interface = (std::string)GLUEService["ComputingEndpoint"]["InterfaceName"];
    } else if (GLUEService["ComputingEndpoint"]["Interface"]) {
      target.Interface = (std::string)GLUEService["ComputingEndpoint"]["Interface"];
    }
 
    if (target.Interface == "") {
      logger.msg(WARNING, "The Service doesn't advertise its Interface.");
    }

    if (GLUEService["ComputingEndpoint"]["InterfaceExtension"]) {
      target.InterfaceExtension = (std::string)GLUEService["ComputingEndpoint"]["InterfaceExtension"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Interface Extension.");
    }

    if (GLUEService["ComputingEndpoint"]["SupportedProfile"]) {
      target.SupportedProfile = (std::string)GLUEService["ComputingEndpoint"]["SupportedProfile"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Supported Profile.");
    }

    if (GLUEService["ComputingEndpoint"]["ImplementationVersion"]) {
      target.ImplementationVersion = (std::string)GLUEService["ComputingEndpoint"]["ImplementationVersion"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Implementation Version.");
    }

    if (GLUEService["ComputingEndpoint"]["ServingState"]) {
      target.ServingState = (std::string)GLUEService["ComputingEndpoint"]["ServingState"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Serving State.");
    }

    if (GLUEService["ComputingEndpoint"]["IssuerCA"]) {
      target.IssuerCA = (std::string)GLUEService["ComputingEndpoint"]["IssuerCA"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Issuer CA.");
    }

    if (GLUEService["ComputingEndpoint"]["TrustedCA"]) {
      XMLNode n = GLUEService["ComputingEndpoint"]["TrustedCA"];
      while (n) {
        target.TrustedCA.push_back((std::string)n);
        ++n; //The increment operator works in an unusual manner (returns void)
      }
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Trusted CA.");
    }
/*    for (std::list<std::string>::iterator dbgit = target.TrustedCA.begin(); dbgit != target.TrustedCA.end(); ++dbgit) {
      std::cout << *dbgit << std::endl; //for dubugging
    }*/

    if (GLUEService["ComputingEndpoint"]["DowntimeStart"]) {
      Arc::Time tmp((std::string)GLUEService["ComputingEndpoint"]["DowntimeStart"]);
      if (tmp != -1) target.DowntimeStarts = tmp;
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Downtime Start.");
    }

    if (GLUEService["ComputingEndpoint"]["DowntimeEnd"]) {
      Arc::Time tmp((std::string)GLUEService["ComputingEndpoint"]["DowntimeEnd"]);
      if (tmp != -1) target.DowntimeEnds = tmp;
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Downtime End.");
    }

/*    std::cout << "Dowtime start:\t" << target.DowntimeStarts << std::endl; //for dubugging
    std::cout << "Dowtime end:\t" << target.DowntimeEnds << std::endl; //for dubugging*/

    if (GLUEService["ComputingEndpoint"]["Staging"]) {
      target.Staging = (std::string)GLUEService["ComputingEndpoint"]["Staging"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Staging capabilities.");
    }

    if (GLUEService["ComputingEndpoint"]["Jobdescription"]) {
      target.Jobdescription = (std::string)GLUEService["ComputingEndpoint"]["Jobdescription"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise what Job Description type it accepts.");
    }

    //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).

    if (GLUEService["ComputingEndpoint"]["TotalJobs"]) {
      target.TotalJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["TotalJobs"]);
    } else if (GLUEService["TotalJobs"]) {
      target.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Total Number of Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["RunningJobs"]) {
      target.RunningJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["RunningJobs"]);
    } else if (GLUEService["RunningJobs"]) {
      target.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Running Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["WaitingJobs"]) {
      target.WaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["WaitingJobs"]);
    } else if (GLUEService["WaitingJobs"]) {
      target.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Waiting Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["StagingJobs"]) {
      target.StagingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["StagingJobs"]);
    } else if (GLUEService["StagingJobs"]) {
      target.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Staging Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["SuspendedJobs"]) {
      target.SuspendedJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["SuspendedJobs"]);
    } else if (GLUEService["SuspendedJobs"]) {
      target.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Suspended Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["PreLRMSWaitingJobs"]) {
      target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["PreLRMSWaitingJobs"]);
    } else if (GLUEService["PreLRMSWaitingJobs"]) {
      target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Jobs not yet in the LRMS.");
    }

    if (GLUEService["ComputingEndpoint"]["LocalRunningJobs"]) {
      target.LocalRunningJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["LocalRunningJobs"]);
    } else if (GLUEService["LocalRunningJobs"]) {
      target.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Local Running Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["LocalWaitingJobs"]) {
      target.LocalWaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["LocalWaitingJobs"]);
    } else if (GLUEService["LocalWaitingJobs"]) {
      target.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Local Waiting Jobs.");
    }

/*
    if (GLUEService["ComputingEndpoint"]["Jobdescription"]) {
      target.TotalJobs = (std::string)GLUEService["ComputingEndpoint"]["Jobdescription"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise what Jod Description type it accepts.");
    }*/



//     if (ServerStatus["LocalResourceManagerType"])
//       target.ManagerType =
// 	(std::string)ServerStatus["LocalResourceManagerType"];

    mom.AddTarget(target);

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
