#pragma once

#include "core/math/vector2.h"
#include "core/color.h"
#include "core/rid.h"
#include "core/math/transform_2d.h"
#include "servers/visual/rasterizer.h"


namespace Batch {

// dummys which will eventually be cast to the material type used by the particular renderer
// better than using a void * ...
// and avoids circular includes and API specific dependencies
struct AliasMaterial {};
struct AliasShader {};
struct AliasLight {};
struct AliasItem {};
////////////////////////////////////////////////////

enum ItemChangeFlags
{
	CF_SCISSOR = 1 << 0,
	CF_COPY_BACK_BUFFER = 1 << 1,
	CF_SKELETON = 1 << 2,
	CF_MATERIAL = 1 << 3,
	CF_BLEND_MODE = 1 << 4,
	CF_FINAL_MODULATE = 1 << 5,
	CF_MODEL_VIEW = 1 << 6,
	CF_EXTRA = 1 << 7,
	CF_LIGHT = 1 << 8,
};


// pod versions of vector and color and RID, need to be 32 bit for vertex format
struct BVec2 {
	float x, y;
	void set(const Vector2 &o) {
		x = o.x;
		y = o.y;
	}
	void to(Vector2 &o) const {
		o.x = x;
		o.y = y;
	}
};

struct BColor {
	float r, g, b, a;
	void set(float rr, float gg, float bb, float aa) {r = rr; g = gg; b = bb; a = aa;}
	void set(const Color &c) {
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}
	void to(Color &c) const {
		c.r = r; c.g = g; c.b = b; c.a = a;
	}
	bool equals(const Color &c) const {
		return (r == c.r) && (g == c.g) && (b == c.b) && (a == c.a);
	}
	const float *get_data() const { return &r; }
};

struct BVert {
	// must be 32 bit pod
	BVec2 pos;
	BVec2 uv;
};

struct BVert_colored : public BVert {
	// must be 32 bit pod
	BColor col;
};

/*
struct BRectI
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

struct BRectF// must be 32 bit
{
	void zero() {x = y = width = height = 0.0f;}
	void set(const Rect2 &r) {x = r.position.x; y = r.position.y; width = r.size.x; height = r.size.y;}
	void to(Rect2 &r) const {r.position.x = x; r.position.y = y; r.size.x = width; r.size.y = height;}
	float x; float y; float width; float height;
};

// a few different bits of extra data are needed for batches that are too big to fit in the batch,
// but conveniently fit in 128 bits. This involves an indirection, but won't happen super often except
// for colors, and if it does the verts will be translated to colored verts.
struct B128
{
	union
	{
		BRectI recti;
		BRectF rectf;
		BColor color;
	};
};

struct BTransform
{
	Transform2D tr;
};

struct BMaterial
{
	RID RID_material;
	AliasMaterial * m_pMaterial;
	bool m_bChangedShader;
};

struct BTex {
	enum TileMode : uint32_t {
		TILE_OFF,
		TILE_NORMAL,
		TILE_FORCE_REPEAT,
	};
	RID RID_texture;
	RID RID_normal;
	TileMode tile_mode;
	BVec2 tex_pixel_size;
};
*/



} // namespace
