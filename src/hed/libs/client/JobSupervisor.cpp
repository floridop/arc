// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");

  JobSupervisor::JobSupervisor(const UserConfig& usercfg,
                               const std::list<std::string>& jobs) {
    URLListMap jobids;

    Config jobstorage;
    if (!usercfg.JobListFile().empty()) {
      FileLock lock(usercfg.JobListFile());
      jobstorage.ReadFromFile(usercfg.JobListFile());
    }

    std::list<std::string> controllers;

    if (!jobs.empty()) {

      logger.msg(VERBOSE, "Identifying needed job controllers according to "
                 "specified jobs");

      for (std::list<std::string>::const_iterator it = jobs.begin();
           it != jobs.end(); it++) {

        XMLNodeList xmljobs =
          jobstorage.XPathLookup("//Job[JobID='" + *it + "' or "
                                 "Name='" + *it + "']", NS());

        if (xmljobs.empty()) {
          logger.msg(WARNING, "Job not found in job list: %s", *it);
          continue;
        }

        for (XMLNodeList::iterator xit = xmljobs.begin();
             xit != xmljobs.end(); xit++) {

          URL jobid = (std::string)(*xit)["JobID"];
          std::string flavour = (std::string)(*xit)["Flavour"];

          jobids[flavour].push_back(jobid);

          if (std::find(controllers.begin(), controllers.end(),
                        flavour) == controllers.end()) {
            logger.msg(VERBOSE, "Need job controller for grid flavour %s",
                       flavour);
            controllers.push_back(flavour);
          }
        }
      }
    }

    if (!usercfg.GetSelectedServices(COMPUTING).empty()) {

      logger.msg(VERBOSE, "Identifying needed job controllers according to "
                 "specified clusters");

      for (URLListMap::const_iterator it = usercfg.GetSelectedServices(COMPUTING).begin();
           it != usercfg.GetSelectedServices(COMPUTING).end(); it++)
        if (std::find(controllers.begin(), controllers.end(),
                      it->first) == controllers.end()) {
          logger.msg(VERBOSE, "Need job controller for grid flavour %s",
                     it->first);
          controllers.push_back(it->first);
        }
    }

    if (jobs.empty() && usercfg.GetSelectedServices(COMPUTING).empty()) {

      logger.msg(VERBOSE, "Identifying needed job controllers according to "
                 "all jobs present in job list");

      XMLNodeList xmljobs = jobstorage.XPathLookup("/ArcConfig/Job", NS());

      if (xmljobs.empty()) {
        logger.msg(INFO, "No jobs to handle");
        return;
      }

      for (XMLNodeList::iterator it = xmljobs.begin();
           it != xmljobs.end(); it++)
        if (std::find(controllers.begin(), controllers.end(),
                      (std::string)(*it)["Flavour"]) == controllers.end()) {
          std::string flavour = (*it)["Flavour"];
          logger.msg(VERBOSE, "Need job controller for grid flavour %s",
                     flavour);
          controllers.push_back(flavour);
        }
    }

    for (std::list<std::string>::iterator it = controllers.begin();
         it != controllers.end(); it++) {
      JobController *JC = loader.load(*it, usercfg);
      if (JC)
        JC->FillJobStore(jobids[*it]);
    }
  }

  JobSupervisor::~JobSupervisor() {}

} // namespace Arc
