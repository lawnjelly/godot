#include "gdscript_optimizer.h"
#include "gdscript_reconstructor.h"

bool GDScriptOptimizer::active_inlining = true;

template <class T>
T *GDScriptOptimizer::alloc_node() const {
	T *t = memnew(T);

	t->next = data.parser->list;
	data.parser->list = t;

	if (!data.parser->head) {
		data.parser->head = t;
	}

	t->line = 0;
	t->column = 0;
	return t;
}

GDScriptOptimizer::ASTNode::~ASTNode() {
	for (uint32_t n = 0; n < children.size(); n++) {
		memdelete(children[n]);
	}
}

GDScriptOptimizer::ASTNode::ASTNode(GDScriptParser::Node *p_node) {
	node = p_node;

	switch (node->type) {
		default: {
		} break;
		case GDScriptParser::Node::TYPE_FUNCTION: {
			GDScriptParser::FunctionNode *func = (GDScriptParser::FunctionNode *)p_node;
			if (func->body) {
				children.push_back(memnew(ASTNode(func->body)));
			}
		} break;
		case GDScriptParser::Node::TYPE_BLOCK: {
			GDScriptParser::BlockNode *block = (GDScriptParser::BlockNode *)p_node;
			for (int n = 0; n < block->statements.size(); n++) {
				children.push_back(memnew(ASTNode(block->statements[n])));
			}
		} break;
		case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
			const GDScriptParser::ControlFlowNode *cf = (const GDScriptParser::ControlFlowNode *)p_node;
			if (cf->body) {
				children.push_back(memnew(ASTNode(cf->body)));
			}
			if (cf->body_else) {
				children.push_back(memnew(ASTNode(cf->body)));
			}
		} break;
		case GDScriptParser::Node::TYPE_OPERATOR: {
			const GDScriptParser::OperatorNode *op = (const GDScriptParser::OperatorNode *)p_node;
			for (int n = 0; n < op->arguments.size(); n++) {
				children.push_back(memnew(ASTNode(op->arguments[n])));
			}
		}
	}
}

GDScriptParser::Node *GDScriptOptimizer::Chain::find_call_parent() const {
	if (_count < 2) {
		return nullptr;
	}
	return nodes[_count - 2];
}

GDScriptParser::BlockNode *GDScriptOptimizer::Chain::find_enclosing_block(GDScriptParser::Node *&r_first_child) const {
	if (!_count) {
		return nullptr;
	}

	GDScriptParser::Node *prev = nullptr;

	for (int32_t i = _count - 1; i >= 0; i--) {
		GDScriptParser::Node *node = nodes[i];
		if (node->type == GDScriptParser::Node::TYPE_BLOCK) {
			if (prev) {
				GDScriptParser::BlockNode *block = (GDScriptParser::BlockNode *)node;
				r_first_child = prev;
				return block;
			} else {
				return nullptr;
			}
		}

		prev = node;
	}

	return nullptr;
}

void GDScriptOptimizer::search_for_inlines(LocalVector<Call> &r_calls, GDScriptParser::Node *p_node, Chain p_chain, GDScriptParser::FunctionNode *p_parent_func) {
	if (!p_node) {
		return;
	}

	p_chain.push_back(p_node);

	switch (p_node->type) {
		default: {
			return;
		} break;
		case GDScriptParser::Node::TYPE_LOCAL_VAR: {
			GDScriptParser::LocalVarNode *var = (GDScriptParser::LocalVarNode *)p_node;
			search_for_inlines(r_calls, var->assign, p_chain, p_parent_func);
			return;
		} break;
		case GDScriptParser::Node::TYPE_BLOCK: {
			GDScriptParser::BlockNode *block = (GDScriptParser::BlockNode *)p_node;
			Chain chain(p_node);
			for (int n = 0; n < block->statements.size(); n++) {
				search_for_inlines(r_calls, block->statements[n], chain, p_parent_func);
			}
			return;
		} break;
		case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
			const GDScriptParser::ControlFlowNode *cf = (const GDScriptParser::ControlFlowNode *)p_node;
			search_for_inlines(r_calls, cf->body, p_chain, p_parent_func);
			search_for_inlines(r_calls, cf->body_else, p_chain, p_parent_func);
			return;
		} break;
		case GDScriptParser::Node::TYPE_OPERATOR: {
			const GDScriptParser::OperatorNode *op = (const GDScriptParser::OperatorNode *)p_node;
			for (int n = 0; n < op->arguments.size(); n++) {
				search_for_inlines(r_calls, op->arguments[n], p_chain, p_parent_func);
			}

		} break;
	}

	// Only interested in operator calls.
	GDScriptParser::OperatorNode *op = static_cast<GDScriptParser::OperatorNode *>(p_node);
	if (op->op != GDScriptParser::OperatorNode::OP_CALL) {
		return;
	}

	// Only interested in self calls.
	if (op->arguments[0]->type != GDScriptParser::Node::TYPE_SELF) {
		return;
	}
	if (op->arguments[1]->type != GDScriptParser::Node::TYPE_IDENTIFIER) {
		return;
	}

	const GDScriptParser::IdentifierNode *func_identifier = (const GDScriptParser::IdentifierNode *)op->arguments[1];

	String name = func_identifier->name;

	// Don't allow recursive inlines.
	if (name != p_parent_func->name) {
		// Don't allow the same call node to feature in the call list more than once.
		for (uint32_t n = 0; n < r_calls.size(); n++) {
			Call &dcall = r_calls[n];
			if (dcall.call_node == op) {
				// Duplicate.
				dcall.call_parent = p_chain.find_call_parent();
				print_line("Identified duplicate call " + name);
				return;
			}
		}

		Call call;
		call.call_node = op;
		call.parent_block = p_chain.find_enclosing_block(call.parent_block_first_child);
		call.call_parent = p_chain.find_call_parent();
		call.call_identifier = func_identifier;
		print_line("Identified call " + name);

		if (call.parent_block) {
			r_calls.push_back(call);
		}
	}
}

void GDScriptOptimizer::search_for_inlines(LocalVector<Call> &r_calls, GDScriptParser::FunctionNode *p_func) {
	DEV_ASSERT(p_func);
	GDScriptParser::BlockNode *body = p_func->body;
	ERR_FAIL_NULL(body);

	for (int n = 0; n < body->statements.size(); n++) {
		search_for_inlines(r_calls, body->statements[n], body, p_func);
	}
}

void GDScriptOptimizer::search_for_inlines(LocalVector<Call> &r_calls) {
	data.functions.resize(data.root->functions.size());

	for (int n = 0; n < data.root->functions.size(); n++) {
		GDScriptParser::FunctionNode *func_decl = data.root->functions[n];
		data.functions[n].node = func_decl;
		init_function_declaration(n);
	}

	for (int n = 0; n < data.root->functions.size(); n++) {
		GDScriptParser::FunctionNode *func_decl = data.functions[n].node;
		search_for_inlines(r_calls, func_decl);
	}
}

void GDScriptOptimizer::init_function_declaration(int p_func_id) {
	Function &func_decl = data.functions[p_func_id];

	// Build a local AST tree to analyze this function.
	ASTNode *tree = memnew(ASTNode(func_decl.node));

	// Traverse the AST tree using a lambda to extract useful info about the function.
	tree->traverse([&func_decl](ASTNode *p_node, uint32_t p_depth) {
		// print_line("node addr " + String::num_uint64((uint64_t)p_node->node, 16) + ", " + itos(p_node->node->type));

		switch (p_node->node->type) {
			default:
				break;
			case GDScriptParser::Node::TYPE_FUNCTION: {
				GDScriptParser::FunctionNode *func = (GDScriptParser::FunctionNode *)p_node->node;
				for (int n = 0; n < func->arguments.size(); n++) {
					FunctionParam fp;
					fp.name = func->arguments[n];
					func_decl.params.push_back(fp);
				}
			} break;
			case GDScriptParser::Node::TYPE_OPERATOR: {
				const GDScriptParser::OperatorNode *op = (const GDScriptParser::OperatorNode *)p_node->node;
				if (op->arguments.size() && (op->arguments[0]->type == GDScriptParser::OperatorNode::TYPE_IDENTIFIER)) {
					const GDScriptParser::IdentifierNode *ident = (const GDScriptParser::IdentifierNode *)op->arguments[0];
					switch (op->op) {
						default:
							break;

						case GDScriptParser::OperatorNode::OP_INIT_ASSIGN:
						case GDScriptParser::OperatorNode::OP_ASSIGN_ADD:
						case GDScriptParser::OperatorNode::OP_ASSIGN_SUB:
						case GDScriptParser::OperatorNode::OP_ASSIGN_MUL:
						case GDScriptParser::OperatorNode::OP_ASSIGN_DIV:
						case GDScriptParser::OperatorNode::OP_ASSIGN_MOD:
						case GDScriptParser::OperatorNode::OP_ASSIGN_SHIFT_LEFT:
						case GDScriptParser::OperatorNode::OP_ASSIGN_SHIFT_RIGHT:
						case GDScriptParser::OperatorNode::OP_ASSIGN_BIT_AND:
						case GDScriptParser::OperatorNode::OP_ASSIGN_BIT_OR:
						case GDScriptParser::OperatorNode::OP_ASSIGN_BIT_XOR:
						case GDScriptParser::OperatorNode::OP_ASSIGN: {
							uint32_t param_id = func_decl.find_param_by_name(ident->name);
							if (param_id != -1) {
								func_decl.params[param_id].is_written = true;
							}
						} break;
					}
				}

			} break;
			case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
				const GDScriptParser::ControlFlowNode *cf = (const GDScriptParser::ControlFlowNode *)p_node->node;
				if ((cf->cf_type == GDScriptParser::ControlFlowNode::CF_RETURN) && (cf->arguments.size())) {
					// We are using a trick here.
					// If the return statement is within e.g. an If statement,
					// then removing it entirely can cause compile errors,
					// so we will only substitute if the return statement
					// has a depth immediately off of the function root.

					// By setting this to over 1, the later logic will prevent
					// substitution.
					func_decl.num_return_values += p_depth != 2 ? 2 : 1;
				}
			} break;
		}
	},
			0);

	// Finished with the tree.
	memdelete(tree);
}

GDScriptParser::Node *GDScriptOptimizer::duplicate_node_recursive(const GDScriptParser::Node &p_source, InlineInfo &p_changes) {
	// Special treatment for return instructions.

	if (p_source.type == GDScriptParser::Node::TYPE_CONTROL_FLOW) {
		const GDScriptParser::ControlFlowNode *source_cf = (const GDScriptParser::ControlFlowNode *)&p_source;
		if (source_cf->cf_type == GDScriptParser::ControlFlowNode::CF_RETURN) {
			// If there are arguments...
			if (source_cf->arguments.size()) {
				// Make sure the arguments to the return value are duplicated.
				GDScriptParser::Node *returned_expression = duplicate_node_recursive(*source_cf->arguments[0], p_changes);

				if (p_changes.return_intermediate) {
					// If using an intermediate,
					// assign the expression instead of returning it.

					GDScriptParser::OperatorNode *assignment = make_inline_declare_assignment(p_changes.return_intermediate, returned_expression);

					return assignment;
				}

				// Blank out return values if we are passing directly to the calling code.
				if (source_cf->arguments.size() > 0) {
					p_changes.returned_expression = returned_expression;
					return nullptr;
				}
			} // if there are arguments
			else {
				// No arguments, NYI.
				DEV_ASSERT(0);
			}
		}
	}

	GDScriptParser::Node *dest = duplicate_node(p_source);

	// duplicate any children.
	switch (p_source.type) {
		default:
			break;
		case GDScriptParser::Node::TYPE_LOCAL_VAR: {
			const GDScriptParser::LocalVarNode *source_var = (const GDScriptParser::LocalVarNode *)&p_source;
			GDScriptParser::LocalVarNode *dest_var = (GDScriptParser::LocalVarNode *)dest;

			String unique_var_name = "__" + source_var->name + itos(data.unique_identifier_count++);
			dest_var->name = unique_var_name;

			InlineChange change;
			change.identifier_from = source_var->name;
			change.identifier_to = unique_var_name;
			p_changes.changes.push_back(change);

			if (source_var->assign) {
				dest_var->assign = duplicate_node_recursive(*source_var->assign, p_changes);
			}

		} break;
		case GDScriptParser::Node::TYPE_IDENTIFIER: {
			const GDScriptParser::IdentifierNode *source_ident = (const GDScriptParser::IdentifierNode *)&p_source;
			GDScriptParser::IdentifierNode *dest_ident = (GDScriptParser::IdentifierNode *)dest;

			// Make any changes.
			for (uint32_t n = 0; n < p_changes.changes.size(); n++) {
				const InlineChange &change = p_changes.changes[n];
				if (source_ident->name == change.identifier_from) {
					dest_ident->name = change.identifier_to;
				}
			}
		} break;
		case GDScriptParser::Node::TYPE_BLOCK: {
			const GDScriptParser::BlockNode *source_block = (const GDScriptParser::BlockNode *)&p_source;
			GDScriptParser::BlockNode *dest_block = (GDScriptParser::BlockNode *)dest;

			int dest_n = 0;
			for (int n = 0; n < source_block->statements.size(); n++) {
				GDScriptParser::Node *dup = duplicate_node_recursive(*source_block->statements[n], p_changes);
				if (dup) {
					dest_block->statements.set(dest_n, dup);
				} else {
					dest_block->statements.remove(dest_n);
					dest_n--;

					// Predictive remove newline?
					if ((n + 1 < source_block->statements.size()) && (source_block->statements[n + 1]->type == GDScriptParser::Node::TYPE_NEWLINE)) {
						dest_block->statements.remove(dest_n + 1);
						n++;
					}
				}
				dest_n++;
			}
		} break;
		case GDScriptParser::Node::TYPE_OPERATOR: {
			const GDScriptParser::OperatorNode *source_op = (const GDScriptParser::OperatorNode *)&p_source;
			GDScriptParser::OperatorNode *dest_op = (GDScriptParser::OperatorNode *)dest;
			for (int n = 0; n < source_op->arguments.size(); n++) {
				dest_op->arguments.set(n, duplicate_node_recursive(*source_op->arguments[n], p_changes));
			}
		} break;
		case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
			const GDScriptParser::ControlFlowNode *source_cf = (const GDScriptParser::ControlFlowNode *)&p_source;
			GDScriptParser::ControlFlowNode *dest_cf = (GDScriptParser::ControlFlowNode *)dest;
			if (source_cf->body) {
				dest_cf->body = (GDScriptParser::BlockNode *)duplicate_node_recursive(*source_cf->body, p_changes);
			}
			if (source_cf->body_else) {
				dest_cf->body_else = (GDScriptParser::BlockNode *)duplicate_node_recursive(*source_cf->body_else, p_changes);
			}
		} break;
	}

	return dest;
}

GDScriptParser::Node *GDScriptOptimizer::duplicate_node(const GDScriptParser::Node &p_source) const {
	GDScriptParser::Node *res = nullptr;

	GDScriptParser::Node *source_next = p_source.next;

#define GDSCRIPTOPTIMIZER_COPY_CASE(TYPE, CLASS)                                                     \
	case GDScriptParser::Node::TYPE: {                                                               \
		const GDScriptParser::CLASS *source = static_cast<const GDScriptParser::CLASS *>(&p_source); \
		GDScriptParser::CLASS *node = alloc_node<GDScriptParser::CLASS>();                           \
		*node = *source;                                                                             \
		node->next = source_next;                                                                    \
		res = node;                                                                                  \
	} break;

	switch (p_source.type) {
		default: {
			DEV_ASSERT(0);
		} break;
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_CLASS, ClassNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_FUNCTION, FunctionNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_BUILT_IN_FUNCTION, BuiltInFunctionNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_BLOCK, BlockNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_IDENTIFIER, IdentifierNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_TYPE, TypeNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_CONSTANT, ConstantNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_ARRAY, ArrayNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_DICTIONARY, DictionaryNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_SELF, SelfNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_OPERATOR, OperatorNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_CONTROL_FLOW, ControlFlowNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_LOCAL_VAR, LocalVarNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_CAST, CastNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_ASSERT, AssertNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_BREAKPOINT, BreakpointNode)
			GDSCRIPTOPTIMIZER_COPY_CASE(TYPE_NEWLINE, NewLineNode)
	}

	return res;
}

void GDScriptOptimizer::make_inline_params(const Call &p_call, const Function &p_source_func, InlineInfo &r_info, int &r_insert_statement_id) {
	// Some local aliases.
	Vector<GDScriptParser::Node *> &statements = p_call.parent_block->statements;
	GDScriptParser::FunctionNode *source_node = p_source_func.node;

	// If there is more than 1 return value, we need to create a return value intermediate local var to store these in.
	if (p_source_func.num_return_values > 1) // 1
	{
		String unique_return_name = "__return" + itos(data.unique_identifier_count++);
		r_info.return_intermediate = make_inline_declare_local_var(unique_return_name, nullptr, statements, r_insert_statement_id);
		//statements.insert(r_insert_statement_id++, alloc_node<GDScriptParser::NewLineNode>());
	}

	// A change for each argument.
	r_info.changes.resize(source_node->arguments.size());
	DEV_ASSERT(r_info.changes.size() == p_source_func.params.size());

	// Parameters.
	for (uint32_t p = 0; p < source_node->arguments.size(); p++) {
		const StringName &source_param_name = source_node->arguments[p];

		// Find the passed argument from the call node.
		DEV_ASSERT(p_call.call_node->arguments.size() == (source_node->arguments.size() + 2));
		GDScriptParser::Node *passed_argument = p_call.call_node->arguments[p + 2];

		const FunctionParam &function_param = p_source_func.params[p];

		// If the argument is not written, we can directly use it in the inlined function body,
		// but ONLY if it is an identifier (otherwise it may be a complex expression, and we are better off
		// evaluating it once as a local variable.
		bool substitute_directly = !function_param.is_written;
		if (passed_argument->type != GDScriptParser::Node::TYPE_IDENTIFIER) {
			substitute_directly = false;
		}

		// Create a change for each argument.
		InlineChange &change = r_info.changes[p];
		change.identifier_from = source_param_name;

		if (substitute_directly) {
			// If the argument is not written, we can directly use it in the inlined function body,
			// but ONLY if it is an identifier (otherwise it may be a complex expression, and we are better off
			// evaluating it once as a local variable.
			change.identifier_to = ((GDScriptParser::IdentifierNode *)passed_argument)->name;
		} else {
			String unique_param_name = "__" + source_param_name + itos(data.unique_identifier_count++);
			change.identifier_to = unique_param_name;

			make_inline_declare_local_var(unique_param_name, passed_argument, statements, r_insert_statement_id);
		}
	}
}

// returns the local var name identifier.
GDScriptParser::IdentifierNode *GDScriptOptimizer::make_inline_declare_local_var(const String &p_local_var_name, GDScriptParser::Node *p_assigned_node, Vector<GDScriptParser::Node *> &r_statements, int &r_insert_statement_id) {
	// Write out the variable declaration statement.
	GDScriptParser::LocalVarNode *local_var = alloc_node<GDScriptParser::LocalVarNode>();
	local_var->name = p_local_var_name;

	GDScriptParser::IdentifierNode *param_identifier_node = alloc_node<GDScriptParser::IdentifierNode>();
	param_identifier_node->name = p_local_var_name;

	r_statements.insert(r_insert_statement_id++, local_var);

	local_var->assign_op = make_inline_declare_assignment(param_identifier_node, p_assigned_node);

	local_var->assign = local_var->assign_op->arguments[1];
	r_statements.insert(r_insert_statement_id++, local_var->assign_op);
	r_statements.insert(r_insert_statement_id++, alloc_node<GDScriptParser::NewLineNode>());

	return param_identifier_node;
}

GDScriptParser::OperatorNode *GDScriptOptimizer::make_inline_declare_assignment(GDScriptParser::IdentifierNode *p_var_name, GDScriptParser::Node *p_assigned_node) {
	// Assign constant nil if not specified.
	if (!p_assigned_node) {
		p_assigned_node = alloc_node<GDScriptParser::ConstantNode>();
	}

	// Assign
	GDScriptParser::OperatorNode *assign_op = alloc_node<GDScriptParser::OperatorNode>();
	assign_op->op = GDScriptParser::OperatorNode::OP_ASSIGN;
	assign_op->arguments.push_back(p_var_name);
	assign_op->arguments.push_back(p_assigned_node);

	return assign_op;
}

int GDScriptOptimizer::find_insert_statement(const Vector<GDScriptParser::Node *> &p_statements, GDScriptParser::Node *p_search_node) const {
	//	int last_statement = -1;

	for (int n = 0; n < p_statements.size(); n++) {
		GDScriptParser::Node *s = p_statements[n];

		//		switch (s->type) {
		//			default: {
		//			} break;
		//			case GDScriptParser::Node::TYPE_LOCAL_VAR:
		//			{
		//			}
		//			case GDScriptParser::Node::TYPE_OPERATOR: {
		//				last_statement = n;
		//			} break;
		//		}

		if (s == p_search_node) {
			return n;
		}
	}

	return -1;
}

void GDScriptOptimizer::make_inline(const Call &p_call) {
	// Find which function is being inlined.
	uint32_t inlined_func_id = UINT32_MAX;

	for (uint32_t n = 0; n < data.functions.size(); n++) {
		if (data.functions[n].node->name == p_call.call_identifier->name) {
			inlined_func_id = n;
			break;
		}
	}
	ERR_FAIL_COND(inlined_func_id == UINT32_MAX);

	const Function &func = data.functions[inlined_func_id];
	GDScriptParser::FunctionNode *source_node = func.node;
	ERR_FAIL_NULL(source_node);

	print_line("Inlining function " + source_node->name);

	GDScriptParser::BlockNode *source_body = source_node->body;
	ERR_FAIL_NULL(source_body);

	//	int64_t insert_statement_id64 = p_call.parent_block->statements.find(p_call.parent_block_first_child);
	//	ERR_FAIL_COND(insert_statement_id64 == -1);
	//	int insert_statement_id = insert_statement_id64;
	int insert_statement_id = find_insert_statement(p_call.parent_block->statements, p_call.parent_block_first_child);
	ERR_FAIL_COND(insert_statement_id == -1);

	InlineInfo info;
	make_inline_params(p_call, func, info, insert_statement_id);

	bool ignore_newline = false;

	for (uint32_t n = 0; n < source_body->statements.size(); n++) {
		GDScriptParser::Node *statement = source_body->statements[n];
		ERR_FAIL_NULL(statement);

		// Ignore any newlines after deleted statements.
		if (ignore_newline && statement->type == GDScriptParser::Node::TYPE_NEWLINE) {
			ignore_newline = false;
			continue;
		}

		// Make a copy of the statement.
		GDScriptParser::Node *dest_statement = duplicate_node_recursive(*statement, info);

		if (dest_statement) {
			// Add the statement to the call site.
			p_call.parent_block->statements.insert(insert_statement_id++, dest_statement);
		} else {
			ignore_newline = true;
		}
	}

	// Finally change the call expression itself to be the return expression or return identifier..
	// or remove completely if there is no return value.
	if (info.returned_expression) {
		node_exchange_child(*p_call.call_parent, p_call.call_node, info.returned_expression);
	} else if (info.return_intermediate) {
		node_exchange_child(*p_call.call_parent, p_call.call_node, info.return_intermediate);
	} else {
		node_exchange_child(*p_call.call_parent, p_call.call_node, nullptr);
	}
}

bool GDScriptOptimizer::node_exchange_child(GDScriptParser::Node &r_parent, GDScriptParser::Node *p_old_child, GDScriptParser::Node *p_new_child) {
	// This is so complex because Nodes don't have generic children.
	// *sigh*.
	switch (r_parent.type) {
		default: {
			return false;
		} break;

		case GDScriptParser::Node::TYPE_BLOCK: {
			GDScriptParser::BlockNode *block = (GDScriptParser::BlockNode *)&r_parent;
			for (int n = 0; n < block->statements.size(); n++) {
				if (block->statements[n] == p_old_child) {
					block->statements.set(n, p_new_child);
					return true;
				}
			}
		} break;
			//		case GDScriptParser::Node::TYPE_CONTROL_FLOW: {
			//			GDScriptParser::ControlFlowNode *cf = (GDScriptParser::ControlFlowNode *)&r_parent;
			//			if (cf->body == p_old_child) {
			//				cf->body = p_new_child;
			//				return true;
			//			}
			//			if (cf->body_else == p_old_child) {
			//				cf->body_else = p_new_child;
			//				return true;
			//			}
			//		} break;
		case GDScriptParser::Node::TYPE_OPERATOR: {
			GDScriptParser::OperatorNode *op = (GDScriptParser::OperatorNode *)&r_parent;
			for (int n = 0; n < op->arguments.size(); n++) {
				if (op->arguments[n] == p_old_child) {
					op->arguments.set(n, p_new_child);
					return true;
				}
			}

		} break;
	}

	return false;
}

Error GDScriptOptimizer::optimize(GDScriptParser &r_parser) {
	if (!active_inlining) {
		return OK;
	}

	GDScriptParser::Node *root = r_parser.head;
	ERR_FAIL_COND_V(root->type != GDScriptParser::Node::TYPE_CLASS, ERR_INVALID_DATA);

	data.parser = &r_parser;
	data.root = static_cast<GDScriptParser::ClassNode *>(root);

	GDScriptReconstructor rc;
	String text;

	//	rc.output(r_parser, text);
	//	print_line(text);
	//return OK;

	LocalVector<Call> calls;
	search_for_inlines(calls);

	for (uint32_t n = 0; n < calls.size(); n++) {
		make_inline(calls[n]);
	}

	rc.output(r_parser, text);
	print_line(text);

	return OK;
}
