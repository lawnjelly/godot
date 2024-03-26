#pragma once

#include "core/color.h"

namespace LM {

struct ColorSample {
	Color albedo;
	Color emission;
	bool is_opaque = true;
	bool is_emitter = false;
};

} //namespace LM
