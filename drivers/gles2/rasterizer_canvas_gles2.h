#pragma once

#include "core/math/camera_matrix.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rasterizer.h"
#include "servers/rendering_server.h"
//#include "rasterizer_gles2.h"


class RasterizerCanvasGLES2 : public RasterizerCanvas {
public:
	PolygonID request_polygon(const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), const Vector<int> &p_bones = Vector<int>(), const Vector<float> &p_weights = Vector<float>()) override { return 0; }
	void free_polygon(PolygonID p_polygon) override {}
	
	void canvas_render_items(RID p_to_render_target, Item *p_item_list, const Color &p_modulate, Light *p_light_list, Light *p_directional_list, const Transform2D &p_canvas_transform, RS::CanvasItemTextureFilter p_default_filter, RS::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel) override {}
	void canvas_debug_viewport_shadows(Light *p_lights_with_shadow) override {}
	
	RID light_create() override { return RID(); }
	void light_set_texture(RID p_rid, RID p_texture) override {}
	void light_set_use_shadow(RID p_rid, bool p_enable) override {}
	void light_update_shadow(RID p_rid, int p_shadow_index, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders) override {}
	void light_update_directional_shadow(RID p_rid, int p_shadow_index, const Transform2D &p_light_xform, int p_light_mask, float p_cull_distance, const Rect2 &p_clip_rect, LightOccluderInstance *p_occluders) override {}
	
	RID occluder_polygon_create() override { return RID(); }
	void occluder_polygon_set_shape_as_lines(RID p_occluder, const Vector<Vector2> &p_lines) override {}
	void occluder_polygon_set_cull_mode(RID p_occluder, RS::CanvasOccluderPolygonCullMode p_mode) override {}
	void set_shadow_texture_size(int p_size) override {}
	
	void draw_window_margins(int *p_margins, RID *p_margin_textures) override {}
	
	bool free(RID p_rid) override { return true; }
	void update() override {}
	
	RasterizerCanvasGLES2() {}
	~RasterizerCanvasGLES2() {}
};
