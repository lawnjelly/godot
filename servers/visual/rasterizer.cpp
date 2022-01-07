/*************************************************************************/
/*  rasterizer.cpp                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "rasterizer.h"

#include "core/os/os.h"
#include "core/print_string.h"

Rasterizer *(*Rasterizer::_create_func)() = nullptr;

Rasterizer *Rasterizer::create() {
	return _create_func();
}

RasterizerStorage *RasterizerStorage::base_singleton = nullptr;

RasterizerStorage::RasterizerStorage() {
	base_singleton = this;
}

bool RasterizerStorage::material_uses_tangents(RID p_material) {
	return false;
}

bool RasterizerStorage::material_uses_ensure_correct_normals(RID p_material) {
	return false;
}

RID RasterizerStorage::multimesh_create() {
	return _multimesh_create();
}

void RasterizerStorage::multimesh_allocate(RID p_multimesh, int p_instances, VS::MultimeshTransformFormat p_transform_format, VS::MultimeshColorFormat p_color_format, VS::MultimeshCustomDataFormat p_data) {
	return _multimesh_allocate(p_multimesh, p_instances, p_transform_format, p_color_format, p_data);
}

int RasterizerStorage::multimesh_get_instance_count(RID p_multimesh) const {
	return _multimesh_get_instance_count(p_multimesh);
}

void RasterizerStorage::multimesh_set_mesh(RID p_multimesh, RID p_mesh) {
	_multimesh_set_mesh(p_multimesh, p_mesh);
}

void RasterizerStorage::multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) {
	_multimesh_instance_set_transform(p_multimesh, p_index, p_transform);
}

void RasterizerStorage::multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) {
	_multimesh_instance_set_transform_2d(p_multimesh, p_index, p_transform);
}

void RasterizerStorage::multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) {
	_multimesh_instance_set_color(p_multimesh, p_index, p_color);
}
void RasterizerStorage::multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) {
	_multimesh_instance_set_custom_data(p_multimesh, p_index, p_color);
}

RID RasterizerStorage::multimesh_get_mesh(RID p_multimesh) const {
	return _multimesh_get_mesh(p_multimesh);
}

Transform RasterizerStorage::multimesh_instance_get_transform(RID p_multimesh, int p_index) const {
	return _multimesh_instance_get_transform(p_multimesh, p_index);
}

Transform2D RasterizerStorage::multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const {
	return _multimesh_instance_get_transform_2d(p_multimesh, p_index);
}

Color RasterizerStorage::multimesh_instance_get_color(RID p_multimesh, int p_index) const {
	return _multimesh_instance_get_color(p_multimesh, p_index);
}

Color RasterizerStorage::multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const {
	return _multimesh_instance_get_custom_data(p_multimesh, p_index);
}

void RasterizerStorage::multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) {
	_multimesh_set_as_bulk_array(p_multimesh, p_array);
}

void RasterizerStorage::multimesh_set_visible_instances(RID p_multimesh, int p_visible) {
	_multimesh_set_visible_instances(p_multimesh, p_visible);
}

int RasterizerStorage::multimesh_get_visible_instances(RID p_multimesh) const {
	return _multimesh_get_visible_instances(p_multimesh);
}

AABB RasterizerStorage::multimesh_get_aabb(RID p_multimesh) const {
	return _multimesh_get_aabb(p_multimesh);
}
