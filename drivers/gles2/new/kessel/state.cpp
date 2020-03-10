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

	m_pCachedShader = NULL;

	m_bRebindShader = true;
	m_bPrevUseSkeleton = false;
//	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);

//	state.current_tex = RID();
//	state.current_tex_ptr = NULL;
//	state.current_normal = RID();
//	state.canvas_texscreen_used = false;

//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	m_iLastBlendMode = -1;

	m_RID_CanvasLastMaterial = RID();

	m_iBlendMode = 0;
	m_pMaterial = NULL;

}


void State_Item::Reset()
{
	m_pSkeleton = NULL;
	m_bUseSkeleton = false;
	m_bUnshaded = false;
	m_bReclip = false;
//	m_iBlendMode = 0;
//	m_pMaterial = NULL;
}

String State_Item::ChangeFlagsToString() const // for debugging
{
	uint32_t f = m_ChangeFlags;
	String sz = "";

	if (!f)
	{
		sz = "none";
		return sz;
	}

	if (f & CF_SCISSOR)
		sz += " | scissor";
	if (f & CF_COPY_BACK_BUFFER)
		sz += " | copy_back_buffer";
	if (f & CF_SKELETON)
		sz += " | skeleton";
	if (f & CF_MATERIAL)
		sz += " | material";
	if (f & CF_BLEND_MODE)
		sz += " | blend_mode";
	if (f & CF_FINAL_MODULATE)
		sz += " | final_modulate";
	if (f & CF_MODEL_VIEW)
		sz += " | model_view";
	if (f & CF_EXTRA)
		sz += " | extra";
	if (f & CF_LIGHT)
		sz += " | light";

	return sz;
}

} // namespace
