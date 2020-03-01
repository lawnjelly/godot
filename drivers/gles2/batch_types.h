#pragma once

#include "core/math/vector2.h"
#include "core/color.h"
#include "core/rid.h"
#include "core/math/transform_2d.h"
//#include "rasterizer_canvas_gles2.h"
//#include "servers/visual/rasterizer.h"

namespace Batch
{

// dummys which will eventually be cast to the material type used by the particular renderer
// better than using a void * ...
// and avoids circular includes and API specific dependencies
struct AliasMaterial {};
struct AliasShader {};
struct AliasLight {};
struct AliasItem {};
////////////////////////////////////////////////////


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

// we want the batch to be as small as possible
struct Batch {
	enum CommandType : uint16_t {
		BT_DEFAULT = 0,
		BT_CHANGE_ITEM,
		BT_CHANGE_MATERIAL,
		BT_CHANGE_BLEND_MODE,
		BT_CHANGE_TRANSFORM,
		BT_CHANGE_COLOR,
		BT_CHANGE_COLOR_MODULATE,
		BT_SCISSOR_ON,
		BT_SCISSOR_OFF,
		BT_COPY_BACK_BUFFER,
		BT_SET_EXTRA_MATRIX,
		BT_UNSET_EXTRA_MATRIX,
		BT_LIGHT_BEGIN,
		BT_LIGHT_END,


		// place compactable (types with batch verts) after here
		BT_COMPACTABLE,
		BT_RECT,
		BT_LINE,
	};

	enum Constants {
		BATCH_TEX_ID_UNTEXTURED = 65535,
	};

	CommandType type;
	bool is_compactable() const {return type > BT_COMPACTABLE;}

	struct UPrimitive
	{
		uint16_t batch_texture_id;
		uint16_t first_quad;
		uint16_t num_commands;
	};
	struct UDefault
	{
		uint16_t batch_texture_id;
		uint16_t first_command;
		uint16_t num_commands;
	};
	/*
	struct URectI
	{
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	};
	struct URectF// must be 32 bit
	{
		void zero() {x = y = width = height = 0.0f;}
		void set(const Rect2 &r) {x = r.position.x; y = r.position.y; width = r.size.x; height = r.size.y;}
		void to(Rect2 &r) const {r.position.x = x; r.position.y = y; r.size.x = width; r.size.y = height;}
		float x; float y; float width; float height;
	};
	*/
	struct ULightBegin
	{
		AliasLight * m_pLight;
//		int light_mode; // VS::CanvasLightMode
	};
	//struct UColorChange {BColor color;};// bool bRedundant;};
	struct UItemChange {AliasItem * m_pItem;};
	struct UMaterialChange {int batch_material_id;};
	struct UBlendModeChange {int blend_mode;};
	struct UTransformChange {int transform_id;};
	struct UIndex {int id;};
	union
	{
		UPrimitive primitive;
		UDefault def;
		UIndex index;
//		URectI scissor_rect;
//		URectF copy_back_buffer_rect;
//		UColorChange color_change;
//		UColorChange color_modulate_change;
		UItemChange item_change;
		UMaterialChange material_change;
		UBlendModeChange blend_mode_change;
		UTransformChange transform_change;
		ULightBegin light_begin;
	};
};

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
//	AliasShader * m_pShader;
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



} // namespace
