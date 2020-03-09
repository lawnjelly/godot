#include "batcher_state.h"

namespace Batch {

void BatcherState::State_SetItemGroup(int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform)
{
	BItemGroup * p = m_Data.itemgroups.request_with_grow();
	p->z = p_z;
	p->pLight = p_light;
	p->colModulate = p_modulate;
	p->trBase = p_base_transform;

	Batch * b = RequestNewBatch(Batch::BT_CHANGE_ITEM_GROUP);
	b->change_itemgroup.id = m_Data.itemgroups.size()-1;
}

void BatcherState::State_SetItemGroup_End()
{
	Batch * b = RequestNewBatch(Batch::BT_CHANGE_ITEM_GROUP_END);
}

void BatcherState::State_SetItem(RasterizerCanvas::Item * p_item)
{
	Batch * b = RequestNewBatch(Batch::BT_CHANGE_ITEM);
	b->change_item.m_pItem = p_item;
}


Batch * BatcherState::RequestNewBatch(Batch::CommandType ct)
{
	Batch * p = m_Data.request_new_batch();
	p->type = ct;
	return p;
}



} // namespace
