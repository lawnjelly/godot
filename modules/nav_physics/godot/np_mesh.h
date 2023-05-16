#pragma once

#include "core/resource.h"

class NPMesh : public Resource {
	GDCLASS(NPMesh, Resource);
	OBJ_SAVE_TYPE(NPMesh);
	RES_BASE_EXTENSION("npmesh");
};
