#pragma once

#include "scene/3d/spatial.h"

//class NPBakeParams {
class NPBakeParams : public Object {
	GDCLASS(NPBakeParams, Object);

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

	enum Param {
		PARAM_CELL_SIZE,
		PARAM_CELL_HEIGHT,
		PARAM_AGENT_HEIGHT,
		PARAM_AGENT_RADIUS,
		PARAM_AGENT_MAX_CLIMB,
		PARAM_AGENT_MAX_SLOPE,
		PARAM_REGION_MIN_SIZE,
		PARAM_REGION_MERGE_SIZE,
		PARAM_EDGE_MAX_LENGTH,
		PARAM_EDGE_MAX_ERROR,
		PARAM_VERTS_PER_POLY,
		PARAM_DETAIL_SAMPLE_DISTANCE,
		PARAM_DETAIL_SAMPLE_MAX_ERROR,
		PARAM_EXIT_LIP,
		PARAM_EXIT_MAX_STEP_UP,
		PARAM_EXIT_MAX_DROP,
		PARAM_MAX,
	};

	enum ParamEnabled {
		PARAM_ENABLED_FILTER_LOW_HANGING_OBSTACLES,
		PARAM_ENABLED_FILTER_LEDGE_SPANS,
		PARAM_ENABLED_FILTER_WALKABLE_LOW_HEIGHT_SPANS,
		PARAM_ENABLED_MAX,
	};

protected:
	struct Data {
		float params[PARAM_MAX];
		bool params_enabled[PARAM_ENABLED_MAX];
		Data();
	} data;

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

	void set_param_enabled(ParamEnabled p_param, bool p_enabled);
	bool get_param_enabled(ParamEnabled p_param) const;

	void set_param(Param p_param, float p_value);
	float get_param(Param p_param) const;

	static void _bind_methods();

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
};

VARIANT_ENUM_CAST(NPBakeParams::Param);
VARIANT_ENUM_CAST(NPBakeParams::ParamEnabled);
VARIANT_ENUM_CAST(NPBakeParams::SamplePartitionType);
VARIANT_ENUM_CAST(NPBakeParams::ParsedGeometryType);
VARIANT_ENUM_CAST(NPBakeParams::SourceGeometryMode);
