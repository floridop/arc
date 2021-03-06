#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "../DTR.h"

/**
 * A simple program to create a number of DTR objects. The memory
 * consumption can be checked running a program such as top.
 */
int main() {

  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");
  Arc::UserConfig cfg;

  const int no_dtrs = 10000;

  std::cout<<"Creating "<<no_dtrs<<" DTRs"<<std::endl;
  time_t start = time(NULL);

  DataStaging::DTR_ptr dtrs[no_dtrs];
  DataStaging::DTRLogger log = NULL;
  for(int i = 0; i<no_dtrs; ++i) {
    dtrs[i] = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), log);
  }

  time_t end = time(NULL);
  std::cout<<no_dtrs<<" DTRs created in "<<(end-start)<<" seconds\nSleeping for 60 seconds..."<<std::endl;

  sleep(60);
  return 0;

}
