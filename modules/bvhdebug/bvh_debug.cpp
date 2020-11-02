#include "bvh_debug.h"

#include "core/math/bvh.h"
#include "core/engine.h"

BVH_Manager<void> m_BVH;


void BVHDebug::_bind_methods()
{

}

void BVHDebug::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

			if (!Engine::get_singleton()->is_editor_hint()) {
			set_process(true);
			}

		} break;
	case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				bool bVisible = is_visible_in_tree();
				set_process(bVisible);
			}
		}
			break;
	case NOTIFICATION_PROCESS: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				m_BVH.draw_debug(this);
			}
		} break;
	}
}
