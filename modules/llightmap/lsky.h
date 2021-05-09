#pragma once

#include "core/math/vector3.h"
#include "llightimage.h"
#include "llighttypes.h"

namespace LM {

class LSky {
public:
	LSky();
	void backward_trace_sky(const Vector3 &ptSource, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, FColor &color);

	bool is_active() const { return _active; }

	void read_sky(const Vector3 &ptDir, FColor &color);

private:
	FColor _bilinear_sample(const Vector2 &p_uv, bool p_clamp_x, bool p_clamp_y);

	LightImage<FColor> _texture;
	bool _active = true;
};

} // namespace LM
