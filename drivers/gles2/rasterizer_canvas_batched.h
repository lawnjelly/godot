#pragma once

#include "servers/visual/rasterizer.h"
#include "rasterizer_storage_gles2.h"
#include "shaders/canvas.glsl.gen.h"
#include "canvas_batcher.h"

//namespace Batch {class Batcher2d;}

class RasterizerCanvasBatched : public RasterizerCanvas
{
protected:
	struct GLState
	{
		Transform2D m_matModelView;
		Color m_Color;
		GLState() {reset();}
		void reset() {m_matModelView = Transform2D(); m_Color = Color(1, 1, 1, 1); m_pItem = 0; m_pMaterial = 0; m_pShader = 0; m_pClipItem = 0; m_bChangedShader = false; m_bExtraMatrixSet = false;}

		RasterizerCanvas::Item * m_pItem;
		RasterizerStorageGLES2::Material * m_pMaterial;
		RasterizerStorageGLES2::Shader * m_pShader;
		RasterizerCanvas::Item * m_pClipItem;

		bool m_bChangedShader;
		bool m_bExtraMatrixSet;
	} _gl_state;

	struct GLData
	{
		GLuint gl_vertex_buffer;
		GLuint gl_index_buffer;

		uint32_t vertex_buffer_size_bytes;
		uint32_t index_buffer_size_bytes;
		GLData() {gl_vertex_buffer = 0; gl_index_buffer = 0; vertex_buffer_size_bytes = 0; index_buffer_size_bytes = 0;}
	} _gl_data;

	void batch_initialize();
	void batch_terminate();

	void batch_pass_begin();
	void batch_pass_end();

	void batch_canvas_render_items(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform);

public:
	virtual Batch::AliasMaterial * batch_get_material_and_shader_from_RID(const RID &p_rid, Batch::AliasShader ** r_shader) const = 0;
	virtual int batch_get_blendmode_from_shader(Batch::AliasShader * p_shader) const = 0;
	virtual bool batch_get_texture_pixel_size_and_tile(const RID &p_rid, float &r_x, float &r_y, bool p_tile, bool &r_npot) const = 0;
	virtual void _batch_get_item_clip_rect(const Item &p_item, int &r_x, int &r_y, int &r_width, int &r_height) const = 0;

private:
	void batch_flush();
	void batch_flush_render();
	void batch_upload_buffers();

	virtual void _batch_render_rects(const Batch::Batch &p_batch, RasterizerStorageGLES2::Material *p_material) = 0;
	virtual void _batch_render_lines(const Batch::Batch &p_batch, RasterizerStorageGLES2::Material *p_material) = 0;


	virtual void _batch_render_default_batch(int p_batch_id, Item *p_item, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material) = 0;
	virtual void _batch_change_material() = 0;
	virtual void _batch_copy_back_buffer(const Rect2 &p_rect) = 0;
	virtual void _batch_change_blend_mode(int p_blend_mode) = 0;
	virtual void _batch_change_color_modulate(const Color &p_col) = 0;
	virtual void _batch_unset_extra_matrix() = 0;
	virtual void _batch_light_begin(Batch::AliasLight * p_light, Item *p_item) = 0;
	virtual void _batch_light_end() = 0;


protected:
	Batch::CanvasBatcher _canvas_batcher;
	bool _use_batching;
};
