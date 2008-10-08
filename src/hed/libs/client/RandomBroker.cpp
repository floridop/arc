#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/RandomBroker.h>

namespace Arc {

    RandomBroker::RandomBroker( Arc::TargetGenerator& targen) : Broker( targen ) {
          sort_Targets();
  }
  //RandomBroker::~RandomBroker() {}
  
  void RandomBroker::sort_Targets() {
              std::cout <<"Random sort: not implemented yet"<<std::endl;
              //not implemented yet
  };

} // namespace Arc
