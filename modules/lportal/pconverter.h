#pragma once

#include "lvector.h"

class LPortal;

namespace Lawn {

class PRooms;

class PConverter
{
public:
	void convert(PRooms &rooms);

private:
	void convert_rooms_recursive(Spatial * p_node);
	void convert_room(Spatial * p_node);
	void convert_portal(MeshInstance * pMI);

	void second_pass_portals();

	// useful funcs
	bool name_starts_with(const Node * pNode, String szSearch) const;
	String find_name_after(Node * pNode, String szStart) const;

	template <class T> T * change_node_type(Spatial * pNode, String szPrefix, bool bDelete = true);

	LVector<LPortal *> _portals;
};

template <class T> T * PConverter::change_node_type(Spatial * pNode, String szPrefix, bool bDelete)
{
	String szFullName = pNode->get_name();

	// owner should normally be root
	Node * pOwner = pNode->get_owner();

	// create the new class T object
	T * pNew = memnew(T);
	pNew->set_name(szPrefix + szFullName);

	// needs to set owner to appear in IDE
	pNode->get_parent()->add_child(pNew);
	pNew->set_owner(pOwner);

	// new lroom should have same transform
	pNew->set_transform(pNode->get_transform());

	// delete old node
	if (bDelete)
		pNode->queue_delete();

	return pNew;
}


} // namespace
