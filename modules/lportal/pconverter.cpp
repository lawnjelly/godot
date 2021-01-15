#include "pconverter.h"
#include "pdebug.h"
#include "prooms.h"
#include "lroom.h"
#include "lportal.h"

using namespace Lawn;

bool PConverter::name_starts_with(const Node * pNode, String szSearch) const
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


String PConverter::find_name_after(Node * pNode, String szStart) const
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


void PConverter::convert(PRooms &rooms)
{
	LPRINT(2, "Convert");

	_portals.clear(true);

	convert_rooms_recursive(rooms.get_roomlist());

	second_pass_portals();
}

void PConverter::second_pass_portals()
{
	LPRINT(4, "SecondPass_Portals");

	Node *pRootNode = SceneTree::get_singleton()->get_edited_scene_root();

	for (int n=0; n<_portals.size(); n++)
	{
		LPortal * pPortal = _portals[n];
		String szLinkRoom = find_name_after(pPortal, "lportal_");
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


void PConverter::convert_portal(MeshInstance * pMI)
{
	String szFullName = pMI->get_name();
	LPRINT(4, "ConvertPortal : " + szFullName);

	LPortal * pPortal = change_node_type<LPortal>(pMI, "l", false);

	// copy mesh
	pPortal->set_mesh(pMI->get_mesh());
	if (pMI->get_surface_material_count())
		pPortal->set_surface_material(0, pMI->get_surface_material(0));


	pMI->queue_delete();

	// link rooms
	pPortal->update_from_mesh();

	// keep a list of portals for second pass
	_portals.push_back(pPortal);
}


void PConverter::convert_room(Spatial * p_node)
{
	// get the room part of the name
	String szFullName = p_node->get_name();
	String szRoom = find_name_after(p_node, "room_");

	LPRINT(4, "ConvertRoom : " + szFullName);

	// owner should normally be root
	Node * pOwner = p_node->get_owner();

	// create an LRoom
	LRoom * pLRoom = change_node_type<LRoom>(p_node, "l");

	// move each child
	while (p_node->get_child_count())
	{
		Node * child = p_node->get_child(0);
		p_node->remove_child(child);

		// needs to set owner to appear in IDE
		pLRoom->add_child(child);
		child->set_owner(pOwner);
	}


	// find portals
	for (int n=0; n<pLRoom->get_child_count(); n++)
	{
		MeshInstance * child = Object::cast_to<MeshInstance>(pLRoom->get_child(n));

		if (child)
		{
			if (name_starts_with(child, "portal_"))
			{
				convert_portal(child);
			}
		}
	}

}


void PConverter::convert_rooms_recursive(Spatial * p_node)
{
	// is this a room?
	if (name_starts_with(p_node, "room_"))
	{
		convert_room(p_node);
	}

	// recurse through children
	for (int n=0; n<p_node->get_child_count(); n++)
	{
		Spatial * child = Object::cast_to<Spatial>(p_node->get_child(n));

		if (child)
		{
			convert_rooms_recursive(child);
		}
	}

}
