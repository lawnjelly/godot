#pragma once

#include "lvector.h"

class LPortal;

namespace Lawn {

class PRooms;

class PConverter
{
public:
	void Convert(PRooms &rooms);

private:
	void ConvertRooms_recursive(Spatial * pNode);
	void ConvertRoom(Spatial * pNode);
	void ConvertPortal(MeshInstance * pMI);

	void SecondPass_Portals();

	// useful funcs
	bool NameStartsWith(const Node * pNode, String szSearch) const;
	String FindNameAfter(Node * pNode, String szStart) const;

	template <class T> T * ChangeNodeType(Spatial * pNode, String szPrefix, bool bDelete = true);

	LVector<LPortal *> m_Portals;
};

template <class T> T * PConverter::ChangeNodeType(Spatial * pNode, String szPrefix, bool bDelete)
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
