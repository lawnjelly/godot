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
}


void State_Item::Reset()
{
	m_pSkeleton = NULL;
	m_bUseSkeleton = false;
	m_bUnshaded = false;
	m_iBlendMode = 0;
	m_bReclip = false;
	m_pMaterial = NULL;
}


} // namespace
