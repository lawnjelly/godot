/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "smoothing.h"

void register_smoothing_types() {

        ClassDB::register_class<Smoothing>();
}

void unregister_smoothing_types() {
   //nothing to do here
}
