#pragma once

#include "llightmapper.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/spatial.h"

class LLightmap : public Spatial {
	GDCLASS(LLightmap, Spatial);

	//Mutex baking_mutex;

public:
	enum eMode {
		MODE_FORWARD = LM::LightMapper::LMMODE_FORWARD,
		MODE_BACKWARD = LM::LightMapper::LMMODE_BACKWARD,
	};

	enum eBakeMode {
		BAKEMODE_LIGHTMAP = LM::LightMapper::LMBAKEMODE_LIGHTMAP,
		BAKEMODE_EMISSION = LM::LightMapper::LMBAKEMODE_EMISSION,
		BAKEMODE_AO = LM::LightMapper::LMBAKEMODE_AO,
		BAKEMODE_MATERIAL = LM::LightMapper::LMBAKEMODE_ORIG_MATERIAL,
		BAKEMODE_BOUNCE = LM::LightMapper::LMBAKEMODE_BOUNCE,
		BAKEMODE_PROBES = LM::LightMapper::LMBAKEMODE_PROBES,
		BAKEMODE_UVMAP = LM::LightMapper::LMBAKEMODE_UVMAP,
		BAKEMODE_COMBINED = LM::LightMapper::LMBAKEMODE_COMBINED,
		BAKEMODE_MERGE = LM::LightMapper::LMBAKEMODE_MERGE,
	};

	enum eQuality {
		QUALITY_LOW = LM::LightMapper::LM_QUALITY_LOW,
		QUALITY_MEDIUM = LM::LightMapper::LM_QUALITY_MEDIUM,
		QUALITY_HIGH = LM::LightMapper::LM_QUALITY_HIGH,
		QUALITY_FINAL = LM::LightMapper::LM_QUALITY_FINAL,
	};

	bool lightmap_mesh(Node *pMeshRoot, Node *pLightRoot, Object *pOutputImage_Lightmap, Object *pOutputImage_AO, Object *pOutputImage_Combined);
	bool lightmap_bake();
	bool lightmap_bake_to_image(Object *pOutputLightmapImage, Object *pOutputAOImage, Object *pOutputCombinedImage);

	bool uvmap();

	void set_mode(LLightmap::eMode p_mode);
	LLightmap::eMode get_mode() const;

	void set_bake_mode(LLightmap::eBakeMode p_mode);
	LLightmap::eBakeMode get_bake_mode() const;

	void set_quality(LLightmap::eQuality p_quality);
	LLightmap::eQuality get_quality() const;

	void set_mesh_path(const NodePath &p_path);
	NodePath get_mesh_path() const;
	void set_lights_path(const NodePath &p_path);
	NodePath get_lights_path() const;

	////////////////////////////
	void set_backward_num_rays(int num_rays);
	int get_backward_num_rays() const;

	////////////////////////////

	//	void set_lightmap_filename(const String &p_filename);
	//	String get_lightmap_filename() const;

	//	void set_ao_filename(const String &p_filename);
	//	String get_ao_filename() const;

	void set_combined_filename(const String &p_filename);
	String get_combined_filename() const;

	// UV
	void set_uv_filename(const String &p_filename);
	String get_uv_filename() const;

	void set_noise_reduction_method(int method);
	int get_noise_reduction_method() const;

	// sky
	void set_sky_filename(const String &p_filename);
	String get_sky_filename() const;

	void set_param(LM::LightMapper::Param p_param, Variant p_value);
	Variant get_param(LM::LightMapper::Param p_param);

private:
	LM::LightMapper m_LM;

	void ShowWarning(String sz, bool bAlert = true) { m_LM.show_warning(sz, bAlert); }

protected:
	static void _bind_methods();
};

VARIANT_ENUM_CAST(LM::LightMapper::Param);
VARIANT_ENUM_CAST(LLightmap::eMode);
VARIANT_ENUM_CAST(LLightmap::eBakeMode);
VARIANT_ENUM_CAST(LLightmap::eQuality);
