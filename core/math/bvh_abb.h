#pragma once

// special optimized version of axis aligned bounding box
struct BVH_ABB
{
	struct ConvexHull
	{
		// convex hulls (optional)
		const Plane * planes;
		int num_planes;
		const Vector3 * points;
		int num_points;
	};

	struct Segment
	{
		Vector3 from;
		Vector3 to;
	};

	enum IntersectResult
	{
		IR_MISS = 0,
		IR_PARTIAL,
		IR_FULL,
	};

	// we store mins with a negative value in order to test them with SIMD
	Vector3 neg_min;
	Vector3 max;

	bool operator==(const BVH_ABB &o) const {return (neg_min == o.neg_min) && (max == o.max);}
	bool operator!=(const BVH_ABB &o) const { return (*this == o) == false; }

	real_t get_proximity_to(const BVH_ABB &b) const {
		const Vector3 d = (-neg_min + max) - (-b.neg_min + b.max);
		return (Math::abs(d.x) + Math::abs(d.y) + Math::abs(d.z));
	}

	int select_by_proximity(const BVH_ABB &a, const BVH_ABB &b) const {
		return (get_proximity_to(a) < get_proximity_to(b) ? 0 : 1);
	}
	
	void set(const Vector3 &_min, const Vector3 &_max)
	{
		neg_min = -_min;
		max = _max;
	}
	
	void from(const AABB &aabb)
	{
		neg_min = -aabb.position;
		max = aabb.position + aabb.size;
	}
	void to(AABB &aabb) const
	{
		aabb.position = -neg_min;
		aabb.size = max - aabb.position;
	}
	void merge(const BVH_ABB &o)
	{
		if (o.max.x > max.x) max.x = o.max.x;
		if (o.max.y > max.y) max.y = o.max.y;
		if (o.max.z > max.z) max.z = o.max.z;
		if (o.neg_min.x > neg_min.x) neg_min.x = o.neg_min.x;
		if (o.neg_min.y > neg_min.y) neg_min.y = o.neg_min.y;
		if (o.neg_min.z > neg_min.z) neg_min.z = o.neg_min.z;
	}
	Vector3 calculate_size() const
	{
		return max + neg_min;
	}

	Vector3 calculate_centre() const
	{
		return Vector3(((max + neg_min) * 0.5f) - neg_min);
	}
	
//	void _fill_points(Vector3 pts[8])
//	{
//		Vector3 min;
//		min = -neg_min;
//		pts[0] = min;
//		pts[1] = max;
//		pts[2] = Vector3(min.x, min.y, max.z);
//		pts[3] = Vector3(min.x, max.y, min.z);
//		pts[4] = Vector3(max.x, min.y, min.z);
		
//		pts[5] = Vector3(max.x, max.y, min.z);
//		pts[6] = Vector3(max.x, min.y, max.z);
//		pts[7] = Vector3(min.x, max.y, max.z);
//	}
	
	uint32_t find_cutting_planes(const BVH_ABB::ConvexHull &hull, uint32_t * plane_ids) const
	{
		uint32_t count = 0;
		
		for (int n=0; n<hull.num_planes; n++)
		{
			const Plane &p = hull.planes[n];
			if (intersects_plane(p))
			{
				plane_ids[count++] = n;
			}
		}
		
		return count;
	}
	
	
	bool intersects_plane(const Plane &p) const
	{
//		Vector3 pts[8];
//		_fill_points(pts);
		
		Vector3 size = max + neg_min;
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = -neg_min + half_extents;

		// forward side of plane?		
		Vector3 point_offset(
				(p.normal.x < 0) ? -half_extents.x : half_extents.x,
				(p.normal.y < 0) ? -half_extents.y : half_extents.y,
				(p.normal.z < 0) ? -half_extents.z : half_extents.z);
		Vector3 point = point_offset + ofs;
		
		float dist = p.distance_to(point);
		if (!p.is_point_over(point))
			return false;
		
		point = -point_offset + ofs;
		dist = p.distance_to(point);
		if (p.is_point_over(point))
			return false;
		
//		for (int n=0; n<8; n++)
//		{
			
//		}

		return true;
	}	
	
	bool intersects_convex_optimized(const ConvexHull &hull, const uint32_t * p_plane_ids, uint32_t p_num_planes) const
	{
		Vector3 size = max + neg_min;
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = -neg_min + half_extents;
		
		for (int i = 0; i < p_num_planes; i++) {
			
			const Plane &p = hull.planes[p_plane_ids[i]];
			Vector3 point(
					(p.normal.x > 0) ? -half_extents.x : half_extents.x,
					(p.normal.y > 0) ? -half_extents.y : half_extents.y,
					(p.normal.z > 0) ? -half_extents.z : half_extents.z);
			point += ofs;
			if (p.is_point_over(point))
				return false;
		}

		return true;		
	}
	
	bool intersects_convex_partial(const ConvexHull &hull) const
	{
		AABB bb;
		to(bb);
		return bb.intersects_convex_shape(hull.planes, hull.num_planes, hull.points, hull.num_points);
	}
	
	IntersectResult intersects_convex(const ConvexHull &hull) const
	{
		if (intersects_convex_partial(hull))
		{
			// fully within? very important for tree checks
			if (is_within_convex(hull))
			{
				return IR_FULL;
			}
			
			return IR_PARTIAL;
		}

		return IR_MISS;
	}

	bool is_within_convex(const ConvexHull &hull) const
	{
		// use half extents routine
		AABB bb;
		to(bb);
		return bb.inside_convex_shape(hull.planes, hull.num_planes);
	}
	
	bool is_point_within_hull(const ConvexHull &hull, const Vector3 &pt) const
	{
		for (int n=0; n<hull.num_planes; n++)
		{
			if (hull.planes[n].distance_to(pt) > 0.0f)
				return false;
		}
		return true;
	}
	
	bool intersects_segment(const Segment &s) const
	{
		AABB bb;
		to(bb);
		return bb.intersects_segment(s.from, s.to);
	}

	bool intersects_point(const Vector3 &p) const
	{
		if (_vector3_any_morethan(p, max)) return false;
		if (_vector3_any_morethan(-p, neg_min)) return false;
		return true;
	}

	bool intersects(const BVH_ABB &o) const
	{
		if (_vector3_any_morethan(-o.neg_min, max)) return false;
		if (_vector3_any_morethan(-neg_min, o.max)) return false;
		return true;
	}

	bool is_other_within(const BVH_ABB &o) const
	{
		if (_vector3_any_morethan(o.max, max)) return false;
		if (_vector3_any_morethan(o.neg_min, neg_min)) return false;
//		if (o.max.x > max.x) return false;
//		if (o.max.y > max.y) return false;
//		if (o.max.z > max.z) return false;
//		if (o.neg_min.x > neg_min.x) return false;
//		if (o.neg_min.y > neg_min.y) return false;
//		if (o.neg_min.z > neg_min.z) return false;
		return true;
	}

	/*
	void grow_fractional(float fraction)
	{
		Vector3 size = calculate_size();
		float longest = MAX(size.x, size.y);
		longest = MAX(longest, size.z);
		//grow(longest * fraction);
		expand(1.0f);
	}
	*/

	void grow(const Vector3 &change)
	{
		max += change;
		neg_min += change;
	}
	
	void expand(float change = 0.5f)
	{
		grow(Vector3(change, change, change));
	}

	void expand_negative()
	{
		expand(-0.5f);
	}
	
	float get_area() const // actually surface area metric
	{
		Vector3 d = max + neg_min;
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}
	void set_to_max_opposite_extents()
	{
		max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		neg_min = max;
	}

	bool _vector3_any_morethan(const Vector3 &a, const Vector3 &b) const
	{
		if (a.x > b.x) return true;
		if (a.y > b.y) return true;
		if (a.z > b.z) return true;
		return false;
	}

	bool _vector3_any_lessthan(const Vector3 &a, const Vector3 &b) const
	{
		if (a.x < b.x) return true;
		if (a.y < b.y) return true;
		if (a.z < b.z) return true;
		return false;
	}

};

