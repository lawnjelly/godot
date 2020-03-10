#pragma once

#include "drivers/gles2/new/defines.h"
#include "drivers/gles2/renderer_2d_old.h"

#include "state.h"
#include "data.h"

namespace Batch {

class Legacy : public Renderer2D_old
{
public:
	Legacy() {m_bDryRun = true;}


protected:
	BData m_Data;
	BState m_State;
	State_ItemGroup m_State_ItemGroup;
	State_Item m_State_Item;
	State_Fill m_State_Fill;


	void GL_SetState_LightBlend(VS::CanvasLightMode mode);
	void GL_SetState_BlendMode(int blend_mode);

	void CanvasShader_SetConditionals_Light(bool has_shadow, Light * light);


protected:
	// when filling we do a dry run and don't change GL states,
	// we have to repeat the same logic to fill
	bool m_bDryRun;
};


} // namespace
