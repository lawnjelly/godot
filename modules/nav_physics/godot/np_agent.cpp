#include "np_agent.h"

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

void NPBakeParams::set_cell_size(float p_value) {
	ERR_FAIL_COND(p_value <= 0);
	cell_size = p_value;
}

float NPBakeParams::get_cell_size() const {
	return cell_size;
}

void NPBakeParams::set_cell_height(float p_value) {
	ERR_FAIL_COND(p_value <= 0);
	cell_height = p_value;
}

float NPBakeParams::get_cell_height() const {
	return cell_height;
}

void NPBakeParams::set_agent_height(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	agent_height = p_value;
}

float NPBakeParams::get_agent_height() const {
	return agent_height;
}

void NPBakeParams::set_agent_radius(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

float NPBakeParams::get_agent_radius() const {
	return agent_radius;
}

void NPBakeParams::set_agent_max_climb(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	agent_max_climb = p_value;
}

float NPBakeParams::get_agent_max_climb() const {
	return agent_max_climb;
}

void NPBakeParams::set_agent_max_slope(float p_value) {
	ERR_FAIL_COND(p_value < 0 || p_value > 90);
	agent_max_slope = p_value;
}

float NPBakeParams::get_agent_max_slope() const {
	return agent_max_slope;
}

void NPBakeParams::set_region_min_size(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	region_min_size = p_value;
}

float NPBakeParams::get_region_min_size() const {
	return region_min_size;
}

void NPBakeParams::set_region_merge_size(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	region_merge_size = p_value;
}

float NPBakeParams::get_region_merge_size() const {
	return region_merge_size;
}

void NPBakeParams::set_edge_max_length(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	edge_max_length = p_value;
}

float NPBakeParams::get_edge_max_length() const {
	return edge_max_length;
}

void NPBakeParams::set_edge_max_error(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	edge_max_error = p_value;
}

float NPBakeParams::get_edge_max_error() const {
	return edge_max_error;
}

void NPBakeParams::set_verts_per_poly(float p_value) {
	ERR_FAIL_COND(p_value < 3);
	verts_per_poly = p_value;
}

float NPBakeParams::get_verts_per_poly() const {
	return verts_per_poly;
}

void NPBakeParams::set_detail_sample_distance(float p_value) {
	ERR_FAIL_COND(p_value < 0.1);
	detail_sample_distance = p_value;
}

float NPBakeParams::get_detail_sample_distance() const {
	return detail_sample_distance;
}

void NPBakeParams::set_detail_sample_max_error(float p_value) {
	ERR_FAIL_COND(p_value < 0);
	detail_sample_max_error = p_value;
}

float NPBakeParams::get_detail_sample_max_error() const {
	return detail_sample_max_error;
}

void NPBakeParams::set_filter_low_hanging_obstacles(bool p_value) {
	filter_low_hanging_obstacles = p_value;
}

bool NPBakeParams::get_filter_low_hanging_obstacles() const {
	return filter_low_hanging_obstacles;
}

void NPBakeParams::set_filter_ledge_spans(bool p_value) {
	filter_ledge_spans = p_value;
}

bool NPBakeParams::get_filter_ledge_spans() const {
	return filter_ledge_spans;
}

void NPBakeParams::set_filter_walkable_low_height_spans(bool p_value) {
	filter_walkable_low_height_spans = p_value;
}

bool NPBakeParams::get_filter_walkable_low_height_spans() const {
	return filter_walkable_low_height_spans;
}
