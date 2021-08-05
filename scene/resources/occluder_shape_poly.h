#pragma once

#include "core/math/geometry.h"
#include "occluder_shape.h"

class Spatial;

class OccluderShapePoly : public OccluderShape {
	GDCLASS(OccluderShapePoly, OccluderShape);

	Geometry::MeshData _mesh_data;
	NodePath _bake_path;

	void _bake_recursive(Spatial *p_node);
	void _try_bake_face(const Face3 &p_face);
	void _sort_face_winding(Geometry::MeshData::Face &r_face);

	void set_bake_path(const NodePath &p_path) { _bake_path = p_path; }
	NodePath get_bake_path() { return _bake_path; }

protected:
	static void _bind_methods();

public:
	//	void set_spheres(const Vector<Plane> &p_spheres);
	//	Vector<Plane> get_spheres() const { return _spheres; }

	//	void set_sphere_position(int p_idx, const Vector3 &p_position);
	//	void set_sphere_radius(int p_idx, real_t p_radius);
	const Geometry::MeshData &get_mesh_data() const { return _mesh_data; }

	virtual void notification_enter_world(RID p_scenario);
	virtual void update_shape_to_visual_server();
	virtual void update_transform_to_visual_server(const Transform &p_global_xform);
	virtual Transform center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap);

	void clear();
	void bake(Node *owner);

	OccluderShapePoly();
};
