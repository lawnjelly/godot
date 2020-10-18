/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "lroom_manager.h"
#include "lroom.h"
#include "lportal.h"
#include "lportal_editor_plugin.h"

void register_lportal_types() {

	ClassDB::register_class<LRoomManager>();
	ClassDB::register_class<LRoom>();
	ClassDB::register_class<LPortal>();

#ifdef TOOLS_ENABLED
    EditorPlugins::add_by_type<LPortalEditorPlugin>();
#endif
}

void unregister_lportal_types() {
   //nothing to do here
}
