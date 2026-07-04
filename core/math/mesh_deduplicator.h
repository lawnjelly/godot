#pragma once

#include <core/color.h>
#include <core/local_vector.h>
#include <core/math/aabb.h>
#include <core/math/vector2.h>
#include <core/math/vector3.h>
#include <core/math/vector3i.h>

struct MeshAttributeStream {
	enum Type {
		ATTR_POSITION = 0, // always required, uses distance
		ATTR_NORMAL, // angular epsilon (radians) - approximated via distance on unit sphere
		ATTR_UV, // distance epsilon
		ATTR_COLOR, // uses is_equal_approx (ignores epsilon)
		ATTR_FLOAT, // absolute difference
		ATTR_MAX,
	};

	String name;
	real_t epsilon = 0; // meaning depends on type
	LocalVector<Vector3> vec3;
	LocalVector<Vector2> vec2;
	LocalVector<Color> color;
	LocalVector<float> float_input;

	void set_type(Type p_type, float p_epsilon = -1);

private:
	friend class MeshDeduplicator;
	// Internal use, no need to change from client code.
	float internal_epsilon_squared = 0;
	Type type = ATTR_POSITION;
};

class MeshDeduplicator {
	struct Data {
		LocalVector<MeshAttributeStream> attributes;
		LocalVector<MeshAttributeStream> out_attributes;
		LocalVector<uint32_t> out_indices;

		// Output vertex to input vertex
		// (so we can optionally reuse existing input vertex data,
		// and only change the indices used).
		LocalVector<uint32_t> out_mapping;

		// Which attribute stream is the master position stream for spatial partitioning.
		uint32_t position_attribute_id = 0;
	} data;

	struct Vert {
		LocalVector<uint32_t> source_vert_ids;
	};

	struct Bucket {
		LocalVector<uint32_t> vert_ids;
	};

	struct Grid {
		uint32_t grid_size = 65535;
		uint32_t num_buckets = 1024 * 4;

		AABB bound;
		LocalVector<Vert> verts;
		LocalVector<Bucket> buckets;

		void prepare();
		Vector3i find_grid_pos(const Vector3 &p_pos) const;
		uint32_t hash_grid_pos(const Vector3i &p_pos) const;

	} grid;

public:
	void set_num_attribute_streams(uint32_t p_num_streams) {
		data.attributes.resize(p_num_streams);
		data.out_attributes.resize(p_num_streams);
	}
	MeshAttributeStream &get_input_attribute_stream(uint32_t p_idx) {
		return data.attributes[p_idx];
	}
	const MeshAttributeStream &get_output_attribute_stream(uint32_t p_idx) {
		return data.out_attributes[p_idx];
	}
	uint32_t get_output_vertex_mapping_to_input_vertex(uint32_t p_idx) {
		return data.out_mapping[p_idx];
	}

	bool process(const Span<uint32_t> &p_indices, LocalVector<uint32_t> &p_output_indices);
};
