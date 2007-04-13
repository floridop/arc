#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include "common/ArcConfig.h"
#include "MCCLoader.h"
#include "MCCFactory.h"
#include "Service.h"
#include "ServiceFactory.h"
#include "Plexer.h"

namespace Arc {

/** This class processes XML configration and creates message chains.
  Accepted configuration is defined by XML schema mcc.xsd.
  Supported components are of types MCC, Service and Plexer. MCC and 
 Service are loaded from dynamic libraries. For Plexer only internal
 implementation is supported.
  This object is also a container for loaded componets. All components
  are destroyed if this object is destroyed.
  Chains are created in 2 steps. First all components are loaded and
 corresponding objects are created. Constructor are supplied with 
 corresponding configuration subtrees. During next step components
 are linked together by calling their Next() methods. Each creates
 labeled link to next component in a chain.
  2 step method has an advantage over 1 step because it allows loops
 in chains and makes loading procedure simple. But that also means
 during short period of time components are only partly configured.
 Components in such state must produce proper error response if 
 Message arrives.
  Note: Current implementation requires all components and links 
 to be labeled. All labels must be unique.
*/
class Loader 
{
    public:
        typedef std::map<std::string, MCC *>     mcc_container_t;
        typedef std::map<std::string, Service *> service_container_t;
        typedef std::map<std::string, Plexer *>  plexer_container_t;

    private:
        /** Link to Factory responsible for loading and creation of Service objects */
        ServiceFactory *service_factory;

        /** Link to Factory responsible for loading and creation of MCC objects */
        MCCFactory *mcc_factory;

        //PlexerFactory *plexer_factory;

        /** Set of labeled MCC objects */
        mcc_container_t     mccs_;

        /** Set of labeled Service objects */
        service_container_t services_;

        /** Set of labeled Plexer objects */
        plexer_container_t  plexers_;

        /** Internal method which performs whole stuff except creation of Factories. 
          It is taken out from constructor to make it easier to reconfigure chains
          in a future. */
        void make_elements(Config *cfg);

    public:
        /** Constructor takes whole XML configuration and creates components' chains */
        Loader(Config *cfg);
        /** Destructor destroys all components created by constructor */
        ~Loader(void);
};

}
#endif /* __ARC_LOADER_H__ */
