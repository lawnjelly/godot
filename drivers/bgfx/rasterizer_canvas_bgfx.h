#pragma once

#include "drivers/gles_common/rasterizer_canvas_batcher.h"
#include "rasterizer_canvas_base_bgfx.h"

class RasterizerCanvasBGFX : public RasterizerCanvasBaseBGFX, public RasterizerCanvasBatcher<RasterizerCanvasBGFX, RasterizerStorageBGFX> {
	friend class RasterizerCanvasBatcher<RasterizerCanvasBGFX, RasterizerStorageBGFX>;

	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_transform);

private:
	// legacy codepath .. to remove after testing
	void _legacy_canvas_render_item(Item *p_ci, RenderItemState &r_ris);

	// high level batch funcs
	void canvas_render_items_implementation(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void render_joined_item(const BItemJoined &p_bij, RenderItemState &r_ris);
	bool try_join_item(Item *p_ci, RenderItemState &r_ris, bool &r_batch_break);
	void render_batches(Item *p_current_clip, bool &r_reclip, RasterizerStorageBGFX::Material *p_material);

	// low level batch funcs
	void _batch_upload_buffers();
	void _batch_render_generic(const Batch &p_batch, RasterizerStorageBGFX::Material *p_material);
	void _batch_render_lines(const Batch &p_batch, RasterizerStorageBGFX::Material *p_material, bool p_anti_alias);

	// funcs used from rasterizer_canvas_batcher template
	void gl_enable_scissor(int p_x, int p_y, int p_width, int p_height) const;
	void gl_disable_scissor() const;

public:
	RasterizerCanvasBGFX() {}
	~RasterizerCanvasBGFX() {}
};
