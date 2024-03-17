/**************************************************************************/
/*  node_path.cpp                                                         */
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

#include "node_path.h"

#include "core/print_string.h"

NodePath::NodePathMap NodePath::_unique_map;

//#define GODOT_NODE_PATH_MAP_VERBOSE

void NodePath::NodePathMap::reref(NodePath::Data &r_data, const String &p_string, uint32_t p_hash) {
	unref(r_data);
	ref(r_data, p_string, p_hash);
}

void NodePath::NodePathMap::ref(NodePath::Data &r_data, const String &p_string, uint32_t p_hash) {
	//	return;
	MutexLock lock(_mutex);

	if (!_initialized) {
		_initialized = true;
		// Make sure zero is never used in the pool.
		// (It is used to indicate no entry.)
		uint32_t dummy;
		_paths.request(dummy)->create();
	}

	DEV_ASSERT(!r_data.unique_map_id);
	DEV_ASSERT(p_string.size());

	// Does it exist already?
	for (uint32_t n = 0; n < _paths.active_size(); n++) {
		uint32_t id = _paths.get_active_id(n);
		UniqueNodePath &path = _paths[id];

		if (id) {
			DEV_ASSERT(path.path_string);

			if ((path.hash == p_hash) && (*path.path_string == p_string)) {
				path.refcount++;
				r_data.unique_map_id = id;
#ifdef GODOT_NODE_PATH_MAP_VERBOSE
				print_line("[" + itos(_paths.active_size()) + "] Referencing upath : " + p_string + " (" + itos(n) + ") " + String::addr(&r_data) + ", refcount " + itos(path.refcount));
#endif
				return;
			}
		}
	}

	// Not found, create.
	UniqueNodePath *upath = _paths.request(r_data.unique_map_id);
	upath->create();
	upath->refcount = 1;
	upath->path_string = memnew(String);
	*upath->path_string = p_string;
	upath->hash = p_hash;

#ifdef GODOT_NODE_PATH_MAP_VERBOSE
	print_line("[" + itos(_paths.active_size()) + "] Adding upath : " + p_string + " (" + itos(r_data.unique_map_id) + ") " + String::addr(&r_data) + ", refcount " + itos(upath->refcount));
#endif
}

void NodePath::NodePathMap::unref(NodePath::Data &r_data) {
	//	return;
	MutexLock lock(_mutex);
	if (r_data.unique_map_id) {
		UniqueNodePath &path = _paths[r_data.unique_map_id];
		DEV_ASSERT(path.refcount);

		path.refcount--;

		if (!path.refcount) {
			DEV_ASSERT(path.path_string);
#ifdef GODOT_NODE_PATH_MAP_VERBOSE
			print_line("[" + itos(_paths.active_size()) + "] Deleting upath : " + *path.path_string + ", refcount " + itos(path.refcount));
#endif
			memdelete(path.path_string);
			path.path_string = nullptr;

			path.refcount = 0;

			_paths.free(r_data.unique_map_id);
		} else {
#ifdef GODOT_NODE_PATH_MAP_VERBOSE
			DEV_ASSERT(path.path_string);
			print_line("[" + itos(_paths.active_size()) + "] Unreferencing upath : " + *path.path_string + ", refcount " + itos(path.refcount));
#endif
		}

		r_data.unique_map_id = 0;
	}
}

void NodePath::_update_hash_cache() const {
	uint32_t h = data->absolute ? 1 : 0;
	int pc = data->path.size();
	const StringName *sn = data->path.ptr();
	for (int i = 0; i < pc; i++) {
		h = h ^ sn[i].hash();
	}
	int spc = data->subpath.size();
	const StringName *ssn = data->subpath.ptr();
	for (int i = 0; i < spc; i++) {
		h = h ^ ssn[i].hash();
	}

	data->hash_cache_valid = true;
	data->hash_cache = h;
}

void NodePath::prepend_period() {
	if (data->path.size() && data->path[0].operator String() != ".") {
		data->path.insert(0, ".");
		data->hash_cache_valid = false;
		_unique_map.reref(*data, (String) * this, hash());
	}
}

bool NodePath::is_absolute() const {
	if (!data) {
		return false;
	}

	return data->absolute;
}
int NodePath::get_name_count() const {
	if (!data) {
		return 0;
	}

	return data->path.size();
}
StringName NodePath::get_name(int p_idx) const {
	ERR_FAIL_COND_V(!data, StringName());
	ERR_FAIL_INDEX_V(p_idx, data->path.size(), StringName());
	return data->path[p_idx];
}

int NodePath::get_subname_count() const {
	if (!data) {
		return 0;
	}

	return data->subpath.size();
}
StringName NodePath::get_subname(int p_idx) const {
	ERR_FAIL_COND_V(!data, StringName());
	ERR_FAIL_INDEX_V(p_idx, data->subpath.size(), StringName());
	return data->subpath[p_idx];
}

void NodePath::unref() {
	if (data && data->refcount.unref()) {
		// Remove from unique map.
		_unique_map.unref(*data);
		memdelete(data);
	}
	data = nullptr;
}

bool NodePath::operator==(const NodePath &p_path) const {
	if (data == p_path.data) {
		return true;
	}

	if (!data || !p_path.data) {
		return false;
	}

	// prediction
	bool same = data->unique_map_id == p_path.data->unique_map_id;
	return same;

	if (data->absolute != p_path.data->absolute) {
		return false;
	}

	int path_size = data->path.size();

	if (path_size != p_path.data->path.size()) {
		return false;
	}

	int subpath_size = data->subpath.size();

	if (subpath_size != p_path.data->subpath.size()) {
		return false;
	}

	const StringName *l_path_ptr = data->path.ptr();
	const StringName *r_path_ptr = p_path.data->path.ptr();

	for (int i = 0; i < path_size; i++) {
		if (l_path_ptr[i] != r_path_ptr[i]) {
			return false;
		}
	}

	const StringName *l_subpath_ptr = data->subpath.ptr();
	const StringName *r_subpath_ptr = p_path.data->subpath.ptr();

	for (int i = 0; i < subpath_size; i++) {
		if (l_subpath_ptr[i] != r_subpath_ptr[i]) {
			return false;
		}
	}

	return true;
}
bool NodePath::operator!=(const NodePath &p_path) const {
	return (!(*this == p_path));
}

void NodePath::operator=(const NodePath &p_path) {
	if (this == &p_path) {
		return;
	}

	unref();

	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

NodePath::operator String() const {
	if (!data) {
		return String();
	}

	String ret;
	if (data->absolute) {
		ret = "/";
	}

	for (int i = 0; i < data->path.size(); i++) {
		if (i > 0) {
			ret += "/";
		}
		ret += data->path[i].operator String();
	}

	for (int i = 0; i < data->subpath.size(); i++) {
		ret += ":" + data->subpath[i].operator String();
	}

	return ret;
}

NodePath::NodePath(const NodePath &p_path) {
	data = nullptr;

	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

Vector<StringName> NodePath::get_names() const {
	if (data) {
		return data->path;
	}
	return Vector<StringName>();
}

Vector<StringName> NodePath::get_subnames() const {
	if (data) {
		return data->subpath;
	}
	return Vector<StringName>();
}

StringName NodePath::get_concatenated_subnames() const {
	ERR_FAIL_COND_V(!data, StringName());

	if (!data->concatenated_subpath) {
		int spc = data->subpath.size();
		String concatenated;
		const StringName *ssn = data->subpath.ptr();
		for (int i = 0; i < spc; i++) {
			concatenated += i == 0 ? ssn[i].operator String() : ":" + ssn[i];
		}
		data->concatenated_subpath = concatenated;
	}
	return data->concatenated_subpath;
}

NodePath NodePath::rel_path_to(const NodePath &p_np) const {
	ERR_FAIL_COND_V(!is_absolute(), NodePath());
	ERR_FAIL_COND_V(!p_np.is_absolute(), NodePath());

	Vector<StringName> src_dirs = get_names();
	Vector<StringName> dst_dirs = p_np.get_names();

	//find common parent
	int common_parent = 0;

	while (true) {
		if (src_dirs.size() == common_parent) {
			break;
		}
		if (dst_dirs.size() == common_parent) {
			break;
		}
		if (src_dirs[common_parent] != dst_dirs[common_parent]) {
			break;
		}
		common_parent++;
	}

	common_parent--;

	Vector<StringName> relpath;
	relpath.resize(src_dirs.size() + dst_dirs.size() + 1);

	StringName *relpath_ptr = relpath.ptrw();

	int path_size = 0;
	StringName back_str("..");
	for (int i = common_parent + 1; i < src_dirs.size(); i++) {
		relpath_ptr[path_size++] = back_str;
	}

	for (int i = common_parent + 1; i < dst_dirs.size(); i++) {
		relpath_ptr[path_size++] = dst_dirs[i];
	}

	if (path_size == 0) {
		relpath_ptr[path_size++] = ".";
	}

	relpath.resize(path_size);

	return NodePath(relpath, p_np.get_subnames(), false);
}

NodePath NodePath::get_as_property_path() const {
	if (!data || !data->path.size()) {
		return *this;
	} else {
		Vector<StringName> new_path = data->subpath;

		String initial_subname = data->path[0];

		for (int i = 1; i < data->path.size(); i++) {
			initial_subname += "/" + data->path[i];
		}
		new_path.insert(0, initial_subname);

		return NodePath(Vector<StringName>(), new_path, false);
	}
}

NodePath::NodePath(const Vector<StringName> &p_path, bool p_absolute) {
	data = nullptr;

	if (p_path.size() == 0) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->has_slashes = true;
	data->hash_cache_valid = false;

	// Add to the unique map.
	_unique_map.ref(*data, (String)(*this), hash());
}

NodePath::NodePath(const Vector<StringName> &p_path, const Vector<StringName> &p_subpath, bool p_absolute) {
	data = nullptr;

	if (p_path.size() == 0 && p_subpath.size() == 0) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->subpath = p_subpath;
	data->has_slashes = true;
	data->hash_cache_valid = false;

	// Add to the unique map.
	_unique_map.ref(*data, (String)(*this), hash());
}

void NodePath::simplify() {
	if (!data) {
		return;
	}
	for (int i = 0; i < data->path.size(); i++) {
		if (data->path.size() == 1) {
			break;
		}
		if (data->path[i].operator String() == ".") {
			data->path.remove(i);
			i--;
		} else if (data->path[i].operator String() == ".." && i > 0 && data->path[i - 1].operator String() != "." && data->path[i - 1].operator String() != "..") {
			//remove both
			data->path.remove(i - 1);
			data->path.remove(i - 1);
			i -= 2;
			if (data->path.size() == 0) {
				data->path.push_back(".");
				break;
			}
		}
	}
	data->hash_cache_valid = false;
	_unique_map.reref(*data, (String) * this, hash());
}

NodePath NodePath::simplified() const {
	NodePath np = *this;
	np.simplify();
	return np;
}

NodePath::NodePath(const String &p_path) {
	data = nullptr;

	if (p_path.length() == 0) {
		return;
	}

	String path = p_path;
	Vector<StringName> subpath;

	bool absolute = (path[0] == '/');
	bool last_is_slash = true;
	bool has_slashes = false;
	int slices = 0;
	int subpath_pos = path.find(":");

	if (subpath_pos != -1) {
		int from = subpath_pos + 1;

		for (int i = from; i <= path.length(); i++) {
			if (path[i] == ':' || path[i] == 0) {
				String str = path.substr(from, i - from);
				if (str == "") {
					if (path[i] == 0) {
						continue; // Allow end-of-path :
					}

					ERR_FAIL_MSG("Invalid NodePath '" + p_path + "'.");
				}
				subpath.push_back(str);

				from = i + 1;
			}
		}

		path = path.substr(0, subpath_pos);
	}

	for (int i = (int)absolute; i < path.length(); i++) {
		if (path[i] == '/') {
			last_is_slash = true;
			has_slashes = true;
		} else {
			if (last_is_slash) {
				slices++;
			}

			last_is_slash = false;
		}
	}

	if (slices == 0 && !absolute && !subpath.size()) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = absolute;
	data->has_slashes = has_slashes;
	data->subpath = subpath;
	data->hash_cache_valid = false;

	if (slices == 0) {
		return;
	}
	data->path.resize(slices);
	last_is_slash = true;
	int from = (int)absolute;
	int slice = 0;

	for (int i = (int)absolute; i < path.length() + 1; i++) {
		if (path[i] == '/' || path[i] == 0) {
			if (!last_is_slash) {
				String name = path.substr(from, i - from);
				ERR_FAIL_INDEX(slice, data->path.size());
				data->path.write[slice++] = name;
			}
			from = i + 1;
			last_is_slash = true;
		} else {
			last_is_slash = false;
		}
	}

	// Add to the unique map.
	_unique_map.ref(*data, (String)(*this), hash());
}

bool NodePath::is_empty() const {
	return !data;
}
NodePath::NodePath() {
	data = nullptr;
}

NodePath::~NodePath() {
	unref();
}
