#pragma once

#include "gdscript_parser.h"

class GDScriptReconstructor {
	struct Output {
		int indent = 0;
		String *text = nullptr;

		Output(int p_indent, String *p_text) {
			indent = p_indent;
			text = p_text;
		}
	};

	struct Data {
		mutable bool output_tab_required = true;
	} data;

	bool output_node(const GDScriptParser::Node *p_node, Output out) const;
	bool output_class(const GDScriptParser::ClassNode *p_node, Output out) const;
	bool output_function(const GDScriptParser::FunctionNode *p_node, Output out) const;
	bool output_block(const GDScriptParser::BlockNode *p_node, Output out) const;
	bool output_control_flow(const GDScriptParser::ControlFlowNode *p_node, Output out) const;
	bool output_operator(const GDScriptParser::OperatorNode *p_node, Output out) const;
	void output_indent(const Output &p_out, bool p_force = false) const;

	static const char *OPStrings[];

public:
	bool output(GDScriptParser &r_parser, String &r_text);
};
