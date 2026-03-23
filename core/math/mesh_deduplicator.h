#pragma once

#include <core/color.h>
#include <core/local_vector.h>
#include <core/math/aabb.h>
#include <core/math/vector2.h>
#include <core/math/vector3.h>
#include <core/math/vector3i.h>

class MeshDeduplicator {
public:
	enum AttributeType {
		ATTR_POSITION = 0, // always required, uses distance
		ATTR_NORMAL, // angular epsilon (radians)
		ATTR_UV, // distance epsilon
		ATTR_COLOR, // sum of absolute differences (RGBA)
		ATTR_FLOAT, // absolute difference
		ATTR_MAX,
	};

	struct AttributeStream {
		String name;
		AttributeType type = ATTR_POSITION;
		float epsilon = 0; // meaning depends on type
		LocalVector<Vector3> vec3;
		LocalVector<Vector2> vec2;
		LocalVector<Color> color;
		LocalVector<float> float_input;

		// Internal use, no need to change from client code.
		uint32_t internal_vert_sum_index = 0;
		float internal_epsilon_squared = 0;
	};

private:
	struct VertexInfo {
		//		int new_index = -1;
		//		Vector3 sum_pos;
		//		LocalVector<Vector3> sum_normals;
		//		LocalVector<Vector2> sum_uvs;
		//		LocalVector<Color>   sum_colors;
		//		LocalVector<float>   sum_floats;
		//		int count = 0;
	};

	struct Data {
		LocalVector<AttributeStream> attributes;
		LocalVector<AttributeStream> out_attributes;
		LocalVector<uint32_t> out_indices;

		// Each attribute stream corresponds to a particular sum index in the vertex format.
		uint32_t sum_index_count[ATTR_MAX] = {};

		// Which attribute stream is the master position stream for spatial partitioning.
		uint32_t position_attribute_id = 0;
	} data;

	struct Vert {
		LocalVector<uint32_t> source_vert_ids;

#if 0
		uint32_t count = 0;
		Vector3 average_pos;

		Vector3 sum_pos;
		LocalVector<Vector3> sum_normals;
		LocalVector<Vector2> sum_uvs;
		LocalVector<Color> sum_colors;
		LocalVector<float> sum_floats;

		// Only for special case if we use multiple positions in FVF. Probably not used currently.
		LocalVector<Vector3> sum_positions;
#endif
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
	AttributeStream &get_input_attribute_stream(uint32_t p_idx) {
		return data.attributes[p_idx];
	}
	const AttributeStream &get_output_attribute_stream(uint32_t p_idx) {
		return data.out_attributes[p_idx];
	}

	bool process(const Span<uint32_t> &p_indices, LocalVector<uint32_t> &p_output_indices);

	//bool deduplicate_verts(const Span<uint32_t> &p_in_inds, const Span<Vector3> &p_in_verts, LocalVector<Vector3> &r_out_verts, LocalVector<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);
};
