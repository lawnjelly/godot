#pragma once

#include "gdscript_parser.h"

class GDScriptOptimizer {
	template <class T>
	T *alloc_node() const;

	struct ASTNode {
		GDScriptParser::Node *node = nullptr;
		LocalVector<ASTNode *> children;

		template <typename Func>
		void traverse(Func p_callback, uint32_t p_depth) {
			p_callback(this, p_depth);
			for (uint32_t n = 0; n < children.size(); n++) {
				children[n]->traverse(p_callback, p_depth + 1);
			}
		}

		ASTNode(GDScriptParser::Node *p_node);
		~ASTNode();
	};

	// Call site of a function.
	struct Call {
		GDScriptParser::OperatorNode *call_node = nullptr;
		GDScriptParser::Node *call_parent = nullptr;
		const GDScriptParser::IdentifierNode *call_identifier = nullptr;
		GDScriptParser::BlockNode *parent_block = nullptr;
		GDScriptParser::Node *parent_block_first_child = nullptr;
	};

	struct FunctionParam {
		String name;
		bool is_written = false;
		bool is_used = false;
	};

	// Declaration.
	struct Function {
		GDScriptParser::FunctionNode *node = nullptr;
		LocalVector<FunctionParam> params;

		// If there is a single return value, we can substitute it
		// directly in the calling code.
		// If there are multiple return values, we need an intermediate.
		uint32_t num_return_values = 0;

		uint32_t find_param_by_name(String p_name) const {
			for (uint32_t n = 0; n < params.size(); n++) {
				if (params[n].name == p_name) {
					return n;
				}
			}
			return UINT32_MAX;
		}
	};

	struct InlineChange {
		String identifier_from;
		String identifier_to;
	};

	struct InlineInfo {
		LocalVector<InlineChange> changes;
		GDScriptParser::IdentifierNode *return_intermediate = nullptr;
		GDScriptParser::Node *returned_expression = nullptr;
	};

	class Chain {
		static inline const uint32_t MAX_CHAIN_LENGTH = 8;
		GDScriptParser::Node *nodes[MAX_CHAIN_LENGTH] = {};
		uint32_t _count = 0;

	public:
		bool push_back(GDScriptParser::Node *p_node) {
			if (_count >= MAX_CHAIN_LENGTH) {
				return false;
			}
			nodes[_count++] = p_node;
			return true;
		}
		void clear() { _count = 0; }
		GDScriptParser::BlockNode *find_enclosing_block(GDScriptParser::Node *&r_first_child) const;
		GDScriptParser::Node *find_call_parent() const;
		Chain(GDScriptParser::Node *p_start) { push_back(p_start); }
	};

	struct Data {
		GDScriptParser *parser = nullptr;
		GDScriptParser::ClassNode *root = nullptr;
		uint32_t unique_identifier_count = 0;

		LocalVector<Function> functions;
	} data;

	void search_for_inlines(LocalVector<Call> &r_calls);
	void search_for_inlines(LocalVector<Call> &r_calls, GDScriptParser::FunctionNode *p_func);
	void search_for_inlines(LocalVector<Call> &r_calls, GDScriptParser::Node *p_node, Chain p_chain, GDScriptParser::FunctionNode *p_parent_func);

	GDScriptParser::IdentifierNode *make_inline_declare_local_var(const String &p_local_var_name, GDScriptParser::Node *p_assigned_node, Vector<GDScriptParser::Node *> &r_statements, int &r_insert_statement_id);

	GDScriptParser::OperatorNode *make_inline_declare_assignment(GDScriptParser::IdentifierNode *p_var_name, GDScriptParser::Node *p_assigned_node);

	void make_inline_params(const Call &p_call, const Function &p_source_func, InlineInfo &r_info, int &r_insert_statement_id);
	void make_inline(const Call &p_call);

	GDScriptParser::Node *duplicate_node(const GDScriptParser::Node &p_source) const;
	GDScriptParser::Node *duplicate_node_recursive(const GDScriptParser::Node &p_source, InlineInfo &p_changes);

	bool node_exchange_child(GDScriptParser::Node &r_parent, GDScriptParser::Node *p_old_child, GDScriptParser::Node *p_new_child);
	int find_insert_statement(const Vector<GDScriptParser::Node *> &p_statements, GDScriptParser::Node *p_search_node) const;

	void init_function_declaration(int p_func_id);

public:
	static bool active_inlining;
	Error optimize(GDScriptParser &r_parser);
};
