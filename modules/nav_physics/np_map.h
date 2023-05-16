#pragma once

#include "core/local_vector.h"
#include "np_defines.h"
#include "np_sap.h"
#include "np_structs.h"

class NavMap;
class NavMesh;
class Transform;

namespace NavPhysics {
class Mesh;
class Region;

class Map {
	TrackedPooledList<uint32_t> _meshes;
	SAP _sap;
	bool update_agent_mesh(Agent &r_agent, uint32_t p_agent_id);

public:
	void clear();
	void tick_update(real_t p_delta);

	void body_teleport(Agent &r_agent, uint32_t p_agent_id, const Vector3 &p_pos);
	void iterate_agent(uint32_t p_agent_id);

	uint32_t register_mesh(uint32_t p_mesh_id);
	void unregister_mesh(uint32_t p_mesh_id, uint32_t p_mesh_slot_id);

	void register_body(uint32_t p_body_id);
	void unregister_body(uint32_t p_body_id);

	~Map();
};

class World {
	friend class NavPhysicsServer;

	struct MapContainer {
		uint32_t revision;
		Map *map;
	};

	struct RegionContainer {
		uint32_t revision;
		Region *region;
	};

	struct MeshContainer {
		uint32_t revision;
		Mesh *mesh;
	};

	// The pooled list zeros on first request .. this is important
	// so that we initialize the revision to zero. Other than that, it
	// is treated as a POD type.
	TrackedPooledList<MapContainer, uint32_t, true, true> _maps;
	TrackedPooledList<RegionContainer, uint32_t, true, true> _regions;
	TrackedPooledList<MeshContainer, uint32_t, true, true> _meshes;
	TrackedPooledList<Agent, uint32_t, true, true> _agents;

	// 24 bit ID, 8 bit revision
	uint32_t handle_to_id(np_handle p_handle, uint32_t &r_revision) const {
		r_revision = p_handle;
		r_revision >>= 24;
		return p_handle & 0xffffff;
	}

	np_handle id_to_handle(uint32_t p_id, uint32_t p_revision) const {
		np_handle h;
		h = p_id;
		p_revision <<= 24;
		h |= p_revision;
		return h;
	}

	void wrapped_increment_revision(uint32_t &r_revision) const {
		r_revision++;
		if (r_revision >= 256) {
			// skip 0, as that is reserved for unused
			r_revision = 1;
		}
	}

public:
	void clear();
	void tick_update(real_t p_delta);

	NavPhysics::Agent *safe_get_body(np_handle p_body, uint32_t *r_id = nullptr);
	NavPhysics::Mesh *safe_get_mesh(np_handle p_mesh, uint32_t *r_id = nullptr);
	NavPhysics::Region *safe_get_region(np_handle p_region, uint32_t *r_id = nullptr);
	NavPhysics::Map *safe_get_map(np_handle p_map, uint32_t *r_id = nullptr);

	NavPhysics::Agent &get_body(uint32_t p_id) { return _agents[p_id]; }
	NavPhysics::Mesh &get_mesh(uint32_t p_id) { return *_meshes[p_id].mesh; }
	NavPhysics::Region &get_region(uint32_t p_id) { return *_regions[p_id].region; }
	NavPhysics::Map &get_map(uint32_t p_id) { return *_maps[p_id].map; }

	np_handle safe_body_create();
	np_handle safe_mesh_create();
	np_handle safe_region_create();
	np_handle safe_map_create();

	void safe_body_free(np_handle p_body);
	void safe_mesh_free(np_handle p_mesh);
	void safe_region_free(np_handle p_region);
	void safe_map_free(np_handle p_map);

	World();
	~World();
};

extern World g_world;

} //namespace NavPhysics
