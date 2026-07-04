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
	real_t internal_epsilon_squared = 0;
	Type type = ATTR_POSITION;
};

class MeshDeduplicator {
	enum { MAX_ATTRIBUTES_PER_VERT = 6 };

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

		uint32_t get_num_attributes() const { return attributes.size() - 1; } // not counting position
	} data;

	struct Attribute {
		Attribute() {}
		MeshAttributeStream::Type type = MeshAttributeStream::ATTR_MAX; // unset
		union {
			// Ensure all are zeroed by adding constructor on the largest..
			Color color = Color(0, 0, 0, 0);
			Vector3 vec3;
			Vector2 vec2;
			float flt;
		};

		void copy_from_stream(const MeshAttributeStream &p_as, uint32_t p_index) {
			type = p_as.type;

			switch (type) {
				case MeshAttributeStream::ATTR_POSITION: {
					vec3 = p_as.vec3[p_index];
				} break;
				case MeshAttributeStream::ATTR_NORMAL: {
					vec3 = p_as.vec3[p_index];
				} break;
				case MeshAttributeStream::ATTR_UV: {
					vec2 = p_as.vec2[p_index];
				} break;
				case MeshAttributeStream::ATTR_COLOR: {
					color = p_as.color[p_index];
				} break;
				case MeshAttributeStream::ATTR_FLOAT: {
					flt = p_as.float_input[p_index];
				} break;
				default: {
					DEV_ASSERT(0);
				} break;
			}
		}
	};

	// A wedge is a position vertex.
	// We basically deduplicate on position,
	// but at each position we have a list of deduplicated attribute verts
	// (normal, UV etc) where all attributes should roughly match in order to merge.
	struct Wedge {
		// Position of the initial vertex.
		// Determines grid_pos, particularly if we are on borders.
		// There is thus a small effect of order of insertion, but not much.
		Vector3 position_orig;

		// Average of all the verts holding the position.
		Vector3 position_average;

		// Used to update the average position
		Vector3 position_total;
		uint32_t source_vertex_count = 0;
		Vector3i grid_pos;
		LocalVector<uint32_t> vert_ids;
	};

	struct AttributeStack {
		Attribute a[MAX_ATTRIBUTES_PER_VERT];

		void operator+=(const AttributeStack &p_o) {
			for (uint32_t n = 0; n < MAX_ATTRIBUTES_PER_VERT; n++) {
				a[n].color += p_o.a[n].color;
			}
		}
		void operator/=(uint32_t p_value) {
			for (uint32_t n = 0; n < MAX_ATTRIBUTES_PER_VERT; n++) {
				a[n].color /= p_value;
			}
		}
	};

	// Vertex is a unique vertex with matching attributes.
	// The position is defined by the wedge
	struct Vert {
		// How many source vertices have been merged into this attribute vertex.
		uint32_t source_vertex_count = 0;
		uint32_t wedge_id = 0;
		AttributeStack averages;
		AttributeStack totals;
		LocalVector<uint32_t> source_vert_ids;
	};

	struct Bucket {
		LocalVector<uint32_t> wedge_ids;
	};

	struct Grid {
		uint32_t grid_size = 65535;
		uint32_t num_buckets = 1024 * 4;
		AABB bound;

		LocalVector<Wedge> wedges;
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
