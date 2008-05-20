#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DataPointLFC.h"
#include "DMCLFC.h"

namespace Arc {

  Logger DMCLFC::logger(DMC::logger, "LFC");

  DMCLFC::DMCLFC(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCLFC::~DMCLFC() {
    Unregister(this);
  }

  DMC *DMCLFC::Instance(Config *cfg, ChainContext *) {
    return new DMCLFC(cfg);
  }

  DataPoint *DMCLFC::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "lfc")
      return NULL;
    return new DataPointLFC(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  {"lfc", 0, &Arc::DMCLFC::Instance},
  {NULL, 0, NULL}
};
