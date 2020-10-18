#include "pconverter.h"
#include "pdebug.h"
#include "prooms.h"
#include "lroom.h"
#include "lportal.h"

using namespace Lawn;

bool PConverter::NameStartsWith(const Node * pNode, String szSearch) const
{
	int sl = szSearch.length();

	String name = pNode->get_name();
	int l = name.length();

	if (l < sl)
		return false;

	String szStart = name.substr(0, sl);

	//print_line("\t\tNameStartsWith szStart is " + szStart);

	if (szStart == szSearch)
		return true;

	return false;
}


String PConverter::FindNameAfter(Node * pNode, String szStart) const
{
	String szRes;
	String name = pNode->get_name();
	szRes = name.substr(szStart.length());

	// because godot doesn't support multiple nodes with the same name, we will strip e.g. a number
	// after an @ on the end of the name...
	// e.g. portal_kitchen@2
	for (int c=0; c<szRes.length(); c++)
	{
		if (szRes[c] == '*')
		{
			// remove everything after and including this character
			szRes = szRes.substr(0, c);
			break;
		}
	}

	//print("\t\tNameAfter is " + szRes);
	return szRes;
}


void PConverter::Convert(PRooms &rooms)
{
	LPRINT(2, "Convert");

	m_Portals.clear(true);

	ConvertRooms_recursive(rooms.GetRoomList());

	SecondPass_Portals();
}

void PConverter::SecondPass_Portals()
{
	LPRINT(4, "SecondPass_Portals");

	Node *pRootNode = SceneTree::get_singleton()->get_edited_scene_root();

	for (int n=0; n<m_Portals.size(); n++)
	{
		LPortal * pPortal = m_Portals[n];
		String szLinkRoom = FindNameAfter(pPortal, "lportal_");
		szLinkRoom = "lroom_" + szLinkRoom;

		String sz = "\tLinkRoom : " + szLinkRoom;


		LRoom * pLinkedRoom = Object::cast_to<LRoom>(pRootNode->find_node(szLinkRoom, true, false));
		if (pLinkedRoom)
		{
			sz += "\tfound.";
			NodePath path = pPortal->get_path_to(pLinkedRoom);
			pPortal->set_linked_room(path);
		}
		else
			sz += "\tnot found.";

		LPRINT(4, sz);
	}
}


void PConverter::ConvertPortal(MeshInstance * pMI)
{
	String szFullName = pMI->get_name();
	LPRINT(4, "ConvertPortal : " + szFullName);

	LPortal * pPortal = ChangeNodeType<LPortal>(pMI, "l", false);

	// copy mesh
	pPortal->set_mesh(pMI->get_mesh());
	if (pMI->get_surface_material_count())
		pPortal->set_surface_material(0, pMI->get_surface_material(0));


	pMI->queue_delete();

	// link rooms
	pPortal->UpdateFromMesh();

	// keep a list of portals for second pass
	m_Portals.push_back(pPortal);
}


void PConverter::ConvertRoom(Spatial * pNode)
{
	// get the room part of the name
	String szFullName = pNode->get_name();
	String szRoom = FindNameAfter(pNode, "room_");

	LPRINT(4, "ConvertRoom : " + szFullName);

	// owner should normally be root
	Node * pOwner = pNode->get_owner();

	// create an LRoom
	LRoom * pLRoom = ChangeNodeType<LRoom>(pNode, "l");

	// move each child
	while (pNode->get_child_count())
	{
		Node * pChild = pNode->get_child(0);
		pNode->remove_child(pChild);

		// needs to set owner to appear in IDE
		pLRoom->add_child(pChild);
		pChild->set_owner(pOwner);
	}


	// find portals
	for (int n=0; n<pLRoom->get_child_count(); n++)
	{
		MeshInstance * pChild = Object::cast_to<MeshInstance>(pLRoom->get_child(n));

		if (pChild)
		{
			if (NameStartsWith(pChild, "portal_"))
			{
				ConvertPortal(pChild);
			}
		}
	}

}


void PConverter::ConvertRooms_recursive(Spatial * pNode)
{
	// is this a room?
	if (NameStartsWith(pNode, "room_"))
	{
		ConvertRoom(pNode);
	}

	// recurse through children
	for (int n=0; n<pNode->get_child_count(); n++)
	{
		Spatial * pChild = Object::cast_to<Spatial>(pNode->get_child(n));

		if (pChild)
		{
			ConvertRooms_recursive(pChild);
		}
	}

}
