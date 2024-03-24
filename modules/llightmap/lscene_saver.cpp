#include "lscene_saver.h"
#include "core/io/resource_saver.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

namespace LM {

void SceneSaver::set_filename_recursive(Node *pNode) {
	pNode->set_filename("");

	for (int n = 0; n < pNode->get_child_count(); n++) {
		set_filename_recursive(pNode->get_child(n));
	}
}

void SceneSaver::set_owner_recursive(Node *pNode, Node *pOwner) {
	//	String sz;
	//	sz = "node " + pNode->get_name();
	//	if (pNode->get_owner())
	//	{
	//		sz += ", owner was " + pNode->get_owner()->get_name();
	//	}
	//	print_line(sz);

	if (pNode != pOwner)
		pNode->set_owner(pOwner);

	for (int n = 0; n < pNode->get_child_count(); n++) {
		set_owner_recursive(pNode->get_child(n), pOwner);
	}
}

bool SceneSaver::save_scene(Node *pNode, String szFilename, bool reset_filenames) {
	// for subscenes, it doesn't seem to save edited stuff correctly unless we blank
	// the filenames.
	if (reset_filenames)
		set_filename_recursive(pNode);

	Node *pPreviousOwner = pNode->get_owner();

	// godot needs owner to be set on nodes that are to be saved as part of a packed scene
	set_owner_recursive(pNode, pNode);

	//PackedScene ps;
	// reference should self delete on exiting func .. check!
	Ref<PackedScene> ps = memnew(PackedScene);
	//Ref<PackedScene> ref_ps = &ps;

	ps->pack(pNode);

	ResourceSaver rs;
	Error err = rs.save(szFilename, ps);

	// set back previous owner
	set_owner_recursive(pNode, pPreviousOwner);

	// reimport
	//	ResourceLoader::import(szFilename);

	return err == OK;
}

} //namespace LM
