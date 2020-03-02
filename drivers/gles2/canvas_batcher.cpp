#include "canvas_batcher.h"
#include "rasterizer_canvas_batched.h"
//#include <stdlib.h> // for rand
#include "core/engine.h"

//#define BATCH_DEBUG_FRAMES




namespace Batch
{

void CanvasBatcher::process_prepare(RasterizerCanvasBatched * pOwner, int p_z, const Color &p_modulate, const Transform2D &p_base_transform)
{
	m_PCommon.m_pOwner = pOwner;
	m_PCommon.m_iZ = p_z;
	m_PCommon.m_colModulate = p_modulate;
	m_PCommon.m_pLight = NULL;
	m_PCommon.m_trBase = p_base_transform;
}

Batch * CanvasBatcher::request_new_batch()
{
	Batch * b = m_Data.request_new_batch();
	m_FState.m_pCurrentBatch = b;
//	int i = sizeof (Batch);
	return b;
}

B128 * CanvasBatcher::_request_new_B128(Batch &batch)
{
	batch.index.id = m_Data.generic_128s.size();
	B128 * p = m_Data.request_new_B128();
	return p;
}



void CanvasBatcher::state_light_begin(AliasLight * pLight)
{
	Batch * b = request_new_batch();
	b->type = Batch::BT_LIGHT_BEGIN;
	b->light_begin.m_pLight = pLight;

	// starting the lighting invalidates the blend mode
	//state_invalidate_blend_mode();
}

void CanvasBatcher::state_light_end()
{
	Batch * b = request_new_batch();
	b->type = Batch::BT_LIGHT_END;
}


void CanvasBatcher::state_set_item(RasterizerCanvas::Item * pItem)
{
	m_FState.m_pItem = (AliasItem *) pItem;
	m_FState.m_bDirty_Item = true;
}

void CanvasBatcher::state_copy_back_buffer(RasterizerCanvas::Item *pItem)
{
	if (!pItem->copy_back_buffer)
		return;

	Batch * b = request_new_batch();
	b->type = Batch::BT_COPY_BACK_BUFFER;

	BRectF * pRect = request_new_rectf(*b);

	if (pItem->copy_back_buffer->full) {
		pRect->zero();
	} else {
		const Rect2 &r = pItem->copy_back_buffer->rect;
		pRect->set(r);
	}
}


void CanvasBatcher::state_set_scissor(RasterizerCanvas::Item *pClipItem)
{
	if ((RasterizerCanvas::Item *) m_FState.m_pClipItem == pClipItem)
		return;

	// changes
	m_FState.m_pClipItem = (AliasItem *) pClipItem;

	if (pClipItem) {
		Batch * b = request_new_batch();
		b->type = Batch::BT_SCISSOR_ON;

		BRectI &r = *request_new_recti(*b);

		get_owner()->_batch_get_item_clip_rect(*pClipItem,
		r.x,
		r.y,
		r.width,
		r.height);
	} else {
		Batch * b = request_new_batch();
		b->type = Batch::BT_SCISSOR_OFF;
	}
}


void CanvasBatcher::state_flush_item()
{
	if (!m_FState.m_bDirty_Item)
		return;

	m_FState.m_bDirty_Item = false;

	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_ITEM;
	b->item_change.m_pItem = m_FState.m_pItem;

	m_FState.m_pCurrentBatch = b;
}


void CanvasBatcher::state_set_RID_material(const RID &rid)
{
	if (m_State.m_RID_curr_Material == rid)
		return;

	m_State.m_RID_curr_Material = rid;

	AliasShader * pShader = 0;
	AliasMaterial * pMat = get_owner()->batch_get_material_and_shader_from_RID(rid, &pShader);
	m_State.m_pMaterial = pMat;

	BMaterial mat;
	mat.RID_material = rid;
	mat.m_pMaterial = pMat;
	mat.m_bChangedShader = false;

	if (pShader != m_State.m_pShader)
	{
		m_State.m_pShader = pShader;
		mat.m_bChangedShader = true;
	}


	int mat_id = m_Data.push_back_new_material(mat);

	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_MATERIAL;

	b->material_change.batch_material_id = mat_id;
}

void CanvasBatcher::state_set_blend_mode(int mode)
{
	if (mode == m_State.m_iBlendMode)
		return;

	m_State.m_iBlendMode = mode;

	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_BLEND_MODE;
	b->blend_mode_change.blend_mode = mode;
}

void CanvasBatcher::state_set_color_modulate(const Color &col)
{
	if (m_FState.m_Col_modulate.equals(col))
	{
		return;
	}

	m_FState.m_Col_modulate.set(col);
	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_COLOR_MODULATE;

	BColor &dcol = *request_new_color(*b);

	dcol.set(col);
}


bool CanvasBatcher::state_set_color(const Color &col)
{
	if (m_FState.m_CurrentCol.equals(col))
		return false;

	m_FState.m_CurrentCol.set(col);

	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_COLOR;

	BColor &dcol = *request_new_color(*b);
	dcol.set(col);

	return true;
}


void CanvasBatcher::state_set_extra_matrix(const Transform2D &tr)
{
	state_flush_modelview();

	if (!m_State.m_bExtraMatrixSet)
	{
		// store the original model view
		m_State.m_matModelView = m_State.m_matCombined;

		m_State.m_bExtraMatrixSet = true;

		Batch * b = request_new_batch();
		b->type = Batch::BT_SET_EXTRA_MATRIX;
	}

	m_State.m_matExtra = tr;
	m_State.m_matCombined = m_State.m_matModelView * tr;

	choose_software_transform_mode();
}

void CanvasBatcher::state_unset_extra_matrix()
{
	if (m_State.m_bExtraMatrixSet)
	{
		m_State.m_matCombined = m_State.m_matModelView;

		m_State.m_bExtraMatrixSet = false;
		Batch * b = request_new_batch();
		b->type = Batch::BT_UNSET_EXTRA_MATRIX;
	}
}

void CanvasBatcher::choose_software_transform_mode()
{
	const Transform2D &tr = m_State.m_matCombined;

	// decided whether to do translate only for software transform
	if ((tr.elements[0].x == 1.0) &&
	(tr.elements[0].y == 0.0) &&
	(tr.elements[1].x == 0.0) &&
	(tr.elements[1].y == 1.0))
	{
		m_State.m_eTransformMode = BState::TM_TRANSLATE;
	}
	else
	{
		m_State.m_eTransformMode = BState::TM_ALL;
	}
}


void CanvasBatcher::state_set_modelview(const Transform2D &tr)
{
	BATCH_DEBUG_CRASH_COND(m_State.m_bExtraMatrixSet);

	if (tr == m_State.m_matCombined)
		return;

	m_State.m_matCombined = tr;

	// defer this as only needed when not doing software transform
	m_State.m_bDirty_ModelView = true;

	// decided whether to do translate only for software transform
	choose_software_transform_mode();
}

void CanvasBatcher::state_flush_modelview()
{
	if (!m_State.m_bDirty_ModelView)
		return;

	// only send once per item
	if (m_State.m_bModelView_SentThisItem)
		return;

	m_State.m_bDirty_ModelView = false;
	m_State.m_bModelView_SentThisItem = true;

	BTransform * p = m_Data.request_new_transform();
	if (!m_State.m_bExtraMatrixSet)
		p->tr = m_State.m_matCombined;
	else
		p->tr = m_State.m_matModelView;

	Batch * b = request_new_batch();
	b->type = Batch::BT_CHANGE_TRANSFORM;
	b->transform_change.transform_id = m_Data.transforms.size()-1;
}


static int g_MyFrameCount = 0;
static bool g_bBatcherLogFrame = false;

void CanvasBatcher::pass_end()
{

	if ((g_MyFrameCount % 50) == 0)
		g_bBatcherLogFrame = true;
	else
		g_bBatcherLogFrame = false;

	g_MyFrameCount++;

	m_Data.reset_pass();
	m_State.reset_pass();
	m_FState.reset_pass();
}


int CanvasBatcher::find_or_create_tex(const RID &p_texture, const RID &p_normal, bool p_tile, int p_previous_match) {

	// optimization .. in 99% cases the last matched value will be the same, so no need to traverse the list
	if (p_previous_match != Batch::BATCH_TEX_ID_UNTEXTURED) // if it is zero, it will get hit first in the linear search anyway
	{
		const BTex &batch_texture = m_Data.batch_textures[p_previous_match];

		// note for future reference, if RID implementation changes, this could become more expensive
		if ((batch_texture.RID_texture == p_texture) && (batch_texture.RID_normal == p_normal)) {
			// tiling mode must also match
			bool tiles = batch_texture.tile_mode != BTex::TILE_OFF;

			if (tiles == p_tile)
				// match!
				return p_previous_match;
		}
	}

	// not the previous match .. we will do a linear search ... slower, but should happen
	// not very often except with non-batchable runs, which are going to be slow anyway

	// note this could be converted to a high speed hash table, profile though to check faster
	for (int n = 0; n < m_Data.batch_textures.size(); n++) {
		const BTex &batch_texture = m_Data.batch_textures[n];
		if ((batch_texture.RID_texture == p_texture) && (batch_texture.RID_normal == p_normal)) {

			// tiling mode must also match
			bool tiles = batch_texture.tile_mode != BTex::TILE_OFF;

			if (tiles == p_tile)
				// match!
				return n;
		}
	}

	// pushing back from local variable .. not ideal but has to use a Vector because non pod
	// due to RIDs
	BTex b;
	b.RID_texture = p_texture;
	b.RID_normal = p_normal;

	bool npot = false;

	bool texture_found = get_owner()->batch_get_texture_pixel_size_and_tile(p_texture, b.tex_pixel_size.x, b.tex_pixel_size.y, p_tile, npot);


	if (texture_found && p_tile)
	{
		if (npot)
			b.tile_mode = BTex::TILE_FORCE_REPEAT;
		else
			b.tile_mode = BTex::TILE_NORMAL;
	}
	else
	{
		b.tile_mode = BTex::TILE_OFF;
	}

	// push back
	m_Data.batch_textures.push_back(b);

	return m_Data.batch_textures.size() - 1;
}

void CanvasBatcher::debug_log_type_col(String sz, const BColor &col, int spacer)
{
	String s = "";
	for (int n=0; n<spacer; n++)
		s += "\t";

	Color c;
	col.to(c);
	m_szDebugLog += (s + sz + "\t" + String(c) + "\n");
}


void CanvasBatcher::debug_log_type(String sz, int spacer)
{
	String s = "";
	for (int n=0; n<spacer; n++)
		s += "\t";

	m_szDebugLog += (s + sz + "\n");
}

void CanvasBatcher::debug_log_color(String field, const BColor &col)
{
	Color c;
	col.to(c);
	m_szDebugLog += "\t\t" + field +" " +  String(c) + "\n";
}


void CanvasBatcher::debug_log_int(String field, int val)
{
	m_szDebugLog += "\t\t" + field +" " +  itos(val) + "\n";
}


void CanvasBatcher::debug_log_run()
{
	m_szDebugLog = "";

	int num_batches = m_Data.batches.size();
	debug_log_type("\n\n_________________________________________________________\nRUN");
	debug_log_int("num_batches", num_batches);

	for (int batch_num = 0; batch_num < num_batches; batch_num++) {
		const Batch &batch = m_Data.batches[batch_num];

		switch (batch.type) {
			case Batch::BT_RECT: {
				debug_log_type("RECT", 1);
				debug_log_int("btex_id", batch.primitive.batch_texture_id);
				debug_log_int("first_quad", batch.primitive.first_quad);
				debug_log_int("num_quads", batch.primitive.num_quads);
			} break;
		case Batch::BT_LINE: {
			debug_log_type("LINE", 1);
			debug_log_int("first_quad", batch.primitive.first_quad);
			debug_log_int("num_quads", batch.primitive.num_quads);
			debug_log_int("first_vert", batch.primitive.first_vert);
			debug_log_int("num_verts", batch.primitive.num_verts);
		} break;
			case Batch::BT_CHANGE_ITEM: {
				debug_log_type("CHANGE_ITEM");
			} break;
		case Batch::BT_LIGHT_BEGIN: {
			debug_log_type("LIGHT_BEGIN");
		} break;
		case Batch::BT_LIGHT_END: {
			debug_log_type("LIGHT_END");
		} break;

		case Batch::BT_CHANGE_BLEND_MODE: {
				debug_log_type("CHANGE_BLEND_MODE");
				debug_log_int("mode", batch.blend_mode_change.blend_mode);
			} break;
		case Batch::BT_CHANGE_MATERIAL: {
				debug_log_type("CHANGE_MATERIAL");
				int bmat_id = batch.material_change.batch_material_id;
				debug_log_int("bmat_id", bmat_id);
			} break;
		case Batch::BT_COPY_BACK_BUFFER: {
				debug_log_type("COPY_BACK_BUFFER");
			} break;
		case Batch::BT_CHANGE_TRANSFORM: {
				debug_log_type("CHANGE_TRANSFORM");
			} break;
		case Batch::BT_SET_EXTRA_MATRIX: {
				debug_log_type("SET_EXTRA_MATRIX");
			} break;
		case Batch::BT_UNSET_EXTRA_MATRIX: {
				debug_log_type("UNSET_EXTRA_MATRIX");
			} break;
		case Batch::BT_CHANGE_COLOR: {
				debug_log_type_col("CHANGE_COLOR", get_color(batch.index.id));
			} break;
		case Batch::BT_CHANGE_COLOR_MODULATE: {
				debug_log_type_col("CHANGE_COLOR_MODULATE", get_color(batch.index.id));
			} break;
		case Batch::BT_SCISSOR_ON: {
				debug_log_type("SCISSOR_ON");
			} break;
		case Batch::BT_SCISSOR_OFF: {
				debug_log_type("SCISSOR_OFF");
			} break;
			case Batch::BT_DEFAULT: {
				debug_log_type("DEFAULT", 1);
				debug_log_int("num_commands", batch.def.num_commands);
			} break;
			default: {
				debug_log_type("UNRECOGNISED", 1);
			} break;
		} // switch

	} // for

	print_line(m_szDebugLog);
}

// as a result of adding lines, we may get out of sync,
// we need to be in sync of groups of 4 verts for rects
//void Batcher2d::fill_sync_verts()
//{
//	int remainder = m_Data.vertices.size() & 3;
//	if (!remainder)
//		return;

//	remainder = 4-remainder;

//	for (int n=0; n<remainder; n++)
//	{
//		m_Data.vertices.request();
//	}

//	// keep the start quad up to date
//	m_Data.total_quads = m_Data.vertices.size() / 4;
//}


int CanvasBatcher::fill(int p_command_start)
{
	RasterizerCanvas::Item * pItem = m_FState.get_item();


	// we will prefill batches and vertices ready for sending in one go to the vertex buffer
	int command_count = pItem->commands.size();
	RasterizerCanvas::Item::Command * const*commands = pItem->commands.ptr(); // ptrw? or ptr

	int batch_tex_id = Batch::BATCH_TEX_ID_UNTEXTURED;

	// re-entrant
	int quad_count = m_Data.total_quads;

	//BATCH_DEBUG_CRASH_COND (quad_count != bdata.debug_batch_quads_total);


	Vector2 texpixel_size(1, 1);

	if (m_FState.m_pCurrentBatch)
	{
		if (m_FState.m_pCurrentBatch->type > Batch::BT_COMPACTABLE)
		{
			batch_tex_id = m_FState.m_pCurrentBatch->primitive.batch_texture_id;

			if (batch_tex_id != Batch::BATCH_TEX_ID_UNTEXTURED)
			{
				// tex pixel size
				const BTex &tex = m_Data.batch_textures[batch_tex_id];
				tex.tex_pixel_size.to(texpixel_size);
			}
		}
	}
	else
	{
		// after a flush always start with an item (to make sure there is something as the current batch
		state_set_item(pItem);
		state_flush_item();
	}

	// we need to return which command we got up to, so
	// store this outside the loop
	int command_num;

#define GLES2_BATCH_DEFAULT state_flush_item();\
state_flush_modelview();\
if (m_FState.m_pCurrentBatch->type == Batch::BT_DEFAULT) {\
m_FState.m_pCurrentBatch->def.num_commands++;\
} else {\
m_FState.m_pCurrentBatch = m_Data.request_new_batch();\
m_FState.m_pCurrentBatch->type = Batch::BT_DEFAULT;\
m_FState.m_pCurrentBatch->def.first_command = command_num;\
m_FState.m_pCurrentBatch->def.num_commands = 1;\
}

	// do as many commands as possible until the vertex buffer will be full up
	for (command_num = p_command_start; command_num < command_count; command_num++) {

		RasterizerCanvas::Item::Command *command = commands[command_num];

		switch (command->type) {

			default: {
				GLES2_BATCH_DEFAULT
			} break;
			case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
				// send a batch to notify this, we don't need to send the matrix, it
				// will come as the next default
				RasterizerCanvas::Item::CommandTransform *transform = static_cast<RasterizerCanvas::Item::CommandTransform *>(command);
				state_set_extra_matrix(transform->xform);
				GLES2_BATCH_DEFAULT
			}
			break;

			case RasterizerCanvas::Item::Command::TYPE_RECT: {

				if (!fill_rect(command_num, command, batch_tex_id, quad_count, texpixel_size))
				{
					goto cleanup;
				}
			} break;

		case RasterizerCanvas::Item::Command::TYPE_LINE: {

			if (!fill_line(command_num, command, batch_tex_id, quad_count, texpixel_size))
			{
				goto cleanup;
			}
		} break;

		case RasterizerCanvas::Item::Command::TYPE_NINEPATCH: {

			if (!fill_ninepatch(command_num, command, batch_tex_id, quad_count, texpixel_size))
			{
				goto cleanup;
			}
		} break;
		}
	}

	// gotos are cool, never use goto kids
cleanup:;

	// resave this
	m_Data.total_quads = quad_count;

	// important, we return how far we got through the commands.
	// We may not yet have reached the end, the vertex buffer may be full,
	// and need a draw / reset / fill / rinse repeat
	return command_num;

}


bool CanvasBatcher::fill_rect(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size)
{
	Batch *curr_batch = m_FState.m_pCurrentBatch;

	// modelview will need to be refreshed after drawing rects
	m_State.m_bDirty_ModelView = true;

	RasterizerCanvas::Item::CommandRect *rect = static_cast<RasterizerCanvas::Item::CommandRect *>(command);

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	BATCH_DEBUG_CRASH_COND (quad_count != (m_Data.vertices.size() / 4));

	// request FOUR vertices at a time, this is more efficient
	BVert *bvs = m_Data.vertices.request_four();
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		m_Data.buffer_full = true;

		return false;
	}


	const Color &col = rect->modulate;

	// instead of doing all the texture preparation for EVERY rect,
	// we build a list of texture combinations and do this once off.
	// This means we have a potentially rather slow step to identify which texture combo
	// using the RIDs.
	int old_bti = batch_tex_id;
	batch_tex_id = find_or_create_tex(rect->texture, rect->normal_map, rect->flags & RasterizerCanvas::CANVAS_RECT_TILE, old_bti);


	bool change_batch = false;

	// conditions for creating a new batch
	if ((curr_batch->type != Batch::BT_RECT) || (old_bti != batch_tex_id)) {
		change_batch = true;
	}

	// we need to treat color change separately because we need to count these
	// to decide whether to switch on the fly to colored vertices.
	if (state_set_color(col))
	{
		curr_batch = m_FState.m_pCurrentBatch;
		change_batch = true;
		m_Data.total_color_changes++;
	}

	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
		m_Data.batch_textures[batch_tex_id].tex_pixel_size.to(texpixel_size);

		// open new batch (this should never fail, it dynamically grows)
		curr_batch = request_new_batch();

		curr_batch->type = Batch::BT_RECT;
		//curr_batch->common.color.set(col);
		curr_batch->primitive.batch_texture_id = batch_tex_id;
		//curr_batch->primitive.first_command = command_num;
		curr_batch->primitive.num_quads = 1;
		curr_batch->primitive.first_quad = quad_count;
		//curr_batch->primitive.first_vert = quad_count * 4;

		// keep the state version up to date
		m_FState.m_pCurrentBatch = curr_batch;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		curr_batch->primitive.num_quads++;
	}

	// fill the quad geometry
	Rect2 trect = rect->rect;
	// software transform
	if (m_State.m_eTransformMode == BState::TM_TRANSLATE)
		software_translate_rect(trect);

	const Vector2 &mins = trect.position;
	Vector2 maxs = mins + trect.size;

	// just aliases
	BVert *bA = &bvs[0];
	BVert *bB = &bvs[1];
	BVert *bC = &bvs[2];
	BVert *bD = &bvs[3];

	// top left, top right, bot right, bot left
	bA->pos.x = mins.x;
	bA->pos.y = mins.y;

	bB->pos.x = maxs.x;
	bB->pos.y = mins.y;

	bC->pos.x = maxs.x;
	bC->pos.y = maxs.y;

	bD->pos.x = mins.x;
	bD->pos.y = maxs.y;

	if (rect->rect.size.x < 0) {
		SWAP(bA->pos, bB->pos);
		SWAP(bC->pos, bD->pos);
	}
	if (rect->rect.size.y < 0) {
		SWAP(bA->pos, bD->pos);
		SWAP(bB->pos, bC->pos);
	}

	// complex software transform
	if (m_State.m_eTransformMode == BState::TM_ALL)
	{
		software_transform_vert(*bA);
		software_transform_vert(*bB);
		software_transform_vert(*bC);
		software_transform_vert(*bD);
	}

	// uvs
	Rect2 src_rect = (rect->flags & RasterizerCanvas::CANVAS_RECT_REGION) ? Rect2(rect->source.position * texpixel_size, rect->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

	// 10% faster calculating the max first
	Vector2 pos_max = src_rect.position + src_rect.size;
	Vector2 uvs[4] = {
		src_rect.position,
		Vector2(pos_max.x, src_rect.position.y),
		pos_max,
		Vector2(src_rect.position.x, pos_max.y),
	};

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_TRANSPOSE) {
		SWAP(uvs[1], uvs[3]);
	}

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_H) {
		SWAP(uvs[0], uvs[1]);
		SWAP(uvs[2], uvs[3]);
	}
	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_V) {
		SWAP(uvs[0], uvs[3]);
		SWAP(uvs[1], uvs[2]);
	}

	bA->uv.set(uvs[0]);
	bB->uv.set(uvs[1]);
	bC->uv.set(uvs[2]);
	bD->uv.set(uvs[3]);

	// increment quad count
	quad_count++;

	return true;
}

bool CanvasBatcher::fill_line(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size)
{
	Batch *curr_batch = m_FState.m_pCurrentBatch;

	// modelview will need to be refreshed after drawing rects
	m_State.m_bDirty_ModelView = true;

	RasterizerCanvas::Item::CommandLine *line = static_cast<RasterizerCanvas::Item::CommandLine *>(command);

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	BATCH_DEBUG_CRASH_COND (quad_count != (m_Data.vertices.size() / 4));

	// decide whether we are changing batch type
	bool change_batch = false;

	// conditions for creating a new batch
	if (curr_batch->type != Batch::BT_LINE) {
		change_batch = true;
	}


	// is there room for 4 verts?
	if ((m_Data.vertices.size() + 4) > m_Data.vertices.max_size())
	{
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		m_Data.buffer_full = true;
		return false;
	}

	// always room for 4 verts if we got to here, it will not fail, so safe to create color batch
	const Color &col = line->color;

	// never a texture for lines
	batch_tex_id = Batch::BATCH_TEX_ID_UNTEXTURED; //-1;

	// we need to treat color change separately because we need to count these
	// to decide whether to switch on the fly to colored vertices.
	if (state_set_color(col))
	{
		curr_batch = m_FState.m_pCurrentBatch;
		change_batch = true;
		m_Data.total_color_changes++;
	}


	BVert *bvs;
	int first_vert;
	if (!change_batch && m_FState.m_bPreviousLineOdd)
	{
		first_vert = m_Data.vertices.size()-2;
		bvs = &m_Data.vertices[first_vert];
		m_FState.m_bPreviousLineOdd = false;
	}
	else
	{
		first_vert = m_Data.vertices.size();

		// request FOUR vertices at a time, this is more efficient
		bvs = m_Data.vertices.request_four();
		if (!bvs) {
			// run out of space in the vertex buffer .. finish this function and draw what we have so far
			m_Data.buffer_full = true;

			return false;
			//goto cleanup;
		}
		m_FState.m_bPreviousLineOdd = true;
	}



	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
//		m_Data.batch_textures[batch_tex_id].tex_pixel_size.to(texpixel_size);
		texpixel_size = Vector2(1, 1);

		// open new batch (this should never fail, it dynamically grows)
		curr_batch = request_new_batch();

		curr_batch->type = Batch::BT_LINE;
		curr_batch->primitive.batch_texture_id = batch_tex_id;
		curr_batch->primitive.first_quad = quad_count;

		curr_batch->primitive.num_quads = 1;

		curr_batch->primitive.first_vert = first_vert;
		curr_batch->primitive.num_verts = 2;

		// keep the state version up to date
		m_FState.m_pCurrentBatch = curr_batch;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		curr_batch->primitive.num_verts += 2;
		if (m_FState.m_bPreviousLineOdd)
			curr_batch->primitive.num_quads++;
	}


	// only dealing with single width so far
	Vector2 ptFrom = line->from;
	Vector2 ptTo = line->to;

	// software transform
	if (m_State.m_eTransformMode == BState::TM_TRANSLATE)
	{
		software_translate_vert(ptFrom);
		software_translate_vert(ptTo);
	}
	else
	{
		software_transform_vert(ptFrom);
		software_transform_vert(ptTo);
	}

	// just aliases
	BVert *bA = &bvs[0];
	BVert *bB = &bvs[1];
//	BVert *bC = &bvs[2];
//	BVert *bD = &bvs[3];

	// top left, top right, bot right, bot left
	bA->pos.set(ptFrom);
	bB->pos.set(ptTo);
//	bC->pos.set(ptFrom);
//	bD->pos.set(ptTo);

	// uvs
	BVec2 zero;
	zero.x = 0;
	zero.y = 0;
	bA->uv = zero;
	bB->uv = zero;
//	bC->uv = zero;
//	bD->uv = zero;

	// increment quad count
	if (m_FState.m_bPreviousLineOdd)
		quad_count++;

	return true;
}

bool CanvasBatcher::fill_ninepatch(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size)
{
	Batch *curr_batch = m_FState.m_pCurrentBatch;

	// modelview will need to be refreshed after drawing rects
	m_State.m_bDirty_ModelView = true;

	RasterizerCanvas::Item::CommandNinePatch *np = static_cast<RasterizerCanvas::Item::CommandNinePatch *>(command);

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	BATCH_DEBUG_CRASH_COND (quad_count != (m_Data.vertices.size() / 4));

	// request all vertices at a time, this is more efficient
	BVert *bvs = m_Data.vertices.request_thirtysix();
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		m_Data.buffer_full = true;

		return false;
		//goto cleanup;
	}

	const Color &col = np->color;

	// instead of doing all the texture preparation for EVERY rect,
	// we build a list of texture combinations and do this once off.
	// This means we have a potentially rather slow step to identify which texture combo
	// using the RIDs.
	int old_bti = batch_tex_id;
	batch_tex_id = find_or_create_tex(np->texture, np->normal_map, false, old_bti);


	bool change_batch = false;

	// conditions for creating a new batch
	if ((curr_batch->type != Batch::BT_RECT) || (old_bti != batch_tex_id)) {
		change_batch = true;
	}

	// we need to treat color change separately because we need to count these
	// to decide whether to switch on the fly to colored vertices.
	if (state_set_color(col))
	{
		curr_batch = m_FState.m_pCurrentBatch;
		change_batch = true;
		m_Data.total_color_changes++;
	}

	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
		m_Data.batch_textures[batch_tex_id].tex_pixel_size.to(texpixel_size);

		// open new batch (this should never fail, it dynamically grows)
		curr_batch = request_new_batch();

		curr_batch->type = Batch::BT_RECT;
		//curr_batch->common.color.set(col);
		curr_batch->primitive.batch_texture_id = batch_tex_id;
	//	curr_batch->primitive.first_command = command_num;
		curr_batch->primitive.num_quads = 9;
		curr_batch->primitive.first_quad = quad_count;
		//curr_batch->primitive.first_vert = quad_count * 4;

		// keep the state version up to date
		m_FState.m_pCurrentBatch = curr_batch;
	} else {
		curr_batch->primitive.num_quads += 9;
	}

	// we need 16 verts to make the corners of each quad in the ninepatch (9 quads)
	BVert verts[16];

	Rect2 source = np->source;
	if (source.size.x == 0 && source.size.y == 0) {

		// tex width and height is 1.0 / texpixel_size
		source.size.x = 1.0 / texpixel_size.x;
		source.size.y = 1.0 / texpixel_size.y;
//		source.size.x = tex->width;
//		source.size.y = tex->height;
	}

	float screen_scale = 1.0;

	if (source.size.x != 0 && source.size.y != 0) {

		screen_scale = MIN(np->rect.size.x / source.size.x, np->rect.size.y / source.size.y);
		screen_scale = MIN(1.0, screen_scale);
	}

	// 1st row
	verts[0].pos.set(np->rect.position);
	verts[0].uv.set(source.position * texpixel_size);

	verts[1] = verts[0];
	verts[1].pos.x += np->margin[MARGIN_LEFT] * screen_scale;
	verts[1].uv.x += np->margin[MARGIN_LEFT] * texpixel_size.x;

	verts[3] = verts[0];
	verts[3].pos.x += np->rect.size.x;
	verts[3].uv.x += source.size.x * texpixel_size.x;

	verts[2] = verts[3];
	verts[2].pos.x -= np->margin[MARGIN_RIGHT] * screen_scale;
	verts[2].uv.x -= np->margin[MARGIN_RIGHT] * texpixel_size.x;

	// 2nd row
	// precalculate y
	float y = np->rect.position.y + np->margin[MARGIN_TOP] * screen_scale;
	float v = (source.position.y + np->margin[MARGIN_TOP]) * texpixel_size.y;

	verts[4] = verts[0];
	verts[5] = verts[1];
	verts[6] = verts[2];
	verts[7] = verts[3];

	verts[4].pos.y = y;
	verts[4].uv.y = v;
	verts[5].pos.y = y;
	verts[5].uv.y = v;
	verts[6].pos.y = y;
	verts[6].uv.y = v;
	verts[7].pos.y = y;
	verts[7].uv.y = v;

	// third row
	y = np->rect.position.y + np->rect.size.y - np->margin[MARGIN_BOTTOM] * screen_scale;
	v = (source.position.y + source.size.y - np->margin[MARGIN_BOTTOM]) * texpixel_size.y;

	verts[8] = verts[0];
	verts[9] = verts[1];
	verts[10] = verts[2];
	verts[11] = verts[3];

	verts[8].pos.y = y;
	verts[8].uv.y = v;
	verts[9].pos.y = y;
	verts[9].uv.y = v;
	verts[10].pos.y = y;
	verts[10].uv.y = v;
	verts[11].pos.y = y;
	verts[11].uv.y = v;

	// fourth row
	y = np->rect.position.y + np->rect.size.y;
	v = (source.position.y + source.size.y) * texpixel_size.y;

	verts[12] = verts[0];
	verts[13] = verts[1];
	verts[14] = verts[2];
	verts[15] = verts[3];

	verts[12].pos.y = y;
	verts[12].uv.y = v;
	verts[13].pos.y = y;
	verts[13].uv.y = v;
	verts[14].pos.y = y;
	verts[14].uv.y = v;
	verts[15].pos.y = y;
	verts[15].uv.y = v;

	// software transform
	if (m_State.m_eTransformMode == BState::TM_TRANSLATE)
	{
		for (int n=0; n<16; n++)
		{
			software_translate_vert(verts[n]);
		}
	}
	else
	{
		for (int n=0; n<16; n++)
		{
			software_transform_vert(verts[n]);
		}
	}

	// now convert into quads
	int count = 0;
	for (int y=0; y<3; y++)
	{
		for (int x=0; x<3; x++)
		{
			// top left, top right, bot right, bot left
			bvs[count++] = verts[(y * 4) + x];
			bvs[count++] = verts[(y * 4) + x + 1];
			bvs[count++] = verts[((y+1) * 4) + x + 1];
			bvs[count++] = verts[((y+1) * 4) + x];
		}
	}

	// increment quad count
	quad_count += 9;

	return true;
}

void CanvasBatcher::flush()
{
	// debug
#ifdef BATCH_DEBUG_FRAMES
//	if (g_bBatcherLogFrame)
	{
//		if ((rand() % 1000) == 0)

		if (!Engine::get_singleton()->is_editor_hint())
			debug_log_run();
	}
#endif


	// zero all the batch data ready for the next run
	m_Data.reset_flush();
	m_FState.reset_flush();
}


// false if buffers full
bool CanvasBatcher::process_commands()
{
	RasterizerCanvas::Item * pItem = m_FState.get_item();

	int command_count = pItem->commands.size();
//	int command_start = m_FState.m_iCommand;

	// to start with we will allow using the legacy non-batched method
	bool use_batching = true;

	// while there are still more batches to fill...
	// we may have to do this multiple times because there is a limit of 65535
	// verts referenced in the index buffer (each potential run of this loop)
	while (m_FState.m_iCommand < command_count) {
		if (use_batching) {
			// fill as many batches as possible (until all done, or the vertex buffer is full)
			m_FState.m_iCommand = fill(m_FState.m_iCommand);
		} else {
			// legacy .. just create one massive batch and render everything as before
			m_Data.reset_pass();
			Batch *batch = m_Data.request_new_batch();
			batch->type = Batch::BT_CHANGE_ITEM;
			batch->item_change.m_pItem = m_FState.m_pItem;

			batch = m_Data.request_new_batch();
			batch->type = Batch::BT_DEFAULT;
			batch->def.num_commands = command_count;

			// signify to do only one while loop
			m_FState.m_iCommand = command_count;
		}

		// only flush when buffer full
		if (m_Data.buffer_full)
		{
			return false;
		}
			//flush();

	} // while there are still more batches to fill


	// if extra matrix was set, we need to unset it
	state_unset_extra_matrix();

	//_flush();

	return true;
}

void CanvasBatcher::process_lit_item_end()
{
	state_light_end();
	state_invalidate_blend_mode();

	// final modulate?
//	Batch * b = request_new_batch();
//	b->type = Batch::BT_CHANGE_COLOR_MODULATE;
//	b->color_change.color = m_FState.m_Col_modulate;

}

bool CanvasBatcher::process_lit_item_start(RasterizerCanvas::Item *ci, AliasLight *p_light)
{
	// lights need to know the item, no opportunity to batch around this
	//state_set_item(ci);
	state_flush_item();

	m_PCommon.m_pLight = p_light;
	state_light_begin(p_light);

	process_item_start(ci);

	// prevent flushing item twice
	m_FState.m_bDirty_Item = false;



	//m_FState.light_used = false;
	//m_FState.light_mode = VS::CANVAS_LIGHT_MODE_ADD;

	// NOT DONE YET
	//state.uniforms.final_modulate = ci->final_modulate; // remove the canvas modulate

//	if (!light_used || mode != light->mode) {

//		mode = light->mode;

	/*
		RasterizerCanvas::Light * pLight = (RasterizerCanvas::Light *) p_light;

		switch (pLight->mode) {

			case VS::CANVAS_LIGHT_MODE_ADD: {
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			} break;
			case VS::CANVAS_LIGHT_MODE_SUB: {
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			} break;
			case VS::CANVAS_LIGHT_MODE_MIX:
			case VS::CANVAS_LIGHT_MODE_MASK: {
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			} break;
		}
	//}

		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, true);
*/
	return true;
}

bool CanvasBatcher::process_item_start(RasterizerCanvas::Item *ci)
{
	m_State.reset_item();
	m_FState.reset_item();

	// changed item?
	state_set_item(ci);
//	state_flush_item();


	// scissor?
	state_set_scissor(ci->final_clip_owner);

	// copy back buffer?
	state_copy_back_buffer(ci);

	RasterizerCanvas::Item *material_owner;
	if (ci->material_owner)
		material_owner = ci->material_owner;
	else
		material_owner = ci;

	state_set_RID_material(material_owner->material);



	/*
	if (material != canvas_last_material || rebind_shader) {

		RasterizerStorageGLES2::Shader *shader_ptr = NULL;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = NULL; // not a canvas item shader, don't use.
			}
		}

		if (shader_ptr) {
			if (shader_ptr->canvas_item.uses_screen_texture) {
				if (!state.canvas_texscreen_used) {
					//copy if not copied before
					_copy_texscreen(Rect2());

					// blend mode will have been enabled so make sure we disable it again later on
					//last_blend_mode = last_blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_DISABLED ? last_blend_mode : -1;
				}

				if (storage->frame.current_rt->copy_screen_effect.color) {
					glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
					glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->copy_screen_effect.color);
				}
			}

			if (shader_ptr != shader_cache) {

				if (shader_ptr->canvas_item.uses_time) {
					VisualServerRaster::redraw_request();
				}

				state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
				state.canvas_shader.bind();
			}

			int tc = material_ptr->textures.size();
			Pair<StringName, RID> *textures = material_ptr->textures.ptrw();

			ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

			for (int i = 0; i < tc; i++) {

				glActiveTexture(GL_TEXTURE0 + i);

				RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(textures[i].second);

				if (!t) {

					switch (texture_hints[i]) {
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
						} break;
						default: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
						} break;
					}

					continue;
				}

				if (t->redraw_if_visible) {
					VisualServerRaster::redraw_request();
				}

				t = t->get_ptr();

#ifdef TOOLS_ENABLED
				if (t->detect_normal && texture_hints[i] == ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL) {
					t->detect_normal(t->detect_normal_ud);
				}
#endif
				if (t->render_target)
					t->render_target->used_in_frame = true;

				glBindTexture(t->target, t->tex_id);
			}

		} else {
			state.canvas_shader.set_custom_shader(0);
			state.canvas_shader.bind();
		}
		state.canvas_shader.use_material((void *)material_ptr);

		shader_cache = shader_ptr;

		canvas_last_material = material;

		rebind_shader = false;
	}
*/
	state_set_blend_mode(get_owner()->batch_get_blendmode_from_shader(m_State.m_pShader));


	//	int blend_mode = shader_cache ? shader_cache->canvas_item.blend_mode : RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX;
	//	bool unshaded = shader_cache && (shader_cache->canvas_item.light_mode == RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA));
	//	bool reclip = false;

		/*
		if (last_blend_mode != blend_mode) {

			switch (blend_mode) {

				case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					}

				} break;
				case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_ADD: {

					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
					}

				} break;
				case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_SUB: {

					glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
					}
				} break;
				case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MUL: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
					} else {
						glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
					}
				} break;
				case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					} else {
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					}
				} break;
			}
		}
	*/
		//state.uniforms.final_modulate = unshaded ? ci->final_modulate : Color(ci->final_modulate.r * p_modulate.r, ci->final_modulate.g * p_modulate.g, ci->final_modulate.b * p_modulate.b, ci->final_modulate.a * p_modulate.a);

	RasterizerStorageGLES2::Shader * pShader = (RasterizerStorageGLES2::Shader *) m_State.m_pShader;
	int blend_mode = m_State.m_iBlendMode;

	bool unshaded = pShader && (pShader->canvas_item.light_mode == RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA));

	// the base modulate color of this set of items
	const Color &p_modulate = m_PCommon.m_colModulate;

	Color col_mod = unshaded ? ci->final_modulate : Color(ci->final_modulate.r * p_modulate.r, ci->final_modulate.g * p_modulate.g, ci->final_modulate.b * p_modulate.b, ci->final_modulate.a * p_modulate.a);

	// only send the modulate color if we are not doing lighting
	if (!is_lit())
		state_set_color_modulate(col_mod);

		// set modelview matrix
	//	state.uniforms.modelview_matrix = ci->final_transform;
	state_set_modelview(ci->final_transform);

		// only do this for testing
		//state_flush_modelview();

	//	bfstate.modelview_matrix = ci->final_transform;
	//	state.uniforms.extra_matrix = Transform2D();

		// does this need to be done here?
		//_set_uniforms();

	bool bRenderNoLighting = (unshaded || (col_mod.a > 0.001 && (!pShader || pShader->canvas_item.light_mode != RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !ci->light_masked));

	return bRenderNoLighting;
}

bool CanvasBatcher::process_lit_item(RasterizerCanvas::Item *ci)
{
	return process_item(ci);
}


bool CanvasBatcher::process_item(RasterizerCanvas::Item *ci)
{
	//m_State.reset_item();

//	if (unshaded || (state.uniforms.final_modulate.a > 0.001 && (!shader_cache || shader_cache->canvas_item.light_mode != RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !ci->light_masked))
	{
//		_canvas_item_render_commands(p_item_list, NULL, reclip, material_ptr);
		if (!process_commands())
			return false;

		// last item?
//		if (!p_item_list->next)
//			_flush();
	}

//	rebind_shader = true; // hacked in for now.


	//_flush();
	return true;
}

void CanvasBatcher::batch_translate_to_colored()
{
	BatchData &bdata = m_Data;
	bdata.use_colored_vertices = true;

	bdata.vertices_colored.reset();
	bdata.batches_temp.reset();

	// As the vertices_colored and batches_temp are 'mirrors' of the non-colored version,
	// the sizes should be equal, and allocations should never fail. Hence the use of debug
	// asserts to check program flow, these should not occur at runtime unless the allocation
	// code has been altered.
	BATCH_DEBUG_CRASH_COND(bdata.vertices_colored.max_size() != bdata.vertices.max_size());
	BATCH_DEBUG_CRASH_COND(bdata.batches_temp.max_size() != bdata.batches.max_size());

	BColor curr_col;
	curr_col.set(-1, -1, -1, -1);

	//Batch *pSourceBatch = 0;
	Batch *pDestBatch = 0;

	// translate the batches into vertex colored batches
	for (int n = 0; n < bdata.batches.size(); n++) {
		const Batch &b = bdata.batches[n];

		// changing color
		if (b.type == Batch::BT_CHANGE_COLOR)
		{
			curr_col = get_color(b.index.id);
			//curr_col = b.color_change.color;
			continue;
		}

		bool needs_new_batch = true;

		// is the d batch the same except for the color?
		if (b.type == Batch::BT_RECT && pDestBatch)
		{
			if (b.primitive.batch_texture_id == pDestBatch->primitive.batch_texture_id)
			{
				needs_new_batch = false;

				// add to previous batch
				pDestBatch->primitive.num_quads += b.primitive.num_quads;

				// create colored verts
				add_colored_verts(b, curr_col);
			}
		}

		if (needs_new_batch) {
			pDestBatch = bdata.batches_temp.request();
			BATCH_DEBUG_CRASH_COND(!pDestBatch);

			*pDestBatch = b;

			add_colored_verts(*pDestBatch, curr_col);

			// create the colored verts (only if not default)
			if (b.type == Batch::BT_RECT)
//			if (b.is_compactable())
				;
			else
				// there is no valid destination batch, we need to start again
				pDestBatch = 0;
		}
	}

	// copy the temporary batches to the master batch list (this could be avoided but it makes the code cleaner)
	bdata.batches.copy_from(bdata.batches_temp);
}



} // namespace
