#pragma once

#include "shader_bgfx.h"

class SceneShaderBGFX : public ShaderBGFX {
	virtual String get_shader_name() const { return "SceneShaderBGFX"; }

public:
	virtual void init() {
	}
};
