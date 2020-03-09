#pragma once

#include "types.h"

namespace Batch {


struct BItemGroup {
	Transform2D p_base_transform;
	Color p_modulate;
	RasterizerCanvas::Light * p_light;
	int p_z;
};


// Take particular care of the packing of batch, we want the size 16 bytes
// so as to stay aligned and also not take too much memory because
// we will create a lot...
// Also because we are handling alignment ourself, take particular care on ARM
// i.e. Align 64 bit to 8 byte boundary, 32 bit to 4 byte, 16 bit to 2 byte, etc.
#pragma pack(push, 4)

// we want the batch to be as small as possible
struct Batch {
	enum CommandType : uint32_t {
		BT_DEFAULT = 0,
		BT_CHANGE_ITEM,
//		BT_CHANGE_MATERIAL,
//		BT_CHANGE_BLEND_MODE,
//		BT_CHANGE_TRANSFORM,
//		BT_CHANGE_COLOR,
//		BT_CHANGE_COLOR_MODULATE,
//		BT_SCISSOR_ON,
//		BT_SCISSOR_OFF,
//		BT_COPY_BACK_BUFFER,
//		BT_SET_EXTRA_MATRIX,
//		BT_UNSET_EXTRA_MATRIX,
//		BT_LIGHT_BEGIN,
//		BT_LIGHT_END,
//		BT_ITEM_FULL_STATE,
		BT_CHANGE_ITEM_GROUP,
		BT_CHANGE_ITEM_GROUP_END,


		// place compactable (types with batch verts) after here
		BT_COMPACTABLE,
		BT_RECT,
		BT_LINE,
	};

	// the best value for this depends on the data type used to store batch_texture_id.
	// however, due to the vertex buffer size limit of 65535, the actual max possible batch_texture_ids
	// will be around 65535 / 4, so there should be no danger of reaching this value at runtime.
	enum Constants {
		BATCH_TEX_ID_UNTEXTURED = 65535,
	};

	// we have to use a pack to prevent the compiler using 8 bytes to store this..
	CommandType type;

	// batches that can be joined by removing color change batches (to form colored verts)
	bool is_compactable() const {return type > BT_COMPACTABLE;}

	// rects, lines
//	struct UPrimitive
//	{
//		uint16_t batch_texture_id;
//		uint16_t first_quad;
//		uint16_t num_quads;
//		uint16_t first_vert;
//		uint16_t num_verts;
//	};

	// unhandled primitives passed to the old style rendering method
	struct UDefault
	{
//		uint32_t batch_texture_id;
		uint32_t first_command;
		uint32_t num_commands;
	};

	// dummys are to align the data better for 64 bit access.
	// as well as making access faster, this may be necessary on some platforms (e.g. ARM)
	// to prevent alignment crashes
//	struct ULightBegin {uint32_t dummy; AliasLight * m_pLight;};
	struct UChangeItem {uint32_t dummy; RasterizerCanvas::Item * m_pItem;};
//	struct UMaterialChange {int batch_material_id;};
//	struct UBlendModeChange {int blend_mode;};
//	struct UTransformChange {int transform_id;};
	struct UIndex {int id;};
	union
	{
//		UPrimitive primitive;
//		UDefault def;
		UChangeItem change_item;
//		UMaterialChange material_change;
//		UBlendModeChange blend_mode_change;
//		UTransformChange transform_change;
//		ULightBegin light_begin;
		UIndex change_itemgroup;
	};
};

#pragma pack(pop)


} // namespace
