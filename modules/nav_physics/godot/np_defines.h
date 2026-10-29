#pragma once

#include "core/math/transform.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "thirdparty/navphysics/navphysics_pointf.h"
#include "thirdparty/navphysics/navphysics_pointi.h"
#include "thirdparty/navphysics/navphysics_transform.h"

struct NPHelper {
	static const NavPhysics::Transform &transform_to_np_transform(const Transform &p_xform) {
		return *(const NavPhysics::Transform *)&p_xform;
	}

	static const Transform &np_transform_to_transform(const NavPhysics::Transform &p_xform) {
		return *(const Transform *)&p_xform;
	}

	static const NavPhysics::FPoint3 &vector3_to_np_fpoint3(const Vector3 &p_pt) {
		return *(const NavPhysics::FPoint3 *)&p_pt;
	}

	static const Vector3 &np_fpoint3_to_vector3(const NavPhysics::FPoint3 &p_pt) {
		return *(const Vector3 *)&p_pt;
	}
};
