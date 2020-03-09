#pragma once

#include "drivers/gles2/new/defines.h"
#include "drivers/gles2/renderer_2d_old.h"

#include "state.h"
#include "data.h"

namespace Batch {

class Legacy : public Renderer2D_old
{
public:


protected:
	BData m_Data;
	BState m_State;
	State_ItemGroup m_State_ItemGroup;
};


} // namespace
