/**************************************************************************/
/*  merge_group_editor_plugin.cpp                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "merge_group_editor_plugin.h"

#include "core/io/resource_saver.h"
#include "editor/spatial_editor_gizmos.h"
#include "scene/3d/mesh_instance.h"
#include "scene/resources/merging_tool.h"
#include "scene/resources/packed_scene.h"

EditorProgress *MergeGroupEditorPlugin::tmp_progress = nullptr;
EditorProgress *MergeGroupEditorPlugin::tmp_subprogress = nullptr;

void MergeGroupEditorBakeDialog::_bake_confirm() {
	_owner_plugin->dialog_pressed_bake(_single_scene->is_pressed(), (int)_face_count_threshold->get_value());
}

MergeGroupEditorBakeDialog::MergeGroupEditorBakeDialog(MergeGroupEditorPlugin *p_owner) {
	_owner_plugin = p_owner;

	set_title("Bake MergeGroup");

	get_ok()->connect("pressed", this, "_bake_confirm");

	VBoxContainer *vbc = memnew(VBoxContainer);
	add_child(vbc);

	_single_scene = memnew(CheckBox);
	_single_scene->set_text(TTR("Single Scene"));

	MarginContainer *mc = memnew(MarginContainer);
	mc->add_constant_override("margin_left", 0);
	mc->add_child(_single_scene);
	vbc->add_child(mc);

	_face_count_threshold = memnew(SpinBox);
	_face_count_threshold->set_min(0);
	_face_count_threshold->set_max(1024 * 128);
	_face_count_threshold->set_step(64);
	_face_count_threshold->set_value(1024);
	vbc->add_margin_child(TTR("Face Count Threshold:"), _face_count_threshold);
}

void MergeGroupEditorBakeDialog::_bind_methods() {
	ClassDB::bind_method("_bake_confirm", &MergeGroupEditorBakeDialog::_bake_confirm);
}

//////////////////////////////////////////////////////////

bool MergeGroupEditorPlugin::bake_func_step(float p_progress, const String &p_description, void *, bool p_force_refresh) {
	if (!tmp_progress) {
		tmp_progress = memnew(EditorProgress("bake_merge_group", TTR("Bake MergeGroup"), 1000, true));
		ERR_FAIL_COND_V(tmp_progress == nullptr, false);
	}
	return tmp_progress->step(p_description, p_progress * 1000, p_force_refresh);
}

bool MergeGroupEditorPlugin::bake_func_substep(float p_progress, const String &p_description, void *, bool p_force_refresh) {
	if (!tmp_subprogress) {
		tmp_subprogress = memnew(EditorProgress("bake_merge_group_substep", "", 1000, true));
		ERR_FAIL_COND_V(tmp_subprogress == nullptr, false);
	}
	return tmp_subprogress->step(p_description, p_progress * 1000, p_force_refresh);
}

void MergeGroupEditorPlugin::bake_func_end(uint32_t p_time_started) {
	if (tmp_progress != nullptr) {
		memdelete(tmp_progress);
		tmp_progress = nullptr;
	}

	if (tmp_subprogress != nullptr) {
		memdelete(tmp_subprogress);
		tmp_subprogress = nullptr;
	}

	const int time_taken = (OS::get_singleton()->get_ticks_msec() - p_time_started) * 0.001;
	if (time_taken >= 1) {
		// Only print a message and request attention if baking took at least 1 second.
		print_line(vformat("Done baking MergeGroup in %02d:%02d:%02d.", time_taken / 3600, (time_taken % 3600) / 60, time_taken % 60));

		// Request attention in case the user was doing something else.
		OS::get_singleton()->request_attention();
	}
}

void MergeGroupEditorPlugin::dialog_pressed_bake(bool p_single_scene, int p_face_count_threshold) {
	if (!_merge_group) {
		return;
	}

	_params.single_scene = p_single_scene;
	_params.face_count_threshold = p_face_count_threshold;

	Node *root = _merge_group->get_tree()->get_edited_scene_root();

	if (root == _merge_group) {
		EditorNode::get_singleton()->show_warning(TTR("Cannot bake MergeGroup when it is scene root."));
		return;
	}

	String scene_path = _merge_group->get_filename();
	if (scene_path == String()) {
		scene_path = _merge_group->get_owner()->get_filename();
	}
	if (scene_path == String()) {
		EditorNode::get_singleton()->show_warning(TTR("Can't determine a save path for merge group.\nSave your scene and try again."));
		return;
	}
	scene_path = scene_path.get_basename() + ".tscn";

	file_dialog->set_current_path(scene_path);
	file_dialog->popup_centered_ratio();
}

void MergeGroupEditorPlugin::_bake() {
	bake_dialog->show();
}

Spatial *MergeGroupEditorPlugin::_convert_merge_group_to_spatial(MergeGroup *p_merge_group) {
	ERR_FAIL_NULL_V(p_merge_group, nullptr);
	Node *parent = p_merge_group->get_parent();
	ERR_FAIL_NULL_V(parent, nullptr);

	Spatial *spatial = memnew(Spatial);
	parent->add_child(spatial);

	// They can't have the same name at the same time
	String name = p_merge_group->get_name();
	p_merge_group->set_name(name + "_temp");
	spatial->set_name(name);

	// Identical transforms
	spatial->set_transform(p_merge_group->get_transform());

	// move the children
	// GODOT is abysmally bad at moving children in order unfortunately.
	// So reverse order for now.
	for (int n = p_merge_group->get_child_count() - 1; n >= 0; n--) {
		Node *child = p_merge_group->get_child(n);
		p_merge_group->remove_child(child);
		spatial->add_child(child);
	}

	// change owners
	MergingTool::_invalidate_owner_recursive(spatial, nullptr, p_merge_group->get_owner());

	// delete AND detach the merge group from the tree
	p_merge_group->_delete_node(p_merge_group);

	return spatial;
}

void MergeGroupEditorPlugin::_bake_select_file(const String &p_file) {
	if (!_merge_group) {
		return;
	}

	// special treatment for scene root
	Node *root = _merge_group->get_tree()->get_edited_scene_root();

	// cannot bake scene root
	ERR_FAIL_COND(root == _merge_group);

	Node *parent = _merge_group->get_parent();
	ERR_FAIL_NULL(parent);

	// Ensure to reset this when exiting this routine!
	// Spatial gizmos, especially for meshes are very expensive
	// in terms of RAM and performance, and are totally
	// unnecessary for temporary objects
	SpatialEditor::_prevent_gizmo_generation = true;

#ifdef GODOT_MERGING_VERBOSE
	MergingTool::debug_branch(_merge_group, "START_SCENE");
#endif

	Spatial *hanger = memnew(Spatial);
	hanger->set_name("hanger");
	parent->add_child(hanger);
	hanger->set_owner(_merge_group->get_owner());

	uint32_t time_start = OS::get_singleton()->get_ticks_msec();
	bake_func_step(0.0, "Duplicating Branch", nullptr, true);

	_duplicate_branch(_merge_group, hanger);

	// temporarily hide source branch, to speed things up in the editor
	bool was_visible = _merge_group->is_visible_in_tree();
	_merge_group->hide();

	MergeGroup *merge_group_copy = Object::cast_to<MergeGroup>(hanger->get_child(0));

	if (merge_group_copy->merge_meshes_in_editor()) {
		if (!bake_func_step(1.0, "Saving Scene", nullptr, true)) {
			// Convert the merge node to a spatial..
			// Once baked we don't want baked scenes to be merged AGAIN
			// when incorporated into scenes.
			Spatial *final_branch = _convert_merge_group_to_spatial(merge_group_copy);

			// Only save if not cancelled by user
			_save_scene(final_branch, p_file);
		}
	}

#ifdef GODOT_MERGING_VERBOSE
	MergingTool::debug_branch(hanger, "END_SCENE");
#endif

	// finished
	hanger->queue_delete();
	_merge_group->set_visible(was_visible);

	SpatialEditor::_prevent_gizmo_generation = false;

	bake_func_end(time_start);
}

void MergeGroupEditorPlugin::_remove_queue_deleted_nodes_recursive(Node *p_node) {
	if (p_node->is_queued_for_deletion()) {
		p_node->get_parent()->remove_child(p_node);
		return;
	}

	for (int n = p_node->get_child_count() - 1; n >= 0; n--) {
		_remove_queue_deleted_nodes_recursive(p_node->get_child(n));
	}
}

uint32_t MergeGroupEditorPlugin::_get_mesh_poly_count(const MeshInstance &p_mi) const {
	Ref<Mesh> rmesh = p_mi.get_mesh();
	if (rmesh.is_valid()) {
		return rmesh->get_face_count();
	}

	return 0;
}

bool MergeGroupEditorPlugin::_replace_with_branch_scene(const String &p_file, Node *p_base) {
	Node *old_owner = p_base->get_owner();

	Ref<PackedScene> sdata = ResourceLoader::load(p_file);
	if (!sdata.is_valid()) {
		ERR_PRINT("Error loading scene from \"" + p_file + "\"");
		return false;
	}

	Node *instanced_scene = sdata->instance(PackedScene::GEN_EDIT_STATE_INSTANCE);
	if (!instanced_scene) {
		ERR_PRINT("Error instancing scene from \"" + p_file + "\"");
		return false;
	}

	Node *parent = p_base->get_parent();
	int pos = p_base->get_index();

	parent->remove_child(p_base);
	parent->add_child(instanced_scene);
	parent->move_child(instanced_scene, pos);

	List<Node *> owned;
	p_base->get_owned_by(p_base->get_owner(), &owned);
	Array owners;
	for (List<Node *>::Element *F = owned.front(); F; F = F->next()) {
		owners.push_back(F->get());
	}

	instanced_scene->set_owner(old_owner);

	p_base->queue_delete();

	return true;
}

bool MergeGroupEditorPlugin::_save_subscene(Node *p_root, Node *p_branch, String p_base_filename, int &r_subscene_count) {
	bake_func_substep(0.0, p_branch->get_name(), nullptr, false);

	Node *scene_root = p_root;

	Map<Node *, Node *> reown;
	reown[scene_root] = p_branch;

	Node *copy = p_branch->duplicate_and_reown(reown);

	if (copy) {
		Ref<PackedScene> sdata = memnew(PackedScene);
		Error err = sdata->pack(copy);
		memdelete(copy);

		if (err != OK) {
			WARN_PRINT("Couldn't save subscene \"" + p_branch->get_name() + "\" . Likely dependencies (instances) couldn't be satisfied. Saving as part of main scene instead.");
			return false;
		}

		String filename = p_base_filename + "_" + itos(r_subscene_count++) + ".scn";

#ifdef DEV_ENABLED
		print_verbose("Save subscene: " + filename);
#endif

		err = ResourceSaver::save(filename, sdata, ResourceSaver::FLAG_COMPRESS);
		if (err != OK) {
			WARN_PRINT("Error saving subscene \"" + p_branch->get_name() + "\" , saving as part of main scene instead.");
			return false;
		}
		_replace_with_branch_scene(filename, p_branch);
	} else {
		WARN_PRINT("Error duplicating subscene \"" + p_branch->get_name() + "\" , saving as part of main scene instead.");
		return false;
	}

	return true;
}

void MergeGroupEditorPlugin::_save_mesh_subscenes_recursive(Node *p_root, Node *p_node, String p_base_filename, int &r_subscene_count) {
	if (p_node->is_queued_for_deletion()) {
		return;
	}
	// is a subscene already?
	if (p_node->get_filename().length() && (p_node != p_root)) {
		return;
	}

	// is it a mesh instance?
	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);

	// don't save subscenes for trivially sized meshes
	if (mi && (!_params.face_count_threshold || ((int)_get_mesh_poly_count(*mi) > _params.face_count_threshold))) {
		// save as subscene
		if (_save_subscene(p_root, p_node, p_base_filename, r_subscene_count)) {
			return;
		}
	}

	// replaced subscenes will be added to the last child, so going in reverse order is necessary
	for (int n = p_node->get_child_count() - 1; n >= 0; n--) {
		_save_mesh_subscenes_recursive(p_root, p_node->get_child(n), p_base_filename, r_subscene_count);
	}
}

void MergeGroupEditorPlugin::_push_mesh_data_to_gpu_recursive(Node *p_node) {
	// is it a mesh instance?
	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);

	if (mi) {
		Ref<Mesh> rmesh = mi->get_mesh();
		if (rmesh.is_valid()) {
			rmesh->set_storage_mode(Mesh::STORAGE_MODE_GPU);
		}
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		_push_mesh_data_to_gpu_recursive(p_node->get_child(n));
	}
}

bool MergeGroupEditorPlugin::_save_scene(Node *p_branch, String p_filename) {
	// For some reason the saving machinery doesn't deal well with nodes queued for deletion,
	// so we will remove them from the scene tree (as risk of leaks, but the queue delete machinery
	// should still work when detached).
	_remove_queue_deleted_nodes_recursive(p_branch);

	// All mesh data must be on the GPU for the Mesh saving routines to work
	_push_mesh_data_to_gpu_recursive(p_branch);

	Node *scene_root = p_branch->get_tree()->get_edited_scene_root();

	Map<Node *, Node *> reown;
	reown[scene_root] = p_branch;

	Node *copy = p_branch->duplicate_and_reown(reown);

#ifdef GODOT_MERGING_VERBOSE
	MergingTool::debug_branch(copy, "SAVE SCENE:");
#endif

	if (copy) {
		// Save any large meshes as compressed resources
		if (!_params.single_scene) {
			int subscene_count = 0;
			_save_mesh_subscenes_recursive(copy, copy, p_filename.get_basename(), subscene_count);
		}

		Ref<PackedScene> sdata = memnew(PackedScene);
		Error err = sdata->pack(copy);
		memdelete(copy);

		if (err != OK) {
			EditorNode::get_singleton()->show_warning(TTR("Couldn't save merged branch.\nLikely dependencies (instances) couldn't be satisfied."));
			return false;
		}

		err = ResourceSaver::save(p_filename, sdata, ResourceSaver::FLAG_COMPRESS);
		if (err != OK) {
			EditorNode::get_singleton()->show_warning(TTR("Error saving scene."));
			return false;
		}
	} else {
		EditorNode::get_singleton()->show_warning(TTR("Error duplicating scene to save it."));
		return false;
	}

	return true;
}

void MergeGroupEditorPlugin::_duplicate_branch(Node *p_branch, Node *p_new_parent) {
	Node *dup = p_branch->duplicate();

	ERR_FAIL_NULL(dup);

	p_new_parent->add_child(dup);

	Node *new_owner = p_new_parent->get_owner();
	dup->set_owner(new_owner);

	MergingTool::_invalidate_owner_recursive(dup, nullptr, new_owner);
}

void MergeGroupEditorPlugin::edit(Object *p_object) {
	MergeGroup *mg = Object::cast_to<MergeGroup>(p_object);
	if (!mg) {
		return;
	}

	_merge_group = mg;
}

bool MergeGroupEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("MergeGroup");
}

void MergeGroupEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		button_bake->show();
	} else {
		button_bake->hide();
		bake_dialog->hide();
	}
}

void MergeGroupEditorPlugin::_bind_methods() {
	ClassDB::bind_method("_bake", &MergeGroupEditorPlugin::_bake);
	ClassDB::bind_method("_bake_select_file", &MergeGroupEditorPlugin::_bake_select_file);
}

MergeGroupEditorPlugin::MergeGroupEditorPlugin(EditorNode *p_node) {
	editor = p_node;

	button_bake = memnew(ToolButton);
	button_bake->set_icon(editor->get_gui_base()->get_icon("Bake", "EditorIcons"));
	button_bake->set_text(TTR("Bake"));
	button_bake->hide();
	button_bake->connect("pressed", this, "_bake");

	file_dialog = memnew(EditorFileDialog);
	file_dialog->set_mode(EditorFileDialog::MODE_SAVE_FILE);
	file_dialog->add_filter("*.tscn ; " + TTR("Scene"));
	file_dialog->add_filter("*.scn ; " + TTR("Binary Scene"));
	file_dialog->set_title(TTR("Save Merged Scene As..."));
	file_dialog->connect("file_selected", this, "_bake_select_file");
	button_bake->add_child(file_dialog);

	bake_dialog = memnew(MergeGroupEditorBakeDialog(this));
	bake_dialog->set_anchors_and_margins_preset(Control::PRESET_CENTER);
	bake_dialog->hide();
	button_bake->add_child(bake_dialog);

	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, button_bake);

	_merge_group = nullptr;

	MergeGroup::bake_step_function = bake_func_step;
	MergeGroup::bake_substep_function = bake_func_substep;
	MergeGroup::bake_end_function = bake_func_end;
}

MergeGroupEditorPlugin::~MergeGroupEditorPlugin() {
}
