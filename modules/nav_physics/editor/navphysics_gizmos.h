#pragma once

#include "editor/editor_plugin.h"
#include "editor/spatial_editor_gizmos.h"

class NavPhysicsBlockSpatialGizmoPlugin : public EditorSpatialGizmoPlugin {
	GDCLASS(NavPhysicsBlockSpatialGizmoPlugin, EditorSpatialGizmoPlugin);

public:
	bool has_gizmo(Spatial *p_spatial);
	String get_name() const;
	int get_priority() const;
	void redraw(EditorSpatialGizmo *p_gizmo);

	NavPhysicsBlockSpatialGizmoPlugin();
};

class NavPhysicsPointSpatialGizmoPlugin : public EditorSpatialGizmoPlugin {
	GDCLASS(NavPhysicsPointSpatialGizmoPlugin, EditorSpatialGizmoPlugin);

public:
	bool has_gizmo(Spatial *p_spatial);
	String get_name() const;
	int get_priority() const;
	void redraw(EditorSpatialGizmo *p_gizmo);

	NavPhysicsPointSpatialGizmoPlugin();
};

class NavPhysicsMeshSpatialGizmoPlugin : public EditorSpatialGizmoPlugin {
	GDCLASS(NavPhysicsMeshSpatialGizmoPlugin, EditorSpatialGizmoPlugin);

	// Mutex _mutex;
public:
	bool has_gizmo(Spatial *p_spatial);
	String get_name() const;
	int get_priority() const;
	void redraw(EditorSpatialGizmo *p_gizmo);

	NavPhysicsMeshSpatialGizmoPlugin();
};
