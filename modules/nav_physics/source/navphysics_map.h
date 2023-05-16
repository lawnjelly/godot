#pragma once

#include "navphysics_path.h"
#include "navphysics_pooled_list.h"
#include "navphysics_sap.h"

#define NPWORLD NavPhysics::g_world

namespace NavPhysics {

typedef void (*np_agent_callback)(u64 p_user_data, const FPoint3 &p_position, const FPoint3 &p_velocity);

class Mesh;
class MeshInstance;
class Region;
struct Agent;

class Map {
	TrackedPooledList<u32> _mesh_instances;
	SAP _sap;
	u32 _map_id = UINT32_MAX;
	bool update_agent_mesh(Agent &r_agent, u32 p_agent_id, bool p_teleport_if_changed);

public:
	struct IterateResult {
		FPoint3 position;
		FPoint3 velocity;
	};

	void clear();
	void tick_update(freal p_delta);

	void body_teleport(Agent &r_agent, u32 p_agent_id, const FPoint3 &p_pos);
	void body_teleport_to_agent_status_jump_target(Agent &r_agent, u32 p_agent_id);
	bool iterate_agent(u32 p_agent_id, IterateResult &r_result);

	void register_body(u32 p_body_id);
	void unregister_body(u32 p_body_id);

	u32 register_mesh_instance(u32 p_mesh_instance_id);
	void unregister_mesh_instance(u32 p_mesh_instance_id, u32 p_mesh_slot_id);

	u32 find_best_fit_agent_mesh(Agent &r_agent, const FPoint3 &p_world_pos, u32 p_ignore_mesh_id, const JumpFinderData *p_jump_data = nullptr) const;

	void set_map_id(u32 p_id) { _map_id = p_id; }

	~Map();
};

class World {
	friend class NavPhysicsServer;

	struct MapContainer {
		u32 revision;
		Map *map;
	};

	struct RegionContainer {
		u32 revision;
		Region *region;
	};

	struct MeshContainer {
		u32 revision;
		Mesh *mesh;
	};

	struct MeshInstanceContainer {
		u32 revision;
		MeshInstance *mesh_instance;
	};

	// The pooled list zeros on first request .. this is important
	// so that we initialize the revision to zero. Other than that, it
	// is treated as a POD type.
	TrackedPooledList<MapContainer, u32, true, true> _maps;
	TrackedPooledList<RegionContainer, u32, true, true> _regions;
	TrackedPooledList<MeshContainer, u32, true, true> _meshes;
	TrackedPooledList<MeshInstanceContainer, u32, true, true> _mesh_instances;
	TrackedPooledList<Agent, u32, true, true> _agents;

	PlanStore _plan_store;

	// Create a default map for the world, to make use simpler
	// for some uses of the library.
	np_handle _default_map = 0;

	static np_agent_callback agent_callback;

	// 24 bit ID, 8 bit revision
	u32 handle_to_id(np_handle p_handle, u32 &r_revision) const {
		r_revision = p_handle;
		r_revision >>= 24;
		return p_handle & 0xffffff;
	}

	np_handle id_to_handle(u32 p_id, u32 p_revision) const {
		np_handle h;
		h = p_id;
		p_revision <<= 24;
		h |= p_revision;
		return h;
	}

	void wrapped_increment_revision(u32 &r_revision) const {
		r_revision++;
		if (r_revision >= 256) {
			// skip 0, as that is reserved for unused
			r_revision = 1;
		}
	}

public:
	void clear();
	void tick_update(u64 p_tick, freal p_delta);
	static void set_timestep(freal p_delta);
	static void set_agent_callback(np_agent_callback p_callback);

	NavPhysics::Agent *safe_get_body(np_handle p_body, u32 *r_id = nullptr);
	NavPhysics::Mesh *safe_get_mesh(np_handle p_mesh, u32 *r_id = nullptr);
	NavPhysics::MeshInstance *safe_get_mesh_instance(np_handle p_mesh_instance, u32 *r_id = nullptr);
	NavPhysics::Region *safe_get_region(np_handle p_region, u32 *r_id = nullptr);
	NavPhysics::Map *safe_get_map(np_handle p_map, u32 *r_id = nullptr);

	NavPhysics::Agent &get_body(u32 p_id) { return _agents[p_id]; }
	NavPhysics::Mesh &get_mesh(u32 p_id) { return *_meshes[p_id].mesh; }
	NavPhysics::MeshInstance &get_mesh_instance(u32 p_id) { return *_mesh_instances[p_id].mesh_instance; }
	NavPhysics::Region &get_region(u32 p_id) { return *_regions[p_id].region; }
	NavPhysics::Map &get_map(u32 p_id) { return *_maps[p_id].map; }

	const NavPhysics::Mesh &get_mesh(u32 p_id) const { return *_meshes[p_id].mesh; }
	np_handle get_mesh_instance_handle(u32 p_id) const;

	np_handle safe_body_create();
	np_handle safe_mesh_create();
	np_handle safe_mesh_instance_create();
	np_handle safe_region_create();
	np_handle safe_map_create();

	void safe_body_free(np_handle p_body);
	void safe_mesh_free(np_handle p_mesh);
	void safe_mesh_instance_free(np_handle p_mesh_instance);
	void safe_region_free(np_handle p_region);
	void safe_map_free(np_handle p_map);

	bool safe_link_mesh(np_handle p_mesh_instance, np_handle p_mesh);

	bool safe_link_mesh_instance(np_handle p_mesh_instance, np_handle p_map);
	bool safe_unlink_mesh_instance(np_handle p_mesh_instance, np_handle p_map);
	void safe_set_mesh_instance_active(np_handle p_mesh_instance, bool p_active);

	bool safe_link_body(np_handle p_body, np_handle p_map);
	bool safe_unlink_body(np_handle p_body, np_handle p_map);

	NavPhysics::Map *safe_get_default_map() { return safe_get_map(_default_map); }
	np_handle get_handle_default_map() { return _default_map; }

	bool safe_toggle_mesh_wall_connection(np_handle p_mesh, const FPoint3 &p_from, const FPoint3 &p_to, bool p_external_or_internal);

	np_handle safe_get_agent_mesh_instance_handle(np_handle p_body);
	const NavPhysics::Transform &safe_get_agent_mesh_instance_transform(np_handle p_body);

	PlanStore &get_plan_store() { return _plan_store; }

	World();
	~World();
};

extern World g_world;
} // namespace NavPhysics
