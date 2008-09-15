#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[filename ...]"),
			    istring("The arcsub command is used for "
				    "submitting jobs to grid enabled "
				    "computing\nresources."),
			    istring("Argument to -i has the format "
				    "Flavour:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "mds-vo-name=sweden,O=grid\n"
				    "CREAM:ldap://cream.grid.upjs.sk:2170/"
				    "o=grid\n"
				    "\n"
				    "Argument to -c has the format "
				    "Flavour:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "nordugrid-cluster-name=grid.tsl.uu.se,"
				    "Mds-Vo-name=local,o=grid"));

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
		    istring("explicity select or reject a specific cluster"),
		    istring("[-]name"),
		    clusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
		    istring("explicity select or reject an index server"),
		    istring("[-]name"),
		    indexurls);

  std::list<std::string> jobdescriptionstrings;
  options.AddOption('e', "jobdescrstring",
		    istring("jobdescription string describing the job to "
			    "be submitted"),
		    istring("string"),
		    jobdescriptionstrings);

  std::list<std::string> jobdescriptionfiles;
  options.AddOption('f', "jobdescrfile",
		    istring("jobdescription file describing the job to "
			    "be submitted"),
		    istring("string"),
		    jobdescriptionfiles);

  std::string joblist;
  options.AddOption('j', "joblist",
		    istring("file where the jobs will be stored"),
		    istring("filename"),
		    joblist);

  /*
  bool dryrun = false;
  options.AddOption('D', "dryrun", istring("add dryrun option"),
		    dryrun);

  bool dumpdescription = false;
  options.AddOption('x', "dumpdescription",
		    istring("do not submit - dump job description"),
		    dumpdescription);
  */
  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
		    istring("configuration file (default ~/.arc/client.xml)"),
		    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
	      << std::endl;
    return 0;
  }

  jobdescriptionfiles.insert(jobdescriptionfiles.end(),
			     params.begin(), params.end());

  if (joblist.empty())
    joblist = usercfg.JobListFile();
  else {
    struct stat st;
    if (stat(joblist.c_str(), &st) != 0) {
      if (errno == ENOENT) {
	Arc::NS ns;
	Arc::Config(ns).SaveToFile(joblist);
	logger.msg(Arc::INFO, "Created empty ARC job list file: %s", joblist);
	stat(joblist.c_str(), &st);
      }
      else {
	logger.msg(Arc::ERROR, "Can not access ARC job list file: %s (%s)",
		   joblist, Arc::StrError());
	return 1;
      }
    }
    if (!S_ISREG(st.st_mode)) {
      logger.msg(Arc::ERROR, "ARC job list file is not a regular file: %s",
		 joblist);
      return 1;
    }
  }

  if (jobdescriptionfiles.empty() && jobdescriptionstrings.empty()) {
    logger.msg(Arc::ERROR, "No job description input specified");
    return 1;
  }

  std::list<Arc::JobDescription> jobdescriptionlist;

  //Loop over input job description files
  for (std::list<std::string>::iterator it = jobdescriptionfiles.begin();
       it != jobdescriptionfiles.end(); it++) {

    std::ifstream descriptionfile(it->c_str());

    if (!descriptionfile) {
      logger.msg(Arc::ERROR, "Can not open job description file: %s", *it);
      return 1;
    }

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';
    Arc::JobDescription jobdesc;
    jobdesc.setSource((std::string) buffer);

    if (jobdesc.isValid())
      jobdescriptionlist.push_back(jobdesc);
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << buffer << std::endl;
      delete[] buffer;
      return 1;
    }
    delete[] buffer;
  }

  //Loop over job description input strings
  for(std::list<std::string>::iterator it = jobdescriptionstrings.begin();
      it != jobdescriptionstrings.end(); it++) {

    Arc::JobDescription jobdesc;

    jobdesc.setSource(*it);

    if (jobdesc.isValid())
      jobdescriptionlist.push_back(jobdesc);
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << *it << std::endl;
      return 1;
    }
  }

  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
    logger.msg(Arc::ERROR, "Job submission failed because none of "
	       "the specified clusters returned any information");
    return 1;
  }

  std::map<int, std::string> notsubmitted;
  
  int jobnr = 1;
  std::list<std::string> jobids;

  Arc::NS ns;
  Arc::Config jobstorage(ns);

  for (std::list<Arc::JobDescription>::iterator it =
	 jobdescriptionlist.begin(); it != jobdescriptionlist.end();
       it++, jobnr++) {

    // perform brokering (not yet implemented)
    // for now use execution targets in the order found...

    for (std::list<Arc::ExecutionTarget>::const_iterator target =
	   targen.FoundTargets().begin();
	 target != targen.FoundTargets().end(); target++) {

      Arc::Submitter *submitter = target->GetSubmitter(usercfg);

      Arc::NS ns;
      Arc::XMLNode info(ns, "Job");
      if (!submitter->Submit(*it, info)) {
	logger.msg(Arc::ERROR, "Submission to %s failed", target->url.str());
	continue;
      }

      if (it->getXML()["JobIdentification"]["JobName"])
	info.NewChild("Name") =
	  (std::string) it->getXML()["JobIdentification"]["JobName"];
      info.NewChild("Flavour") = target->GridFlavour;
      info.NewChild("Cluster") = target->Cluster.str();
      info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();

      jobstorage.NewChild("Job").Replace(info);

      std::cout << Arc::IString("Job submitted with jobid: %s",
				(std::string) info["JobID"]) << std::endl;
      break;
    }
  }

  {
    Arc::FileLock lock(joblist);
    Arc::Config jobs;
    jobs.ReadFromFile(joblist);
    for (Arc::XMLNode j = jobstorage["Job"]; j; ++j) {
      jobs.NewChild(j);
    }
    jobs.SaveToFile(joblist);
  }

  if (jobdescriptionlist.size() > 1) {
    std::cout << std::endl << Arc::IString("Job submission summary:")
	      << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted",
			      jobdescriptionlist.size() - notsubmitted.size(),
			      jobdescriptionlist.size()) << std::endl;
    if (notsubmitted.size()) {
      std::cout << Arc::IString("The following %d were not submitted",
				notsubmitted.size()) << std::endl;
      /*
      std::map<int, std::string>::iterator it;
      for (it = notsubmitted.begin(); it != notsubmitted.end(); it++) {
	std::cout << _("Job nr.") << " " << it->first;
	if (it->second.size()>0) std::cout << ": " << it->second;
	std::cout << std::endl;
      }
      */
    }
  }

  return 0;
}
