#pragma once

#include "navphysics_allocator.h"
#include "navphysics_typedefs.h"
#include "navphysics_vector.h"

namespace NavPhysics {

struct FPoint2;
struct FPoint3;
struct IPoint2;
struct IPoint3;
struct Basis;
struct Transform;
struct AABB;

class String {
	Vector<char> _chars;

public:
	String(const char *p_sz);
	String(const String &p_from);
	String(i64 p_value);
	String(i32 p_value);
	String(u64 p_value);
	String(u32 p_value);
	String(f32 p_value);
	String(f64 p_value);
	String(void *p_addr);
	String(FPoint2 p_value);
	String(FPoint3 p_value);
	String(IPoint2 p_value);
	String(IPoint3 p_value);
	String(Basis p_value);
	String(Transform p_value);
	String(AABB p_value);
	String();
	~String();

	void set(const char *p_sz);
	const char *get_cstring() const { return _chars.ptr(); }
	u32 size() const { return _chars.size(); }
	u32 length() const { return size() - 1; }
	bool is_empty() const { return size() <= 1; }

	String operator+(const String &p_o) const;
	String operator+(const char *p_o) const;
	void operator+=(const String &p_o);
	void operator+=(const char *p_o);

	String &operator=(const String &p_from) {
		_chars = p_from._chars;
		return *this;
	}
};

} // namespace NavPhysics
