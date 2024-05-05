#pragma once

#include "latomic.h"
#include "llightimage.h"
#include "llightscene.h"
#include "lqmc.h"
#include "lsky.h"
#include "scene/3d/light.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/spatial.h"

#define LLIGHTMAP_MULTITHREADED
//#define LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
//#define LLIGHTMAP_DEBUG_SPECIFIC_TEXEL

#ifdef LLIGHTMAP_MULTITHREADED
#define RAYBANK_USE_THREADING
#define BACKWARD_TRACE_MULTITHEADED
#endif

namespace LM {

class LightMapper_Base {
	friend class LightScene;
	friend class LightProbes;

	void debug_save(LightImage<uint32_t> &p_im, String p_filename);

protected:
	class LLight {
	public:
		enum LType {
			LT_OMNI,
			LT_SPOT,
			LT_DIRECTIONAL,
		};

		LType type;
		Vector3 pos;
		Vector3 dir;
		Vector3 scale;
		Color color;
		float energy;
		float indirect_energy;
		float range;
		float spot_angle_radians; // radians
		// precalculated dot product threshold for inside / outside light cone
		float spot_dot_max;
		// behind the origin, takes account of the scale in order to cull spotlights
		Vector3 spot_emanation_point;
		const Light *m_pLight;

		// for directional lights
		// all that is needed for a random distribution
		Vector3 dl_plane_pt;
		Vector3 dl_tangent;
		Vector3 dl_bitangent;
		float dl_tangent_range;
		float dl_bitangent_range;
	};

public:
	enum eLMMode {
		LMMODE_FORWARD,
		LMMODE_BACKWARD,
	};

	enum eLMBakeMode {
		LMBAKEMODE_LIGHTMAP,
		LMBAKEMODE_AO,
		LMBAKEMODE_ORIG_MATERIAL,
		LMBAKEMODE_BOUNCE,
		LMBAKEMODE_PROBES,
		LMBAKEMODE_UVMAP,
		LMBAKEMODE_COMBINED,
		LMBAKEMODE_MERGE,
	};

	enum eLMBakeQuality {
		LM_QUALITY_LOW,
		LM_QUALITY_MEDIUM,
		LM_QUALITY_HIGH,
		LM_QUALITY_FINAL,
	};

	enum eNRMethod {
		NR_DISABLE,
		NR_SIMPLE,
		NR_ADVANCED,
	};

	// these enable feedback in the Godot UI as we bake
	//	typedef void (*BakeBeginFunc)(int);
	//	typedef bool (*BakeStepFunc)(int, const String &);
	typedef bool (*BakeStepFunc)(float, const String &);
	typedef bool (*BakeSubStepFunc)(float, const String &, bool);
	typedef void (*BakeEndFunc)();

	//	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeSubStepFunc bake_substep_function;
	static BakeEndFunc bake_end_function;

	void show_warning(String sz, bool bAlert = true);
	void calculate_quality_adjusted_settings();

protected:
	void base_reset();

	void find_lights_recursive(const Node *pNode);
	void find_light(const Node *pNode);
	void prepare_lights();

	bool prepare_image_maps();
	void _normalize(LightImage<FColor> &r_image, float p_normalize_multiplier = 1.0f);
	void _mark_dilated_area(LightImage<FColor> &r_image);
	void normalize_AO();
	void apply_noise_reduction();
	void stitch_seams();

	void merge_for_ambient_bounces();
	void merge_to_combined();

	void light_to_plane(LLight &light);
	Plane find_containment_plane(const Vector3 &dir, Vector3 pts[8], float &range, float padding);

	float safe_acosf(float f) const {
		f = CLAMP(f, -1.0f, 1.0f);
		return acosf(f);
	}

protected:
	// Composite image.
	LightImage<FColor> _image_main;
	LightImage<FColor> _image_main_mirror;

	// Component image maps.
	// These will be combined to form the final main image.
	LightImage<FColor> _image_lightmap;
	LightImage<FColor> _image_emission;
	LightImage<FColor> _image_glow;
	LightImage<FColor> _image_bounce;

	LightImage<float> _image_AO;
	LightImage<Color> _image_orig_material;

	// just in case of overlapping triangles, for anti aliasing we will maintain 2 lists of triangles per pixel
	LightImage<uint32_t> _image_tri_ids_p1;

#ifdef LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
	LightImage<uint8_t> _image_reclaimed_texels;
#endif

	// store multiple triangles per texel
	LightImage<MiniList> _image_tri_minilists;
	LVector<uint32_t> _minilist_tri_ids;

	LightImage<Vector3> _image_barycentric;

	LocalVector<Vector2i> _aa_kernel;
	LocalVector<Vector2> _aa_kernel_f;

	int _width;
	int _height;
	int _num_rays; // this will be modified from the settings_numrays
	int _num_rays_forward = 0;

	LightScene _scene;
	LVector<LLight> _lights;

	QMC _QMC;
	LAtomic _atomic;
	LSky _sky;

	// for stats
	int _num_tests;

	// if user cancels bake in editor
	bool _pressed_cancel;

	// these need to be public because can't get friend to work
	// with LLightMapper_Base. It is not recognised in global namespace.
	// Perhaps some Godot template fu is involved.
public:
	enum Param {
		PARAM_TEX_WIDTH,
		PARAM_TEX_HEIGHT,
		PARAM_MAX_LIGHT_DISTANCE,
		PARAM_AA_KERNEL_SIZE,
		PARAM_AA_NUM_LIGHT_SAMPLES,
		PARAM_HIGH_SHADOW_QUALITY,
		PARAM_SURFACE_BIAS,
		PARAM_MATERIAL_SIZE,
		PARAM_MATERIAL_KERNEL_SIZE,
		PARAM_VOXEL_DENSITY,
		PARAM_NUM_PRIMARY_RAYS,
		PARAM_NUM_BOUNCES,
		PARAM_BOUNCE_POWER,
		PARAM_ROUGHNESS,
		PARAM_NUM_AMBIENT_BOUNCES,
		PARAM_NUM_AMBIENT_BOUNCE_RAYS,
		PARAM_AMBIENT_BOUNCE_POWER,
		PARAM_AMBIENT_BOUNCE_MIX,
		//PARAM_EMISSION_ENABLED,
		PARAM_EMISSION_DENSITY,
		PARAM_EMISSION_POWER,
		PARAM_GLOW,
		PARAM_AO_NUM_SAMPLES,
		PARAM_AO_RANGE,
		PARAM_AO_ERROR_METRIC,
		PARAM_AO_ABORT_TIMEOUT,
		PARAM_SKY_SIZE,
		PARAM_SKY_SAMPLES,
		PARAM_SKY_BLUR,
		PARAM_SKY_BRIGHTNESS,
		PARAM_NORMALIZE,
		PARAM_NORMALIZE_MULTIPLIER,
		PARAM_AO_LIGHT_RATIO,
		PARAM_GAMMA,
		PARAM_DILATE_ENABLED,
		//PARAM_COMBINE_ORIG_MATERIAL_ENABLED,
		PARAM_NOISE_REDUCTION,
		PARAM_NOISE_THRESHOLD,
		PARAM_SEAM_STITCHING_ENABLED,
		PARAM_VISUALIZE_SEAMS_ENABLED,
		PARAM_SEAM_DISTANCE_THRESHOLD,
		PARAM_SEAM_NORMAL_THRESHOLD,
		PARAM_PROBE_DENSITY,
		PARAM_PROBE_SAMPLES,
		PARAM_UV_PADDING,
		PARAM_MERGE_FLAG_LIGHTS,
		PARAM_MERGE_FLAG_AO,
		PARAM_MERGE_FLAG_BOUNCE,
		PARAM_MERGE_FLAG_EMISSION,
		PARAM_MERGE_FLAG_GLOW,
		PARAM_MERGE_FLAG_MATERIAL,
		PARAM_MAX,
	};

	void set_param(LightMapper_Base::Param p_param, Variant p_value);
	Variant get_param(LightMapper_Base::Param p_param);

	struct Data {
		Variant params[PARAM_MAX];
	} data;

	// params
	//	int settings.Forward_NumRays;
	//	float settings.Forward_RayPower;
	//float settings.Forward_BouncePower;
	//float settings.Forward_BounceDirectionality;

	struct Settings {
		void set_images_filename(String p_filename);

		int backward_num_rays;
		float backward_ray_power;
		//float settings.Backward_BouncePower;

		// this number means nothing itself .. it is
		// standardized so that 32 is the normal amount, for backward and forward
		// and is translated into number of forward or backward rays
		//int num_primary_rays;
		//int num_ambient_bounces;

		//int num_ambient_bounce_rays;
		//int num_directional_bounces;
		//float ambient_bounce_power;
		//float directional_bounce_power;
		//float smoothness;
		//float emission_density;
		//float glow;

		//int AO_samples;
		//float AO_range;
		//float AO_cut_range;
		//float AO_reverse_bias;

		eLMMode mode;
		eLMBakeMode bake_mode;
		eLMBakeQuality quality;

		// for faster baking, limit length that light can reach
		//int max_light_dist;

		//int voxel_density; // number of units on largest axis
		//float surface_bias;

		//int tex_width;
		//int tex_height;

		//int max_material_size;

		//bool normalize;
		//float normalize_bias;
		//float light_AO_ratio;
		//float gamma;

		NodePath path_mesh;
		NodePath path_lights;

		//		String lightmap_filename;
		//		bool lightmap_is_HDR;

		//		String ambient_filename;
		//		bool ambient_is_HDR;

		String image_filename_base;

		String combined_filename;
		bool combined_is_HDR;

		String lightmap_filename;
		String AO_filename;
		String emission_filename;
		String glow_filename;
		String bounce_filename;
		String orig_material_filename;
		String tri_ids_filename;

		String UV_filename;
		//int UV_padding;

		String probe_filename;
		//int probe_density; // number of units on largest axis

		//int num_probe_samples;

		//float noise_threshold;
		//float noise_reduction;
		eNRMethod noise_reduction_method;

		//bool use_seam_stitching;

		//		float seam_distance_threshold;
		//		float seam_normal_threshold;

		String sky_filename;
		//		float sky_blur_amount;
		//		int sky_size;
		//		int sky_num_samples;
		//		float sky_brightness;
	} settings;

	// actual params (after applying quality)
	struct AdjustedSettings {
		int tex_width;
		int tex_height;

		int max_light_distance;
		float surface_bias;

		int forward_num_rays;
		int backward_num_rays;
		//int m_Forward_NumBounces;

		int num_primary_rays;
		//int m_Backward_NumBounces;

		int num_ambient_bounces;
		int num_ambient_bounce_rays;

		int num_directional_bounces;
		float directional_bounce_power;

		float emission_density;
		float emission_power;

		float glow;
		float smoothness;
		int AO_num_samples;

		float AO_range;
		float AO_error_metric;
		int AO_abort_timeout;

		int max_material_size;
		int material_kernel_size;

		float normalize_multiplier;

		int num_sky_samples;
		float sky_brightness;

		int antialias_samples_width = 1;
		int antialias_samples_per_texel = 1;

		int antialias_num_light_samples = 1;
	} adjusted_settings;

	struct Logic {
		// some internal logic based on the bake state
		bool process_emission = true;
		bool process_glow = true;
		bool process_lightmap = true;
		bool process_AO = true;
		bool process_bounce = true;
		bool process_orig_material = true;
		bool process_probes = true;
		bool rasterize_mini_lists = true;

		bool output_final = true;
	} logic;

	LightMapper_Base();

protected:
	//	static void _bind_methods();

	//////////////////////////////////////////////////
	// These functions are used from templated load / save image routines
	Color _to_color(FColor fc) const {
		return Color(fc.r, fc.g, fc.b, 1);
	}

	Color _to_color(float f) const {
		return Color(f, f, f, 1);
	}

	Color _to_color(Color c) const { return c; }

	void _fill_from_color(float &r_f, const Color &p_col) const {
		r_f = p_col.r;
	}

	void _fill_from_color(FColor &r_f, const Color &p_col) const {
		r_f = FColor::from_color(p_col);
	}

	void _fill_from_color(Color &r_col, const Color &p_col) const { r_col = p_col; }
	//////////////////////////////////////////////////

	float light_distance_drop_off(float dist, const LLight &light, float power) const {
		float local_power;

		if (light.type != LLight::LT_DIRECTIONAL) {
			local_power = power * inverse_square_drop_off(dist);
		} else
			local_power = power;

		return local_power;
	}

	float normalize_and_find_length(Vector3 &v) const {
		float l = v.length();
		if (l > 0.0f)
			v /= l;
		return l;
	}

	float inverse_square_drop_off(float dist) const {
		dist *= 0.2f;
		dist += 0.282f;
		// 4 PI = 12.5664
		float area = 4.0f * ((float)Math_PI) * (dist * dist);
		return 1.0f / area;
	}

	float largest_color_component(const Color &c) const {
		float l = c.r;
		if (c.g > l)
			l = c.g;
		if (c.b > l)
			l = c.b;
		return l;
	}

	// use the alpha as a multiplier to increase dynamic range
	void color_to_RGBM(Color &c) const {
		c.a = 0.0f;
		if (largest_color_component(c) > 1.0f) {
			c *= 0.125f;
			//c.a = 255.0f;
		}
		return;

		//		c *= 5;
		// if multiplier 1x = x16;
		//		Color o = c;
		Color o = c * (1.0f / 6.0f);
		//		o.a = 16.0f / l;

		// first find the largest component
		float l = largest_color_component(o);
		if (l > 1.0f)
			l = 1.0f;

		o.a = l;

		// quantize the multiplier
		o.a = ceilf(o.a * 255.0f) / 255.0f;

		// zero color pass as is, prevents divide by zero
		if (o.a == 0.0f) {
			c.a = 0.0f;
			return;
		}

		// scale the color balance for best result
		c.r = o.r / o.a;
		c.g = o.g / o.a;
		c.b = o.b / o.a;
		c.a = o.a;

		//		print_line("a is " + String(Variant(c.a)));

		//		float4 rgbm;
		//	  color *= 1.0 / 6.0;
		//	  rgbm.a = saturate( max( max( color.r, color.g ), max( color.b, 1e-6 ) ) );
		//	  rgbm.a = ceil( rgbm.a * 255.0 ) / 255.0;
		//	  rgbm.rgb = color / rgbm.a;
		//	  return rgbm;
	}

	bool load_texel_data(int32_t p_x, int32_t p_y, uint32_t &r_tri_id, const Vector3 **r_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const;
	void _load_texel_data(uint32_t p_tri_id, const Vector3 &p_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const;
};

} // namespace LM
