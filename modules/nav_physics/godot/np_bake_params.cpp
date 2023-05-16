#include "np_bake_params.h"

void NPBakeParams::_bind_methods() {
	BIND_ENUM_CONSTANT(PARAM_ENABLED_FILTER_LOW_HANGING_OBSTACLES);
	BIND_ENUM_CONSTANT(PARAM_ENABLED_FILTER_LEDGE_SPANS);
	BIND_ENUM_CONSTANT(PARAM_ENABLED_FILTER_WALKABLE_LOW_HEIGHT_SPANS);

	BIND_ENUM_CONSTANT(PARAM_CELL_SIZE);
	BIND_ENUM_CONSTANT(PARAM_CELL_HEIGHT);
	BIND_ENUM_CONSTANT(PARAM_AGENT_HEIGHT);
	BIND_ENUM_CONSTANT(PARAM_AGENT_RADIUS);
	BIND_ENUM_CONSTANT(PARAM_AGENT_MAX_CLIMB);
	BIND_ENUM_CONSTANT(PARAM_AGENT_MAX_SLOPE);
	BIND_ENUM_CONSTANT(PARAM_REGION_MIN_SIZE);
	BIND_ENUM_CONSTANT(PARAM_REGION_MERGE_SIZE);
	BIND_ENUM_CONSTANT(PARAM_EDGE_MAX_LENGTH);
	BIND_ENUM_CONSTANT(PARAM_EDGE_MAX_ERROR);
	BIND_ENUM_CONSTANT(PARAM_VERTS_PER_POLY);
	BIND_ENUM_CONSTANT(PARAM_DETAIL_SAMPLE_DISTANCE);
	BIND_ENUM_CONSTANT(PARAM_DETAIL_SAMPLE_MAX_ERROR);

	BIND_ENUM_CONSTANT(PARAM_EXIT_LIP);
	BIND_ENUM_CONSTANT(PARAM_EXIT_MAX_STEP_UP);
	BIND_ENUM_CONSTANT(PARAM_EXIT_MAX_DROP);

	BIND_ENUM_CONSTANT(SAMPLE_PARTITION_WATERSHED);
	BIND_ENUM_CONSTANT(SAMPLE_PARTITION_MONOTONE);
	BIND_ENUM_CONSTANT(SAMPLE_PARTITION_LAYERS);

	BIND_ENUM_CONSTANT(PARSED_GEOMETRY_MESH_INSTANCES);
	BIND_ENUM_CONSTANT(PARSED_GEOMETRY_STATIC_COLLIDERS);
	BIND_ENUM_CONSTANT(PARSED_GEOMETRY_BOTH);

	BIND_ENUM_CONSTANT(SOURCE_GEOMETRY_NAVMESH_CHILDREN);
	BIND_ENUM_CONSTANT(SOURCE_GEOMETRY_GROUPS_WITH_CHILDREN);
	BIND_ENUM_CONSTANT(SOURCE_GEOMETRY_GROUPS_EXPLICIT);
}

NPBakeParams::Data::Data() {
	params[PARAM_CELL_SIZE] = 0.25;
	params[PARAM_CELL_HEIGHT] = 0.25;
	params[PARAM_AGENT_HEIGHT] = 1.5;
	params[PARAM_AGENT_RADIUS] = 0.5;
	params[PARAM_AGENT_MAX_CLIMB] = 0.25;
	params[PARAM_AGENT_MAX_SLOPE] = 45;
	params[PARAM_REGION_MIN_SIZE] = 2;
	params[PARAM_REGION_MERGE_SIZE] = 20;
	params[PARAM_EDGE_MAX_LENGTH] = 12;
	params[PARAM_EDGE_MAX_ERROR] = 1.3;
	params[PARAM_VERTS_PER_POLY] = 6;
	params[PARAM_DETAIL_SAMPLE_DISTANCE] = 6;
	params[PARAM_DETAIL_SAMPLE_MAX_ERROR] = 1;

	params[PARAM_EXIT_LIP] = 0.1;
	params[PARAM_EXIT_MAX_STEP_UP] = 0.5;
	params[PARAM_EXIT_MAX_DROP] = 1.0;

	params_enabled[PARAM_ENABLED_FILTER_LOW_HANGING_OBSTACLES] = false;
	params_enabled[PARAM_ENABLED_FILTER_LEDGE_SPANS] = false;
	params_enabled[PARAM_ENABLED_FILTER_WALKABLE_LOW_HEIGHT_SPANS] = false;
}

void NPBakeParams::set_param_enabled(ParamEnabled p_param, bool p_enabled) {
	data.params_enabled[p_param] = p_enabled;
}

bool NPBakeParams::get_param_enabled(ParamEnabled p_param) const {
	return data.params_enabled[p_param];
}

void NPBakeParams::set_param(Param p_param, float p_value) {
	ERR_FAIL_COND(p_value < 0);
	data.params[p_param] = p_value;
}

float NPBakeParams::get_param(Param p_param) const {
	return data.params[p_param];
}

void NPBakeParams::set_sample_partition_type(SamplePartitionType p_value) {
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NPBakeParams::SamplePartitionType NPBakeParams::get_sample_partition_type() const {
	return partition_type;
}

void NPBakeParams::set_parsed_geometry_type(ParsedGeometryType p_value) {
	ERR_FAIL_INDEX(p_value, PARSED_GEOMETRY_MAX);
	parsed_geometry_type = p_value;
	//_change_notify();
}

NPBakeParams::ParsedGeometryType NPBakeParams::get_parsed_geometry_type() const {
	return parsed_geometry_type;
}

void NPBakeParams::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
}

uint32_t NPBakeParams::get_collision_mask() const {
	return collision_mask;
}

void NPBakeParams::set_collision_mask_bit(int p_bit, bool p_value) {
	ERR_FAIL_INDEX_MSG(p_bit, 32, "Collision mask bit must be between 0 and 31 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << p_bit;
	} else {
		mask &= ~(1 << p_bit);
	}
	set_collision_mask(mask);
}

bool NPBakeParams::get_collision_mask_bit(int p_bit) const {
	ERR_FAIL_INDEX_V_MSG(p_bit, 32, false, "Collision mask bit must be between 0 and 31 inclusive.");
	return get_collision_mask() & (1 << p_bit);
}

void NPBakeParams::set_source_geometry_mode(SourceGeometryMode p_geometry_mode) {
	ERR_FAIL_INDEX(p_geometry_mode, SOURCE_GEOMETRY_MAX);
	source_geometry_mode = p_geometry_mode;
	//_change_notify();
}

NPBakeParams::SourceGeometryMode NPBakeParams::get_source_geometry_mode() const {
	return source_geometry_mode;
}

void NPBakeParams::set_source_group_name(StringName p_group_name) {
	source_group_name = p_group_name;
}

StringName NPBakeParams::get_source_group_name() const {
	return source_group_name;
}
