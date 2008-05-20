#ifndef __ARC_TARGETRETRIEVERCREAM_H__
#define __ARC_TARGETRETRIEVERCREAM_H__

#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;
  class URL;

  class TargetRetrieverCREAM
    : public TargetRetriever {
    TargetRetrieverCREAM(Config *cfg);
    ~TargetRetrieverCREAM();

  public:
    void GetTargets(TargetGenerator& mom, int TargetType, int DetailLevel);
    void InterrogateTarget(TargetGenerator& mom, std::string url,
			   int TargetType, int DetailLevel);
    static ACC *Instance(Config *cfg, ChainContext *ctx);
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERCREAM_H__
