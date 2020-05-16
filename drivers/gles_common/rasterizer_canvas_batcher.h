#pragma once

#include "rasterizer_canvas_batcher_base.h"

// We are using the curiously recurring template pattern
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// For static polymorphism.

#define PREAMBLE template <class T, typename T_STORAGE>
#define TDECLARE RasterizerCanvasBatcher<T, T_STORAGE>

PREAMBLE
class RasterizerCanvasBatcher : public RasterizerCanvasBatcherBase<T, T_STORAGE>
{
public:

};


//PREAMBLE
//void TDECLARE::RenderItemState::reset() {



#undef PREAMBLE
#undef TDECLARE
