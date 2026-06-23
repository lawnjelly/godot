#include "vector3i.h"

#include "vector3.h"
#include "vector3_64.h"

Vector3i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ", " + itos(z) + ")";
}

Vector3i::operator Vector3() const {
	return Vector3(x, y, z);
}

Vector3i::operator Vector3_64() const {
	return Vector3_64(x, y, z);
}

double Vector3i::calculate_triangle_area(const Vector3i &p_a, const Vector3i &p_b) const {
	Vector3_64 ab = Vector3_64(p_b - p_a);
	Vector3_64 ac = Vector3_64(*this - p_a);
	Vector3_64 cross = ab.cross(ac);
	return cross.length() * 0.5;
}
