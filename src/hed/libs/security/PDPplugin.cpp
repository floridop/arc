#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/PDPLoader.h>
#include "SimpleListPDP.h"
#include "ArcPDP.h"

using namespace Arc;

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &Arc::SimpleListPDP::get_simplelist_pdp},
    { "arc.pdp", 0, &Arc::ArcPDP::get_arc_pdp},
    { NULL, 0, NULL }
};

