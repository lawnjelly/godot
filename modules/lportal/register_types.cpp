/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "lroom_manager.h"
#include "lightmapper/llightmapper.h"
#include "lightmapper/llightmapper_editor_plugin.h"


void register_lportal_types() {

	ClassDB::register_class<LRoomManager>();
	ClassDB::register_class<LLightMapper>();

#ifdef TOOLS_ENABLED
    EditorPlugins::add_by_type<LLightMapperEditorPlugin>();
#endif
}

void unregister_lportal_types() {
   //nothing to do here
}
