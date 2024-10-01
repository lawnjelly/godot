#pragma once

#include "editor/editor_plugin.h"
#include "editor/spatial_editor_gizmos.h"

class NavPhysicsMeshSpatialGizmoPlugin : public EditorSpatialGizmoPlugin {
	GDCLASS(NavPhysicsMeshSpatialGizmoPlugin, EditorSpatialGizmoPlugin);

public:
	bool has_gizmo(Spatial *p_spatial);
	String get_name() const;
	int get_priority() const;
	void redraw(EditorSpatialGizmo *p_gizmo);

	NavPhysicsMeshSpatialGizmoPlugin();
};
