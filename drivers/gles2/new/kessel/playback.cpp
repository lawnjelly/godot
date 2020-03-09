#include "playback.h"

namespace Batch {

void Playback::Playback_Change_Item(const Batch &batch)
{

}


void Playback::Playback_Change_ItemGroup(const Batch &batch)
{
	m_State_ItemGroup.Reset();

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
	state.current_tex = RID();
	state.current_tex_ptr = NULL;
	state.current_normal = RID();
	state.canvas_texscreen_used = false;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
}

void Playback::Playback_Change_ItemGroup_End(const Batch &batch)
{
	if (m_State_ItemGroup.current_clip) {
		glDisable(GL_SCISSOR_TEST);
	}

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
}




} // namespace
