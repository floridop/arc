#include <arc/misc/ClientInterface.h>
#include <arc/loader/Loader.h>
#include "ExecutionTarget.h"
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace Arc {

  ExecutionTarget::ExecutionTarget()
    : Latitude(0),
      Longitude(0),
      TotalJobs(-1),
      RunningJobs(-1),
      WaitingJobs(-1),
      StagingJobs(-1),
      SuspendedJobs(-1),
      PreLRMSWaitingJobs(-1),
      MaxWallTime(-1),
      MinWallTime(-1),
      DefaultWallTime(-1),
      MaxCPUTime(-1),
      MinCPUTime(-1),
      DefaultCPUTime(-1),
      MaxTotalJobs(-1),
      MaxRunningJobs(-1),
      MaxWaitingJobs(-1),
      NodeMemory(-1),
      MaxPreLRMSWaitingJobs(-1),
      MaxUserRunningJobs(-1),
      MaxSlotsPerJobs(-1),
      MaxStageInStreams(-1),
      MaxStageOutStreams(-1),
      MaxMemory(-1),
      MaxDiskSpace(-1),
      DefaultStorageService(),
      Preemption(false),
      EstimatedAverageWaitingTime(-1),
      EstimatedWorstWaitingTime(-1),
      FreeSlots(-1),
      UsedSlots(-1),
      RequestedSlots(-1) {}

  ExecutionTarget::~ExecutionTarget() {}

  Submitter *ExecutionTarget::GetSubmitter() {

    LogStream logcerr(std::cerr);
    Logger::getRootLogger().addDestination(logcerr);
    Logger::getRootLogger().setThreshold(DEBUG);

    ACCConfig acccfg;
    NS ns;
    Config cfg(ns);
    acccfg.MakeConfig(cfg);

    XMLNode SubmitterComp = cfg.NewChild("ArcClientComponent");
    SubmitterComp.NewAttribute("name") = "Submitter" + GridFlavour;
    SubmitterComp.NewAttribute("id") = "submitter";
    SubmitterComp.NewChild("Endpoint") = url.str();
    SubmitterComp.NewChild("Source") = Source;
    SubmitterComp.NewChild("MappingQueue") = MappingQueue;

    loader = new Loader(&cfg);

    return dynamic_cast<Submitter *>(loader->getACC("submitter"));

  }

} // namespace Arc
