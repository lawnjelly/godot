#pragma once

#include "scene/3d/spatial.h"

class NPAgentClass {
public:
};

class NPBakeParams {
	friend class NPMesh;

public:
	enum SamplePartitionType {
		SAMPLE_PARTITION_WATERSHED = 0,
		SAMPLE_PARTITION_MONOTONE,
		SAMPLE_PARTITION_LAYERS,
		SAMPLE_PARTITION_MAX
	};

	enum ParsedGeometryType {
		PARSED_GEOMETRY_MESH_INSTANCES = 0,
		PARSED_GEOMETRY_STATIC_COLLIDERS,
		PARSED_GEOMETRY_BOTH,
		PARSED_GEOMETRY_MAX
	};

	enum SourceGeometryMode {
		SOURCE_GEOMETRY_NAVMESH_CHILDREN = 0,
		SOURCE_GEOMETRY_GROUPS_WITH_CHILDREN,
		SOURCE_GEOMETRY_GROUPS_EXPLICIT,
		SOURCE_GEOMETRY_MAX
	};

protected:
	float cell_size = 0.25f;
	float cell_height = 0.25f;
	float agent_height = 1.5f;
	float agent_radius = 0.5f;
	float agent_max_climb = 0.25f;
	float agent_max_slope = 45.0f;
	float region_min_size = 2.0f;
	float region_merge_size = 20.0f;
	float edge_max_length = 12.0f;
	float edge_max_error = 1.3f;
	float verts_per_poly = 6.0f;
	float detail_sample_distance = 6.0f;
	float detail_sample_max_error = 1.0f;

	SamplePartitionType partition_type = SAMPLE_PARTITION_WATERSHED;
	ParsedGeometryType parsed_geometry_type = PARSED_GEOMETRY_MESH_INSTANCES;
	uint32_t collision_mask = 0xFFFFFFFF;

	SourceGeometryMode source_geometry_mode = SOURCE_GEOMETRY_NAVMESH_CHILDREN;
	StringName source_group_name = "navmesh";

	bool filter_low_hanging_obstacles = false;
	bool filter_ledge_spans = false;
	bool filter_walkable_low_height_spans = false;
	AABB filter_baking_aabb;
	Vector3 filter_baking_aabb_offset;

public:
	// Recast settings
	void set_sample_partition_type(SamplePartitionType p_value);
	SamplePartitionType get_sample_partition_type() const;

	void set_parsed_geometry_type(ParsedGeometryType p_value);
	ParsedGeometryType get_parsed_geometry_type() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_bit(int p_bit, bool p_value);
	bool get_collision_mask_bit(int p_bit) const;

	void set_source_geometry_mode(SourceGeometryMode p_geometry_mode);
	SourceGeometryMode get_source_geometry_mode() const;

	void set_source_group_name(StringName p_group_name);
	StringName get_source_group_name() const;

	void set_cell_size(float p_value);
	float get_cell_size() const;

	void set_cell_height(float p_value);
	float get_cell_height() const;

	void set_agent_height(float p_value);
	float get_agent_height() const;

	void set_agent_radius(float p_value);
	float get_agent_radius() const;

	void set_agent_max_climb(float p_value);
	float get_agent_max_climb() const;

	void set_agent_max_slope(float p_value);
	float get_agent_max_slope() const;

	void set_region_min_size(float p_value);
	float get_region_min_size() const;

	void set_region_merge_size(float p_value);
	float get_region_merge_size() const;

	void set_edge_max_length(float p_value);
	float get_edge_max_length() const;

	void set_edge_max_error(float p_value);
	float get_edge_max_error() const;

	void set_verts_per_poly(float p_value);
	float get_verts_per_poly() const;

	void set_detail_sample_distance(float p_value);
	float get_detail_sample_distance() const;

	void set_detail_sample_max_error(float p_value);
	float get_detail_sample_max_error() const;

	void set_filter_low_hanging_obstacles(bool p_value);
	bool get_filter_low_hanging_obstacles() const;

	void set_filter_ledge_spans(bool p_value);
	bool get_filter_ledge_spans() const;

	void set_filter_walkable_low_height_spans(bool p_value);
	bool get_filter_walkable_low_height_spans() const;
};

class NPAgent : public Spatial {
	GDCLASS(NPAgent, Spatial);
};
