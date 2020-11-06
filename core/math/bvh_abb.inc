// special optimized version of axis aligned bounding box
struct ABB
{
	// we store mins with a negative value in order to test them with SIMD
	Vector3 neg_min;
	Vector3 max;
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
	void merge(const ABB &o)
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

	bool is_within(const ABB &o) const
	{
//		AABB a;
//		to(a);
//		AABB b;
//		o.to(b);

		if (o.max.x > max.x) return false;
		if (o.max.y > max.y) return false;
		if (o.max.z > max.z) return false;
		if (o.neg_min.x > neg_min.x) return false;
		if (o.neg_min.y > neg_min.y) return false;
		if (o.neg_min.z > neg_min.z) return false;
		return true;
	}

	void grow_fractional(float fraction)
	{
		Vector3 size = calculate_size();
		float longest = MAX(size.x, size.y);
		longest = MAX(longest, size.z);
		//grow(longest * fraction);
		grow (1.0f);
	}

	void grow(const Vector3 &change)
	{
		max += change;
		neg_min += change;
	}

	void grow(float change)
	{
		grow(Vector3(change, change, change));
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
};

