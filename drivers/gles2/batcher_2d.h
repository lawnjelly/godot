#pragma once

#include "batch_types.h"
#include "batch_flags.h"
#include "batch_data.h"
#include "servers/visual/rasterizer.h"

class Renderer2dGles2;

namespace Batch
{

class Batcher2d
{
public:
	BatchData m_Data;
private:

	struct BState {
		enum StateFlags {
			SF_FORCE_FLUSH = 1 << 0,
			SF_SCISSOR = 1 << 1,
			SF_SKELETON = 1 << 2,
		};

		// which modelview software transformations to apply on vertices before
		// putting in buffer
		enum TransformMode {
			TM_NONE,
			TM_TRANSLATE,
			TM_ALL,
		};


		BState() {reset_pass();}
		void reset_pass() {
			m_Flags.clear_flags();
			m_pMaterial = 0;
			//m_iBatchMatID = -1;
			m_iBlendMode = -1;
			m_pShader = 0;
			m_RID_curr_Material = RID();
			m_bDirty_ModelView = false;
			m_bExtraMatrixSet = false;
			m_bModelView_SentThisItem = false;
			//m_bTransform_Translate_Only = false;

			m_eTransformMode = TM_ALL;
		}
		//bool software_transform_needed() const {return m_bDirty_ModelView;}

		void reset_item()
		{
			m_bModelView_SentThisItem = false;
		}

	private:
		BFlags<uint32_t> m_Flags;

	public:
		AliasMaterial *m_pMaterial;
		AliasShader *m_pShader;
		int m_iBlendMode;
		RID m_RID_curr_Material;
		//int m_iBatchMatID;

		// usually the combined matrix will just be the model view, except when an extra
		// matrix is set. Extra matrix in the shader is for instancing (e.g. particles) and may be able to be
		// removed at some point.
		Transform2D m_matCombined;

		Transform2D m_matModelView;
		Transform2D m_matExtra;


		bool m_bDirty_ModelView;

		// we only need to send model view at most once per item
		bool m_bModelView_SentThisItem;

		bool m_bExtraMatrixSet;
		//bool m_bTransform_Translate_Only;

		TransformMode m_eTransformMode;
	//		Item * m_pCurrentClipItem;

	//		bool m_bRebindShader;
		//bool m_bUseSkeleton;
	//		bool m_bTexScreen_Used;

	//		bool m_bConditional_UseSkeleton;
	//		RID m_RID_curr_Tex;
	//		RID m_RID_curr_Norm;

	//		RasterizerStorageGLES2::Texture *m_pCurrTex;
	} m_State;


	struct FillState {
		FillState() {reset_pass();}
		void reset_pass()
		{
			m_pItem = 0;
			m_pClipItem = 0;
			m_iCommand = 0;
			reset_flush();
		}
		void reset_flush()
		{
			m_pCurrentBatch = 0;
			m_bDirty_Item = false;
			m_CurrentCol.r = -100.0;
			reset_col_modulate();
			m_bDirty = false;
			m_bPreviousLineOdd = false;
		}
		void reset_item()
		{
			// start of commands
			m_iCommand = 0;
		}
		void reset_if_dirty()
		{
			if (!m_bDirty) return;
			m_bDirty = false;
			reset_col_modulate();
		}
		void reset_col_modulate() {m_Col_modulate.r = -100.0;}
		void make_dirty() {m_bDirty = true;}
		RasterizerCanvas::Item * get_item() const {return (RasterizerCanvas::Item *) m_pItem;}
		AliasItem * m_pItem;
		AliasItem * m_pClipItem;
		bool m_bDirty_Item;
		int m_iCommand;
		Batch * m_pCurrentBatch;
		BColor m_CurrentCol;
		BColor m_Col_modulate;

		// if unhandled default batches have been processed, the opengl state might need
		// refreshing
		bool m_bDirty;

		// lines are special, they use 2 verts instead of 4,
		// so we have to keep track of whether we can reuse the previous 2 verts for the next line
		bool m_bPreviousLineOdd;
	} m_FState;

	String m_szDebugLog;

public:
	// usually the end of a frame, but can be a pass
	void pass_end();

	// called at the start of each canvas_render_items
//	void process_prepare(Renderer2dGles2 * pOwner, int p_z, const Color &p_modulate, AliasLight*p_light, const Transform2D &p_base_transform);
	void process_prepare(Renderer2dGles2 * pOwner, int p_z, const Color &p_modulate, const Transform2D &p_base_transform);

	//void process_next_light(AliasLight *p_light) {m_PCommon.m_pLight = p_light;	}

	// call this repeatedly until all items processed
	// returns true if finished, or
	// false if buffers full and needs rendering
	bool process_item(RasterizerCanvas::Item *ci);
	bool process_lit_item(RasterizerCanvas::Item *ci);

	// returns whether to render with no lighting
	bool process_item_start(RasterizerCanvas::Item *ci);
	bool process_lit_item_start(RasterizerCanvas::Item *ci, AliasLight *p_light);
	void process_lit_item_end();

	void flush();// {flush_heavy();}
	void batch_translate_to_colored();


	const BRectI &get_recti(int index) const {return m_Data.generic_128s[index].recti;}
	const BRectF &get_rectf(int index) const {return m_Data.generic_128s[index].rectf;}
	const BColor &get_color(int index) const {return m_Data.generic_128s[index].color;}

	//void reset_batches();
private:
	// false if buffers full
	bool process_commands();

	void fill_sync_verts();
	int fill(int p_command_start);
	bool fill_rect(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size);
	bool fill_line(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size);
	bool fill_ninepatch(int command_num, RasterizerCanvas::Item::Command *command, int &batch_tex_id, int &quad_count, Vector2 &texpixel_size);

	int find_or_create_tex(const RID &p_texture, const RID &p_normal, bool p_tile, int p_previous_match);

	void debug_log_run();
	void debug_log_type(String sz, int spacer = 0);
	void debug_log_int(String field, int val);
	void debug_log_color(String field, const BColor &col);
	void debug_log_type_col(String sz, const BColor &col, int spacer = 0);

	Renderer2dGles2 * get_owner() const {return m_PCommon.m_pOwner;}

	void state_set_item(RasterizerCanvas::Item *pItem);
	void state_set_RID_material(const RID &rid);
	void state_set_blend_mode(int mode);
	void state_invalidate_blend_mode() {m_State.m_iBlendMode = -1;}
	void state_set_modelview(const Transform2D &tr);
	bool state_set_color(const Color &col);
	void state_set_color_modulate(const Color &col);
	void state_set_scissor(RasterizerCanvas::Item *pClipItem);
	void state_copy_back_buffer(RasterizerCanvas::Item *pItem);
	void state_set_extra_matrix(const Transform2D &tr);
	void state_unset_extra_matrix();
	void state_light_begin(AliasLight * pLight);
	void state_light_end();


	void state_flush_modelview();
	void state_flush_item();
	void choose_software_transform_mode();

	Batch * request_new_batch();
	B128 * _request_new_B128(Batch &batch);

	BRectI * request_new_recti(Batch & batch)
	{
		B128 * p = _request_new_B128(batch);
		return &p->recti;
	}
	BRectF * request_new_rectf(Batch & batch)
	{
		B128 * p = _request_new_B128(batch);
		return &p->rectf;
	}
	BColor * request_new_color(Batch & batch)
	{
		B128 * p = _request_new_B128(batch);
		return &p->color;
	}


	struct ProcessCommon
	{
		Renderer2dGles2 * m_pOwner;
		int m_iZ;
		Color m_colModulate;
		AliasLight * m_pLight;
		Transform2D m_trBase;
	} m_PCommon;

	bool is_lit() const {return m_PCommon.m_pLight != 0;}

	void software_transform_vert(BVert &v) const
	{
		Vector2 vc(v.pos.x, v.pos.y);
		vc = m_State.m_matCombined.xform(vc);
		v.pos.set(vc);

		// test
		//v.pos.x -= 60.0f;
	}

	void software_translate_vert(BVert &v) const
	{
		v.pos.x += m_State.m_matCombined.elements[2].x;
		v.pos.y += m_State.m_matCombined.elements[2].y;
	}

	void software_transform_vert(Vector2 &v) const
	{
		v = m_State.m_matCombined.xform(v);
	}

	void software_translate_vert(Vector2 &v) const
	{
		v.x += m_State.m_matCombined.elements[2].x;
		v.y += m_State.m_matCombined.elements[2].y;
	}

	void software_translate_rect(Rect2 &rect) const
	{
//		if (m_State.software_transform_needed())
//		{
//			if (m_State.m_bTransform_Translate_Only)
//			{
				rect.position.x += m_State.m_matCombined.elements[2].x;
				rect.position.y += m_State.m_matCombined.elements[2].y;
//			}
//			else
//				rect = m_State.m_matModelView.xform(rect);
//		}
	}

	void add_colored_verts(const Batch &b, const BColor &curr_col)
	{
		if (!b.is_compactable())
			return;

		// create colored verts
		int first_vert = b.primitive.first_quad * 4;
		int end_vert = 4 * (b.primitive.first_quad + b.primitive.num_quads);
		//int first_vert = b.primitive.first_vert;
		//int end_vert = first_vert + (4 * b.primitive.num_commands);

		/*
		String sz = "";
		sz += "first_vert : " + itos(first_vert) + ",\tend_vert : " + itos(end_vert);

		if (b.type == Batch::BT_LINE)
		{
			sz += "\tfirst_quad : " + itos(b.primitive.first_quad);
		}
		print_line(sz);
		*/

		CRASH_COND(first_vert != m_Data.vertices_colored.size());

		for (int v = first_vert; v < end_vert; v++)
		{
			const BVert &bv = m_Data.vertices[v];
			BVert_colored *cv = m_Data.vertices_colored.request();
#ifdef DEBUG_ENABLED
			CRASH_COND(!cv);
#endif
			cv->pos = bv.pos;
			cv->uv = bv.uv;
			cv->col = curr_col;
		}
	}
};


} // namespace
