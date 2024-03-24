#pragma once

namespace LM {

class SceneSaver {
public:
	bool save_scene(Node *pNode, String szFilename, bool reset_filenames = false);
	void set_owner_recursive(Node *pNode, Node *pOwner);
	void set_filename_recursive(Node *pNode);
};

} //namespace LM
