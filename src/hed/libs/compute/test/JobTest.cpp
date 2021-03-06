#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/compute/Job.h>

class JobTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobTest);
  CPPUNIT_TEST(XMLToJobTest);
  CPPUNIT_TEST(JobToXMLTest);
  CPPUNIT_TEST(XMLToJobStateTest);
  CPPUNIT_TEST(VersionTwoFormatTest);
  CPPUNIT_TEST(VersionOneFormatTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobTest();
  void setUp() {}
  void tearDown() {}
  void XMLToJobTest();
  void JobToXMLTest();
  void XMLToJobStateTest();
  void VersionTwoFormatTest();
  void VersionOneFormatTest();
  
private:
  Arc::XMLNode xmlJob;
};

JobTest::JobTest() : xmlJob(Arc::XMLNode("<ComputingActivity>"
    "<ServiceInformationURL>https://testbed-emi4.grid.upjs.sk:60000/arex</ServiceInformationURL>"
    "<ServiceInformationInterfaceName>org.ogf.glue.emies.resourceinfo</ServiceInformationInterfaceName>"
    "<JobStatusURL>https://testbed-emi4.grid.upjs.sk:60000/arex</JobStatusURL>"
    "<JobStatusInterfaceName>org.ogf.glue.emies.activitymanagement</JobStatusInterfaceName>"
    "<JobManagementURL>https://testbed-emi4.grid.upjs.sk:60000/arex</JobManagementURL>"
    "<JobManagementInterfaceName>org.ogf.glue.emies.activitymanagement</JobManagementInterfaceName>"
    "<JobID>https://testbed-emi4.grid.upjs.sk:60000/arex/HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm</JobID>"
    "<IDFromEndpoint>HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm</IDFromEndpoint>"
    "<StageInDir>https://testbed-emi4.grid.upjs.sk:60000/arex</StageInDir>"
    "<StageOutDir>https://testbed-emi4.grid.upjs.sk:60000/arex</StageOutDir>"
    "<SessionDir>https://testbed-emi4.grid.upjs.sk:60000/arex</SessionDir>"
    "<Name>mc08.1050.J7</Name>"
    "<Type>single</Type>"
    "<LocalIDFromManager>345.ce01</LocalIDFromManager>"
    "<JobDescription>nordugrid:xrsl</JobDescription>"
    "<JobDescriptionDocument>&amp;(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")</JobDescriptionDocument>"
    "<State>bes:failed</State>"
    "<State>nordugrid:FAILED</State>"
    "<RestartState>bes:running</RestartState>"
    "<RestartState>nordugrid:FINISHING</RestartState>"
    "<ExitCode>0</ExitCode>"
    "<ComputingManagerExitCode>0</ComputingManagerExitCode>"
    "<Error>Uploading timed out</Error>"
    "<Error>Failed stage-out</Error>"
    "<WaitingPosition>0</WaitingPosition>"
    "<UserDomain>vo:atlas</UserDomain>"
    "<Owner>CONFIDENTIAL</Owner>"
    "<LocalOwner>grid02</LocalOwner>"
    "<RequestedTotalWallTime>5000</RequestedTotalWallTime>"
    "<RequestedTotalCPUTime>20000</RequestedTotalCPUTime>"
    "<RequestedSlots>4</RequestedSlots>"
    "<RequestedApplicationEnvironment>ENV/JAVA/JRE-1.6.0</RequestedApplicationEnvironment>"
    "<RequestedApplicationEnvironment>APPS/HEP/ATLAS-14.2.23.4</RequestedApplicationEnvironment>"
    "<StdIn>input.dat</StdIn>"
    "<StdOut>job.out</StdOut>"
    "<StdErr>err.out</StdErr>"
    "<LogDir>celog</LogDir>"
    "<ExecutionNode>wn043</ExecutionNode>"
    "<ExecutionNode>wn056</ExecutionNode>"
    "<Queue>pbs-short</Queue>"
    "<UsedTotalWallTime>2893</UsedTotalWallTime>"
    "<UsedTotalCPUTime>12340</UsedTotalCPUTime>"
    "<UsedMainMemory>4453</UsedMainMemory>"
    "<LocalSubmissionTime>2008-04-21T10:04:36Z</LocalSubmissionTime>"
    "<SubmissionTime>2008-04-21T10:05:12Z</SubmissionTime>"
    "<ComputingManagerSubmissionTime>2008-04-20T06:05:12Z</ComputingManagerSubmissionTime>"
    "<StartTime>2008-04-20T06:45:12Z</StartTime>"
    "<ComputingManagerEndTime>2008-04-20T10:05:12Z</ComputingManagerEndTime>"
    "<EndTime>2008-04-20T10:15:12Z</EndTime>"
    "<WorkingAreaEraseTime>2008-04-24T10:05:12Z</WorkingAreaEraseTime>"
    "<ProxyExpirationTime>2008-04-30T10:05:12Z</ProxyExpirationTime>"
    "<SubmissionHost>pc4.niif.hu:3432</SubmissionHost>"
    "<SubmissionClientName>nordugrid-arc-0.94</SubmissionClientName>"
    "<OtherMessages>Cached input file is outdated; downloading again</OtherMessages>"
    "<OtherMessages>User proxy has expired</OtherMessages>"
    "<Associations>"
      "<ActivityOldID>https://example-ce.com:443/arex/765234</ActivityOldID>"
      "<ActivityOldID>https://helloworld-ce.com:12345/arex/543678</ActivityOldID>"
      "<LocalInputFile>"
        "<Source>helloworld.sh</Source>"
        "<CheckSum>c0489bec6f7f4454d6cfe1b0a07ad5b8</CheckSum>"
      "</LocalInputFile>"
      "<LocalInputFile>"
        "<Source>random.dat</Source>"
        "<CheckSum>e52b14b10b967d9135c198fd11b9b8bc</CheckSum>"
      "</LocalInputFile>"
    "</Associations>"
  "</ComputingActivity>")) {}

void JobTest::XMLToJobTest() {
  Arc::Job job;
  job = xmlJob;

  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex/HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm", job.JobID);
  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", job.Name);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.ServiceInformationURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.resourceinfo", job.ServiceInformationInterfaceName);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.JobStatusURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.activitymanagement", job.JobStatusInterfaceName);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.JobManagementURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.activitymanagement", job.JobManagementInterfaceName);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.StageInDir);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.StageOutDir);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://testbed-emi4.grid.upjs.sk:60000/arex"), job.SessionDir);
  CPPUNIT_ASSERT_EQUAL((std::string)"single", job.Type);
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", job.LocalIDFromManager);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", job.JobDescription);
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", job.JobDescriptionDocument);
  CPPUNIT_ASSERT(job.State == Arc::JobState::OTHER);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:failed", job.State());
  CPPUNIT_ASSERT(job.RestartState == Arc::JobState::OTHER);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:running", job.RestartState());
  CPPUNIT_ASSERT_EQUAL(0, job.ExitCode);
  CPPUNIT_ASSERT_EQUAL((std::string)"0", job.ComputingManagerExitCode);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.Error.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Uploading timed out", job.Error.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"Failed stage-out", job.Error.back());
  CPPUNIT_ASSERT_EQUAL(0, job.WaitingPosition);
  CPPUNIT_ASSERT_EQUAL((std::string)"vo:atlas", job.UserDomain);
  CPPUNIT_ASSERT_EQUAL((std::string)"CONFIDENTIAL", job.Owner);
  CPPUNIT_ASSERT_EQUAL((std::string)"grid02", job.LocalOwner);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(5000), job.RequestedTotalWallTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(20000), job.RequestedTotalCPUTime);
  CPPUNIT_ASSERT_EQUAL(4, job.RequestedSlots);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.RequestedApplicationEnvironment.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ENV/JAVA/JRE-1.6.0", job.RequestedApplicationEnvironment.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"APPS/HEP/ATLAS-14.2.23.4", job.RequestedApplicationEnvironment.back());
  CPPUNIT_ASSERT_EQUAL((std::string)"input.dat", job.StdIn);
  CPPUNIT_ASSERT_EQUAL((std::string)"job.out", job.StdOut);
  CPPUNIT_ASSERT_EQUAL((std::string)"err.out", job.StdErr);
  CPPUNIT_ASSERT_EQUAL((std::string)"celog", job.LogDir);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.ExecutionNode.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"wn043", job.ExecutionNode.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"wn056", job.ExecutionNode.back());
  CPPUNIT_ASSERT_EQUAL((std::string)"pbs-short", job.Queue);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(2893), job.UsedTotalWallTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(12340), job.UsedTotalCPUTime);
  CPPUNIT_ASSERT_EQUAL(4453, job.UsedMainMemory);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-21T10:04:36Z"), job.LocalSubmissionTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-21T10:05:12Z"), job.SubmissionTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T06:05:12Z"), job.ComputingManagerSubmissionTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T06:45:12Z"), job.StartTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T10:05:12Z"), job.ComputingManagerEndTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T10:15:12Z"), job.EndTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-24T10:05:12Z"), job.WorkingAreaEraseTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-30T10:05:12Z"), job.ProxyExpirationTime);
  CPPUNIT_ASSERT_EQUAL((std::string)"pc4.niif.hu:3432", job.SubmissionHost);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid-arc-0.94", job.SubmissionClientName);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.OtherMessages.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Cached input file is outdated; downloading again", job.OtherMessages.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"User proxy has expired", job.OtherMessages.back());
  CPPUNIT_ASSERT_EQUAL(2, (int)job.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", job.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", job.ActivityOldID.back());
  CPPUNIT_ASSERT_EQUAL(2, (int)job.LocalInputFiles.size());
  std::map<std::string, std::string>::const_iterator itFiles = job.LocalInputFiles.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", itFiles->second);
  itFiles++;
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", itFiles->second);
}

void JobTest::JobToXMLTest() {
  Arc::Job job;
  job = xmlJob;
  Arc::XMLNode xmlOut("<ComputingActivity/>");

  job.ToXML(xmlOut);

  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex/HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm", (std::string)xmlOut["JobID"]); xmlOut["JobID"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", (std::string)xmlOut["Name"]); xmlOut["Name"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["ServiceInformationURL"]); xmlOut["ServiceInformationURL"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.resourceinfo", (std::string)xmlOut["ServiceInformationInterfaceName"]); xmlOut["ServiceInformationInterfaceName"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["JobStatusURL"]); xmlOut["JobStatusURL"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.activitymanagement", (std::string)xmlOut["JobStatusInterfaceName"]); xmlOut["JobStatusInterfaceName"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["JobManagementURL"]); xmlOut["JobManagementURL"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"org.ogf.glue.emies.activitymanagement", (std::string)xmlOut["JobManagementInterfaceName"]); xmlOut["JobManagementInterfaceName"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["StageInDir"]); xmlOut["StageInDir"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["StageOutDir"]); xmlOut["StageOutDir"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://testbed-emi4.grid.upjs.sk:60000/arex", (std::string)xmlOut["SessionDir"]); xmlOut["SessionDir"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"single", (std::string)xmlOut["Type"]); xmlOut["Type"].Destroy();
  CPPUNIT_ASSERT(xmlOut["IDFromEndpoint"]); xmlOut["IDFromEndpoint"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", (std::string)xmlOut["LocalIDFromManager"]); xmlOut["LocalIDFromManager"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", (std::string)xmlOut["JobDescription"]); xmlOut["JobDescription"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", (std::string)xmlOut["JobDescriptionDocument"]); xmlOut["JobDescriptionDocument"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["State"]["General"]); xmlOut["State"]["General"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:failed", (std::string)xmlOut["State"]["Specific"]); xmlOut["State"]["Specific"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["State"].Size()); xmlOut["State"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["RestartState"]["General"]); xmlOut["RestartState"]["General"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:running", (std::string)xmlOut["RestartState"]["Specific"]); xmlOut["RestartState"]["Specific"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["RestartState"].Size()); xmlOut["RestartState"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ExitCode"]); xmlOut["ExitCode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ComputingManagerExitCode"]); xmlOut["ComputingManagerExitCode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Uploading timed out", (std::string)xmlOut["Error"]); xmlOut["Error"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Failed stage-out", (std::string)xmlOut["Error"]); xmlOut["Error"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["WaitingPosition"]); xmlOut["WaitingPosition"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"vo:atlas", (std::string)xmlOut["UserDomain"]); xmlOut["UserDomain"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"CONFIDENTIAL", (std::string)xmlOut["Owner"]); xmlOut["Owner"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"grid02", (std::string)xmlOut["LocalOwner"]); xmlOut["LocalOwner"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT1H23M20S", (std::string)xmlOut["RequestedTotalWallTime"]); xmlOut["RequestedTotalWallTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT5H33M20S", (std::string)xmlOut["RequestedTotalCPUTime"]); xmlOut["RequestedTotalCPUTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"4", (std::string)xmlOut["RequestedSlots"]); xmlOut["RequestedSlots"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"ENV/JAVA/JRE-1.6.0", (std::string)xmlOut["RequestedApplicationEnvironment"]); xmlOut["RequestedApplicationEnvironment"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"APPS/HEP/ATLAS-14.2.23.4", (std::string)xmlOut["RequestedApplicationEnvironment"]); xmlOut["RequestedApplicationEnvironment"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"input.dat", (std::string)xmlOut["StdIn"]); xmlOut["StdIn"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"job.out", (std::string)xmlOut["StdOut"]); xmlOut["StdOut"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"err.out", (std::string)xmlOut["StdErr"]); xmlOut["StdErr"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"celog", (std::string)xmlOut["LogDir"]); xmlOut["LogDir"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"wn043", (std::string)xmlOut["ExecutionNode"]); xmlOut["ExecutionNode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"wn056", (std::string)xmlOut["ExecutionNode"]); xmlOut["ExecutionNode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"pbs-short", (std::string)xmlOut["Queue"]); xmlOut["Queue"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT48M13S", (std::string)xmlOut["UsedTotalWallTime"]); xmlOut["UsedTotalWallTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT3H25M40S", (std::string)xmlOut["UsedTotalCPUTime"]); xmlOut["UsedTotalCPUTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"4453", (std::string)xmlOut["UsedMainMemory"]); xmlOut["UsedMainMemory"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-21T10:04:36Z", (std::string)xmlOut["LocalSubmissionTime"]); xmlOut["LocalSubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-21T10:05:12Z", (std::string)xmlOut["SubmissionTime"]); xmlOut["SubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T06:05:12Z", (std::string)xmlOut["ComputingManagerSubmissionTime"]); xmlOut["ComputingManagerSubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T06:45:12Z", (std::string)xmlOut["StartTime"]); xmlOut["StartTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T10:05:12Z", (std::string)xmlOut["ComputingManagerEndTime"]); xmlOut["ComputingManagerEndTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T10:15:12Z", (std::string)xmlOut["EndTime"]); xmlOut["EndTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-24T10:05:12Z", (std::string)xmlOut["WorkingAreaEraseTime"]); xmlOut["WorkingAreaEraseTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-30T10:05:12Z", (std::string)xmlOut["ProxyExpirationTime"]); xmlOut["ProxyExpirationTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"pc4.niif.hu:3432", (std::string)xmlOut["SubmissionHost"]); xmlOut["SubmissionHost"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid-arc-0.94", (std::string)xmlOut["SubmissionClientName"]); xmlOut["SubmissionClientName"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Cached input file is outdated; downloading again", (std::string)xmlOut["OtherMessages"]); xmlOut["OtherMessages"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"User proxy has expired", (std::string)xmlOut["OtherMessages"]); xmlOut["OtherMessages"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", (std::string)xmlOut["Associations"]["ActivityOldID"]); xmlOut["Associations"]["ActivityOldID"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", (std::string)xmlOut["Associations"]["ActivityOldID"]); xmlOut["Associations"]["ActivityOldID"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", (std::string)xmlOut["Associations"]["LocalInputFile"]["Source"]); xmlOut["Associations"]["LocalInputFile"]["Source"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", (std::string)xmlOut["Associations"]["LocalInputFile"]["CheckSum"]); xmlOut["Associations"]["LocalInputFile"]["CheckSum"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"]["LocalInputFile"].Size()); xmlOut["Associations"]["LocalInputFile"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", (std::string)xmlOut["Associations"]["LocalInputFile"]["Source"]); xmlOut["Associations"]["LocalInputFile"]["Source"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", (std::string)xmlOut["Associations"]["LocalInputFile"]["CheckSum"]); xmlOut["Associations"]["LocalInputFile"]["CheckSum"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"]["LocalInputFile"].Size()); xmlOut["Associations"]["LocalInputFile"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"].Size()); xmlOut["Associations"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut.Size());

  Arc::Job emptyJob;
  emptyJob.ToXML(xmlOut);
  CPPUNIT_ASSERT_EQUAL(0, xmlOut.Size());
}

void JobTest::XMLToJobStateTest() {
  Arc::XMLNode xml(
  "<ComputingActivity>"
    "<State>"
      "<General>Preparing</General>"
      "<Specific>PREPARING</Specific>"
    "</State>"
  "</ComputingActivity>"
  );
  Arc::Job job;
  job = xml;
  CPPUNIT_ASSERT_EQUAL((std::string)"PREPARING", job.State());
  CPPUNIT_ASSERT_EQUAL((std::string)"Preparing", job.State.GetGeneralState());
}

void JobTest::VersionTwoFormatTest() {
  Arc::XMLNode xml(
  "<ComputingActivity>"
    "<JobID>gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112</JobID>"
    "<Cluster>ldap://example-ce.nordugrid.org:2135/Mds-Vo-name=local,o=Grid??base?(objectClass=*)</Cluster>"
    "<InterfaceName>org.nordugrid.gridftpjob</InterfaceName>"
    "<IDFromEndpoint>ldap://example-ce.nordugrid.org:2135/Mds-Vo-name=local,o=Grid??sub?(nordugrid-job-globalid=gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112)</IDFromEndpoint>"
    "<LocalSubmissionTime>2010-09-24 16:17:46</LocalSubmissionTime>"
    "<JobDescription>&amp;(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")</JobDescription>"
    "<OldJobID>gsiftp://example-ce.nordugrid.org:2811/jobs/765234</OldJobID>"
    "<OldJobID>https://helloworld-ce.nordugrid.org:12345/arex/543678</OldJobID>"
    "<LocalInputFiles>"
      "<File>"
        "<Source>helloworld.sh</Source>"
        "<CheckSum>c0489bec6f7f4454d6cfe1b0a07ad5b8</CheckSum>"
      "</File>"
      "<File>"
        "<Source>random.dat</Source>"
        "<CheckSum>e52b14b10b967d9135c198fd11b9b8bc</CheckSum>"
      "</File>"
    "</LocalInputFiles>"
  "</ComputingActivity>"
  );
  Arc::Job job;
  job = xml;

  CPPUNIT_ASSERT_EQUAL((std::string)"gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112", job.JobID);
  
  CPPUNIT_ASSERT_EQUAL(Arc::URL("ldap://example-ce.nordugrid.org:2135/Mds-Vo-name=local,o=Grid??base?(objectClass=*)"), job.ServiceInformationURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapng", job.ServiceInformationInterfaceName);

  CPPUNIT_ASSERT_EQUAL(Arc::URL("ldap://example-ce.nordugrid.org:2135/Mds-Vo-name=local,o=Grid??sub?(nordugrid-job-globalid=gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112)"), job.JobStatusURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapng", job.JobStatusInterfaceName);
  
  CPPUNIT_ASSERT_EQUAL(Arc::URL("gsiftp://example-ce.nordugrid.org:2811/jobs"), job.JobManagementURL);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", job.JobManagementInterfaceName);

  CPPUNIT_ASSERT_EQUAL(Arc::URL("gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112"), job.StageInDir);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112"), job.StageOutDir);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("gsiftp://example-ce.nordugrid.org:2811/jobs/3456789101112"), job.SessionDir);

  CPPUNIT_ASSERT_EQUAL((std::string)"3456789101112", job.IDFromEndpoint);

  CPPUNIT_ASSERT_EQUAL(Arc::Time("2010-09-24 16:17:46"), job.LocalSubmissionTime);
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", job.JobDescriptionDocument);

  CPPUNIT_ASSERT_EQUAL(2, (int)job.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"gsiftp://example-ce.nordugrid.org:2811/jobs/765234", job.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.nordugrid.org:12345/arex/543678", job.ActivityOldID.back());

  CPPUNIT_ASSERT_EQUAL(2, (int)job.LocalInputFiles.size());
  std::map<std::string, std::string>::const_iterator itFiles = job.LocalInputFiles.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", itFiles->second);
  itFiles++;
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", itFiles->second);
}

void JobTest::VersionOneFormatTest() {
  Arc::XMLNode xml(
  "<ComputingActivity>"
    "<IDFromEndpoint>gsiftp://grid.example.com:2811/jobs/1234567890</IDFromEndpoint>"
    "<Flavour>ARC0</Flavour>"
    "<Cluster>ldap://grid.example.com:2135/Mds-Vo-name=local, o=Grid??sub?(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue)(nordugrid-authuser-sn=somedn))</Cluster>"
    "<InfoEndpoint>ldap://grid.example.com:2135/Mds-Vo-name=local, o=Grid??sub?(nordugrid-job-globalid=gsiftp:\\2f\\2fgrid.example.com:2811\\2fjobs\\2f1234567890)</InfoEndpoint>"
    "<LocalSubmissionTime>2010-09-24 16:17:46</LocalSubmissionTime>"
    "<JobDescription>&amp;(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")</JobDescription>"
    "<OldJobID>https://example-ce.com:443/arex/765234</OldJobID>"
    "<OldJobID>https://helloworld-ce.com:12345/arex/543678</OldJobID>"
    "<LocalInputFiles>"
      "<File>"
        "<Source>helloworld.sh</Source>"
        "<CheckSum>c0489bec6f7f4454d6cfe1b0a07ad5b8</CheckSum>"
      "</File>"
      "<File>"
        "<Source>random.dat</Source>"
        "<CheckSum>e52b14b10b967d9135c198fd11b9b8bc</CheckSum>"
      "</File>"
    "</LocalInputFiles>"
  "</ComputingActivity>"
  );
  Arc::Job job;
  job = xml;

  CPPUNIT_ASSERT_EQUAL((std::string)"gsiftp://grid.example.com:2811/jobs/1234567890", job.JobID);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", job.JobManagementInterfaceName);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("ldap://grid.example.com:2135/Mds-Vo-name=local, o=Grid??sub?(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue)(nordugrid-authuser-sn=somedn))").fullstr(), job.ServiceInformationURL.fullstr());
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2010-09-24 16:17:46"), job.LocalSubmissionTime);
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", job.JobDescriptionDocument);

  CPPUNIT_ASSERT_EQUAL(2, (int)job.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", job.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", job.ActivityOldID.back());

  CPPUNIT_ASSERT_EQUAL(2, (int)job.LocalInputFiles.size());
  std::map<std::string, std::string>::const_iterator itFiles = job.LocalInputFiles.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", itFiles->second);
  itFiles++;
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", itFiles->second);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobTest);
