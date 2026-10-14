#pragma once

#include "np_mesh.h"
#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPPoint;

class NPMeshInstance : public Spatial {
	GDCLASS(NPMeshInstance, Spatial);

	friend class NPPoint;

	struct PointRef {
		NPPoint *point = nullptr;
		//uint32_t nav_physics_id = UINT32_MAX;
		bool operator==(const PointRef &p_other) const {
			return this->point == p_other.point;
		}
	};

	struct Data {
		np_handle h_mesh_instance = 0;
		Ref<NPMesh> mesh;

		LocalVector<PointRef> points;
	} data;

	struct DebugData {
		RID debug_polys;
		bool show_debug_visuals = false;
	} debug_data;

	void resource_changed(RES res);
	void _update_server();
	void _update_visibility();
	bool _refresh_debug_visuals();
	void resync_points();

	void point_add(NPPoint &p_point);
	void point_remove(NPPoint &p_point);
	void point_notify_transform(NPPoint &p_point);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_mesh(const Ref<NPMesh> &p_mesh);
	Ref<NPMesh> get_mesh() const;
	Vector3 choose_random_location() const;

	void set_debug_visuals(bool p_enable);
	bool has_debug_visuals() const { return debug_data.show_debug_visuals; }

	String get_configuration_warning() const;

	NPMeshInstance();
	~NPMeshInstance();
};
