#pragma once

#include "rasterizer_canvas_simple_gles2.h"

class RasterizerCanvasGLES2 : public RasterizerCanvasSimpleGLES2 {
public:

	virtual void canvas_begin();
	virtual void canvas_end();

	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);


	// batching inherited virtuals
	virtual Batch::AliasMaterial * batch_get_material_and_shader_from_RID(const RID &rid, Batch::AliasShader ** ppShader) const;
	virtual int batch_get_blendmode_from_shader(Batch::AliasShader * pShader) const;
	virtual bool batch_get_texture_pixel_size_and_tile(const RID &rid, float &x, float &y, bool p_tile, bool &npot) const;
	virtual void _batch_get_item_clip_rect(const Item &item, int &x, int &y, int &width, int &height) const;

	RasterizerStorageGLES2::Texture *batch_get_canvas_texture(const RID &p_texture) const;

	virtual void _batch_render_rects(const Batch::Batch &batch, RasterizerStorageGLES2::Material *p_material);
	virtual void _batch_render_lines(const Batch::Batch &batch, RasterizerStorageGLES2::Material *p_material);

	void _batch_canvas_shader_bind(RasterizerStorageGLES2::Material * p_material, bool use_texture_rect = false);
	void _batch_canvas_shader_bind_simple();

	virtual void _batch_render_default_batch(int batch_id, Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material);
	virtual void _batch_copy_back_buffer(const Rect2 &rect);
	virtual void _batch_change_material();
	virtual void _batch_change_blend_mode(int p_blend_mode);
	virtual void _batch_change_color_modulate(const Color &col);
	//virtual void _batch_change_color_modulate(const Color &col, bool bRedundant);
	virtual void _batch_unset_extra_matrix();
	virtual void _batch_light_begin(Batch::AliasLight * pLight, Item *p_item);
	virtual void _batch_light_end();

	RasterizerCanvasGLES2();
};
