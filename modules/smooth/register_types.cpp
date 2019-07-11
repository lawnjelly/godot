/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "smooth.h"

void register_smooth_types() {

        ClassDB::register_class<Smooth>();
}

void unregister_smooth_types() {
   //nothing to do here
}
