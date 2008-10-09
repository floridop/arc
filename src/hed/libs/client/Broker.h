#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <vector>

#include <arc/Logger.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class Broker {
       public:
                 ExecutionTarget& get_Target();
       protected:
	 Broker( Arc::TargetGenerator& targen,  Arc::JobDescription jobd );
                 virtual ~Broker();
	 virtual void sort_Targets()=0;
	 
	 std::vector<Arc::ExecutionTarget> found_Targets;
       private:	 
	 std::vector<Arc::ExecutionTarget>::iterator current;		//current Target for a Job
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__
