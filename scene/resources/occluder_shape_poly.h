#pragma once

#include "core/local_vector.h"
#include "core/math/geometry.h"
#include "occluder_shape.h"

class Spatial;

class OccluderShapePoly : public OccluderShape {
	GDCLASS(OccluderShapePoly, OccluderShape);

	Geometry::MeshData _mesh_data;

	// used in baking
	LocalVector<real_t, int32_t> _faces_areas;
	NodePath _bake_path;
	real_t _threshold_size = 1.0;
	real_t _threshold_size_squared = 1.0;

	void _bake_recursive(Spatial *p_node);
	void _try_bake_face(const Face3 &p_face);
	bool _try_merge_face(const Face3 &p_face, int p_ignore_face);
	bool _sort_face_winding(Geometry::MeshData::Face &r_face, real_t p_new_triangle_area, real_t p_old_face_area, real_t &r_new_total_area);
	String _vec3_to_string(const Vector3 &p_pt) const;

	void set_bake_path(const NodePath &p_path) { _bake_path = p_path; }
	NodePath get_bake_path() { return _bake_path; }

	void set_threshold_size(real_t p_threshold) {
		_threshold_size = p_threshold;
		// can't be zero, we want to prevent zero area triangles which could
		// cause divide by zero in the occlusion culler goodness of fit
		_threshold_size_squared = MAX(p_threshold * p_threshold, 0.01);
	}
	real_t get_threshold_size() const { return _threshold_size; }

	void _log(String p_string);

protected:
	static void _bind_methods();

public:
	const Geometry::MeshData &get_mesh_data() const { return _mesh_data; }

	virtual void notification_enter_world(RID p_scenario);
	virtual void update_shape_to_visual_server();
	virtual void update_transform_to_visual_server(const Transform &p_global_xform);
	virtual Transform center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap);

	void clear();
	void bake(Node *owner);

	OccluderShapePoly();
};
