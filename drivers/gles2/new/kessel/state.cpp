#include "state.h"

namespace Batch {

void State_Fill::Reset()
{
	m_pBatch_ItemGroup = NULL;
	m_pBatch_Item = NULL;
}


void State_ItemGroup::Reset()
{
	m_pItemGroup = NULL;


	m_pScissorItem = NULL;
	m_bScissorActive = false;

	current_clip = NULL;
	shader_cache = NULL;

	rebind_shader = true;
	prev_use_skeleton = false;
//	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);

//	state.current_tex = RID();
//	state.current_tex_ptr = NULL;
//	state.current_normal = RID();
//	state.canvas_texscreen_used = false;

//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	last_blend_mode = -1;

	canvas_last_material = RID();
}


void State_Item::Reset()
{
	skeleton = NULL;
	use_skeleton = false;
	unshaded = false;
	blend_mode = 0;
	reclip = false;
	material_ptr = NULL;
}


} // namespace
