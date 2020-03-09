#pragma once

#include "servers/visual/rasterizer.h"
#include "drivers/gles2/rasterizer_storage_gles2.h"
#include "batch.h"

namespace Batch {

class BState
{



};

class State_ItemGroup
{
public:
	void Reset();
	BItemGroup * m_pItemGroup;
	RasterizerCanvas::Item *current_clip;
	RasterizerStorageGLES2::Shader *shader_cache;
	bool rebind_shader;
	bool prev_use_skeleton;
	int last_blend_mode;
	RID canvas_last_material;
};

class State_Item
{
public:
	void Reset();
	RasterizerStorageGLES2::Skeleton *skeleton;
	bool use_skeleton;
};


} // namespace
