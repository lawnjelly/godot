/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "laudio_player.h"
#include "lbus.h"
#include "linstrument.h"
#include "lpattern_instance.h"
#include "lsong.h"

void register_laudio_types() {
	ClassDB::register_class<LBus>();
	ClassDB::register_class<LInstrument>();
	ClassDB::register_class<LAudioPlayer>();
	ClassDB::register_class<Pattern>();
	ClassDB::register_class<Song>();

#ifdef TOOLS_ENABLED
	//   EditorPlugins::add_by_type<LLightmapEditorPlugin>();
#endif
}

void unregister_laudio_types() {
	//nothing to do here
}
