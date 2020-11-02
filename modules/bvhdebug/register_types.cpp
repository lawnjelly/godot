/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "bvh_debug.h"


void register_bvhdebug_types() {

	ClassDB::register_class<BVHDebug>();

}

void unregister_bvhdebug_types() {
   //nothing to do here
}
