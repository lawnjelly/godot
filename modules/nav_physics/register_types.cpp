#include "register_types.h"

#include "core/class_db.h"
#include "godot/np_agent.h"
#include "godot/np_mesh.h"
#include "godot/np_region.h"

#ifdef TOOLS_ENABLED
#include "editor/navphysics_mesh_editor_plugin.h"
#endif

void register_nav_physics_types() {
	ClassDB::register_class<NPRegion>();
	ClassDB::register_class<NPMesh>();
	ClassDB::register_class<NPAgent>();

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<NPMeshEditorPlugin>();
#endif
}

void unregister_nav_physics_types() {
	//nothing to do here
}
