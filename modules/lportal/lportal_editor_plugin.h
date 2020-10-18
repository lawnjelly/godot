
#pragma once

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "lroom_manager.h"
#include "scene/resources/material.h"

class LPortalEditorPlugin : public EditorPlugin {

	GDCLASS(LPortalEditorPlugin, EditorPlugin);

	LRoomManager *m_pRoomManager;

	ToolButton *bake;
	EditorNode *editor;

	static EditorProgress *tmp_progress;
	static void bake_func_begin(int p_steps);
	static bool bake_func_step(int p_step, const String &p_description);
	static void bake_func_end();

	void _bake();

protected:
	static void _bind_methods();

public:
	virtual String get_name() const { return "LPortal"; }
	bool has_main_screen() const { return false; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

	LPortalEditorPlugin(EditorNode *p_node);
	~LPortalEditorPlugin();
};

