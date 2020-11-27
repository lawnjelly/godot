/*************************************************************************/
/*  rasterizer_storage_gles2.h                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef RASTERIZERSTORAGEGLES2_H
#define RASTERIZERSTORAGEGLES2_H

#include "drivers/gles_common/rasterizer_version.h"
#include "drivers/gles_common/rasterizer_asserts.h"
#include "shader_compiler_gles2.h"
#include "shader_gles2.h"
#include "servers/rendering/rasterizer.h"
#include "servers/rendering/shader_language.h"
#include "core/templates/self_list.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"

#include "shaders/copy.glsl.gen.h"
#include "shaders/cubemap_filter.glsl.gen.h"

class RasterizerCanvasGLES2;
class RasterizerSceneGLES2;

class RasterizerStorageGLES2 : public RasterizerStorage {
public:
	RasterizerCanvasGLES2 *canvas;
	RasterizerSceneGLES2 *scene;

	static GLuint system_fbo;

	struct Config {

		bool shrink_textures_x2;
		bool use_fast_texture_filter;
		bool use_skeleton_software;

		int max_vertex_texture_image_units;
		int max_texture_image_units;
		int max_texture_size;

		// TODO implement wireframe in GLES2
		// bool generate_wireframes;

		Set<String> extensions;

		bool float_texture_supported;
		bool s3tc_supported;
		bool etc1_supported;
		bool pvrtc_supported;
		bool rgtc_supported;
		bool bptc_supported;

		bool keep_original_textures;

		bool force_vertex_shading;

		bool use_rgba_2d_shadows;
		bool use_rgba_3d_shadows;

		bool support_32_bits_indices;
		bool support_write_depth;
		bool support_half_float_vertices;
		bool support_npot_repeat_mipmap;
		bool support_depth_texture;
		bool support_depth_cubemaps;

		bool support_shadow_cubemaps;

		bool multisample_supported;
		bool render_to_mipmap_supported;

		GLuint depth_internalformat;
		GLuint depth_type;
		GLuint depth_buffer_internalformat;

		// in some cases the legacy render didn't orphan. We will mark these
		// so the user can switch orphaning off for them.
		bool should_orphan;
	} config;

	struct Resources {

		GLuint white_tex;
		GLuint black_tex;
		GLuint normal_tex;
		GLuint aniso_tex;

		GLuint mipmap_blur_fbo;
		GLuint mipmap_blur_color;

		GLuint radical_inverse_vdc_cache_tex;
		bool use_rgba_2d_shadows;

		GLuint quadie;

		size_t skeleton_transform_buffer_size;
		GLuint skeleton_transform_buffer;
		LocalVector<float> skeleton_transform_cpu_buffer;

	} resources;

	mutable struct Shaders {

		ShaderCompilerGLES2 compiler;

		CopyShaderGLES2 copy;
		CubemapFilterShaderGLES2 cubemap_filter;

		ShaderCompilerGLES2::IdentifierActions actions_canvas;
		ShaderCompilerGLES2::IdentifierActions actions_scene;
		ShaderCompilerGLES2::IdentifierActions actions_particles;

	} shaders;

	struct Info {

		uint64_t texture_mem;
		uint64_t vertex_mem;

		struct Render {
			uint32_t object_count;
			uint32_t draw_call_count;
			uint32_t material_switch_count;
			uint32_t surface_switch_count;
			uint32_t shader_rebind_count;
			uint32_t vertices_count;
			uint32_t _2d_item_count;
			uint32_t _2d_draw_call_count;

			void reset() {
				object_count = 0;
				draw_call_count = 0;
				material_switch_count = 0;
				surface_switch_count = 0;
				shader_rebind_count = 0;
				vertices_count = 0;
				_2d_item_count = 0;
				_2d_draw_call_count = 0;
			}
		} render, render_final, snap;

		Info() :
				texture_mem(0),
				vertex_mem(0) {
			render.reset();
			render_final.reset();
		}

	} info;

	void bind_quad_array() const;

	/////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////DATA///////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////
	
	/*
	struct Instantiable {
		RID self;
		
		SelfList<RasterizerScene::InstanceBase>::List instance_list;

		_FORCE_INLINE_ void instance_change_notify(bool p_aabb, bool p_materials) {

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
*/
	/////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////API////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////

	/* TEXTURE API */
	
	enum GLESTextureFlags {
		TEXTURE_FLAG_MIPMAPS = 1, /// Enable automatic mipmap generation - when available
		TEXTURE_FLAG_REPEAT = 2, /// Repeat texture (Tiling), otherwise Clamping
		TEXTURE_FLAG_FILTER = 4, /// Create texture with linear (or available) filter
		TEXTURE_FLAG_ANISOTROPIC_FILTER = 8,
		TEXTURE_FLAG_CONVERT_TO_LINEAR = 16,
		TEXTURE_FLAG_MIRRORED_REPEAT = 32, /// Repeat texture, with alternate sections mirrored
		TEXTURE_FLAG_USED_FOR_STREAMING = 2048,
		TEXTURE_FLAGS_DEFAULT = TEXTURE_FLAG_REPEAT | TEXTURE_FLAG_MIPMAPS | TEXTURE_FLAG_FILTER
	};
	
	struct RenderTarget;
	
	struct Texture {
		RID self;

		Texture *proxy;
		Set<Texture *> proxy_owners;

		String path;
		uint32_t flags;
		int width, height, depth;
		int alloc_width, alloc_height;
		Image::Format format;
		GD_RD::TextureType type;

		GLenum target;
		GLenum gl_format_cache;
		GLenum gl_internal_format_cache;
		GLenum gl_type_cache;

		int data_size;
		int total_data_size;
		bool ignore_mipmaps;

		bool compressed;

		bool srgb;

		int mipmaps;

		bool resize_to_po2;

		bool active;
		GLenum tex_id;

		uint16_t stored_cube_sides;

		RenderTarget *render_target;

		Vector<Ref<Image> > images;

		bool redraw_if_visible;

		GD_VS::TextureDetectCallback detect_3d;
		void *detect_3d_ud;

		GD_VS::TextureDetectCallback detect_srgb;
		void *detect_srgb_ud;

		GD_VS::TextureDetectCallback detect_normal;
		void *detect_normal_ud;

		Texture() :
				proxy(NULL),
				flags(0),
				width(0),
				height(0),
				alloc_width(0),
				alloc_height(0),
				format(Image::FORMAT_L8),
				type(GD_RD::TEXTURE_TYPE_2D),
				target(0),
				data_size(0),
				total_data_size(0),
				ignore_mipmaps(false),
				compressed(false),
				mipmaps(0),
				resize_to_po2(false),
				active(false),
				tex_id(0),
				stored_cube_sides(0),
				render_target(NULL),
				redraw_if_visible(false),
				detect_3d(NULL),
				detect_3d_ud(NULL),
				detect_srgb(NULL),
				detect_srgb_ud(NULL),
				detect_normal(NULL),
				detect_normal_ud(NULL) {
		}

		_ALWAYS_INLINE_ Texture *get_ptr() {
			if (proxy) {
				return proxy; //->get_ptr(); only one level of indirection, else not inlining possible.
			} else {
				return this;
			}
		}

		~Texture() {
			if (tex_id != 0) {
				glDeleteTextures(1, &tex_id);
			}

			for (Set<Texture *>::Element *E = proxy_owners.front(); E; E = E->next()) {
				E->get()->proxy = NULL;
			}

			if (proxy) {
				proxy->proxy_owners.erase(this);
			}
		}
	};

	mutable RID_PtrOwner<Texture> texture_owner;

	Ref<Image> _get_gl_image_and_format(const Ref<Image> &p_image, Image::Format p_format, uint32_t p_flags, Image::Format &r_real_format, GLenum &r_gl_format, GLenum &r_gl_internal_format, GLenum &r_gl_type, bool &r_compressed, bool p_force_decompress) const;
	
	// new
	RID texture_2d_create(const Ref<Image> &p_image) override;
	RID texture_2d_layered_create(const Vector<Ref<Image>> &p_layers, RS::TextureLayeredType p_layered_type) override { return RID(); }
	RID texture_3d_create(Image::Format, int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>> &p_data) override { return RID(); }
	RID texture_proxy_create(RID p_base) override { return RID(); }
	
	void texture_2d_update_immediate(RID p_texture, const Ref<Image> &p_image, int p_layer = 0) override {}
	void texture_2d_update(RID p_texture, const Ref<Image> &p_image, int p_layer = 0) override {}
	void texture_3d_update(RID p_texture, const Vector<Ref<Image>> &p_data) override {}
	void texture_proxy_update(RID p_proxy, RID p_base) override {}
	
	RID texture_2d_placeholder_create() override { return RID(); }
	RID texture_2d_layered_placeholder_create(RenderingServer::TextureLayeredType p_layered_type) override { return RID(); }
	RID texture_3d_placeholder_create() override { return RID(); }
	
	Ref<Image> texture_2d_get(RID p_texture) const override { return Ref<Image>(); }
	Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer) const override { return Ref<Image>(); }
	Vector<Ref<Image>> texture_3d_get(RID p_texture) const override { return Vector<Ref<Image>>(); }
	
	void texture_replace(RID p_texture, RID p_by_texture) override {}
	//void texture_set_size_override(RID p_texture, int p_width, int p_height) override {}
	
	void texture_add_to_decal_atlas(RID p_texture, bool p_panorama_to_dp = false) override {}
	void texture_remove_from_decal_atlas(RID p_texture, bool p_panorama_to_dp = false) override {}
	
	
	// old
	virtual RID texture_create();
	virtual void texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, GD_RD::TextureType p_type, uint32_t p_flags = TEXTURE_FLAGS_DEFAULT);
	virtual void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer = 0);
	virtual void texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_layer = 0);
	//virtual Ref<Image> texture_get_data(RID p_texture, int p_layer = 0) const;
	virtual void texture_set_flags(RID p_texture, uint32_t p_flags);
	virtual uint32_t texture_get_flags(RID p_texture) const;
	virtual Image::Format texture_get_format(RID p_texture) const;
	virtual GD_RD::TextureType texture_get_type(RID p_texture) const;
	virtual uint32_t texture_get_texid(RID p_texture) const;
	virtual uint32_t texture_get_width(RID p_texture) const;
	virtual uint32_t texture_get_height(RID p_texture) const;
	virtual uint32_t texture_get_depth(RID p_texture) const;
	void texture_set_size_override(RID p_texture, int p_width, int p_height) override;
	
	virtual void texture_bind(RID p_texture, uint32_t p_texture_no);

	virtual void texture_set_path(RID p_texture, const String &p_path) override;
	virtual String texture_get_path(RID p_texture) const override;

	virtual void texture_set_shrink_all_x2_on_set_data(bool p_enable);

	virtual void texture_debug_usage(List<GD_VS::TextureInfo> *r_info) override;

	virtual RID texture_create_radiance_cubemap(RID p_source, int p_resolution = -1) const;

	virtual void textures_keep_original(bool p_enable);

	virtual void texture_set_proxy(RID p_texture, RID p_proxy);
	virtual Size2 texture_size_with_proxy(RID p_texture) override;
	
	virtual void texture_set_detect_3d_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata) override;
	virtual void texture_set_detect_srgb_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata);
	virtual void texture_set_detect_normal_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata) override;
	void texture_set_detect_roughness_callback(RID p_texture, RS::TextureDetectRoughnessCallback p_callback, void *p_userdata) override {}
	
	
	virtual void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) override;
	
	
	/* CANVAS TEXTURE API */
	
	RID canvas_texture_create() override { return RID(); }
	void canvas_texture_set_channel(RID p_canvas_texture, RS::CanvasTextureChannel p_channel, RID p_texture) override {}
	void canvas_texture_set_shading_parameters(RID p_canvas_texture, const Color &p_base_color, float p_shininess) override {}
	
	void canvas_texture_set_texture_filter(RID p_item, RS::CanvasItemTextureFilter p_filter) override {}
	void canvas_texture_set_texture_repeat(RID p_item, RS::CanvasItemTextureRepeat p_repeat) override {}
	
	/* SKY API */
	// not sure if used in godot 4?

	struct Sky {
		
		RID self;
		RID panorama;
		GLuint radiance;
		int radiance_size;
	};

	mutable RID_PtrOwner<Sky> sky_owner;

	virtual RID sky_create();
	virtual void sky_set_texture(RID p_sky, RID p_panorama, int p_radiance_size);

	/* SHADER API */

	struct Material;

	struct Shader {

		RID self;

		GD_VS::ShaderMode mode;
		ShaderGLES2 *shader;
		String code;
		SelfList<Material>::List materials;

		Map<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;

		uint32_t texture_count;

		uint32_t custom_code_id;
		uint32_t version;

		SelfList<Shader> dirty_list;

		Map<StringName, RID> default_textures;

		Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

		bool valid;

		String path;

		uint32_t index;
		uint64_t last_pass;

		struct CanvasItem {

			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
				BLEND_MODE_PMALPHA,
			};

			int blend_mode;

			enum LightMode {
				LIGHT_MODE_NORMAL,
				LIGHT_MODE_UNSHADED,
				LIGHT_MODE_LIGHT_ONLY
			};

			int light_mode;

			// these flags are specifically for batching
			// some of the logic is thus in rasterizer_storage.cpp
			// we could alternatively set bitflags for each 'uses' and test on the fly
			// defined in RasterizerStorageCommon::BatchFlags
			unsigned int batch_flags;

			bool uses_screen_texture;
			bool uses_screen_uv;
			bool uses_time;
			bool uses_modulate;
			bool uses_color;
			bool uses_vertex;

			// all these should disable item joining if used in a custom shader
			bool uses_world_matrix;
			bool uses_extra_matrix;
			bool uses_projection_matrix;
			bool uses_instance_custom;

		} canvas_item;

		struct Spatial {

			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
			};

			int blend_mode;

			enum DepthDrawMode {
				DEPTH_DRAW_OPAQUE,
				DEPTH_DRAW_ALWAYS,
				DEPTH_DRAW_NEVER,
				DEPTH_DRAW_ALPHA_PREPASS,
			};

			int depth_draw_mode;

			enum CullMode {
				CULL_MODE_FRONT,
				CULL_MODE_BACK,
				CULL_MODE_DISABLED,
			};

			int cull_mode;

			bool uses_alpha;
			bool uses_alpha_scissor;
			bool unshaded;
			bool no_depth_test;
			bool uses_vertex;
			bool uses_discard;
			bool uses_sss;
			bool uses_screen_texture;
			bool uses_depth_texture;
			bool uses_time;
			bool uses_tangent;
			bool uses_ensure_correct_normals;
			bool writes_modelview_or_projection;
			bool uses_vertex_lighting;
			bool uses_world_coordinates;

		} spatial;

		struct Particles {

		} particles;

		bool uses_vertex_time;
		bool uses_fragment_time;

		Shader() :
				dirty_list(this) {

			shader = NULL;
			valid = false;
			custom_code_id = 0;
			version = 1;
			last_pass = 0;
		}
	};

	mutable RID_PtrOwner<Shader> shader_owner;
	mutable SelfList<Shader>::List _shader_dirty_list;

	void _shader_make_dirty(Shader *p_shader);
	
	virtual RID shader_create() override;

	virtual void shader_set_code(RID p_shader, const String &p_code) override;
	virtual String shader_get_code(RID p_shader) const override;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const override;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) override;
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const override;

	virtual void shader_add_custom_define(RID p_shader, const String &p_define);
	virtual void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const;
	virtual void shader_remove_custom_define(RID p_shader, const String &p_define);

	void _update_shader(Shader *p_shader) const;
	void update_dirty_shaders();
	
	// new
	Variant shader_get_param_default(RID p_material, const StringName &p_param) const override { return Variant(); }
	
	
	/* COMMON MATERIAL API */

	struct Material {
		
		RID self;
		Shader *shader;
		Map<StringName, Variant> params;
		SelfList<Material> list;
		SelfList<Material> dirty_list;
		Vector<Pair<StringName, RID> > textures;
		float line_width;
		int render_priority;

		RID next_pass;

		uint32_t index;
		uint64_t last_pass;

//		Map<Geometry *, int> geometry_owners;
//		Map<RasterizerScene::InstanceBase *, int> instance_owners;

		bool can_cast_shadow_cache;
		bool is_animated_cache;

		Material() :
				list(this),
				dirty_list(this) {
			can_cast_shadow_cache = false;
			is_animated_cache = false;
			shader = NULL;
			line_width = 1.0;
			last_pass = 0;
			render_priority = 0;
		}
	};

	mutable SelfList<Material>::List _material_dirty_list;
	void _material_make_dirty(Material *p_material) const;

//	void _material_add_geometry(RID p_material, Geometry *p_geometry);
//	void _material_remove_geometry(RID p_material, Geometry *p_geometry);

	void _update_material(Material *p_material);

	mutable RID_PtrOwner<Material> material_owner;
	
	// new
	void material_get_instance_shader_parameters(RID p_material, List<InstanceShaderParam> *r_parameters) override {}
	void material_update_dependency(RID p_material, RasterizerScene::InstanceBase *p_instance) override {}
	
	// old	
	virtual RID material_create() override;

	virtual void material_set_shader(RID p_material, RID p_shader) override;
	virtual RID material_get_shader(RID p_material) const;

	virtual void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) override;
	virtual Variant material_get_param(RID p_material, const StringName &p_param) const override;
	virtual Variant material_get_param_default(RID p_material, const StringName &p_param) const;

	virtual void material_set_line_width(RID p_material, float p_width);
	virtual void material_set_next_pass(RID p_material, RID p_next_material) override;

	virtual bool material_is_animated(RID p_material) override;
	virtual bool material_casts_shadows(RID p_material) override;
	virtual bool material_uses_tangents(RID p_material);
	virtual bool material_uses_ensure_correct_normals(RID p_material);

	virtual void material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);
	virtual void material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);

	virtual void material_set_render_priority(RID p_material, int priority) override;

	void update_dirty_materials();

	/* MESH API */

	struct Mesh;
	
	struct DummySurface {
		uint32_t format;
		RS::PrimitiveType primitive;
		Vector<uint8_t> array;
		int vertex_count;
		Vector<uint8_t> index_array;
		int index_count;
		AABB aabb;
		Vector<Vector<uint8_t>> blend_shapes;
		Vector<AABB> bone_aabbs;
	};
	

	struct MultiMesh;
	
	struct DummyMesh {
		Vector<DummySurface> surfaces;
		int blend_shape_count;
		RS::BlendShapeMode blend_shape_mode;
	};
	
	mutable RID_PtrOwner<DummyMesh> mesh_owner;
	
	
	RID mesh_create() override {
		DummyMesh *mesh = memnew(DummyMesh);
		ERR_FAIL_COND_V(!mesh, RID());
		mesh->blend_shape_count = 0;
		mesh->blend_shape_mode = RS::BLEND_SHAPE_MODE_NORMALIZED;
		return mesh_owner.make_rid(mesh);
	}
	
	void mesh_add_surface(RID p_mesh, const RS::SurfaceData &p_surface) override {}
	
	int mesh_get_blend_shape_count(RID p_mesh) const override {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->blend_shape_count;
	}
	
	void mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode) override {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_mode = p_mode;
	}
	RS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const override {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, RS::BLEND_SHAPE_MODE_NORMALIZED);
		return m->blend_shape_mode;
	}
	
	void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override {}
	
	void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) override {}
	RID mesh_surface_get_material(RID p_mesh, int p_surface) const override { return RID(); }
	
	RS::SurfaceData mesh_get_surface(RID p_mesh, int p_surface) const override { return RS::SurfaceData(); }
	int mesh_get_surface_count(RID p_mesh) const override {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->surfaces.size();
	}
	
	void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) override {}
	AABB mesh_get_custom_aabb(RID p_mesh) const override { return AABB(); }
	
	AABB mesh_get_aabb(RID p_mesh, RID p_skeleton = RID()) override { return AABB(); }
	void mesh_clear(RID p_mesh) override {}
	
	
	/* MULTIMESH API */
	
	RID multimesh_create() override { return RID(); }
	
	void multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, bool p_use_colors = false, bool p_use_custom_data = false) override {}
	int multimesh_get_instance_count(RID p_multimesh) const override { return 0; }
	
	void multimesh_set_mesh(RID p_multimesh, RID p_mesh) override {}
	void multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) override {}
	void multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) override {}
	void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) override {}
	void multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) override {}
	
	RID multimesh_get_mesh(RID p_multimesh) const override { return RID(); }
	AABB multimesh_get_aabb(RID p_multimesh) const override { return AABB(); }
	
	Transform multimesh_instance_get_transform(RID p_multimesh, int p_index) const override { return Transform(); }
	Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const override { return Transform2D(); }
	Color multimesh_instance_get_color(RID p_multimesh, int p_index) const override { return Color(); }
	Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const override { return Color(); }
	void multimesh_set_buffer(RID p_multimesh, const Vector<float> &p_buffer) override {}
	Vector<float> multimesh_get_buffer(RID p_multimesh) const override { return Vector<float>(); }
	
	void multimesh_set_visible_instances(RID p_multimesh, int p_visible) override {}
	int multimesh_get_visible_instances(RID p_multimesh) const override { return 0; }


	/* IMMEDIATE API */
	
	RID immediate_create() override { return RID(); }
	void immediate_begin(RID p_immediate, RS::PrimitiveType p_rimitive, RID p_texture = RID()) override {}
	void immediate_vertex(RID p_immediate, const Vector3 &p_vertex) override {}
	void immediate_normal(RID p_immediate, const Vector3 &p_normal) override {}
	void immediate_tangent(RID p_immediate, const Plane &p_tangent) override {}
	void immediate_color(RID p_immediate, const Color &p_color) override {}
	void immediate_uv(RID p_immediate, const Vector2 &tex_uv) override {}
	void immediate_uv2(RID p_immediate, const Vector2 &tex_uv) override {}
	void immediate_end(RID p_immediate) override {}
	void immediate_clear(RID p_immediate) override {}
	void immediate_set_material(RID p_immediate, RID p_material) override {}
	RID immediate_get_material(RID p_immediate) const override { return RID(); }
	AABB immediate_get_aabb(RID p_immediate) const override { return AABB(); }

	/* SKELETON API */
	
	RID skeleton_create() override { return RID(); }
	void skeleton_allocate(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) override {}
	void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D &p_base_transform) override {}
	int skeleton_get_bone_count(RID p_skeleton) const override { return 0; }
	void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform &p_transform) override {}
	Transform skeleton_bone_get_transform(RID p_skeleton, int p_bone) const override { return Transform(); }
	void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D &p_transform) override {}
	Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone) const override { return Transform2D(); }
	
	/* Light API */
	
	RID light_create(RS::LightType p_type) override { return RID(); }
	
	void light_set_color(RID p_light, const Color &p_color) override {}
	void light_set_param(RID p_light, RS::LightParam p_param, float p_value) override {}
	void light_set_shadow(RID p_light, bool p_enabled) override {}
	void light_set_shadow_color(RID p_light, const Color &p_color) override {}
	void light_set_projector(RID p_light, RID p_texture) override {}
	void light_set_negative(RID p_light, bool p_enable) override {}
	void light_set_cull_mask(RID p_light, uint32_t p_mask) override {}
	void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled) override {}
	void light_set_bake_mode(RID p_light, RS::LightBakeMode p_bake_mode) override {}
	void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade) override {}
	
	void light_omni_set_shadow_mode(RID p_light, RS::LightOmniShadowMode p_mode) override {}
	
	void light_directional_set_shadow_mode(RID p_light, RS::LightDirectionalShadowMode p_mode) override {}
	void light_directional_set_blend_splits(RID p_light, bool p_enable) override {}
	bool light_directional_get_blend_splits(RID p_light) const override { return false; }
	void light_directional_set_shadow_depth_range_mode(RID p_light, RS::LightDirectionalShadowDepthRangeMode p_range_mode) override {}
	RS::LightDirectionalShadowDepthRangeMode light_directional_get_shadow_depth_range_mode(RID p_light) const override { return RS::LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_STABLE; }
	
	RS::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light) override { return RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL; }
	RS::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light) override { return RS::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID; }
	
	bool light_has_shadow(RID p_light) const override { return false; }
	
	RS::LightType light_get_type(RID p_light) const override { return RS::LIGHT_OMNI; }
	AABB light_get_aabb(RID p_light) const override { return AABB(); }
	float light_get_param(RID p_light, RS::LightParam p_param) override { return 0.0; }
	Color light_get_color(RID p_light) override { return Color(); }
	RS::LightBakeMode light_get_bake_mode(RID p_light) override { return RS::LIGHT_BAKE_DISABLED; }
	uint32_t light_get_max_sdfgi_cascade(RID p_light) override { return 0; }
	uint64_t light_get_version(RID p_light) const override { return 0; }

	/* PROBE API */
	
	RID reflection_probe_create() override { return RID(); }
	
	void reflection_probe_set_update_mode(RID p_probe, RS::ReflectionProbeUpdateMode p_mode) override {}
	void reflection_probe_set_intensity(RID p_probe, float p_intensity) override {}
	void reflection_probe_set_ambient_mode(RID p_probe, RS::ReflectionProbeAmbientMode p_mode) override {}
	void reflection_probe_set_ambient_color(RID p_probe, const Color &p_color) override {}
	void reflection_probe_set_ambient_energy(RID p_probe, float p_energy) override {}
	void reflection_probe_set_max_distance(RID p_probe, float p_distance) override {}
	void reflection_probe_set_extents(RID p_probe, const Vector3 &p_extents) override {}
	void reflection_probe_set_origin_offset(RID p_probe, const Vector3 &p_offset) override {}
	void reflection_probe_set_as_interior(RID p_probe, bool p_enable) override {}
	void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable) override {}
	void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable) override {}
	void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers) override {}
	void reflection_probe_set_resolution(RID p_probe, int p_resolution) override {}
	
	AABB reflection_probe_get_aabb(RID p_probe) const override { return AABB(); }
	RS::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe) const override { return RenderingServer::REFLECTION_PROBE_UPDATE_ONCE; }
	uint32_t reflection_probe_get_cull_mask(RID p_probe) const override { return 0; }
	Vector3 reflection_probe_get_extents(RID p_probe) const override { return Vector3(); }
	Vector3 reflection_probe_get_origin_offset(RID p_probe) const override { return Vector3(); }
	float reflection_probe_get_origin_max_distance(RID p_probe) const override { return 0.0; }
	bool reflection_probe_renders_shadows(RID p_probe) const override { return false; }
	
	void base_update_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) override {}
	void skeleton_update_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) override {}
	
	/* DECAL API */
	
	RID decal_create() override { return RID(); }
	void decal_set_extents(RID p_decal, const Vector3 &p_extents) override {}
	void decal_set_texture(RID p_decal, RS::DecalTexture p_type, RID p_texture) override {}
	void decal_set_emission_energy(RID p_decal, float p_energy) override {}
	void decal_set_albedo_mix(RID p_decal, float p_mix) override {}
	void decal_set_modulate(RID p_decal, const Color &p_modulate) override {}
	void decal_set_cull_mask(RID p_decal, uint32_t p_layers) override {}
	void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length) override {}
	void decal_set_fade(RID p_decal, float p_above, float p_below) override {}
	void decal_set_normal_fade(RID p_decal, float p_fade) override {}
	
	AABB decal_get_aabb(RID p_decal) const override { return AABB(); }
	
	
	/* GI PROBE API */
	RID gi_probe_create() override { return RID(); }
	
	void gi_probe_allocate(RID p_gi_probe, const Transform &p_to_cell_xform, const AABB &p_aabb, const Vector3i &p_octree_size, const Vector<uint8_t> &p_octree_cells, const Vector<uint8_t> &p_data_cells, const Vector<uint8_t> &p_distance_field, const Vector<int> &p_level_counts) override {}
	
	AABB gi_probe_get_bounds(RID p_gi_probe) const override { return AABB(); }
	Vector3i gi_probe_get_octree_size(RID p_gi_probe) const override { return Vector3i(); }
	Vector<uint8_t> gi_probe_get_octree_cells(RID p_gi_probe) const override { return Vector<uint8_t>(); }
	Vector<uint8_t> gi_probe_get_data_cells(RID p_gi_probe) const override { return Vector<uint8_t>(); }
	Vector<uint8_t> gi_probe_get_distance_field(RID p_gi_probe) const override { return Vector<uint8_t>(); }
	
	Vector<int> gi_probe_get_level_counts(RID p_gi_probe) const override { return Vector<int>(); }
	Transform gi_probe_get_to_cell_xform(RID p_gi_probe) const override { return Transform(); }
	
	void gi_probe_set_dynamic_range(RID p_gi_probe, float p_range) override {}
	float gi_probe_get_dynamic_range(RID p_gi_probe) const override { return 0; }
	
	void gi_probe_set_propagation(RID p_gi_probe, float p_range) override {}
	float gi_probe_get_propagation(RID p_gi_probe) const override { return 0; }
	
	void gi_probe_set_energy(RID p_gi_probe, float p_range) override {}
	float gi_probe_get_energy(RID p_gi_probe) const override { return 0.0; }
	
	void gi_probe_set_ao(RID p_gi_probe, float p_ao) override {}
	float gi_probe_get_ao(RID p_gi_probe) const override { return 0; }
	
	void gi_probe_set_ao_size(RID p_gi_probe, float p_strength) override {}
	float gi_probe_get_ao_size(RID p_gi_probe) const override { return 0; }
	
	void gi_probe_set_bias(RID p_gi_probe, float p_range) override {}
	float gi_probe_get_bias(RID p_gi_probe) const override { return 0.0; }
	
	void gi_probe_set_normal_bias(RID p_gi_probe, float p_range) override {}
	float gi_probe_get_normal_bias(RID p_gi_probe) const override { return 0.0; }
	
	void gi_probe_set_interior(RID p_gi_probe, bool p_enable) override {}
	bool gi_probe_is_interior(RID p_gi_probe) const override { return false; }
	
	void gi_probe_set_use_two_bounces(RID p_gi_probe, bool p_enable) override {}
	bool gi_probe_is_using_two_bounces(RID p_gi_probe) const override { return false; }
	
	void gi_probe_set_anisotropy_strength(RID p_gi_probe, float p_strength) override {}
	float gi_probe_get_anisotropy_strength(RID p_gi_probe) const override { return 0; }
	
	uint32_t gi_probe_get_version(RID p_gi_probe) override { return 0; }

	/* LIGHTMAP */
	
	RID lightmap_create() override { return RID(); }
	
	void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_haromics) override {}
	void lightmap_set_probe_bounds(RID p_lightmap, const AABB &p_bounds) override {}
	void lightmap_set_probe_interior(RID p_lightmap, bool p_interior) override {}
	void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array &p_points, const PackedColorArray &p_point_sh, const PackedInt32Array &p_tetrahedra, const PackedInt32Array &p_bsp_tree) override {}
	PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap) const override { return PackedVector3Array(); }
	PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap) const override { return PackedColorArray(); }
	PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap) const override { return PackedInt32Array(); }
	PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap) const override { return PackedInt32Array(); }
	AABB lightmap_get_aabb(RID p_lightmap) const override { return AABB(); }
	void lightmap_tap_sh_light(RID p_lightmap, const Vector3 &p_point, Color *r_sh) override {}
	bool lightmap_is_interior(RID p_lightmap) const override { return false; }
	void lightmap_set_probe_capture_update_speed(float p_speed) override {}
	float lightmap_get_probe_capture_update_speed() const override { return 0; }
	

	/* PARTICLES */
	RID particles_create() override { return RID(); }
	
	void particles_emit(RID p_particles, const Transform &p_transform, const Vector3 &p_velocity, const Color &p_color, const Color &p_custom, uint32_t p_emit_flags) override {}
	void particles_set_emitting(RID p_particles, bool p_emitting) override {}
	void particles_set_amount(RID p_particles, int p_amount) override {}
	void particles_set_lifetime(RID p_particles, float p_lifetime) override {}
	void particles_set_one_shot(RID p_particles, bool p_one_shot) override {}
	void particles_set_pre_process_time(RID p_particles, float p_time) override {}
	void particles_set_explosiveness_ratio(RID p_particles, float p_ratio) override {}
	void particles_set_randomness_ratio(RID p_particles, float p_ratio) override {}
	void particles_set_custom_aabb(RID p_particles, const AABB &p_aabb) override {}
	void particles_set_speed_scale(RID p_particles, float p_scale) override {}
	void particles_set_use_local_coordinates(RID p_particles, bool p_enable) override {}
	void particles_set_process_material(RID p_particles, RID p_material) override {}
	void particles_set_fixed_fps(RID p_particles, int p_fps) override {}
	void particles_set_fractional_delta(RID p_particles, bool p_enable) override {}
	void particles_set_subemitter(RID p_particles, RID p_subemitter_particles) override {}
	void particles_set_view_axis(RID p_particles, const Vector3 &p_axis) override {}
	void particles_set_collision_base_size(RID p_particles, float p_size) override {}
	void particles_restart(RID p_particles) override {}
	
	void particles_set_draw_order(RID p_particles, RS::ParticlesDrawOrder p_order) override {}
	
	void particles_set_draw_passes(RID p_particles, int p_count) override {}
	void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) override {}
	
	void particles_request_process(RID p_particles) override {}
	AABB particles_get_current_aabb(RID p_particles) override { return AABB(); }
	AABB particles_get_aabb(RID p_particles) const override { return AABB(); }
	
	void particles_set_emission_transform(RID p_particles, const Transform &p_transform) override {}
	
	bool particles_get_emitting(RID p_particles) override { return false; }
	int particles_get_draw_passes(RID p_particles) const override { return 0; }
	RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) const override { return RID(); }
	
	void particles_add_collision(RID p_particles, RasterizerScene::InstanceBase *p_instance) override {}
	void particles_remove_collision(RID p_particles, RasterizerScene::InstanceBase *p_instance) override {}
	
	void update_particles() override {}
	
	/* PARTICLES COLLISION */
	
	RID particles_collision_create() override { return RID(); }
	void particles_collision_set_collision_type(RID p_particles_collision, RS::ParticlesCollisionType p_type) override {}
	void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask) override {}
	void particles_collision_set_sphere_radius(RID p_particles_collision, float p_radius) override {}
	void particles_collision_set_box_extents(RID p_particles_collision, const Vector3 &p_extents) override {}
	void particles_collision_set_attractor_strength(RID p_particles_collision, float p_strength) override {}
	void particles_collision_set_attractor_directionality(RID p_particles_collision, float p_directionality) override {}
	void particles_collision_set_attractor_attenuation(RID p_particles_collision, float p_curve) override {}
	void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture) override {}
	void particles_collision_height_field_update(RID p_particles_collision) override {}
	void particles_collision_set_height_field_resolution(RID p_particles_collision, RS::ParticlesCollisionHeightfieldResolution p_resolution) override {}
	AABB particles_collision_get_aabb(RID p_particles_collision) const override { return AABB(); }
	bool particles_collision_is_heightfield(RID p_particles_collision) const override { return false; }
	RID particles_collision_get_heightfield_framebuffer(RID p_particles_collision) const override { return RID(); }
	
	/* GLOBAL VARIABLES */
	
	void global_variable_add(const StringName &p_name, RS::GlobalVariableType p_type, const Variant &p_value) override {}
	void global_variable_remove(const StringName &p_name) override {}
	Vector<StringName> global_variable_get_list() const override { return Vector<StringName>(); }
	
	void global_variable_set(const StringName &p_name, const Variant &p_value) override {}
	void global_variable_set_override(const StringName &p_name, const Variant &p_value) override {}
	Variant global_variable_get(const StringName &p_name) const override { return Variant(); }
	RS::GlobalVariableType global_variable_get_type(const StringName &p_name) const override { return RS::GLOBAL_VAR_TYPE_MAX; }
	
	void global_variables_load_settings(bool p_load_textures = true) override {}
	void global_variables_clear() override {}
	
	int32_t global_variables_instance_allocate(RID p_instance) override { return 0; }
	void global_variables_instance_free(RID p_instance) override {}
	void global_variables_instance_update(RID p_instance, int p_index, const Variant &p_value) override {}
	
	bool particles_is_inactive(RID p_particles) const override { return false; }
	
	/* INSTANCE */

//	virtual void instance_add_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance);
//	virtual void instance_remove_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance);

//	virtual void instance_add_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance);
//	virtual void instance_remove_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance);

	/* RENDER TARGET */

	struct RenderTarget {
		RID self;
		GLuint fbo;
		GLuint color;
		GLuint depth;

		GLuint multisample_fbo;
		GLuint multisample_color;
		GLuint multisample_depth;
		bool multisample_active;

		struct Effect {
			GLuint fbo;
			int width;
			int height;

			GLuint color;

			Effect() :
					fbo(0),
					width(0),
					height(0),
					color(0) {
			}
		};

		Effect copy_screen_effect;

		struct MipMaps {

			struct Size {
				GLuint fbo;
				GLuint color;
				int width;
				int height;
			};

			Vector<Size> sizes;
			GLuint color;
			int levels;

			MipMaps() :
					color(0),
					levels(0) {
			}
		};

		MipMaps mip_maps[2];

		struct External {
			GLuint fbo;
			GLuint color;
			GLuint depth;
			RID texture;

			External() :
					fbo(0),
					color(0),
					depth(0) {
			}
		} external;

		int x, y, width, height;

		bool flags[RENDER_TARGET_FLAG_MAX];

		bool used_in_frame;
		GD_VS::ViewportMSAA msaa;

		bool use_fxaa;
		bool use_debanding;

		RID texture;

		bool used_dof_blur_near;
		bool mip_maps_allocated;

		RenderTarget() :
				fbo(0),
				color(0),
				depth(0),
				multisample_fbo(0),
				multisample_color(0),
				multisample_depth(0),
				multisample_active(false),
				x(0),
				y(0),
				width(0),
				height(0),
				used_in_frame(false),
				msaa(GD_VS::VIEWPORT_MSAA_DISABLED),
				use_fxaa(false),
				use_debanding(false),
				used_dof_blur_near(false),
				mip_maps_allocated(false) {
			for (int i = 0; i < RENDER_TARGET_FLAG_MAX; ++i) {
				flags[i] = false;
			}
			external.fbo = 0;
		}
	};

	mutable RID_PtrOwner<RenderTarget> render_target_owner;

	void _render_target_clear(RenderTarget *rt);
	void _render_target_allocate(RenderTarget *rt);

	virtual RID render_target_create() override;
	virtual void render_target_set_position(RID p_render_target, int p_x, int p_y) override;
	virtual void render_target_set_size(RID p_render_target, int p_width, int p_height) override;
	virtual RID render_target_get_texture(RID p_render_target) override;
	virtual void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id) override;

	virtual void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) override;
	virtual bool render_target_was_used(RID p_render_target) override;
	virtual void render_target_clear_used(RID p_render_target);
	virtual void render_target_set_msaa(RID p_render_target, GD_VS::ViewportMSAA p_msaa);
	virtual void render_target_set_use_fxaa(RID p_render_target, bool p_fxaa);
	virtual void render_target_set_use_debanding(RID p_render_target, bool p_debanding);
	
	// new
	void render_target_set_as_unused(RID p_render_target) override {}
	
	void render_target_request_clear(RID p_render_target, const Color &p_clear_color) override {}
	bool render_target_is_clear_requested(RID p_render_target) override { return false; }
	Color render_target_get_clear_request_color(RID p_render_target) override { return Color(); }
	void render_target_disable_clear_request(RID p_render_target) override {}
	void render_target_do_clear_request(RID p_render_target) override {}
	
	/* CANVAS SHADOW */

	struct CanvasLightShadow {
		RID self;
		int size;
		int height;
		GLuint fbo;
		GLuint depth;
		GLuint distance; //for older devices
	};

	RID_PtrOwner<CanvasLightShadow> canvas_light_shadow_owner;

	virtual RID canvas_light_shadow_buffer_create(int p_width);

	/* LIGHT SHADOW MAPPING */
/*
	struct CanvasOccluder {
		RID self;

		GLuint vertex_id; // 0 means, unconfigured
		GLuint index_id; // 0 means, unconfigured
		LocalVector<Vector2> lines;
		int len;
	};

	RID_Owner<CanvasOccluder> canvas_occluder_owner;

	virtual RID canvas_light_occluder_create();
	virtual void canvas_light_occluder_set_polylines(RID p_occluder, const LocalVector<Vector2> &p_lines);
*/

	virtual GD_VS::InstanceType get_base_type(RID p_rid) const override;

	virtual bool free(RID p_rid) override;

	struct Frame {

		RenderTarget *current_rt;

		bool clear_request;
		Color clear_request_color;
		float time[4];
		float delta;
		uint64_t count;

	} frame;

	void initialize();
	void finalize();

	void _copy_screen();

	virtual bool has_os_feature(const String &p_feature) const override;

	virtual void update_dirty_resources() override;

	virtual void set_debug_generate_wireframes(bool p_generate) override;

	virtual void render_info_begin_capture() override;
	virtual void render_info_end_capture() override;
	virtual int get_captured_render_info(GD_VS::RenderInfo p_info) override;

	virtual int get_render_info(GD_VS::RenderInfo p_info) override;
	virtual String get_video_adapter_name() const override;
	virtual String get_video_adapter_vendor() const override;
	
	void capture_timestamps_begin() override {}
	void capture_timestamp(const String &p_name) override {}
	uint32_t get_captured_timestamps_count() const override { return 0; }
	uint64_t get_captured_timestamps_frame() const override { return 0; }
	uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) const override { return 0; }
	uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) const override { return 0; }
	String get_captured_timestamp_name(uint32_t p_index) const override { return String(); }
	
	
	void buffer_orphan_and_upload(unsigned int p_buffer_size, unsigned int p_offset, unsigned int p_data_size, const void *p_data, GLenum p_target = GL_ARRAY_BUFFER, GLenum p_usage = GL_DYNAMIC_DRAW, bool p_optional_orphan = false) const;
	bool safe_buffer_sub_data(unsigned int p_total_buffer_size, GLenum p_target, unsigned int p_offset, unsigned int p_data_size, const void *p_data, unsigned int &r_offset_after) const;

	RasterizerStorageGLES2();
};

inline bool RasterizerStorageGLES2::safe_buffer_sub_data(unsigned int p_total_buffer_size, GLenum p_target, unsigned int p_offset, unsigned int p_data_size, const void *p_data, unsigned int &r_offset_after) const {
	r_offset_after = p_offset + p_data_size;
#ifdef DEBUG_ENABLED
	// we are trying to write across the edge of the buffer
	if (r_offset_after > p_total_buffer_size)
		return false;
#endif
	glBufferSubData(p_target, p_offset, p_data_size, p_data);
	return true;
}

// standardize the orphan / upload in one place so it can be changed per platform as necessary, and avoid future
// bugs causing pipeline stalls
inline void RasterizerStorageGLES2::buffer_orphan_and_upload(unsigned int p_buffer_size, unsigned int p_offset, unsigned int p_data_size, const void *p_data, GLenum p_target, GLenum p_usage, bool p_optional_orphan) const {
	// Orphan the buffer to avoid CPU/GPU sync points caused by glBufferSubData
	// Was previously #ifndef GLES_OVER_GL however this causes stalls on desktop mac also (and possibly other)
	if (!p_optional_orphan || (config.should_orphan)) {
		glBufferData(p_target, p_buffer_size, NULL, p_usage);
#ifdef RASTERIZER_EXTRA_CHECKS
		// fill with garbage off the end of the array
		if (p_buffer_size) {
			unsigned int start = p_offset + p_data_size;
			unsigned int end = start + 1024;
			if (end < p_buffer_size) {
				uint8_t *garbage = (uint8_t *)alloca(1024);
				for (int n = 0; n < 1024; n++) {
					garbage[n] = Math::random(0, 255);
				}
				glBufferSubData(p_target, start, 1024, garbage);
			}
		}
#endif
	}
	RAST_DEV_DEBUG_ASSERT((p_offset + p_data_size) <= p_buffer_size);
	glBufferSubData(p_target, p_offset, p_data_size, p_data);
}

#endif // RASTERIZERSTORAGEGLES2_H
