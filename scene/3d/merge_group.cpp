/**************************************************************************/
/*  merge_group.cpp                                                       */
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

#include "merge_group.h"

#include "core/bitfield_dynamic.h"
#include "core/engine.h"
#include "core/os/os.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/physics_body.h"
#include "scene/resources/merging_tool.h"

#include "modules/modules_enabled.gen.h" // For csg.
#ifdef MODULE_CSG_ENABLED
#include "modules/csg/csg_shape.h"
#endif

#ifdef MODULE_GRIDMAP_ENABLED
#include "modules/gridmap/grid_map.h"
#endif

int MergeGroup::MeshAABB::_sort_axis = 0;

MergeGroup::BakeStepFunc MergeGroup::bake_step_function = nullptr;
MergeGroup::BakeStepFunc MergeGroup::bake_substep_function = nullptr;
MergeGroup::BakeEndFunc MergeGroup::bake_end_function = nullptr;

void MergeGroup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("merge_meshes"), &MergeGroup::merge_meshes);

	ClassDB::bind_method(D_METHOD("set_auto_merge", "enabled"), &MergeGroup::set_auto_merge);
	ClassDB::bind_method(D_METHOD("get_auto_merge"), &MergeGroup::get_auto_merge);

	// ClassDB::bind_method(D_METHOD("set_delete_sources", "enabled"), &MergeGroup::set_delete_sources);
	// ClassDB::bind_method(D_METHOD("get_delete_sources"), &MergeGroup::get_delete_sources);

	// ClassDB::bind_method(D_METHOD("set_convert_sources", "enabled"), &MergeGroup::set_convert_sources);
	// ClassDB::bind_method(D_METHOD("get_convert_sources"), &MergeGroup::get_convert_sources);

	ClassDB::bind_method(D_METHOD("set_convert_csgs", "enabled"), &MergeGroup::set_convert_csgs);
	ClassDB::bind_method(D_METHOD("get_convert_csgs"), &MergeGroup::get_convert_csgs);

	ClassDB::bind_method(D_METHOD("set_convert_gridmaps", "enabled"), &MergeGroup::set_convert_gridmaps);
	ClassDB::bind_method(D_METHOD("get_convert_gridmaps"), &MergeGroup::get_convert_gridmaps);

	ClassDB::bind_method(D_METHOD("set_shadow_proxy", "enabled"), &MergeGroup::set_shadow_proxy);
	ClassDB::bind_method(D_METHOD("get_shadow_proxy"), &MergeGroup::get_shadow_proxy);

	ClassDB::bind_method(D_METHOD("set_clean_meshes", "enabled"), &MergeGroup::set_clean_meshes);
	ClassDB::bind_method(D_METHOD("get_clean_meshes"), &MergeGroup::get_clean_meshes);

	ClassDB::bind_method(D_METHOD("set_join_meshes", "enabled"), &MergeGroup::set_join_meshes);
	ClassDB::bind_method(D_METHOD("get_join_meshes"), &MergeGroup::get_join_meshes);

	// HARD CODED TO ON FOR NOW
	// ClassDB::bind_method(D_METHOD("set_split_by_surface", "enabled"), &MergeGroup::set_split_by_surface);
	// ClassDB::bind_method(D_METHOD("get_split_by_surface"), &MergeGroup::get_split_by_surface);

	ClassDB::bind_method(D_METHOD("set_max_merges", "max_merges"), &MergeGroup::set_max_merges);
	ClassDB::bind_method(D_METHOD("get_max_merges"), &MergeGroup::get_max_merges);

	ClassDB::bind_method(D_METHOD("set_splits_horizontal", "num_splits"), &MergeGroup::set_splits_horizontal);
	ClassDB::bind_method(D_METHOD("get_splits_horizontal"), &MergeGroup::get_splits_horizontal);

	ClassDB::bind_method(D_METHOD("set_splits_vertical", "num_splits"), &MergeGroup::set_splits_vertical);
	ClassDB::bind_method(D_METHOD("get_splits_vertical"), &MergeGroup::get_splits_vertical);

	ClassDB::bind_method(D_METHOD("set_min_split_poly_count", "poly_count"), &MergeGroup::set_min_split_poly_count);
	ClassDB::bind_method(D_METHOD("get_min_split_poly_count"), &MergeGroup::get_min_split_poly_count);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_merge"), "set_auto_merge", "get_auto_merge");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shadow_proxy"), "set_shadow_proxy", "get_shadow_proxy");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "split_by_surface"), "set_split_by_surface", "get_split_by_surface");

	ADD_GROUP("Locality", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_merges", PROPERTY_HINT_RANGE, "0,50000,1"), "set_max_merges", "get_max_merges");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "splits_horizontal", PROPERTY_HINT_RANGE, "0,16,1"), "set_splits_horizontal", "get_splits_horizontal");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "splits_vertical", PROPERTY_HINT_RANGE, "0,16,1"), "set_splits_vertical", "get_splits_vertical");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_split_poly_count", PROPERTY_HINT_RANGE, "0,100000,1"), "set_min_split_poly_count", "get_min_split_poly_count");

	ADD_GROUP("Convert", "convert_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "convert_csgs"), "set_convert_csgs", "get_convert_csgs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "convert_gridmaps"), "set_convert_gridmaps", "get_convert_gridmaps");

	ADD_GROUP("Cleanup", "");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "delete_sources"), "set_delete_sources", "get_delete_sources");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "convert_sources"), "set_convert_sources", "get_convert_sources");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "join_meshes"), "set_join_meshes", "get_join_meshes");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clean_meshes"), "set_clean_meshes", "get_clean_meshes");
}

void MergeGroup::merge_meshes() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		_merge_meshes();
	}
}

bool MergeGroup::merge_meshes_in_editor() {
	return _merge_meshes();
}

void MergeGroup::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			ERR_FAIL_COND(get_world().is_null());

			if (data.auto_merge && !Engine::get_singleton()->is_editor_hint()) {
				_merge_meshes();
			}
		} break;
	}
}

// If this CSGShape is successfully split, all children are added to the first sibling (as the transform
// relationship should be preserved) and the source CSGShape is queue deleted.
// The children of a CSGCombiner are also treated specially, as they may not need to be preserved
// after the baking operation.
void MergeGroup::_split_csg_by_surface(CSGShape *p_shape) {
#ifdef MODULE_CSG_ENABLED
	ERR_FAIL_NULL(p_shape);

	// probably a child of a CSG combiner
	if (p_shape->is_queued_for_deletion()) {
		return;
	}

	// Shapes will not be up to date on the first frame due to a quirk
	// of CSG - it defers updates to the next frame. So we need to explicitly
	// force an update to make sure the CSG is correct on level load.
	p_shape->force_update_shape();

	Array arr = p_shape->get_meshes();
	if (!arr.size()) {
		return;
	}

	Ref<ArrayMesh> arr_mesh = arr[1];
	if (!arr_mesh.is_valid()) {
		return;
	}

	int num_surfaces = arr_mesh->get_surface_count();
	if (num_surfaces == 0) {
		return;
	}

	// first create siblings
	Node *parent = p_shape->get_parent();
	ERR_FAIL_NULL(parent);

	// the first new sibling will be the first new child
	int first_sibling_id = parent->get_child_count();

	Vector<Variant> siblings;
	for (int n = 0; n < num_surfaces; n++) {
		MeshInstance *sib = memnew(MeshInstance);
		MergingTool::_reparent(sib, parent, data.scene_root);

		String new_name = String(p_shape->get_name()) + "_surf_" + itos(n);
		sib->set_name(new_name);

		siblings.push_back(sib);
	}

	_node_changed(parent);

	if (!p_shape->split_by_surface(siblings)) {
		return;
	}

	// failed to split
	if (parent->get_child_count() <= first_sibling_id) {
		return;
	}

	Node *first_sibling = parent->get_child(first_sibling_id);
	ERR_FAIL_NULL(first_sibling);

	// special case, do not move CSG children of a CSG combiner
	if (Object::cast_to<CSGCombiner>(p_shape)) {
		for (int n = 0; n < p_shape->get_child_count(); n++) {
			CSGShape *child = Object::cast_to<CSGShape>(p_shape->get_child(n));
			if (child) {
				if (!child->get_child_count()) {
					child->queue_delete();
#ifdef TOOLS_ENABLED
					_log("CSGShape \"" + child->get_name() + "\" child of CSGCombiner detected, deleting.");
#endif
				} else {
					// panic stations .. convert to spatial to preserve the children
					_convert_source_to_spatial(child);
#ifdef TOOLS_ENABLED
					_log("CSGShape \"" + child->get_name() + "\" child of CSGCombiner detected, converting to Spatial.");
#endif
				}
			}
		}
	}

	_move_children(p_shape, first_sibling);

	// remove source
	_delete_node(p_shape);
#endif
}

void MergeGroup::_logt(int p_tabs, String p_string) {
#ifdef TOOLS_ENABLED
	if (p_tabs) {
		String str;
		for (int n = 0; n < p_tabs; n++) {
			str += "\t";
		}
		str += p_string;
		_log(str);
	} else {
		_log(p_string);
	}
#endif
}

void MergeGroup::_log(String p_string) {
#ifdef TOOLS_ENABLED
#ifdef DEV_ENABLED
	print_line(p_string);
#else
	print_verbose(p_string);
#endif
#endif
}

bool MergeGroup::_split_by_locality() {
	LocalVector<MeshInstance *> mis;
	_find_mesh_instances_recursive(0, this, mis, false);

	if (!mis.size()) {
		return true;
	}

	// find the overall AABB
	AABB aabb = mis[0]->get_transformed_aabb();

	for (unsigned int n = 1; n < mis.size(); n++) {
		aabb.merge_with(mis[n]->get_transformed_aabb());
	}

	for (unsigned int n = 0; n < mis.size(); n++) {
		MeshInstance *mi = mis[n];

		Node *parent = mi->get_parent();
		if (!parent) {
			continue;
		}

		if (bake_substep_function) {
			if (bake_substep_function((float)n / mis.size(), mi->get_name(), nullptr, false)) {
				return false;
			}
		}

		// the first new sibling will be the first new child
		int first_sibling_id = parent->get_child_count();

		MergingTool::split_mesh_instance_by_locality(*mi, aabb, data.splits_horizontal, data.splits_vertical, data.min_split_poly_count);

		// failed to split
		if (parent->get_child_count() <= first_sibling_id) {
			continue;
		}

		Node *first_sibling = parent->get_child(first_sibling_id);

		// this really should not happen
		ERR_FAIL_NULL_V(first_sibling, true);

		_move_children(mi, first_sibling);

		// delete source
		_delete_node(mi);
	}

	return true;
}

// If this MeshInstance is successfully split, all children are added to the first sibling (as the transform
// relationship should be preserved) and the source MeshInstance is queue deleted.
void MergeGroup::_split_mesh_by_surface(MeshInstance *p_mi, int p_num_surfaces) {
	ERR_FAIL_COND(p_num_surfaces <= 1);

	// first create siblings
	Node *parent = p_mi->get_parent();
	ERR_FAIL_NULL(parent);

	Vector<Variant> siblings;

	// the first new sibling will be the first new child
	int first_sibling_id = parent->get_child_count();

	for (int n = 0; n < p_num_surfaces; n++) {
		MeshInstance *sib = memnew(MeshInstance);
		MergingTool::_reparent(sib, parent, data.scene_root);

		String new_name = String(p_mi->get_name()) + "_surf_" + itos(n);
		sib->set_name(new_name);
#ifdef TOOLS_ENABLED
		_log("split by surface to " + new_name + ".");
#endif

		siblings.push_back(sib);
	}

	_node_changed(parent);

	p_mi->split_by_surface(siblings);

	// failed to split
	if (parent->get_child_count() <= first_sibling_id) {
		return;
	}

	Node *first_sibling = parent->get_child(first_sibling_id);
	ERR_FAIL_NULL(first_sibling);

	_move_children(p_mi, first_sibling);

	// remove source
	_delete_node(p_mi);
}

void MergeGroup::_node_changed_internal(Node *p_node) {
	// wipe filenames
	if (p_node->get_filename().size()) {
		// don't wipe the filename of the tree root (well actually the child, as the root is a Viewport)
		if (p_node->get_parent() != (Node *)p_node->get_tree()->get_root()) {
#ifdef TOOLS_ENABLED
#if 0
			_log("\tchanging filename of \"" + p_node->get_name() + "\" from " + p_node->get_filename() + " to NULL");
#endif
#endif
			p_node->set_filename("");

			// invalidate any child / grandchild nodes that were owned by this scene,
			// make them owned by the scene root
			MergingTool::_invalidate_owner_recursive(p_node, p_node, data.scene_root);
		}
	}

	// Terminate if we reach the owning MergeGroup, we don't want to "corrupt" scene trees
	// when baking in the editor.
	// Note, this may cause problems if people attempt to merge at runtime then save at runtime
	// above the MergeGroup, because subscenes won't have been invalidated.
	if (p_node == this) {
		return;
	}

	// set owner to the merge group to clear any subscenes from this point upward.
	MergingTool::_set_owner_logged(p_node, data.scene_root);

	Node *parent = p_node->get_parent();
	if (parent) {
		_node_changed_internal(parent);
	}
}

void MergeGroup::_node_changed(Node *p_node) {
	//MergingTool::_invalidate_owner_recursive(p_node, p_node, data._scene_root);
	_node_changed_internal(p_node);
}

void MergeGroup::_move_children(Node *p_from, Node *p_to, bool p_recalculate_transforms) {
	ERR_FAIL_NULL(p_from);
	ERR_FAIL_NULL(p_to);

	// invalidate any child nodes owned by this
	MergingTool::_invalidate_owner_recursive(p_from, p_from, data.scene_root);

	int num_children = p_from->get_child_count();

	// Note these will be readded in reverse order.
	// This is more efficient but users should not rely on this order.
	for (int n = num_children - 1; n >= 0; n--) {
		Node *child = p_from->get_child(n);

		if (p_recalculate_transforms) {
			Spatial *child_spatial = Object::cast_to<Spatial>(child);
			Transform old_global_xform = child_spatial->get_global_transform();
			MergingTool::_reparent(child, p_to, data.scene_root);

			// only set the new transform if it is out, to prevent float error when not needed
			if (!child_spatial->get_global_transform().is_equal_approx(old_global_xform)) {
				child_spatial->set_global_transform(old_global_xform);
			}

		} else {
			MergingTool::_reparent(child, p_to, data.scene_root);
		}

#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint()) {
			MergingTool::append_editor_description(child, "moved from parent", p_from);
		}
#endif
	}

	// subscenes are also invalidated
	_node_changed(p_to);
}

bool MergeGroup::_merge_meshes() {
#ifdef TOOLS_ENABLED
	uint32_t before = OS::get_singleton()->get_ticks_msec();
#endif
	data.iteration = 0;

	data.scene_root = get_owner();
	ERR_FAIL_NULL_V(data.scene_root, false);

	if (data.convert_csgs) {
		if (bake_step_function) {
			if (bake_step_function(0.0, "Converting CSGs", nullptr, true)) {
				return false;
			}
		}
		// first find csgs and convert to meshes
		LocalVector<CSGShape *> csgs;
		_find_csg_recursive(0, this, csgs);

		if (csgs.size()) {
			_log("converting " + itos(csgs.size()) + " CSGShapes to MeshInstances.");
		}

		for (unsigned int n = 0; n < csgs.size(); n++) {
			CSGShape *csg = csgs[n];
			_split_csg_by_surface(csg);
		}
	}

	if (data.convert_gridmaps) {
		if (bake_step_function) {
			if (bake_step_function(1.0 / 8, "Converting GridMaps", nullptr, true)) {
				return false;
			}
		}
		// first find gridmaps and convert to meshes
		LocalVector<GridMap *> gridmaps;
		_find_gridmap_recursive(0, this, gridmaps);

		if (gridmaps.size()) {
			_log("converting " + itos(gridmaps.size()) + " GridMaps to MeshInstances.");
		}

		for (unsigned int n = 0; n < gridmaps.size(); n++) {
			GridMap *gridmap = gridmaps[n];
			_bake_gridmap(gridmap);
		}
	}

	// split by surface?
	if (data.split_by_surface) {
		if (bake_step_function) {
			if (bake_step_function(2.0 / 8, "Split by Surface", nullptr, true)) {
				return false;
			}
		}
		_log("split by surface");

		LocalVector<MeshInstance *> split_instances;
		_find_mesh_instances_recursive(0, this, split_instances, false);

		for (unsigned int n = 0; n < split_instances.size(); n++) {
			MeshInstance *mi = split_instances[n];

			if (bake_substep_function) {
				if (bake_substep_function((float)n / split_instances.size(), mi->get_name(), nullptr, false)) {
					return false;
				}
			}

			// should be checked in the find recursive routine
			DEV_ASSERT(mi->get_mesh().is_valid());
			int num_surfs = mi->get_mesh()->get_surface_count();
			if (num_surfs > 1) {
				_split_mesh_by_surface(mi, num_surfs);
			}
		} // for n
	}

	// first create a list of mesh instances
	{
		_log("merging meshes");
		if (bake_step_function) {
			if (bake_step_function(3.0 / 8, "Merging Meshes", nullptr, true)) {
				return false;
			}
		}
		LocalVector<MeshInstance *> mesh_instances;
		_find_mesh_instances_recursive(0, this, mesh_instances, false, true);
		unsigned int orig_num_found = mesh_instances.size();

		while (_merge_similar(mesh_instances, false)) {
			if (bake_substep_function && mesh_instances.size()) {
				if (bake_substep_function((float)(orig_num_found - mesh_instances.size()) / orig_num_found, mesh_instances[0]->get_name(), nullptr, false)) {
					return false;
				}
			}

			data.iteration++;
		}
	}

	if (data.clean_meshes) {
		if (bake_step_function) {
			if (bake_step_function(4.0 / 8, "Cleaning Meshes", nullptr, true)) {
				return false;
			}
		}
		_log("cleaning meshes");
		LocalVector<MeshInstance *> mesh_instances_clean;
		_find_mesh_instances_recursive(0, this, mesh_instances_clean, false);
		for (uint32_t n = 0; n < mesh_instances_clean.size(); n++) {
			if (bake_substep_function) {
				if (bake_substep_function((float)n / mesh_instances_clean.size(), mesh_instances_clean[n]->get_name(), nullptr, false)) {
					return false;
				}
			}

			if (MergingTool::clean_mesh_instance(*mesh_instances_clean[n])) {
				_node_changed(mesh_instances_clean[n]);
			}
		}
	}

	if (data.create_shadow_proxy) {
		if (bake_step_function) {
			if (bake_step_function(5.0 / 8, "Creating Shadow Proxy", nullptr, true)) {
				return false;
			}
		}
		_log("creating shadow proxy");
		LocalVector<MeshInstance *> mesh_instances_shadow;
		_find_mesh_instances_recursive(0, this, mesh_instances_shadow, true);
		unsigned int orig_num_found = mesh_instances_shadow.size();

		while (_merge_similar(mesh_instances_shadow, true)) {
			if (bake_substep_function && mesh_instances_shadow.size()) {
				if (bake_substep_function((float)(orig_num_found - mesh_instances_shadow.size()) / orig_num_found, mesh_instances_shadow[0]->get_name(), nullptr, false)) {
					return false;
				}
			}
		}
	}

	bool split_by_locality = (data.splits_horizontal > 1) || (data.splits_vertical > 1);
	if (split_by_locality) {
		_log("split by locality");
		if (bake_step_function) {
			if (bake_step_function(6.0 / 8, "Split by Locality", nullptr, true)) {
				return false;
			}
		}
		if (!_split_by_locality()) {
			return false;
		}
	}

	if (data.join_meshes) {
		if ((data.max_merges > 1) || (split_by_locality)) {
			WARN_PRINT("Mesh Joining is disabled for MergeGroups with split by locality or max merges.");
		} else {
#ifdef GODOT_MERGING_VERBOSE
			MergingTool::debug_branch(this, "BEFORE JOINING");
#endif
			if (bake_step_function) {
				if (bake_step_function(7.0 / 8, "Joining Meshes", nullptr, true)) {
					return false;
				}
			}
			_log("join meshes");
			LocalVector<MeshInstance *> mesh_instances_join;
			_find_mesh_instances_recursive(0, this, mesh_instances_join, false);
			unsigned int orig_num_found = mesh_instances_join.size();

			while (_join_similar(mesh_instances_join)) {
				if (bake_substep_function && mesh_instances_join.size()) {
					if (bake_substep_function((float)(orig_num_found - mesh_instances_join.size()) / orig_num_found, mesh_instances_join[0]->get_name(), nullptr, false)) {
						return false;
					}
				}
			}
#ifdef GODOT_MERGING_VERBOSE
			MergingTool::debug_branch(this, "AFTER JOINING");
#endif
		}
	}

#ifdef TOOLS_ENABLED
	uint32_t after = OS::get_singleton()->get_ticks_msec();
	print_line("Merging for \"" + get_name() + "\" took " + itos(after - before) + " ms.");
#endif

	return true;
}

bool MergeGroup::_join_similar(LocalVector<MeshInstance *> &r_mis) {
	if (!r_mis.size()) {
		return false;
	}

	LocalVector<MeshInstance *> list;

	MeshInstance *first = nullptr;

	for (int n = 0; n < (int)r_mis.size(); n++) {
		MeshInstance *mi = r_mis[n];

		// is this mesh suitable?
		Ref<Mesh> rmesh = mi->get_mesh();
		if (!rmesh.is_valid()) {
			r_mis.remove_unordered(n);
			n--;
			continue;
		}

		// either the first member of the list, or mergeable with the existing list
		if (!first || MergingTool::is_mergeable_with(*first, *mi, false)) {
			first = mi;
			list.push_back(mi);
			r_mis.remove_unordered(n);
			n--;
		}
	}

	// no joins possible for this mi
	if (list.size() <= 1) {
		return true;
	}

	MeshInstance *joined = memnew(MeshInstance);
	MergingTool::_reparent(joined, this, data.scene_root);
	_node_changed(this);

	// rename
	joined->set_name("Joined [" + first->get_name() + "]");

	// either all of them join, or none
	if (!MergingTool::join_meshes(*joined, list)) {
		// failed to join
		_delete_node(joined);
		return false;
	}

	_cleanup_source_meshes(list);
	// MergingTool::debug_mesh_instance(*joined);

	return true;
}

bool MergeGroup::_merge_similar(LocalVector<MeshInstance *> &r_mis, bool p_shadows) {
	if (!r_mis.size()) {
		return false;
	}

	MeshInstance *first = r_mis[0];

	LocalVector<MeshInstance *> list;
	list.push_back(first);

	r_mis.remove_unordered(0);

	for (int n = 0; n < (int)r_mis.size(); n++) {
		MeshInstance *mi = r_mis[n];

		if (first->is_mergeable_with(mi, p_shadows)) {
			list.push_back(mi);
			r_mis.remove_unordered(n);
			n--;
		}
	}

	// don't whittle for shadows for now
	if (p_shadows) {
		_merge_list(list, p_shadows);
		return true;
	}

	// if set to zero, we merge everything we can..
	// if set to 1, we merge nothing.
	if (data.max_merges <= 1) {
		if (!data.max_merges && (list.size() > 1)) {
			_merge_list(list, p_shadows);
		}
		return true;
	}

	int whittle_group = 0;

	LocalVector<MeshAABB> mesh_aabbs;
	mesh_aabbs.resize(list.size());
	for (uint32_t n = 0; n < list.size(); n++) {
		mesh_aabbs[n].mi = list[n];
		mesh_aabbs[n].aabb = list[n]->get_transformed_aabb();
	}

	_recursive_tree_merge(whittle_group, mesh_aabbs);

	return true;
}

void MergeGroup::_recursive_tree_merge(int &r_whittle_group, LocalVector<MeshAABB> p_list) {
	DEV_ASSERT(data.max_merges > 1);

	// if less than the leaf size, merge
	if (p_list.size() <= data.max_merges) {
		if (p_list.size() > 1) {
			_merge_list_ex(p_list, false, r_whittle_group++);
		}
		return;
	}

	// attempt to split
	// calculate AABB
	AABB aabb = p_list[0].aabb;
	for (uint32_t n = 1; n < p_list.size(); n++) {
		aabb.merge_with(p_list[n].aabb);
	}

	int order[3];

	order[0] = aabb.get_longest_axis_index();
	order[2] = aabb.get_shortest_axis_index();
	order[1] = 3 - (order[0] + order[2]);

	bool sort_ok = false;

	// try sorting on each axis in order of longest first
	for (int n = 0; n < 3; n++) {
		int axis = order[n];
		MeshAABB::_sort_axis = axis;
		p_list.sort();

		// is this sorting ok?
		// some epsilon? NYI
		if (p_list[0].aabb.position.coord[axis] != p_list[p_list.size() - 1].aabb.position.coord[axis]) {
			sort_ok = true;
			break;
		}
	}

	// if the sort failed, they are all in kind of the same place, we will just merge them all
	// and abandon the whittling...
	if (!sort_ok) {
		_merge_list_ex(p_list, false, r_whittle_group++);
		return;
	}

	// sort was ok, lets split into 2 lists and call recursive
	LocalVector<MeshAABB> list_b;
	int last = p_list.size() / 2;
	for (int n = (int)p_list.size() - 1; n >= last; n--) {
		list_b.push_back(p_list[n]);
		p_list.remove_unordered(n);
	}

	_recursive_tree_merge(r_whittle_group, p_list);
	_recursive_tree_merge(r_whittle_group, list_b);
}

void MergeGroup::_merge_list_ex(const LocalVector<MeshAABB> &p_mesh_aabbs, bool p_shadows, int p_whittle_group) {
	LocalVector<MeshInstance *> mis;
	mis.resize(p_mesh_aabbs.size());

	for (uint32_t n = 0; n < mis.size(); n++) {
		mis[n] = p_mesh_aabbs[n].mi;
	}

	_merge_list(mis, p_shadows, p_whittle_group);
}

void MergeGroup::_merge_list(const LocalVector<MeshInstance *> &p_mis, bool p_shadows, int p_whittle_group) {
	MeshInstance *merged = memnew(MeshInstance);
	MergingTool::_reparent(merged, this, data.scene_root);
	_node_changed(this);

	String new_name;
	if (p_shadows) {
		new_name = "Shadow";
	} else {
		new_name = "Merged";
	}
	new_name += " [" + get_name() + "]";
	if (!p_shadows) {
		new_name += " " + itos(data.iteration);
	}
	if (p_whittle_group != -1) {
		new_name += " [wg " + itos(p_whittle_group) + "]";
	}

	merged->set_name(new_name);

	Vector<Variant> varlist;
	for (unsigned int n = 0; n < p_mis.size(); n++) {
		varlist.push_back(Variant(p_mis[n]));
	}

	if (!merged->merge_meshes(varlist, false, false, p_shadows)) {
		_log("MERGE_MESHES failed.");
		_delete_node(merged);
		return;
	}

	if (!p_shadows) {
		// For deleting the old MeshInstances, we should not delete
		// nodes that have children (e.g. Static physics). However MeshInstances
		// that are deleted, can then free up parent MeshInstances for deletion,
		// so we should call this in a recursive fashion.
		LocalVector<MeshInstance *> del_list = p_mis;

		if (data.delete_sources) {
			_cleanup_source_meshes(del_list);
		} // if deleting

		if (data.convert_sources) {
			// Any that have not been deleted can now be converted to spatials
			for (unsigned int n = 0; n < del_list.size(); n++) {
				_convert_source_to_spatial(del_list[n]);
			}
		} else {
			// or have their mesh set to NULL
			for (unsigned int n = 0; n < del_list.size(); n++) {
				_reset_mesh_instance(del_list[n]);
			}
		}

	} // if not shadows
	else {
		// shadows .. turn shadow casting off for all these meshes
		for (unsigned int n = 0; n < p_mis.size(); n++) {
			p_mis[n]->set_cast_shadows_setting(GeometryInstance::ShadowCastingSetting::SHADOW_CASTING_SETTING_OFF);
			_node_changed(p_mis[n]);
		}
	}
}

void MergeGroup::_cleanup_source_meshes(LocalVector<MeshInstance *> &r_cleanup_list) {
	for (unsigned int n = 0; n < r_cleanup_list.size(); n++) {
		MeshInstance *mi = r_cleanup_list[n];
		Node *parent = mi->get_parent();
		_move_children(mi, parent, true);
		_delete_node(mi);
		_delete_dangling_spatials(parent);
	}

	// all are deleted
	r_cleanup_list.clear();
}

void MergeGroup::_delete_node(Node *p_node) {
	p_node->queue_delete();
	// This is only a problem in the editor, Godot saving code cannot currently deal with
	// nodes queued for deletion, so they must be detached.
	// This makes the whole process much slower after detaching, because of the logarithmic
	// calling of notifications (Godot doesn't deal well with large numbers of nodes).
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint() && p_node->get_parent()) {
		p_node->get_parent()->remove_child(p_node);
	}
#endif
}

bool MergeGroup::_node_ok_to_delete(Node *p_node) {
	return (!MergingTool::_node_has_valid_children(p_node)) && (!p_node->get_script_instance());
}

void MergeGroup::_delete_dangling_spatials(Node *p_node) {
	while (p_node) {
		if (_node_ok_to_delete(p_node) && (get_class() == "Spatial")) {
			Node *parent = p_node->get_parent();
			_delete_node(p_node);
			p_node = parent;
		} else {
			return;
		}
	}
}

void MergeGroup::_find_gridmap_recursive(int p_depth, Node *p_node, LocalVector<GridMap *> &r_gridmaps) {
#ifdef MODULE_GRIDMAP_ENABLED
	if (_terminate_search(p_node)) {
		return;
	}

	GridMap *gridmap = Object::cast_to<GridMap>(p_node);
	if (gridmap && !gridmap->is_queued_for_deletion() && !gridmap->get_script_instance()) {
		_logt(p_depth, "found GridMap : \"" + gridmap->get_name() + "\"");
		r_gridmaps.push_back(gridmap);
	}

	for (int c = p_node->get_child_count() - 1; c >= 0; c--) {
		_find_gridmap_recursive(p_depth + 1, p_node->get_child(c), r_gridmaps);
	}
#endif
}

void MergeGroup::_bake_gridmap(GridMap *p_gridmap) {
#ifdef MODULE_GRIDMAP_ENABLED
	Node *parent = p_gridmap->get_parent();
	ERR_FAIL_NULL(parent);

	Array meshes = p_gridmap->get_meshes();

	Transform gridmap_xform = p_gridmap->get_transform();

	for (int n = 0; n < meshes.size(); n++) {
		Transform tr = meshes[n];
		n++;
		Ref<Mesh> rmesh = meshes[n];

		MeshInstance *mi = memnew(MeshInstance);
		MergingTool::_reparent(mi, parent, data.scene_root);

		mi->set_mesh(rmesh);
		mi->set_transform(gridmap_xform * tr);

		String new_name = String(p_gridmap->get_name()) + " [cell " + itos(n / 2) + "]";
		mi->set_name(new_name);
	}

	_node_changed(p_gridmap);

	// ALTERNATIVE IMPLEMENTATION - may be better if we decide to bake physics reps
	//	Array cells = p_gridmap->get_used_cells();
	//	Ref<MeshLibrary> rmeshlib = p_gridmap->get_mesh_library();
	//	Transform gridmap_xform = p_gridmap->get_transform();
	//	real_t cell_scale = p_gridmap->get_cell_scale();

	//	for (int32_t k = 0; k < cells.size(); k++) {
	//		Vector3 cell_location = cells[k];
	//		int x = Math::round(cell_location.x);
	//		int y = Math::round(cell_location.y);
	//		int z = Math::round(cell_location.z);

	//		int32_t cell_item = p_gridmap->get_cell_item(x, y, z);
	//		if (cell_item == GridMap::INVALID_CELL_ITEM) {
	//			continue;
	//		}
	//		Transform cell_xform;
	//		int orientation = p_gridmap->get_cell_item_orientation(x, y, z);
	//		DEV_ASSERT(orientation != -1);
	//		cell_xform.basis.set_orthogonal_index(orientation);
	//		cell_xform.basis.scale(Vector3(cell_scale, cell_scale, cell_scale));
	//		cell_xform.set_origin(p_gridmap->map_to_world(x, y, z));

	//		// may not be required, fire didn't have this
	//		//const Transform &item_xform = rmeshlib->get_item_mesh_transform(cell_item);
	//		//cell_xform *= item_xform;

	//		MeshInstance *mi = memnew(MeshInstance);
	//		parent->add_child(mi);
	//		_set_reasonable_owner(mi);

	//		String new_name = "Gridmap [" + p_gridmap->get_name() + "] " + itos(x) + "," + itos(y) + "," + itos(z);
	//		_log("creating " + new_name);
	//		mi->set_name(new_name);

	//		mi->set_mesh(rmeshlib->get_item_mesh(cell_item));
	//		mi->set_transform(gridmap_xform * cell_xform);
	//	}

	// set to invisible rather than delete to allow physics to work
	p_gridmap->set_visible(false);
#endif
}

// certain node types will terminate finding mesh instances etc, for convenience,
// as they will always be part of a different moving "block" and not suitable
// for static merging
bool MergeGroup::_terminate_search(Node *p_node) {
	//return false;
	if (Object::cast_to<RigidBody>(p_node)) {
		return true;
	}
	if (Object::cast_to<KinematicBody>(p_node)) {
		return true;
	}
	if (Object::cast_to<MergeGroup>(p_node) && (p_node != this)) {
		return true;
	}

	return false;
}

void MergeGroup::_find_csg_recursive(int p_depth, Node *p_node, LocalVector<CSGShape *> &r_csgs) {
#ifdef MODULE_CSG_ENABLED
	if (_terminate_search(p_node)) {
		return;
	}

	CSGShape *shape = Object::cast_to<CSGShape>(p_node);
	if (shape && shape->is_merging_allowed() && !shape->is_queued_for_deletion() && !shape->get_script_instance()) {
		// is this the child of a CSG combiner?
		CSGCombiner *parent = Object::cast_to<CSGCombiner>(shape->get_parent());
		if (parent && parent->is_merging_allowed()) {
			// Do not add children of combiners, as the combiner will use the children to generate
			// the mesh.
			// Possible problem:
			// CSGShape children of CSGCombiners that themselves have children (e.g. static bodies?)
			// What should we do with these?
			_logt(p_depth, "found CSGShape with CSGCombiner parent : \"" + shape->get_name() + "\"");
		} else {
			r_csgs.push_back(shape);
			_logt(p_depth, "found CSGShape : \"" + shape->get_name() + "\"");
		}
	}

	for (int c = p_node->get_child_count() - 1; c >= 0; c--) {
		_find_csg_recursive(p_depth + 1, p_node->get_child(c), r_csgs);
	}
#endif
}

void MergeGroup::_find_mesh_instances_recursive(int p_depth, Node *p_node, LocalVector<MeshInstance *> &r_mis, bool p_shadows, bool p_flag_invalid_meshes) {
	if (_terminate_search(p_node)) {
		return;
	}

	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);

	if (mi && mi->is_merging_allowed() && !mi->is_queued_for_deletion() && !mi->get_script_instance()) {
		Ref<Mesh> rmesh = mi->get_mesh();
		if (rmesh.is_valid()) {
			if (rmesh->get_surface_count()) {
				if (!p_shadows || mi->is_mergeable_with(mi, true)) {
					r_mis.push_back(mi);
					// _logt(p_depth, "found MeshInstance : \"" + mi->get_name() + "\"");
				}
			} // contains surfaces
			else if (p_flag_invalid_meshes) {
#ifdef TOOLS_ENABLED
				WARN_PRINT("MeshInstance \"" + mi->get_name() + "\" contains no surfaces.");
#endif
			}
		} // mesh valid
	}

	// Important:
	// Ensure meshes are added in reverse order.
	// This is important for performance because
	// it turns out queue_delete is very inefficient
	// deleteing multiple child nodes from the front of the list
	// due to ordered_remove() etc.
	for (int c = p_node->get_child_count() - 1; c >= 0; c--) {
		_find_mesh_instances_recursive(p_depth + 1, p_node->get_child(c), r_mis, p_shadows, p_flag_invalid_meshes);
	}
}

void MergeGroup::_reset_mesh_instance(MeshInstance *p_mi) {
	p_mi->set_mesh(Ref<Mesh>());
	_node_changed(p_mi);
}

// Convert any dangling MeshInstances to Spatials so they will be cheaper
// in the VIsualServer. They are only required now for relative positioning
// of children.
// Note that this will go horribly wrong if the user code keeps a
// reference / pointer to the source node before this stage,
// hence why this step is optional.
void MergeGroup::_convert_source_to_spatial(Spatial *p_node) {
	ERR_FAIL_NULL(p_node);
	if (p_node->get_script_instance()) {
		return;
	}

#ifdef TOOLS_ENABLED
	_log("converting source to Spatial \"" + p_node->get_name() + "\"");
#endif

	Node *parent = p_node->get_parent();

	// this should not happen, as sources should always be under a merge group
	ERR_FAIL_NULL(parent);

	// change the name of the node to be deleted
	String string_full_name = p_node->get_name();
	p_node->set_name("_merge_source_ " + string_full_name);

	// create the new class T object
	Spatial *pNew = memnew(Spatial);
	pNew->set_name(string_full_name);

	// add the child at the same position as the old node
	// (this is more convenient for users)
	//parent->add_child_below_node(p_node, pNew);
	MergingTool::_reparent(pNew, parent, data.scene_root);

	// new node should have same transform
	pNew->set_transform(p_node->get_transform());

	// move each child
	_move_children(p_node, pNew);

	// delete old node
	_delete_node(p_node);
}

MergeGroup::Data::Data() {
	auto_merge = true;
	delete_sources = true;
	convert_sources = true;
	convert_csgs = true;
	convert_gridmaps = false;
	create_shadow_proxy = true;
	split_by_surface = true;
	clean_meshes = false;
	iteration = 0;
	max_merges = 0;
	splits_horizontal = 1;
	splits_vertical = 1;
	min_split_poly_count = 1024;
	join_meshes = true;
}
