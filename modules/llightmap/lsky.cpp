#include "lsky.h"

using namespace LM;

LSky::LSky() {
	Ref<Image> im;
	im.instance();
	im->load("res://Sky/snowy_field/snowy_field.exr");

	int w = im->get_width();
	int h = im->get_height();

	_texture.Create(w, h);

	im->lock();

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Color c = im->get_pixel(x, y);
			_texture.GetItem(x, y).Set(c.r, c.g, c.b);
		}
	}

	im->unlock();
}

void LSky::backward_trace_sky(const Vector3 &ptSource, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, FColor &color) {
}

void LSky::read_sky(const Vector3 &ptDir, FColor &color) {
	//direction = parameters.environment_transform.xform_inv(direction);
	Vector3 dir = ptDir;
	real_t dir_y = CLAMP(dir.y, 0.0, 1.0);
	Vector2 st = Vector2(Math::atan2(dir.z, dir.x), Math::acos(dir_y));

	// already clamped
	//	if (Math::is_nan(st.y)) {
	//		st.y = direction.y > 0.0 ? 0.0 : Math_PI;
	//	}

	st.x += Math_PI;
	st /= Vector2(Math_TAU, Math_PI);
	st.x = Math::fmod(st.x + 0.75, 1.0);
	FColor c = _bilinear_sample(st, false, true);
	//	color += throughput * Vector3(c.r, c.g, c.b) * c.a;

	color += c;
	//	color.r += c.r;
	//	color.g += c.g;
	//	color.b += c.b;
}

FColor LSky::_bilinear_sample(const Vector2 &p_uv, bool p_clamp_x, bool p_clamp_y) {
	int width = _texture.GetWidth();
	int height = _texture.GetHeight();

	Vector2 uv;
	uv.x = p_clamp_x ? p_uv.x : Math::fposmod(p_uv.x, 1.0f);
	uv.y = p_clamp_y ? p_uv.y : Math::fposmod(p_uv.y, 1.0f);

	float xf = uv.x * width;
	float yf = uv.y * height;

	int xi = (int)xf;
	int yi = (int)yf;

	FColor texels[4];
	for (int i = 0; i < 4; i++) {
		int sample_x = xi + i % 2;
		int sample_y = yi + i / 2;

		sample_x = CLAMP(sample_x, 0, width - 1);
		sample_y = CLAMP(sample_y, 0, height - 1);

		texels[i] = _texture.GetItem(sample_x, sample_y);
	}

	float tx = xf - xi;
	float ty = yf - yi;

	FColor c;
	c.Set(0);
	//	for (int i = 0; i < 4; i++) {
	FColor a = texels[0];
	a.Lerp(texels[1], tx);

	FColor b = texels[2];
	b.Lerp(texels[3], tx);

	c = a;
	c.Lerp(b, ty);

	//		c[i] = Math::lerp(Math::lerp(texels[0][i], texels[1][i], tx), Math::lerp(texels[2][i], texels[3][i], tx), ty);
	//	}
	return c;
}
