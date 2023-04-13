/**************************************************************************/
/*  merge_group.h                                                         */
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

#ifndef MERGE_GROUP_H
#define MERGE_GROUP_H

#include "spatial.h"

class MeshInstance;
class CSGShape;
class GridMap;

class MergeGroup : public Spatial {
	GDCLASS(MergeGroup, Spatial);
	friend class MergeGroupEditorPlugin;

public:
	void merge_meshes();
	bool merge_meshes_in_editor();

	void set_delete_sources(bool p_enabled) { data.delete_sources = p_enabled; }
	bool get_delete_sources() const { return data.delete_sources; }

	void set_convert_sources(bool p_enabled) { data.convert_sources = p_enabled; }
	bool get_convert_sources() const { return data.convert_sources; }

	void set_convert_csgs(bool p_enabled) { data.convert_csgs = p_enabled; }
	bool get_convert_csgs() const { return data.convert_csgs; }

	void set_convert_gridmaps(bool p_enabled) { data.convert_gridmaps = p_enabled; }
	bool get_convert_gridmaps() const { return data.convert_gridmaps; }

	void set_auto_merge(bool p_enabled) { data.auto_merge = p_enabled; }
	bool get_auto_merge() const { return data.auto_merge; }

	void set_shadow_proxy(bool p_enabled) { data.create_shadow_proxy = p_enabled; }
	bool get_shadow_proxy() const { return data.create_shadow_proxy; }

	void set_clean_meshes(bool p_enabled) { data.clean_meshes = p_enabled; }
	bool get_clean_meshes() const { return data.clean_meshes; }

	void set_join_meshes(bool p_enabled) { data.join_meshes = p_enabled; }
	bool get_join_meshes() const { return data.join_meshes; }

	void set_split_by_surface(bool p_enabled) { data.split_by_surface = p_enabled; }
	bool get_split_by_surface() const { return data.split_by_surface; }

	void set_max_merges(int p_max_merges) { data.max_merges = MAX(0, p_max_merges); }
	int get_max_merges() const { return data.max_merges; }

	void set_splits_horizontal(int p_splits) { data.splits_horizontal = MAX(1, p_splits); }
	int get_splits_horizontal() const { return data.splits_horizontal; }

	void set_splits_vertical(int p_splits) { data.splits_vertical = MAX(1, p_splits); }
	int get_splits_vertical() const { return data.splits_vertical; }

	void set_min_split_poly_count(int p_poly_count) { data.min_split_poly_count = MAX(0, p_poly_count); }
	int get_min_split_poly_count() const { return data.min_split_poly_count; }

	// these enable feedback in the Godot UI as we bake
	typedef bool (*BakeStepFunc)(float, const String &, void *, bool); //progress, step description, userdata, force refresh
	typedef void (*BakeEndFunc)(uint32_t); // time_started

	static BakeStepFunc bake_step_function;
	static BakeStepFunc bake_substep_function;
	static BakeEndFunc bake_end_function;

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	struct MeshAABB {
		MeshInstance *mi = nullptr;
		AABB aabb;
		static int _sort_axis;
		bool operator<(const MeshAABB &p_b) const {
			real_t a_min = aabb.position.coord[_sort_axis];
			real_t b_min = p_b.aabb.position.coord[_sort_axis];
			return a_min < b_min;
		}
	};

	// main function
	bool _merge_meshes();

	// merging
	void _find_mesh_instances_recursive(int p_depth, Node *p_node, LocalVector<MeshInstance *> &r_mis, bool p_shadows, bool p_flag_invalid_meshes = false);
	bool _merge_similar(LocalVector<MeshInstance *> &r_mis, bool p_shadows);
	void _merge_list(const LocalVector<MeshInstance *> &p_mis, bool p_shadows, int p_whittle_group = -1);
	void _merge_list_ex(const LocalVector<MeshAABB> &p_mesh_aabbs, bool p_shadows, int p_whittle_group = -1);
	bool _join_similar(LocalVector<MeshInstance *> &r_mis);
	void _split_mesh_by_surface(MeshInstance *p_mi, int p_num_surfaces);
	bool _split_by_locality();

	// helper funcs
	void _convert_source_to_spatial(Spatial *p_node);
	void _reset_mesh_instance(MeshInstance *p_mi);
	void _move_children(Node *p_from, Node *p_to, bool p_recalculate_transforms = false);
	void _recursive_tree_merge(int &r_whittle_group, LocalVector<MeshAABB> p_list);
	void _delete_node(Node *p_node);
	bool _node_ok_to_delete(Node *p_node);
	void _cleanup_source_meshes(LocalVector<MeshInstance *> &r_cleanup_list);
	void _delete_dangling_spatials(Node *p_node);

	void _node_changed(Node *p_node);
	void _node_changed_internal(Node *p_node);

	// CSG
	void _find_csg_recursive(int p_depth, Node *p_node, LocalVector<CSGShape *> &r_csgs);
	void _split_csg_by_surface(CSGShape *p_shape);

	bool _terminate_search(Node *p_node);

	// Gridmap
	void _find_gridmap_recursive(int p_depth, Node *p_node, LocalVector<GridMap *> &r_gridmaps);
	void _bake_gridmap(GridMap *p_gridmap);

	void _log(String p_string);
	void _logt(int p_tabs, String p_string);

	struct Data {
		bool auto_merge : 1;
		bool delete_sources : 1;
		bool convert_sources : 1;
		bool convert_csgs : 1;
		bool convert_gridmaps : 1;
		bool create_shadow_proxy : 1;
		bool split_by_surface : 1;
		bool clean_meshes : 1;
		bool join_meshes : 1;

		// each merge is an iteration
		uint32_t iteration;

		// we can either merge all suitable Meshes,
		// or a fixed number of "best" merges
		uint32_t max_merges;

		// after merging, we can split meshes by
		// locality in order to get some kind of culling
		// (merging and splitting is a trade off between drawcalls and culling)
		uint32_t splits_horizontal;
		uint32_t splits_vertical;
		uint32_t min_split_poly_count;

		Node *scene_root = nullptr;

		Data();
	} data;
};

#endif // MERGE_GROUP_H
