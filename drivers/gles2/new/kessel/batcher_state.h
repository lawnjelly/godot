#pragma once

#include "legacy.h"

namespace Batch {

class BatcherState : public Legacy
{
protected:
	void State_SetItemGroup(int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void State_SetItemGroup_End();
	void State_SetItem(RasterizerCanvas::Item * p_item);


	void State_SetScissorItem(Item * pItem);
	void State_SetScissor(bool bOn);

	void State_SetBlendMode(int iBlendMode);

private:
	Batch * RequestNewBatch(Batch::CommandType ct);
};



} // namespace
