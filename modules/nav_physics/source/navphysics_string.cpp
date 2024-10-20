#include "navphysics_string.h"
#include "navphysics_error.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include <cstring>
#include <string>

namespace NavPhysics {

void String::set(const char *p_sz) {
	if (!p_sz) {
		_chars.resize(1);
		_chars[0] = 0;
		return;
	}

	i32 l = strlen(p_sz);
	_chars.resize(l + 1);
	strncpy(_chars.ptr(), p_sz, l);
	_chars[l] = 0;
}

String String::operator+(const char *p_o) const {
	String o(p_o);
	return *this + o;
}

String String::operator+(const String &p_o) const {
	String tmp = *this;
	tmp += p_o;
	return tmp;
}

void String::operator+=(const String &p_o) {
	if (p_o.is_empty())
		return;

	String tmp = *this;

	i32 l = length();
	i32 new_length = l + p_o.length() + 1;
	_chars.resize(new_length);
	strncpy(_chars.ptr(), tmp.get_cstring(), l);
	strncpy(_chars.ptr() + l, p_o.get_cstring(), p_o.length());

	_chars[new_length - 1] = 0;
}

void String::operator+=(const char *p_o) {
	String o(p_o);
	(*this) += o;
}

String::String(i64 p_value) {
	if (p_value > INT32_MAX) {
		set("high");
		return;
	}
	if (p_value < INT32_MIN) {
		set("low");
		return;
	}

	*this = String((i32)p_value);
}

String::String(i32 p_value) {
	char buffer[64];
	snprintf(buffer, 64, "%d", p_value);
	set(buffer);
}

String::String(u64 p_value) {
	if (p_value > UINT32_MAX) {
		set("high");
		return;
	}
	*this = String((u32)p_value);
}

String::String(u32 p_value) {
	char buffer[64];
	if (p_value != UINT32_MAX) {
		snprintf(buffer, 64, "%u", p_value);
	} else {
		set("*");
		return;
	}
	set(buffer);
}

String::String(f32 p_value) {
	char buffer[64];
	snprintf(buffer, 64, "%f", p_value);
	set(buffer);
}

String::String(f64 p_value) {
	char buffer[64];
	snprintf(buffer, 64, "%f", p_value);
	set(buffer);
}

String::String(void *p_addr) {
	char buffer[64];
	snprintf(buffer, 64, "%p", p_addr);
	set(buffer);
}

String::String(const String &p_from) {
	_chars = p_from._chars;
}

String::String(const char *p_sz) {
	set(p_sz);
}

String::String(FPoint2 p_value) {
	*this = String(p_value.x) + ", " + String(p_value.y);
}

String::String(FPoint3 p_value) {
	*this = String(p_value.x) + ", " + String(p_value.y) + ", " + String(p_value.z);
}

String::String(Basis p_value) {
	*this = String("[ ( ") + String(p_value[0]) + " ), ( " + String(p_value[1]) + " ), ( " + String(p_value[2]) + " )";
}

String::String(Transform p_value) {
	*this = String(p_value.basis) + " [ " + String(p_value.origin) + " ] ";
}

String::String(IPoint2 p_value) {
	*this = String(p_value.x) + ", " + String(p_value.y);
}

String::String(IPoint3 p_value) {
	*this = String(p_value.x) + ", " + String(p_value.y) + ", " + String(p_value.z);
}

String::String(AABB p_value) {
	*this = String("[ ") + p_value.position + ", " + p_value.size + " ] ";
}

String::String() {
	set(nullptr);
}

String::~String() {
}

} // namespace NavPhysics
