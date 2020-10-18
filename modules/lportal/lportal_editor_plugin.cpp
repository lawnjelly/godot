#include "lportal_editor_plugin.h"

void LPortalEditorPlugin::_bake() {

	if (m_pRoomManager)
	{
		//LRoomManager->lightmap_bake();
		m_pRoomManager->rooms_convert();

//#ifdef TOOLS_ENABLED
//	// FIXME: Hack to refresh editor in order to display new properties and signals. See if there is a better alternative.
//	if (Engine::get_singleton()->is_editor_hint()) {
//		EditorNode::get_singleton()->get_inspector()->update_tree();
//		//NodeDock::singleton->update_lists();
//	}
//#endif

	}
}

void LPortalEditorPlugin::edit(Object *p_object) {

	LRoomManager *s = Object::cast_to<LRoomManager>(p_object);
	if (!s)
		return;

	m_pRoomManager = s;
}

bool LPortalEditorPlugin::handles(Object *p_object) const {

	return p_object->is_class("LRoomManager");
}

void LPortalEditorPlugin::make_visible(bool p_visible) {

	if (p_visible) {
		bake->show();
	} else {

		bake->hide();
	}
}

EditorProgress *LPortalEditorPlugin::tmp_progress = NULL;

void LPortalEditorPlugin::bake_func_begin(int p_steps) {

	ERR_FAIL_COND(tmp_progress != NULL);

	tmp_progress = memnew(EditorProgress("convert_rooms", TTR("Convert Rooms"), p_steps, true));
}

bool LPortalEditorPlugin::bake_func_step(int p_step, const String &p_description) {

	ERR_FAIL_COND_V(tmp_progress == NULL, false);

	OS::get_singleton()->delay_usec(1000);

	return tmp_progress->step(p_description, p_step, false);
}

void LPortalEditorPlugin::bake_func_end() {
	ERR_FAIL_COND(tmp_progress == NULL);
	memdelete(tmp_progress);
	tmp_progress = NULL;
}

void LPortalEditorPlugin::_bind_methods() {

	ClassDB::bind_method("_bake", &LPortalEditorPlugin::_bake);
}

LPortalEditorPlugin::LPortalEditorPlugin(EditorNode *p_node) {

	editor = p_node;
	bake = memnew(ToolButton);
	bake->set_icon(editor->get_gui_base()->get_icon("Bake", "EditorIcons"));
	bake->set_text(TTR("Convert Rooms"));
	bake->hide();
	bake->connect("pressed", this, "_bake");
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, bake);
	m_pRoomManager = NULL;

//	LightMapper::bake_begin_function = bake_func_begin;
//	LightMapper::bake_step_function = bake_func_step;
//	LightMapper::bake_end_function = bake_func_end;
}

LPortalEditorPlugin::~LPortalEditorPlugin() {
}
