#pragma once

#include "core/os/os.h"
#include "core/project_settings.h"

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
class RasterizerCanvasBatcher// : public RasterizerCanvasBatcherBase<T, T_STORAGE>
{
#include "batch_initialize.inc"
#include "batch_structs.inc"
#include "batch_sort.inc"
#include "batch_lights.inc"
#include "batch_transform.inc"
#include "batch_misc.inc"
#include "batch_fill.inc"
#include "batch_render.inc"
#include "batch_try.inc"
#include "batch_canvas.inc"

	// no need to compile these in in release, they are unneeded outside the editor and only add to executable size
#ifdef DEBUG_ENABLED
#include "batch_diagnose.inc"
#endif
};


//PREAMBLE
//void TDECLARE::RenderItemState::reset() {



#undef PREAMBLE
#undef TDECLARE
