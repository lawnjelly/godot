/*************************************************************************/
/*  rasterizer_canvas_batcher.h                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#pragma once

#include "core/os/os.h"
#include "core/project_settings.h"
#include "rasterizer_array.h"
#include "rasterizer_storage_common.h"
#include "servers/visual/rasterizer.h"

// We are using the curiously recurring template pattern
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// For static polymorphism.

// This makes it super easy to access
// data / call funcs in the derived rasterizers from the base without writing and
// maintaining a boatload of virtual functions.
// In addition it assures that vtable will not be used and the function calls can be optimized,
// because it gives compile time static polymorphism.

#define PREAMBLE template <class T, typename T_STORAGE>
#define TDECLARE RasterizerCanvasBatcher<T, T_STORAGE>

PREAMBLE
class RasterizerCanvasBatcher {
	/* clang-format off */

	// DON'T CLANG FORMAT THIS .. order of includes is important,
	// and these are not alphabetical

#include "batch_structs.inc"
#include "batch_canvas.inc"
#include "batch_fill.inc"
#include "batch_initialize.inc"
#include "batch_lights.inc"
#include "batch_misc.inc"
#include "batch_render.inc"
#include "batch_sort.inc"
#include "batch_transform.inc"
#include "batch_translate.inc"
#include "batch_try.inc"

	// no need to compile these in in release, they are unneeded outside the editor and only add to executable size
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
#include "batch_diagnose.inc"
#endif

	/* clang-format on */
};

//PREAMBLE
//void TDECLARE::RenderItemState::reset() {

#undef PREAMBLE
#undef TDECLARE
