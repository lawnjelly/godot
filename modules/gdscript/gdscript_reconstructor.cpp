#include "gdscript_reconstructor.h"

const char *GDScriptReconstructor::OPStrings[] = {
	"call",
	"parent_call",
	"yield",
	"is",
	"is_builtin",
	"index",
	"index_named",
	"neg",
	"pos",
	"not",
	"bit_invert",
	"in",
	"==",
	"!=",
	"<",
	"<=",
	">",
	">=",
	"and",
	"or",
	"+",
	"-",
	"*",
	"/",
	"%",
	"<<",
	">>",
	"=",
	"=",
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
	"<<=",
	">>=",
	"&=",
	"|=",
	"~=",
	"&",
	"|",
	"~",
	"?",
	":"
};

bool GDScriptReconstructor::output_control_flow(const GDScriptParser::ControlFlowNode *p_node, Output out) const {
	bool colon = false;
	bool brackets = false;
	String sz;

	switch (p_node->cf_type) {
		default:
			break;
		case GDScriptParser::ControlFlowNode::CF_IF: {
			sz += "if";
			colon = true;
		} break;
		case GDScriptParser::ControlFlowNode::CF_FOR: {
			sz += "for";
			colon = true;
		} break;
		case GDScriptParser::ControlFlowNode::CF_WHILE: {
			sz += "while";
			colon = true;
			brackets = true;
		} break;
		case GDScriptParser::ControlFlowNode::CF_BREAK: {
			sz += "break";
		} break;
		case GDScriptParser::ControlFlowNode::CF_CONTINUE: {
			sz += "continue";
		} break;
		case GDScriptParser::ControlFlowNode::CF_RETURN: {
			sz += "return";
		} break;
		case GDScriptParser::ControlFlowNode::CF_MATCH: {
			sz += "match";
			colon = true;
		} break;
	}

	if (brackets) {
		sz += " (";
	} else {
		sz += " ";
	}
	for (int n = 0; n < p_node->arguments.size(); n++) {
		if (n != 0) {
			sz += " ";
		}
		output_node(p_node->arguments[n], Output(out.indent, &sz));
		//r_text += " ";
	}
	if (brackets) {
		sz += ")";
	}

	if (colon) {
		sz += ":";
	}

	if (p_node->body) {
		output_block(p_node->body, Output(out.indent + 1, &sz));
	}

	if (p_node->body_else) {
		sz += "\n";
		output_indent(Output(out.indent, &sz), true);

		// Detect else if.
		String else_body;
		output_block(p_node->body_else, Output(out.indent + 1, &else_body));

		if (else_body.begins_with("if ")) {
			sz += "else ";
			else_body = "";
			data.output_tab_required = false;
			output_block(p_node->body_else, Output(out.indent, &else_body));
		} else {
			sz += "else:";
		}
		sz += else_body;
	}

	*out.text += sz;
	return true;
}

bool GDScriptReconstructor::output_block(const GDScriptParser::BlockNode *p_node, Output out) const {
	for (int n = 0; n < p_node->statements.size(); n++) {
		output_node(p_node->statements[n], out);
	}

	return true;
}

bool GDScriptReconstructor::output_function(const GDScriptParser::FunctionNode *p_node, Output out) const {
	String sz = "func " + p_node->name + "(";

	for (int n = 0; n < p_node->arguments.size(); n++) {
		sz += p_node->arguments[n];
		if (n != p_node->arguments.size() - 1) {
			sz += ", ";
		}
	}

	sz += "):";

	output_block(p_node->body, Output(out.indent + 1, &sz));
	*out.text += sz;
	return true;
}

bool GDScriptReconstructor::output_class(const GDScriptParser::ClassNode *p_node, Output out) const {
	String sz;
	for (int n = 0; n < p_node->extends_class.size(); n++) {
		sz += "extends " + p_node->extends_class[n] + "\n";
	}
	if (p_node->extends_class.size()) {
		sz += "\n";
	}

	bool has_constants = false;
	for (Map<StringName, GDScriptParser::ClassNode::Constant>::Element *E = p_node->constant_expressions.front(); E; E = E->next()) {
		GDScriptParser::ClassNode::Constant &c = E->get();
		output_node(c.expression, Output(out.indent, &sz));
		sz += "\n";
		has_constants = true;
	}
	if (has_constants) {
		sz += "\n";
	}

	for (int n = 0; n < p_node->variables.size(); n++) {
		const GDScriptParser::ClassNode::Member &var = p_node->variables[n];
		sz += "var ";
		output_node(var.initial_assignment, Output(out.indent, &sz));
		sz += "\n";
	}
	if (p_node->variables.size()) {
		sz += "\n";
	}

	for (int n = 0; n < p_node->static_functions.size(); n++) {
		output_function(p_node->static_functions[n], Output(out.indent, &sz));
	}

	for (int n = 0; n < p_node->functions.size(); n++) {
		output_function(p_node->functions[n], Output(out.indent, &sz));
	}

	*out.text += sz;
	return true;
}

bool GDScriptReconstructor::output_operator(const GDScriptParser::OperatorNode *p_node, Output out) const {
	bool binary_operator = true;
	bool commas = false;
	int first_arg = 0;
	bool brackets = false;
	bool prepend_operator = false;

	String sz;

	switch (p_node->op) {
		default: {
		} break;
		case GDScriptParser::OperatorNode::OP_NEG: {
			prepend_operator = true;
		} break;
			//		case GDScriptParser::OperatorNode::OP_INDEX: {
			//			sz += "[";
			//			output_node(p_node->arguments[0], Output(out.indent, &sz));
			//			sz += "]";
			//			*out.text += sz;
			//			return true;
			//		} break;
		case GDScriptParser::OperatorNode::OP_CALL: {
			binary_operator = false;
			commas = true;
			brackets = true;

			switch (p_node->arguments[0]->type) {
				default: {
					output_node(p_node->arguments[0], Output(out.indent, &sz));
					sz += ".";
					output_node(p_node->arguments[1], Output(out.indent, &sz));
					first_arg = 2;
				} break;
				case GDScriptParser::Node::TYPE_TYPE: {
					first_arg = 1;
				} break;
				case GDScriptParser::Node::TYPE_SELF: {
					output_node(p_node->arguments[1], Output(out.indent, &sz));
					first_arg = 2;
				} break;
				case GDScriptParser::Node::TYPE_BUILT_IN_FUNCTION: {
					output_node(p_node->arguments[0], Output(out.indent, &sz));
					first_arg = 1;
				} break;
			}

		} break;
	}

	if (brackets) {
		sz += "(";
	}

	bool close_index = false;

	for (int n = first_arg; n < p_node->arguments.size(); n++) {
		bool is_last = n == (p_node->arguments.size() - 1);

		if (!prepend_operator) {
			output_node(p_node->arguments[n], Output(out.indent, &sz));
		}

		if (binary_operator && (n == 0)) {
			switch (p_node->op) {
				default: {
					sz += String(" ") + OPStrings[p_node->op] + " ";
				} break;
				case GDScriptParser::OperatorNode::Operator::OP_INDEX_NAMED: {
					sz += ".";
				} break;
				case GDScriptParser::OperatorNode::Operator::OP_INDEX: {
					sz += "[";
					close_index = true;
				} break;
				case GDScriptParser::OperatorNode::Operator::OP_NEG: {
					sz += "-";
				} break;
			}
		}

		if (prepend_operator) {
			output_node(p_node->arguments[n], Output(out.indent, &sz));
		}

		if (commas && !is_last) {
			sz += ", ";
		}
	}
	if (close_index) {
		sz += "]";
	}
	if (brackets) {
		sz += ")";
	}

	*out.text += sz;
	return true;
}

bool GDScriptReconstructor::output_node(const GDScriptParser::Node *p_node, Output out) const {
	//ERR_FAIL_NULL_V(p_node, false);
	DEV_ASSERT(p_node);
	output_indent(out);

	String sz;

	switch (p_node->type) {
		default: {
			;
		} break;
		case GDScriptParser::Node::TYPE_TYPE: {
			const GDScriptParser::TypeNode *tp = (const GDScriptParser::TypeNode *)p_node;
			sz += " : " + Variant::get_type_name(tp->vtype);
		} break;
		case GDScriptParser::Node::TYPE_BLOCK: {
			return output_block((const GDScriptParser::BlockNode *)p_node, out);
		} break;
		case GDScriptParser::Node::TYPE_CLASS: {
			return output_class((const GDScriptParser::ClassNode *)p_node, out);
		} break;
		case GDScriptParser::Node::TYPE_IDENTIFIER: {
			const GDScriptParser::IdentifierNode *ident = (const GDScriptParser::IdentifierNode *)p_node;
			//if (!ident->declared_block) {
			sz += ident->name;
			//			} else {
			//				sz += "\"" + ident->name + "\"";
			//			}
		} break;
		case GDScriptParser::Node::TYPE_OPERATOR: {
			return output_operator((const GDScriptParser::OperatorNode *)p_node, out);
		} break;
		case GDScriptParser::Node::TYPE_CONSTANT: {
			const GDScriptParser::ConstantNode *cons = (const GDScriptParser::ConstantNode *)p_node;
			const Variant &val = cons->value;
			switch (val.get_type()) {
				default: {
					sz += String(cons->value);
				} break;
				case Variant::STRING: {
					sz += "\"" + String(cons->value) + "\"";
				} break;
				case Variant::NIL: {
					sz += "null";
				} break;
				case Variant::BOOL: {
					sz += cons->value ? "true" : "false";
				} break;
			}
		} break;
		case GDScriptParser::Node::TYPE_LOCAL_VAR: {
			const GDScriptParser::LocalVarNode *var = (const GDScriptParser::LocalVarNode *)p_node;
			//r_text += "var " + var->name;
			sz += "var ";

			if (var->assign_op) {
				//output_node(var->assign_op, r_text);
			}
			if (var->assign) {
				//output_node(var->assign, r_text);
			}
		} break;
		case GDScriptParser::Node::TYPE_FUNCTION: {
			const GDScriptParser::FunctionNode *func = (const GDScriptParser::FunctionNode *)p_node;
			sz += func->name;
		} break;
		case GDScriptParser::Node::TYPE_BUILT_IN_FUNCTION: {
			const GDScriptParser::BuiltInFunctionNode *func = (const GDScriptParser::BuiltInFunctionNode *)p_node;
			const char *func_name = GDScriptFunctions::get_func_name(func->function);
			sz += func_name;
		} break;
		case GDScriptParser::Node::TYPE_SELF: {
			sz += "self";
		} break;
		case GDScriptParser::Node::TYPE_ASSERT: {
			sz += "assert";
		} break;
		case GDScriptParser::Node::TYPE_NEWLINE: {
			sz += "\n";
			data.output_tab_required = true;
		} break;
		case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
			return output_control_flow((const GDScriptParser::ControlFlowNode *)p_node, out);
		} break;
	}

	*out.text += sz;
	return true;
}

void GDScriptReconstructor::output_indent(const Output &p_out, bool p_force) const {
	if (data.output_tab_required || p_force) {
		data.output_tab_required = false;
		*p_out.text += String("\t").repeat(p_out.indent);
	}
}

bool GDScriptReconstructor::output(GDScriptParser &r_parser, String &r_text) {
	Output out(0, &r_text);
	output_node(r_parser.get_parse_tree(), out);

	return true;
}
