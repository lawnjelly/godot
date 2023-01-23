#pragma once

#include "bgfx_funcs.h"
#include "bgfx_shader.h"
#include "core/pool_vector.h"
#include "core/self_list.h"
#include "drivers/gles_common/rasterizer_asserts.h"
#include "servers/visual/rasterizer.h"
#include "shader_bgfx.h"

#include "servers/visual/shader_language.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"

class RasterizerSceneBGFX;

class RasterizerStorageBGFX : public RasterizerStorage {
public:
	RasterizerSceneBGFX *scene = nullptr;

	/////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////DATA///////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////

	struct Instantiable : public RID_Data {
		SelfList<RasterizerScene::InstanceBase>::List instance_list;

		_FORCE_INLINE_ void instance_change_notify(bool p_aabb = true, bool p_materials = true) {
			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();
			while (instances) {
				instances->self()->base_changed(p_aabb, p_materials);
				instances = instances->next();
			}
		}

		_FORCE_INLINE_ void instance_remove_deps() {
			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();

			while (instances) {
				instances->self()->base_removed();
				instances = instances->next();
			}
		}

		Instantiable() {}

		virtual ~Instantiable() {}
	};

	struct GeometryOwner : public Instantiable {
	};

	struct Geometry : public Instantiable {
		enum Type {
			GEOMETRY_INVALID,
			GEOMETRY_SURFACE,
			GEOMETRY_IMMEDIATE,
			GEOMETRY_MULTISURFACE
		};

		Type type;
		RID material;
		uint64_t last_pass;
		uint32_t index;

		virtual void material_changed_notify() {}

		Geometry() {
			last_pass = 0;
			index = 0;
		}
	};

	/* TEXTURE API */
	struct RenderTarget;

	struct Texture : public RID_Data {
		int width = 0;
		int height = 0;
		uint32_t flags = 0;
		Image::Format format;
		Ref<Image> image;
		String path;

		bgfx::TextureHandle bg_handle = BGFX_INVALID_HANDLE;

		// Depending on how often this texture is updated, it can either be static
		// (default) or dynamic if the update count is high.
		uint32_t bg_update_count = 0;
		bool bg_is_mutable = false;

		bool redraw_if_visible = false;

		RenderTarget *render_target = nullptr;

		Set<Texture *> proxy_owners;
		Texture *proxy = nullptr;

		VisualServer::TextureDetectCallback detect_3d = nullptr;
		void *detect_3d_ud = nullptr;

		VisualServer::TextureDetectCallback detect_srgb = nullptr;
		void *detect_srgb_ud = nullptr;

		VisualServer::TextureDetectCallback detect_normal = nullptr;
		void *detect_normal_ud = nullptr;

		Texture *get_ptr() {
			if (proxy) {
				return proxy; //->get_ptr(); only one level of indirection, else not inlining possible.
			} else {
				return this;
			}
		}

		~Texture() {
			//			if (tex_id != 0) {
			//				glDeleteTextures(1, &tex_id);
			//			}
			BGFX_DESTROY(bg_handle);

			for (Set<Texture *>::Element *E = proxy_owners.front(); E; E = E->next()) {
				E->get()->proxy = nullptr;
			}

			if (proxy) {
				proxy->proxy_owners.erase(this);
			}
		}
	};

	mutable RID_Owner<Texture> texture_owner;

	RID texture_create();
	void texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, VisualServer::TextureType p_type = VS::TEXTURE_TYPE_2D, uint32_t p_flags = VS::TEXTURE_FLAGS_DEFAULT);
	void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_level);

	void texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_level) {
		Texture *t = texture_owner.get(p_texture);

		ERR_FAIL_COND(!t);
		ERR_FAIL_COND_MSG(p_image.is_null(), "It's not a reference to a valid Image object.");
		ERR_FAIL_COND(t->format != p_image->get_format());
		ERR_FAIL_COND(src_w <= 0 || src_h <= 0);
		ERR_FAIL_COND(src_x < 0 || src_y < 0 || src_x + src_w > p_image->get_width() || src_y + src_h > p_image->get_height());
		ERR_FAIL_COND(dst_x < 0 || dst_y < 0 || dst_x + src_w > t->width || dst_y + src_h > t->height);

		t->image->blit_rect(p_image, Rect2(src_x, src_y, src_w, src_h), Vector2(dst_x, dst_y));
	}

	Ref<Image> texture_get_data(RID p_texture, int p_level) const {
		Texture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, Ref<Image>());
		return t->image;
	}
	void texture_set_flags(RID p_texture, uint32_t p_flags);
	uint32_t texture_get_flags(RID p_texture) const {
		Texture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, 0);
		return t->flags;
	}
	Image::Format texture_get_format(RID p_texture) const {
		Texture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, Image::FORMAT_RGB8);
		return t->format;
	}

	VisualServer::TextureType texture_get_type(RID p_texture) const { return VS::TEXTURE_TYPE_2D; }
	uint32_t texture_get_texid(RID p_texture) const { return 0; }
	uint32_t texture_get_width(RID p_texture) const;
	uint32_t texture_get_height(RID p_texture) const;
	uint32_t texture_get_depth(RID p_texture) const { return 0; }
	void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d);
	void texture_bind(RID p_texture, uint32_t p_texture_no) {}

	void texture_set_path(RID p_texture, const String &p_path) {
		Texture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND(!t);
		t->path = p_path;
	}
	String texture_get_path(RID p_texture) const {
		Texture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, String());
		return t->path;
	}

	void texture_set_shrink_all_x2_on_set_data(bool p_enable) {}

	void texture_debug_usage(List<VS::TextureInfo> *r_info) {}

	RID texture_create_radiance_cubemap(RID p_source, int p_resolution = -1) const { return RID(); }

	void texture_set_detect_3d_callback(RID p_texture, VisualServer::TextureDetectCallback p_callback, void *p_userdata) {}
	void texture_set_detect_srgb_callback(RID p_texture, VisualServer::TextureDetectCallback p_callback, void *p_userdata) {}
	void texture_set_detect_normal_callback(RID p_texture, VisualServer::TextureDetectCallback p_callback, void *p_userdata) {}

	void textures_keep_original(bool p_enable) {}

	void texture_set_proxy(RID p_texture, RID p_proxy);
	virtual Size2 texture_size_with_proxy(RID p_texture) const;
	void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	/* SKY API */

	struct Sky : public RID_Data {
		RID panorama;
		//uint32_t radiance = 0;
		int radiance_size = 0;

		bgfx::TextureHandle bg_tex_radiance = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle bg_tex_irradiance = BGFX_INVALID_HANDLE;

		~Sky() {
			BGFX_DESTROY(bg_tex_radiance);
			BGFX_DESTROY(bg_tex_irradiance);
		}
	};

	mutable RID_Owner<Sky> sky_owner;

	RID sky_create();
	void sky_set_texture(RID p_sky, RID p_panorama, int p_radiance_size);

	/* SHADER API */

	struct Material;

	struct Shader : public RID_Data {
		RID self;

		VS::ShaderMode mode = VS::SHADER_MAX;
		ShaderBGFX *shader = nullptr;
		String code;
		SelfList<Material>::List materials;

		Map<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;

		uint32_t texture_count = 0;

		uint32_t custom_code_id = 0;
		uint32_t version = 0;

		SelfList<Shader> dirty_list;

		Map<StringName, RID> default_textures;

		Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

		bool valid = false;

		String path;

		uint32_t index = 0;
		uint64_t last_pass = 0;

		struct CanvasItem {
			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
				BLEND_MODE_PMALPHA,
			};

			int blend_mode = 0;

			enum LightMode {
				LIGHT_MODE_NORMAL,
				LIGHT_MODE_UNSHADED,
				LIGHT_MODE_LIGHT_ONLY
			};

			int light_mode = 0;

			// these flags are specifically for batching
			// some of the logic is thus in rasterizer_storage.cpp
			// we could alternatively set bitflags for each 'uses' and test on the fly
			// defined in RasterizerStorageCommon::BatchFlags
			unsigned int batch_flags = 0;

			bool uses_screen_texture = false;
			bool uses_screen_uv = false;
			bool uses_time = false;
			bool uses_modulate = false;
			bool uses_color = false;
			bool uses_vertex = false;

			// all these should disable item joining if used in a custom shader
			bool uses_world_matrix = false;
			bool uses_extra_matrix = false;
			bool uses_projection_matrix = false;
			bool uses_instance_custom = false;

		} canvas_item;

		struct Spatial {
			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
			};

			int blend_mode = 0;

			enum DepthDrawMode {
				DEPTH_DRAW_OPAQUE,
				DEPTH_DRAW_ALWAYS,
				DEPTH_DRAW_NEVER,
				DEPTH_DRAW_ALPHA_PREPASS,
			};

			int depth_draw_mode = 0;

			enum CullMode {
				CULL_MODE_FRONT,
				CULL_MODE_BACK,
				CULL_MODE_DISABLED,
			};

			int cull_mode = 0;

			bool uses_alpha = false;
			bool uses_alpha_scissor = false;
			bool unshaded = false;
			bool no_depth_test = false;
			bool uses_vertex = false;
			bool uses_discard = false;
			bool uses_sss = false;
			bool uses_screen_texture = false;
			bool uses_depth_texture = false;
			bool uses_time = false;
			bool uses_tangent = false;
			bool uses_ensure_correct_normals = false;
			bool writes_modelview_or_projection = false;
			bool uses_vertex_lighting = false;
			bool uses_world_coordinates = false;

		} spatial;

		struct Particles {
		} particles;

		bool uses_vertex_time = false;
		bool uses_fragment_time = false;

		Shader() :
				dirty_list(this) {
			shader = nullptr;
			valid = false;
			custom_code_id = 0;
			version = 1;
			last_pass = 0;
		}
	};

	mutable RID_Owner<Shader> shader_owner;
	mutable SelfList<Shader>::List _shader_dirty_list;

	void _shader_make_dirty(Shader *p_shader);

	RID shader_create();

	void shader_set_code(RID p_shader, const String &p_code) {}
	String shader_get_code(RID p_shader) const { return ""; }
	void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const {}

	void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture);
	RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const;

	void shader_add_custom_define(RID p_shader, const String &p_define) {}
	void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const {}
	void shader_remove_custom_define(RID p_shader, const String &p_define) {}

	void set_shader_async_hidden_forbidden(bool p_forbidden) {}
	bool is_shader_async_hidden_forbidden() { return false; }

	void _update_shader(Shader *p_shader) const;
	void update_dirty_shaders();

	/* COMMON MATERIAL API */

	struct Material : public RID_Data {
		Shader *shader;
		Map<StringName, Variant> params;
		SelfList<Material> list;
		SelfList<Material> dirty_list;
		Vector<Pair<StringName, RID>> textures;
		float line_width;
		int render_priority;

		RID next_pass;

		uint32_t index;
		uint64_t last_pass;

		Map<Geometry *, int> geometry_owners;
		Map<RasterizerScene::InstanceBase *, int> instance_owners;

		bool can_cast_shadow_cache;
		bool is_animated_cache;

		Material() :
				list(this),
				dirty_list(this) {
			can_cast_shadow_cache = false;
			is_animated_cache = false;
			shader = nullptr;
			line_width = 1.0;
			last_pass = 0;
			render_priority = 0;
		}
	};

	mutable RID_Owner<Material> material_owner;

	RID material_create();

	void material_set_render_priority(RID p_material, int priority) {}
	void material_set_shader(RID p_shader_material, RID p_shader);
	RID material_get_shader(RID p_shader_material) const;

	void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value);
	Variant material_get_param(RID p_material, const StringName &p_param) const;
	Variant material_get_param_default(RID p_material, const StringName &p_param) const;

	void material_set_line_width(RID p_material, float p_width) {}

	void material_set_next_pass(RID p_material, RID p_next_material) {}

	bool material_is_animated(RID p_material) { return false; }
	bool material_casts_shadows(RID p_material) { return false; }

	void material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);
	void material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);

	void _material_add_geometry(RID p_material, Geometry *p_geometry);
	void _material_remove_geometry(RID p_material, Geometry *p_geometry);

	void update_dirty_materials();
	void _material_make_dirty(Material *p_material) const;
	void _update_material(Material *p_material);
	mutable SelfList<Material>::List _material_dirty_list;

	/* MESH API */

	struct BGFXSurface : public Geometry {
		uint32_t format = 0;
		VS::PrimitiveType primitive = VS::PRIMITIVE_POINTS;
		PoolVector<uint8_t> array;
		int vertex_count = 0;
		PoolVector<uint8_t> index_array;
		int index_count = 0;
		AABB aabb;
		Vector<PoolVector<uint8_t>> blend_shapes;
		Vector<AABB> bone_aabbs;

		enum AttribType {
			AT_UBYTE,
			AT_BYTE,
			AT_FLOAT,
			AT_USHORT,
			AT_SHORT,
			AT_HALF_FLOAT,
			AT_UINT,
		};

		enum AttribCompression {
			ATC_NONE,
			ATC_REGULAR,
			ATC_OCT16,
			ATC_OCT32,
		};

		struct Attrib {
			bool enabled;
			bool integer;
			uint32_t index;
			int size;
			AttribType type;
			AttribCompression compression = ATC_NONE;
			bool normalized;
			int stride;
			uint32_t offset;
		};

		Attrib attribs[VS::ARRAY_MAX];

		LocalVector<BGFX::PosUVNormVertex> bg_verts;
		LocalVector<uint16_t> bg_inds;
		bgfx::VertexBufferHandle bg_vertex_buffer = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle bg_index_buffer = BGFX_INVALID_HANDLE;
		~BGFXSurface() {
			BGFX_DESTROY(bg_vertex_buffer);
			BGFX_DESTROY(bg_index_buffer);
		}
	};

	struct BGFXMesh : public GeometryOwner {
		Vector<BGFXSurface *> surfaces;
		int blend_shape_count = 0;
		VS::BlendShapeMode blend_shape_mode;
		PoolRealArray blend_shape_values;
		~BGFXMesh() {
			for (int n = 0; n < surfaces.size(); n++) {
				if (surfaces[n]) {
					memdelete(surfaces[n]);
					surfaces.set(n, nullptr);
				}
			}
		}
	};

	mutable RID_Owner<BGFXMesh> mesh_owner;

	RID mesh_create() {
		BGFXMesh *mesh = memnew(BGFXMesh);
		ERR_FAIL_COND_V(!mesh, RID());
		mesh->blend_shape_count = 0;
		mesh->blend_shape_mode = VS::BLEND_SHAPE_MODE_NORMALIZED;
		return mesh_owner.make_rid(mesh);
	}

	void mesh_add_surface(RID p_mesh, uint32_t p_format, VS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>());

	void mesh_set_blend_shape_count(RID p_mesh, int p_amount) {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_count = p_amount;
	}
	int mesh_get_blend_shape_count(RID p_mesh) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->blend_shape_count;
	}

	void mesh_set_blend_shape_mode(RID p_mesh, VS::BlendShapeMode p_mode) {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_mode = p_mode;
	}
	VS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, VS::BLEND_SHAPE_MODE_NORMALIZED);
		return m->blend_shape_mode;
	}

	void mesh_set_blend_shape_values(RID p_mesh, PoolVector<float> p_values) {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_values = p_values;
	}
	PoolVector<float> mesh_get_blend_shape_values(RID p_mesh) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolRealArray());
		return m->blend_shape_values;
	}

	void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data) {}

	void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	RID mesh_surface_get_material(RID p_mesh, int p_surface) const;

	int mesh_surface_get_array_len(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface]->vertex_count;
	}
	int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface]->index_count;
	}

	PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolVector<uint8_t>());

		return m->surfaces[p_surface]->array;
	}
	PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolVector<uint8_t>());

		return m->surfaces[p_surface]->index_array;
	}

	uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface]->format;
	}
	VS::PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, VS::PRIMITIVE_POINTS);

		return m->surfaces[p_surface]->primitive;
	}

	AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, AABB());

		return m->surfaces[p_surface]->aabb;
	}
	Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, Vector<PoolVector<uint8_t>>());

		return m->surfaces[p_surface]->blend_shapes;
	}
	Vector<AABB> mesh_surface_get_skeleton_aabb(RID p_mesh, int p_surface) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, Vector<AABB>());

		return m->surfaces[p_surface]->bone_aabbs;
	}

	void mesh_remove_surface(RID p_mesh, int p_index) {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		ERR_FAIL_COND(p_index >= m->surfaces.size());

		m->surfaces.remove(p_index);
	}
	int mesh_get_surface_count(RID p_mesh) const {
		BGFXMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->surfaces.size();
	}

	void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) {}
	AABB mesh_get_custom_aabb(RID p_mesh) const { return AABB(); }

	AABB mesh_get_aabb(RID p_mesh, RID p_skeleton) const;
	void mesh_clear(RID p_mesh) {}

	/* MULTIMESH API */

	virtual RID _multimesh_create() { return RID(); }

	void _multimesh_allocate(RID p_multimesh, int p_instances, VS::MultimeshTransformFormat p_transform_format, VS::MultimeshColorFormat p_color_format, VS::MultimeshCustomDataFormat p_data = VS::MULTIMESH_CUSTOM_DATA_NONE) {}
	int _multimesh_get_instance_count(RID p_multimesh) const { return 0; }
	void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) {}
	void _multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) {}
	void _multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) {}
	void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) {}
	void _multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) {}
	RID _multimesh_get_mesh(RID p_multimesh) const { return RID(); }
	Transform _multimesh_instance_get_transform(RID p_multimesh, int p_index) const { return Transform(); }
	Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const { return Transform2D(); }
	Color _multimesh_instance_get_color(RID p_multimesh, int p_index) const { return Color(); }
	Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const { return Color(); }
	void _multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) {}
	void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}
	int _multimesh_get_visible_instances(RID p_multimesh) const { return 0; }
	AABB _multimesh_get_aabb(RID p_multimesh) const { return AABB(); }

	MMInterpolator *_multimesh_get_interpolator(RID p_multimesh) const { return nullptr; }

	/* IMMEDIATE API */

	RID immediate_create() { return RID(); }
	void immediate_begin(RID p_immediate, VS::PrimitiveType p_rimitive, RID p_texture = RID()) {}
	void immediate_vertex(RID p_immediate, const Vector3 &p_vertex) {}
	void immediate_normal(RID p_immediate, const Vector3 &p_normal) {}
	void immediate_tangent(RID p_immediate, const Plane &p_tangent) {}
	void immediate_color(RID p_immediate, const Color &p_color) {}
	void immediate_uv(RID p_immediate, const Vector2 &tex_uv) {}
	void immediate_uv2(RID p_immediate, const Vector2 &tex_uv) {}
	void immediate_end(RID p_immediate) {}
	void immediate_clear(RID p_immediate) {}
	void immediate_set_material(RID p_immediate, RID p_material) {}
	RID immediate_get_material(RID p_immediate) const { return RID(); }
	AABB immediate_get_aabb(RID p_immediate) const { return AABB(); }

	/* SKELETON API */

	RID skeleton_create() { return RID(); }
	void skeleton_allocate(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) {}
	void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D &p_base_transform) {}
	void skeleton_set_world_transform(RID p_skeleton, bool p_enable, const Transform &p_world_transform) {}
	int skeleton_get_bone_count(RID p_skeleton) const { return 0; }
	void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform &p_transform) {}
	Transform skeleton_bone_get_transform(RID p_skeleton, int p_bone) const { return Transform(); }
	void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D &p_transform) {}
	Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone) const { return Transform2D(); }
	uint32_t skeleton_get_revision(RID p_skeleton) const { return 0; }

	/* Light API */

	RID light_create(VS::LightType p_type) { return RID(); }

	RID directional_light_create() { return light_create(VS::LIGHT_DIRECTIONAL); }
	RID omni_light_create() { return light_create(VS::LIGHT_OMNI); }
	RID spot_light_create() { return light_create(VS::LIGHT_SPOT); }

	void light_set_color(RID p_light, const Color &p_color) {}
	void light_set_param(RID p_light, VS::LightParam p_param, float p_value) {}
	void light_set_shadow(RID p_light, bool p_enabled) {}
	void light_set_shadow_color(RID p_light, const Color &p_color) {}
	void light_set_projector(RID p_light, RID p_texture) {}
	void light_set_negative(RID p_light, bool p_enable) {}
	void light_set_cull_mask(RID p_light, uint32_t p_mask) {}
	void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled) {}
	void light_set_use_gi(RID p_light, bool p_enabled) {}
	void light_set_bake_mode(RID p_light, VS::LightBakeMode p_bake_mode) {}

	void light_omni_set_shadow_mode(RID p_light, VS::LightOmniShadowMode p_mode) {}
	void light_omni_set_shadow_detail(RID p_light, VS::LightOmniShadowDetail p_detail) {}

	void light_directional_set_shadow_mode(RID p_light, VS::LightDirectionalShadowMode p_mode) {}
	void light_directional_set_blend_splits(RID p_light, bool p_enable) {}
	bool light_directional_get_blend_splits(RID p_light) const { return false; }
	void light_directional_set_shadow_depth_range_mode(RID p_light, VS::LightDirectionalShadowDepthRangeMode p_range_mode) {}
	VS::LightDirectionalShadowDepthRangeMode light_directional_get_shadow_depth_range_mode(RID p_light) const { return VS::LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_STABLE; }

	VS::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light) { return VS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL; }
	VS::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light) { return VS::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID; }

	bool light_has_shadow(RID p_light) const { return false; }

	VS::LightType light_get_type(RID p_light) const { return VS::LIGHT_OMNI; }
	AABB light_get_aabb(RID p_light) const { return AABB(); }
	float light_get_param(RID p_light, VS::LightParam p_param) { return 0.0; }
	Color light_get_color(RID p_light) { return Color(); }
	bool light_get_use_gi(RID p_light) { return false; }
	VS::LightBakeMode light_get_bake_mode(RID p_light) { return VS::LightBakeMode::LIGHT_BAKE_DISABLED; }
	uint64_t light_get_version(RID p_light) const { return 0; }

	/* PROBE API */

	RID reflection_probe_create() { return RID(); }

	void reflection_probe_set_update_mode(RID p_probe, VS::ReflectionProbeUpdateMode p_mode) {}
	void reflection_probe_set_intensity(RID p_probe, float p_intensity) {}
	void reflection_probe_set_interior_ambient(RID p_probe, const Color &p_ambient) {}
	void reflection_probe_set_interior_ambient_energy(RID p_probe, float p_energy) {}
	void reflection_probe_set_interior_ambient_probe_contribution(RID p_probe, float p_contrib) {}
	void reflection_probe_set_max_distance(RID p_probe, float p_distance) {}
	void reflection_probe_set_extents(RID p_probe, const Vector3 &p_extents) {}
	void reflection_probe_set_origin_offset(RID p_probe, const Vector3 &p_offset) {}
	void reflection_probe_set_as_interior(RID p_probe, bool p_enable) {}
	void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable) {}
	void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable) {}
	void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers) {}
	void reflection_probe_set_resolution(RID p_probe, int p_resolution) {}

	AABB reflection_probe_get_aabb(RID p_probe) const { return AABB(); }
	VS::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe) const { return VisualServer::REFLECTION_PROBE_UPDATE_ONCE; }
	uint32_t reflection_probe_get_cull_mask(RID p_probe) const { return 0; }
	Vector3 reflection_probe_get_extents(RID p_probe) const { return Vector3(); }
	Vector3 reflection_probe_get_origin_offset(RID p_probe) const { return Vector3(); }
	float reflection_probe_get_origin_max_distance(RID p_probe) const { return 0.0; }
	bool reflection_probe_renders_shadows(RID p_probe) const { return false; }

	void instance_add_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance) {}
	void instance_remove_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance) {}

	void instance_add_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) {}
	void instance_remove_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) {}

	/* GI PROBE API */

	RID gi_probe_create() { return RID(); }

	void gi_probe_set_bounds(RID p_probe, const AABB &p_bounds) {}
	AABB gi_probe_get_bounds(RID p_probe) const { return AABB(); }

	void gi_probe_set_cell_size(RID p_probe, float p_range) {}
	float gi_probe_get_cell_size(RID p_probe) const { return 0.0; }

	void gi_probe_set_to_cell_xform(RID p_probe, const Transform &p_xform) {}
	Transform gi_probe_get_to_cell_xform(RID p_probe) const { return Transform(); }

	void gi_probe_set_dynamic_data(RID p_probe, const PoolVector<int> &p_data) {}
	PoolVector<int> gi_probe_get_dynamic_data(RID p_probe) const {
		PoolVector<int> p;
		return p;
	}

	void gi_probe_set_dynamic_range(RID p_probe, int p_range) {}
	int gi_probe_get_dynamic_range(RID p_probe) const { return 0; }

	void gi_probe_set_energy(RID p_probe, float p_range) {}
	float gi_probe_get_energy(RID p_probe) const { return 0.0; }

	void gi_probe_set_bias(RID p_probe, float p_range) {}
	float gi_probe_get_bias(RID p_probe) const { return 0.0; }

	void gi_probe_set_normal_bias(RID p_probe, float p_range) {}
	float gi_probe_get_normal_bias(RID p_probe) const { return 0.0; }

	void gi_probe_set_propagation(RID p_probe, float p_range) {}
	float gi_probe_get_propagation(RID p_probe) const { return 0.0; }

	void gi_probe_set_interior(RID p_probe, bool p_enable) {}
	bool gi_probe_is_interior(RID p_probe) const { return false; }

	void gi_probe_set_compress(RID p_probe, bool p_enable) {}
	bool gi_probe_is_compressed(RID p_probe) const { return false; }

	uint32_t gi_probe_get_version(RID p_probe) { return 0; }

	RID gi_probe_dynamic_data_create(int p_width, int p_height, int p_depth, GIProbeCompression p_compression) { return RID(); }
	void gi_probe_dynamic_data_update(RID p_gi_probe_data, int p_depth_slice, int p_slice_count, int p_mipmap, const void *p_data) {}

	/* LIGHTMAP CAPTURE */
	//	struct Instantiable : public RID_Data {
	//		SelfList<RasterizerScene::InstanceBase>::List instance_list;

	//		_FORCE_INLINE_ void instance_change_notify(bool p_aabb = true, bool p_materials = true) {
	//			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();
	//			while (instances) {
	//				instances->self()->base_changed(p_aabb, p_materials);
	//				instances = instances->next();
	//			}
	//		}

	//		_FORCE_INLINE_ void instance_remove_deps() {
	//			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();
	//			while (instances) {
	//				SelfList<RasterizerScene::InstanceBase> *next = instances->next();
	//				instances->self()->base_removed();
	//				instances = next;
	//			}
	//		}

	//		Instantiable() {}
	//		virtual ~Instantiable() {
	//		}
	//	};

	struct LightmapCapture : public Instantiable {
		PoolVector<LightmapCaptureOctree> octree;
		AABB bounds;
		Transform cell_xform;
		int cell_subdiv;
		float energy;
		LightmapCapture() {
			energy = 1.0;
			cell_subdiv = 1;
		}
	};

	mutable RID_Owner<LightmapCapture> lightmap_capture_data_owner;
	void lightmap_capture_set_bounds(RID p_capture, const AABB &p_bounds) {}
	AABB lightmap_capture_get_bounds(RID p_capture) const { return AABB(); }
	void lightmap_capture_set_octree(RID p_capture, const PoolVector<uint8_t> &p_octree) {}
	RID lightmap_capture_create() {
		LightmapCapture *capture = memnew(LightmapCapture);
		return lightmap_capture_data_owner.make_rid(capture);
	}
	PoolVector<uint8_t> lightmap_capture_get_octree(RID p_capture) const {
		const LightmapCapture *capture = lightmap_capture_data_owner.getornull(p_capture);
		ERR_FAIL_COND_V(!capture, PoolVector<uint8_t>());
		return PoolVector<uint8_t>();
	}
	void lightmap_capture_set_octree_cell_transform(RID p_capture, const Transform &p_xform) {}
	Transform lightmap_capture_get_octree_cell_transform(RID p_capture) const { return Transform(); }
	void lightmap_capture_set_octree_cell_subdiv(RID p_capture, int p_subdiv) {}
	int lightmap_capture_get_octree_cell_subdiv(RID p_capture) const { return 0; }
	void lightmap_capture_set_energy(RID p_capture, float p_energy) {}
	float lightmap_capture_get_energy(RID p_capture) const { return 0.0; }
	void lightmap_capture_set_interior(RID p_capture, bool p_interior) {}
	bool lightmap_capture_is_interior(RID p_capture) const { return false; }
	const PoolVector<LightmapCaptureOctree> *lightmap_capture_get_octree_ptr(RID p_capture) const {
		const LightmapCapture *capture = lightmap_capture_data_owner.getornull(p_capture);
		ERR_FAIL_COND_V(!capture, NULL);
		return &capture->octree;
	}

	/* PARTICLES */

	RID particles_create() { return RID(); }

	void particles_set_emitting(RID p_particles, bool p_emitting) {}
	void particles_set_amount(RID p_particles, int p_amount) {}
	void particles_set_lifetime(RID p_particles, float p_lifetime) {}
	void particles_set_one_shot(RID p_particles, bool p_one_shot) {}
	void particles_set_pre_process_time(RID p_particles, float p_time) {}
	void particles_set_explosiveness_ratio(RID p_particles, float p_ratio) {}
	void particles_set_randomness_ratio(RID p_particles, float p_ratio) {}
	void particles_set_custom_aabb(RID p_particles, const AABB &p_aabb) {}
	void particles_set_speed_scale(RID p_particles, float p_scale) {}
	void particles_set_use_local_coordinates(RID p_particles, bool p_enable) {}
	void particles_set_process_material(RID p_particles, RID p_material) {}
	void particles_set_fixed_fps(RID p_particles, int p_fps) {}
	void particles_set_fractional_delta(RID p_particles, bool p_enable) {}
	void particles_restart(RID p_particles) {}

	void particles_set_draw_order(RID p_particles, VS::ParticlesDrawOrder p_order) {}

	void particles_set_draw_passes(RID p_particles, int p_count) {}
	void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) {}

	void particles_request_process(RID p_particles) {}
	AABB particles_get_current_aabb(RID p_particles) { return AABB(); }
	AABB particles_get_aabb(RID p_particles) const { return AABB(); }

	void particles_set_emission_transform(RID p_particles, const Transform &p_transform) {}

	bool particles_get_emitting(RID p_particles) { return false; }
	int particles_get_draw_passes(RID p_particles) const { return 0; }
	RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) const { return RID(); }

	virtual bool particles_is_inactive(RID p_particles) const { return false; }

	/* RENDER TARGET */
	struct RenderTarget : public RID_Data {
		bgfx::FrameBufferHandle hFrameBuffer = BGFX_INVALID_HANDLE;
		bgfx::ViewId id_view = UINT16_MAX;
		void associate_frame_buffer();
		//		GLuint fbo = 0;
		//		GLuint color = 0;
		//		GLuint depth = 0;

		//		GLuint multisample_fbo = 0;
		//		GLuint multisample_color = 0;
		//		GLuint multisample_depth = 0;
		bool multisample_active = false;

		struct Effect {
			//GLuint fbo = 0 ;
			int width = 0;
			int height = 0;
			//GLuint color = 0;
		};

		Effect copy_screen_effect;

		struct MipMaps {
			struct Size {
				//GLuint fbo;
				//GLuint color;
				int width = 0;
				int height = 0;
			};

			Vector<Size> sizes;
			//GLuint color = 0;
			int levels = 0;
		};

		MipMaps mip_maps[2];

		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;

		bool flags[RENDER_TARGET_FLAG_MAX];

		bool used_in_frame = false;
		VS::ViewportMSAA msaa = VS::VIEWPORT_MSAA_DISABLED;

		RID texture;
		bool mip_maps_allocated = false;

		RenderTarget() {
			for (int i = 0; i < RENDER_TARGET_FLAG_MAX; ++i) {
				flags[i] = false;
			}
		}
	};

	mutable RID_Owner<RenderTarget> render_target_owner;
	PooledList<uint32_t> _bgfx_view_pool;
	uint32_t _request_bgfx_view();
	void _free_bgfx_view(uint32_t p_id);

	LocalVector<uint16_t> _bgfx_view_order;

	void _render_target_clear(RenderTarget *rt);
	void _render_target_allocate(RenderTarget *rt);
	void _render_target_set_viewport(RenderTarget *rt, uint16_t p_x, uint16_t p_y, uint16_t p_width, uint16_t p_height);
	void _render_target_set_scissor(RenderTarget *rt, uint16_t p_x, uint16_t p_y, uint16_t p_width, uint16_t p_height);
	void _render_target_set_view_clear(RenderTarget *rt, const Color &p_color = Color(0, 0, 0, 1));

	RID render_target_create();
	void render_target_set_position(RID p_render_target, int p_x, int p_y);
	void render_target_set_size(RID p_render_target, int p_width, int p_height);
	RID render_target_get_texture(RID p_render_target) const;
	uint32_t render_target_get_depth_texture_id(RID p_render_target) const;
	void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id, unsigned int p_depth_id) {}
	void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value);
	bool render_target_was_used(RID p_render_target);
	void render_target_clear_used(RID p_render_target);
	void render_target_set_msaa(RID p_render_target, VS::ViewportMSAA p_msaa) {}
	void render_target_set_use_fxaa(RID p_render_target, bool p_fxaa) {}
	void render_target_set_use_debanding(RID p_render_target, bool p_debanding) {}
	void render_target_set_sharpen_intensity(RID p_render_target, float p_intensity) {}

	/* CANVAS SHADOW */

	RID canvas_light_shadow_buffer_create(int p_width) { return RID(); }

	/* LIGHT SHADOW MAPPING */

	RID canvas_light_occluder_create() { return RID(); }
	void canvas_light_occluder_set_polylines(RID p_occluder, const PoolVector<Vector2> &p_lines) {}

	VS::InstanceType get_base_type(RID p_rid) const {
		if (mesh_owner.owns(p_rid)) {
			return VS::INSTANCE_MESH;
		} else if (lightmap_capture_data_owner.owns(p_rid)) {
			return VS::INSTANCE_LIGHTMAP_CAPTURE;
		}

		return VS::INSTANCE_NONE;
	}

	bool free(RID p_rid);

	struct Frame {
		RenderTarget *current_rt;

		bool clear_request;
		Color clear_request_color;
		float time[4];
		float delta;
		uint64_t count;

	} frame;

	bool has_os_feature(const String &p_feature) const;

	void update_dirty_resources();

	void set_debug_generate_wireframes(bool p_generate) {}

	void render_info_begin_capture() {}
	void render_info_end_capture() {}
	int get_captured_render_info(VS::RenderInfo p_info) { return 0; }

	uint64_t get_render_info(VS::RenderInfo p_info) { return 0; }
	String get_video_adapter_name() const { return String(); }
	String get_video_adapter_vendor() const { return String(); }

	static RasterizerStorage *base_singleton;

	RasterizerStorageBGFX();
	~RasterizerStorageBGFX();
};
