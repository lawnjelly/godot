#include "gdscript_pre_processor.h"
#include "core/error_macros.h"

uint32_t GDScriptPreProcessor::Data::current_token = 0;

const char *GDScriptPreProcessor::ASTStrings[] = {
	"unknw",
	"block",
	"group",
	"d_fnc",
	"d_pms",
	"d_var",
	"exprs",
	"extend",
	"if",
	"for",
	"pass",
	"return",
	"break",
	"cont",
	"match",
	"print",
	"lit",
	"var",
	"e_bin",
	"e_una",
	"assign",
	"call",
	"inline",
	"param",
	"memb",
	"tern",
	"self",
	"await",
	"yield",
	"int",
	"float",
	"str",
	"bool",
	"null",
	"array",
	"$node",
	"ident",
	"case",
	"default"
};

const char *GDScriptPreProcessor::TokenStrings[] = {
	"UNK",
	"STR",
	"INT",
	"FLT",
	"FNC",
	"CMP",
	"ASG",
	"KEY",
	"EXT",
	"VAR",
	"BOP",
	"BCL",
	"COL",
	"COM",
	"IDN",
	//"IDENTIFIER_FUNC",
	//"IDENTIFIER_VAR",
	"AND",
	"OR",
	"NOT",

	"MTH",

	"IF",
	"ELS",
	"FOR",
	"WHL",
	"BRK",
	"CNT",
	"PAS",
	"RTN",
	"MAT",
	"IS",
	"CST",
	"ASS",
	"YIE",
	"SIG",
	"IN",
	"RGE",
	"PRT",

};

GDScriptPreProcessor::Variable &GDScriptPreProcessor::Variables::get_var(TokenClassID p_class_id) {
	uint32_t id;
	if (!find(p_class_id, id)) {
		CRASH_NOW_MSG("Error, variable not found.");
	}
	return get(id);
}

bool GDScriptPreProcessor::Variables::find(TokenClassID p_class_id, uint32_t &r_var_id) const {
	for (uint32_t n = 0; n < list.size(); n++) {
		if (list[n].token_class_id == p_class_id) {
			r_var_id = n;
			return true;
		}
	}
	return false;
}

bool GDScriptPreProcessor::Variables::add_variable(TokenClassID p_class_id, uint32_t p_parameter_id) {
	if (!already_exists(p_class_id)) {
		Variable v;
		v.token_class_id = p_class_id;
		v.parameter_id = p_parameter_id;
		//v.is_parameter = p_is_parameter;
		list.push_back(v);
		return true;
	}
	return false;
}

bool GDScriptPreProcessor::Data::parse_advance() {
	if (current_token < tokens.size()) {
		current_token++;
		return true;
	}
	return false;
}

uint32_t GDScriptPreProcessor::Data::find_function(String p_func_name) {
	for (uint32_t n = 0; n < funcs.size(); n++) {
		if (funcs[n].name == p_func_name) {
			return n;
		}
	}

	return UINT32_MAX;
}

GDScriptPreProcessor::Token GDScriptPreProcessor::add_predefined_token(const TokenLocation &p_token_location, TokenType p_tt) {
	TokenClass t;
	t.type = p_tt;
	return add_token(p_token_location, t);
}

String GDScriptPreProcessor::get_token_as_string(const Token &p_token) {
	String sz = get_token_class_as_string(data.token_classes[p_token.class_id.id()]);
	//sz += String(" { vl ") + itos(p_token.location.valid_line) + " } ";
	return sz;
}

String GDScriptPreProcessor::get_token_class_as_string(const TokenClass &p_class) {
	// Short string.
	String sz_class = String(TokenStrings[(int)p_class.type]);
	//	if (sz_class.length() > 4) {
	//		sz_class = sz_class.substr(0, 4);
	//	} else {
	//		uint32_t extra = 4 - sz_class.length();
	//		for (uint32_t n = 0; n < extra; n++) {
	//			sz_class += " ";
	//		}
	//	}

	//	String sz = String(TokenStrings[(int)p_class.type]);
	String sz = sz_class.to_lower();
	if (p_class.text.length()) {
		sz += " \"" + p_class.text + "\"";
	}
	return sz;
}

GDScriptPreProcessor::Token GDScriptPreProcessor::add_token(const TokenLocation &p_token_location, const TokenClass &p_token) {
	String sz = String("\t\t") + get_token_class_as_string(p_token);
	print_line(sz);

	int64_t token_id = data.token_classes.find(p_token);
	if (token_id == -1) {
		token_id = data.token_classes.size();
		data.token_classes.push_back(p_token);
	}

	Token ti;
	ti.class_id = token_id;
	ti.location = p_token_location;

	return ti;
}

bool GDScriptPreProcessor::lex_token_string(const TokenLocation &p_token_location, const String &p_text, int32_t &r_pos, int32_t p_length) {
	TokenClass t;
	t.type = TT_STRING;

	int32_t found = p_text.find_char('"', r_pos);
	if (found == -1) {
		return false;
	}

	t.text = p_text.substr(r_pos, found - r_pos);
	data.tokens.push_back(add_token(p_token_location, t));

	r_pos = found + 1;
	return true;
}

bool GDScriptPreProcessor::_is_delimiter(CharType c, String p_delimiters) const {
	int32_t l = p_delimiters.length();
	for (int32_t n = 0; n < l; n++) {
		if (c == p_delimiters[n]) {
			return true;
		}
	}

	return false;
}

bool GDScriptPreProcessor::_is_num(CharType c) const {
	return (c >= '0' && c <= '9');
}

bool GDScriptPreProcessor::_is_char(CharType c) const {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool GDScriptPreProcessor::lex_identifier(const TokenLocation &p_token_location, const String &p_text, int32_t &r_pos, int32_t p_length) {
	TokenClass t;
	t.type = TT_IDENTIFIER;

	while (r_pos < p_length) {
		CharType c = p_text[r_pos++];

		if (_is_char(c)) {
			t.text += c;
		} else if (_is_num(c)) {
			// Disallow starting identifier with number.
			if (t.text.length() == 0) {
				r_pos--;
				return false;
			}
			t.text += c;
		} else {
			r_pos--;
			break;
		}
	}

	if (t.text.length()) {
		data.tokens.push_back(add_token(p_token_location, t));
		return true;
	}

	return false;
}

bool GDScriptPreProcessor::lex_predefined_token(const TokenLocation &p_token_location, int32_t &r_pos, const String &p_text, const String &p_searchstring, TokenType p_type, bool p_store_search_string) {
	if (p_text.find(p_searchstring, r_pos) == r_pos) {
		TokenClass t;
		t.type = p_type;
		if (p_store_search_string) {
			t.text = p_searchstring;
		}
		data.tokens.push_back(add_token(p_token_location, t));
		//data.tokens.push_back(add_predefined_token(p_token_location, p_type));
		r_pos += p_searchstring.length();
		return true;
	}
	return false;
}

bool GDScriptPreProcessor::lex_predefined_keyword(const TokenLocation &p_token_location, int32_t &r_pos, const String &p_text, const String &p_searchstring, TokenType p_type, int32_t p_text_length, bool p_store_search_string) {
	if (p_text.find(p_searchstring, r_pos) == r_pos) {
		// Do not allow if the character afterwards is also alphabetic,
		// as it could be an identifier.
		int32_t next_char = r_pos + p_searchstring.length();
		if (next_char < p_text_length) {
			CharType c = p_text[next_char];
			if (!_is_delimiter(c, " (\n")) {
				return false;
			}
		}

		TokenClass t;
		t.type = p_type;
		if (p_store_search_string) {
			t.text = p_searchstring;
		}

		data.tokens.push_back(add_token(p_token_location, t));
		r_pos += p_searchstring.length();
		return true;
	}
	return false;
}

bool GDScriptPreProcessor::is_token_number(const String &p_text, int32_t &r_pos, int32_t p_length, GDScriptPreProcessor::TokenType &r_token_type, String &r_num_string) {
	bool minus = false;
	bool point = false;

	int32_t start = r_pos;

	while (r_pos < p_length) {
		CharType c = p_text[r_pos++];

		if (!minus && c == '-') {
			minus = true;
			continue;
		}
		if (!point && c == '.') {
			point = true;
			continue;
		}

		if (_is_num(c)) {
			continue;
		}

		// Not recognised.
		r_pos--;
		break;
	}

	r_num_string = p_text.substr(start, r_pos - start);
	r_token_type = point ? TT_FLOAT : TT_INT;
	return r_pos != start;
}

bool GDScriptPreProcessor::lex_tokens(Line &r_line) {
	const String &text = r_line.text;

	int32_t p = 0;
	int32_t l = text.length();

	TokenLocation tl;
	tl.source_line = r_line.source_line;
	tl.valid_line = r_line.valid_line;
	int32_t column = -1;

#define LEX_KW(KW_STRING, TTYPE)                                           \
	if (lex_predefined_keyword(tl, p, text, KW_STRING, TTYPE, l, false)) { \
		continue;                                                          \
	}

#define LEX_KW_STORE(KW_STRING, TTYPE)                                    \
	if (lex_predefined_keyword(tl, p, text, KW_STRING, TTYPE, l, true)) { \
		continue;                                                         \
	}

#define LEX_SW(KW_STRING, TTYPE)                                     \
	if (lex_predefined_token(tl, p, text, KW_STRING, TTYPE, true)) { \
		continue;                                                    \
	}

	//#define LEX_SW_STORE(KW_STRING, TTYPE)                               \
//	if (lex_predefined_token(tl, p, text, KW_STRING, TTYPE, true)) { \
//				continue;                                              \
//	}

	TokenType tt = TT_UNKNOWN;
	String num_string;

	while (p < l) {
		column++;
		tl.source_column = column;

		CharType c = text[p];

		switch (c) {
			default:
				break;
			case ' ': {
				p++;
				continue;
			} break;
			case '"': {
				p++;
				if (!lex_token_string(tl, text, p, l)) {
					return false;
				}
				continue;
			} break;
		}

		LEX_SW("(", TT_OPEN_BRACKET);
		LEX_SW(")", TT_CLOSE_BRACKET);
		LEX_SW(":", TT_COLON);
		LEX_SW(",", TT_COMMA);

		LEX_SW("==", TT_COMPARISON);
		LEX_SW("!=", TT_COMPARISON);
		LEX_SW("<=", TT_COMPARISON);
		LEX_SW(">=", TT_COMPARISON);
		LEX_SW("<", TT_COMPARISON);
		LEX_SW(">", TT_COMPARISON);

		LEX_SW("*=", TT_ASSIGN);
		LEX_SW("/=", TT_ASSIGN);
		LEX_SW("+=", TT_ASSIGN);
		LEX_SW("-=", TT_ASSIGN);
		LEX_SW("=", TT_ASSIGN);

		LEX_SW("*", TT_MATH);
		LEX_SW("/", TT_MATH);
		LEX_SW("+", TT_MATH);
		LEX_SW("-", TT_MATH);

		// keywords.
		LEX_KW("and", TT_AND);
		LEX_KW("or", TT_OR);
		LEX_KW("not", TT_NOT);

		LEX_KW("if", TT_IF);
		LEX_KW("else", TT_ELSE);
		LEX_KW("for", TT_FOR);
		LEX_KW("while", TT_WHILE);
		LEX_KW("break", TT_BREAK);
		LEX_KW("continue", TT_CONTINUE);
		LEX_KW("pass", TT_PASS);
		LEX_KW("return", TT_RETURN);
		LEX_KW("match", TT_MATCH);
		LEX_KW("is", TT_IS);
		LEX_KW("const", TT_CONST);
		LEX_KW("var", TT_VAR);
		LEX_KW("assert", TT_ASSERT);
		LEX_KW("yield", TT_YIELD);
		LEX_KW("signal", TT_SIGNAL);

		LEX_KW("in", TT_IN);
		LEX_KW("range", TT_RANGE);
		LEX_KW("print", TT_PRINT);
		LEX_KW("func", TT_FUNC);
		LEX_KW("extends", TT_EXTENDS);

		if (is_token_number(text, p, l, tt, num_string)) {
			TokenClass t;
			t.type = tt;
			t.text = num_string;
			data.tokens.push_back(add_token(tl, t));
			continue;
		}

		if (!lex_identifier(tl, text, p, l)) {
			return false;
		}
	}

	return true;
}

GDScriptPreProcessor::TokenID GDScriptPreProcessor::get_current_token_id() const {
	if (Data::current_token >= data.tokens.size()) {
		return TokenID();
	}
	return TokenID(Data::current_token);
}

GDScriptPreProcessor::TokenID GDScriptPreProcessor::get_previous_token_id() const {
	if (Data::current_token > 0) {
		return TokenID(Data::current_token - 1);
	}
	return TokenID();
}

const GDScriptPreProcessor::Token &GDScriptPreProcessor::get_current_token() const {
	//DEV_ASSERT(Data::current_token < data.tokens.size());
	if (Data::current_token >= data.tokens.size()) {
		static Token null;
		return null;
	}
	return get_token(Data::current_token);
}

const GDScriptPreProcessor::Token &GDScriptPreProcessor::get_previous_token() const {
	DEV_ASSERT(Data::current_token <= data.tokens.size());
	if (Data::current_token > 0) {
		return get_token(Data::current_token - 1);
	}
	static Token null;
	return null;
}

const GDScriptPreProcessor::TokenClass *GDScriptPreProcessor::get_previous_token_class() const {
	Token inst = get_previous_token();

	if (!inst.class_id.is_valid()) {
		return nullptr;
	}
	return &data.token_classes[inst.class_id.id()];
}

const GDScriptPreProcessor::TokenClass *GDScriptPreProcessor::get_current_token_class() const {
	Token inst = get_current_token();

	if (!inst.class_id.is_valid()) {
		return nullptr;
	}
	return &data.token_classes[inst.class_id.id()];
}

bool GDScriptPreProcessor::accept(TokenType p_token_type) {
	if (is_parse_finished())
		return false;

	const TokenClass &token = *get_current_token_class();

	if (token.type == p_token_type) {
		data.parse_advance();
		return true;
	}
	return false;
}

bool GDScriptPreProcessor::expect(TokenType p_token_type) {
	if (is_parse_finished())
		return false;

	if (accept(p_token_type)) {
		return true;
	}
	print_line("Syntax error.");
	return false;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_declare_func_params(ASTNodeID p_parent) {
	ASTNodeID params_id = ast_create_node(p_parent, ASTType::DECLARATION_FUNCTION_PARAMETERS);

	do {
		bool found = false;

		if (accept(TT_VAR)) {
			if (!expect(TT_IDENTIFIER))
				return syntax_error("function parameter expected identifier.");
			found = true;
		}
		if (accept(TT_IDENTIFIER)) {
			found = true;
		}
		if (found) {
			add_previous_token_as_local_variable(true, true);
			ASTNodeID param = ast_create_node(params_id, ASTType::LITERAL_IDENTIFIER, get_previous_token_id());
			//ASTNode &params = get_ast_node(params_id);
			ast_add_child(params_id, param);
			//params.children.push_back(param);
		}

	} while (accept(TT_COMMA));

	return params_id;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_func_params(ASTNodeID p_parent) {
	ASTNodeID params_id = ast_create_node(p_parent, ASTType::EXPRESSION_FUNCTION_PARAMETER);

	do {
		ASTNodeID expr = parse_expression(params_id);

		if (expr.is_valid()) {
			ast_add_child(params_id, expr);
			//			ASTNode &params = get_ast_node(params_id);
			//			params.children.push_back(expr);
		}

	} while (accept(TT_COMMA));

	return params_id;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_accept_identifier_function(ASTNodeID p_parent) {
	if (!accept(TT_IDENTIFIER))
		return ASTNodeID();

	// Is the identifier a registered variable?
	if (data.find_function(get_previous_token_class()->text) != UINT32_MAX) {
		return ast_create_node(p_parent, ASTType::LITERAL_IDENTIFIER, get_previous_token_id());
	}

	// Not registered, backup.
	backup();

	return ASTNodeID();
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_accept_variable(ASTNodeID p_parent, bool p_flag_not_found) {
	if (!accept(TT_IDENTIFIER))
		return ASTNodeID();

	// Is the identifier a registered variable?
	//if (data.current_function->vars.already_exists(get_previous_token_id()))
	if (scope_stack_variable_exists(get_previous_token().class_id.id())) {
		return ast_create_node(p_parent, ASTType::EXPRESSION_VARIABLE, get_previous_token_id());
	}

	// Unrecognised identifier can't immediately be a syntax error as it could be a function.
	if (p_flag_not_found) {
		syntax_error("variable not found.");
	}

	// Not registered, backup.
	backup();

	return ASTNodeID();
}

GDScriptPreProcessor::ASTNode &GDScriptPreProcessor::ast_get_node(ASTNodeID p_node_id) {
	DEV_ASSERT(p_node_id.is_valid());
	return data.nodes[p_node_id.id()];
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::ast_create_node(ASTNodeID p_parent, ASTType p_type, TokenID p_token_id, ASTNodeID p_left_child, ASTNodeID p_right_child) {
	ASTNodeID node_id(data.nodes.size());
	data.nodes.resize(node_id.id() + 1);
	ASTNode &node = data.nodes[node_id.id()];
	node.type = p_type;
	node.token_id = p_token_id;
	node.parent = p_parent;
	node.self = node_id;

	if (p_left_child.is_valid()) {
		//		get_ast_node(p_left_child).parent = node_id;
		//		node.children.push_back(p_left_child);
		ast_add_child(node_id, p_left_child);
	}

	if (p_right_child.is_valid()) {
		//		get_ast_node(p_right_child).parent = node_id;
		//		node.children.push_back(p_right_child);
		ast_add_child(node_id, p_right_child);
	}

	return node_id;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_factor(ASTNodeID p_parent) {
	if (accept(TT_OPEN_BRACKET)) {
		ASTNodeID id = parse_expression(p_parent);
		if (!expect(TT_CLOSE_BRACKET)) {
			return syntax_error("expression expected ')'.");
		}
		return id;
	}

	if (accept(TT_INT)) {
		return ast_create_node(p_parent, ASTType::LITERAL_INTEGER, get_previous_token_id());
	}
	if (accept(TT_FLOAT)) {
		return ast_create_node(p_parent, ASTType::LITERAL_FLOAT, get_previous_token_id());
	}
	if (accept(TT_STRING)) {
		return ast_create_node(p_parent, ASTType::LITERAL_STRING, get_previous_token_id());
	}

	ASTNodeID func_ident = parse_accept_identifier_function(p_parent);
	if (func_ident.is_valid()) {
		if (!expect(TT_OPEN_BRACKET))
			return false;
		ASTNodeID func_params = parse_func_params(p_parent);
		if (!expect(TT_CLOSE_BRACKET))
			return false;

		return ast_create_node(p_parent, ASTType::EXPRESSION_FUNCTION_CALL, TokenID(), func_ident, func_params);
	}

	// This is the final way of finding a variable, if not found as a function previously,
	// it is a syntax error.
	ASTNodeID var_id = parse_accept_variable(true);
	if (var_id.is_valid()) {
		return var_id;
	}

	return ASTNodeID();
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::syntax_error(const char *p_message) {
	const Token &ti = get_previous_token();
	String sz = get_token_as_string(ti);
	sz = "Syntax Error, line " + itos(ti.location.source_line) + ", col " + itos(ti.location.source_column) + ", " + sz;
	if (p_message) {
		sz += String(" : ") + p_message;
	}
	ERR_PRINT(sz);
	data.error_found = true;
	return ASTNodeID();
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_term(ASTNodeID p_parent) {
	ASTNodeID factor = parse_factor(p_parent);

	if (factor.is_valid()) {
		if (accept(TT_MATH)) {
			TokenID math_token = get_previous_token_id();

			ASTNodeID factor2 = parse_factor(p_parent);
			if (!factor2.is_valid()) {
				syntax_error("math expression expected second factor.");
				return factor;
			}

			return ast_create_node(p_parent, ASTType::EXPRESSION_BINARY, math_token, factor, factor2);
		}

		return factor;
	}

	return ASTNodeID();
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_expression(ASTNodeID p_parent) {
	return parse_term(p_parent);
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_statement(ASTNodeID p_parent, uint32_t p_depth) {
	ASTNodeID var_id = parse_accept_variable(p_parent);
	if (var_id.is_valid()) {
		if (accept(TT_ASSIGN)) {
			TokenID assign_token = get_previous_token_id();
			ASTNodeID expr = parse_expression(p_parent);
			if (expr.is_valid()) {
				// Special case.
				// If we are assigning to a variable, we mark it as written to.
				// This will help later during inlining.
				const Token &var_token = get_token(ast_get_node(var_id).token_id);
				Variable &var = get_current_scope().vars.get_var(var_token.class_id);
				var.is_written = true;
				var.is_used = true;
#ifdef DEV_ENABLED
				print_line("VAR MARKED AS WRITTEN : " + get_token_as_string(var_token));
#endif

				return ast_create_node(p_parent, ASTType::EXPRESSION_ASSIGNMENT, assign_token, var_id, expr);
			}
		}

		return syntax_error("assign expected second expression.");
	}

	if (accept(TT_PRINT)) {
		return ast_create_node(p_parent, ASTType::STATEMENT_PRINT, TokenID(), parse_expression(p_parent));
	}

	if (accept(TT_RETURN)) {
		TokenID return_token = get_previous_token_id();
		ASTNodeID return_value = parse_expression(p_parent);

		// Mark the function as having a return value.
		// Useful for inlining.
		if (return_value.is_valid()) {
			data.current_function->has_return_value = true;
		}
		return ast_create_node(p_parent, ASTType::STATEMENT_RETURN, return_token, return_value);
	}
	if (accept(TT_PASS)) {
		return ast_create_node(p_parent, ASTType::STATEMENT_PASS, get_previous_token_id());
	}
	if (accept(TT_FOR)) {
		if (!expect(TT_IDENTIFIER))
			return syntax_error("for expected 'identifier'.");

		// Add the identifier as a local variable if not already existing.
		TokenID for_token = get_previous_token_id();
		add_previous_token_as_local_variable(false, false);

		if (!expect(TT_IN))
			return syntax_error("for expected 'in'.");
		if (!expect(TT_RANGE))
			return syntax_error("for expected 'range'.");
		ASTNodeID expr = parse_expression(p_parent);
		if (!expr.is_valid())
			return syntax_error("for expected expression.");
		if (!expect(TT_COLON))
			return syntax_error("for expected ':'.");
		ASTNodeID block = parse_block(p_parent, p_depth + 1);
		if (!block.is_valid())
			return syntax_error("for Invalid block.");

		return ast_create_node(p_parent, ASTType::STATEMENT_FOR, for_token, expr, block);
	}

	return parse_expression(p_parent);
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_block(ASTNodeID p_parent, uint32_t p_depth) {
	ASTNodeID block_id = ast_create_node(p_parent, ASTType::BLOCK);

	while (!data.parse_finished() && !data.error_found) {
		// Check the indentation of the current token.
		const Token &token = get_current_token();

		uint32_t line = token.location.valid_line;
		uint32_t indent = data.lines[line].indent;

		print_line("parse_block token_id " + itos(get_current_token_id().id()) + " ... " + get_token_as_string(token));

		if (indent > p_depth) {
			return syntax_error("unexpected indent.");
		}
		// Finish block condition, indent is less.
		if (indent < p_depth) {
			return block_id;
		}

		ASTNodeID child = parse_block_loop(block_id, p_depth);
		if (child.is_valid()) {
			//ASTNode &block = get_ast_node(block_id);
			//block.children.push_back(child);
			ast_add_child(block_id, child);
		} else {
			return block_id;
		}
	}

	return block_id;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::parse_block_loop(ASTNodeID p_parent, uint32_t p_depth) {
	debug_print_local_tokens();

	if (accept(TT_EXTENDS)) {
		if (!expect(TT_IDENTIFIER)) {
			return syntax_error("var expected 'identifier'.");
		}

		return ast_create_node(p_parent, ASTType::STATEMENT_EXTENDS, get_previous_token_id());
	}

	if (accept(TT_VAR)) {
		if (!expect(TT_IDENTIFIER))
			return syntax_error("var expected 'identifier'.");

		TokenID var_token = get_previous_token_id();
		add_previous_token_as_local_variable(true, false);

		if (accept(TT_ASSIGN)) {
			return ast_create_node(p_parent, ASTType::DECLARATION_VARIABLE, var_token, parse_expression(p_parent));
		}

		return ast_create_node(p_parent, ASTType::DECLARATION_VARIABLE, var_token);
	}

	if (accept(TT_FUNC)) {
		// Special case, first func? quit out and save the file scope into func 0.
		//		if (data.funcs.size() == 1)
		//		{
		//			backup();
		//			data.starting_functions = true;
		//			return ASTNodeID();
		//		}

		if (!expect(TT_IDENTIFIER))
			return syntax_error("func expected 'identifier'.");

		// Remove previous function scope?
		if (data.scope_stack.size() > 1) {
			scope_stack_pop();
		}

		// Create a new scope for the function, with the function name.
		TokenID func_token = get_previous_token_id();
		TokenClassID class_id = get_previous_token().class_id;
		scope_stack_push(class_id);

		if (!expect(TT_OPEN_BRACKET))
			return syntax_error("func expected '('.");
		ASTNodeID params = parse_declare_func_params(p_parent);
		if (!expect(TT_CLOSE_BRACKET))
			return syntax_error("func expected ')'.");
		if (!expect(TT_COLON))
			return syntax_error("func expected ':'.");

		ASTNodeID func_body = parse_block(p_parent, p_depth + 1);
		ASTNodeID func_node = ast_create_node(p_parent, ASTType::DECLARATION_FUNCTION, func_token, params, func_body);
		if (func_node.is_valid()) {
			data.function_nodes.push_back(func_node);

			data.current_function->ast_nodes.push_back(func_node);
		}
		return func_node;
	}

	return parse_statement(p_parent, p_depth);
}

bool GDScriptPreProcessor::add_previous_token_as_local_variable(bool p_error_on_existing, bool p_is_parameter) {
	const Token &ti = get_previous_token();

	// Add the parameter.
	uint32_t parameter_id = UINT32_MAX;

	if (p_is_parameter) {
		FuncParameter fp;
		fp.token_id = get_previous_token_id();
		fp.token_class_id = ti.class_id;
		fp.identifier = get_previous_token_class()->text;
		parameter_id = data.current_function->params.size();
		data.current_function->params.push_back(fp);
	}

	if (!get_current_scope().vars.add_variable(ti.class_id.id(), parameter_id)) {
		if (p_error_on_existing) {
			print_line("Parse error : Token " + get_token_as_string(ti) + " has already been declared locally.");
		}
		return false;
	}

	return true;
}

void GDScriptPreProcessor::output_ast_node(ASTNodeID p_node_id, String &r_output, uint32_t p_indent, bool p_newline) {
	DEV_ASSERT(p_node_id.is_valid());
	const ASTNode &node = ast_get_node(p_node_id);
	const Token *token = node.token_id.is_valid() ? &get_token(node.token_id.id()) : nullptr;
	const TokenClass *tclass = token ? &data.token_classes[token->class_id.id()] : nullptr;

#define AST_OUT_CHILD(WHICH) output_ast_node(node.children[WHICH], r_output, p_indent, false)
#define AST_OUT_CHILD_INDENT(WHICH) output_ast_node(node.children[WHICH], r_output, p_indent + 1, false)
#define AST_OUT_CHILD_NEWLINE(WHICH) output_ast_node(node.children[WHICH], r_output, p_indent, true)

	if (p_newline) {
		for (uint32_t ind = 0; ind < p_indent; ind++) {
			r_output += "\t";
		}
	}

	bool newline_children = false;
	bool tabbed_already = false;

	switch (node.type) {
		default: {
			tabbed_already = p_newline;
		} break;
		case ASTType::STATEMENT_EXTENDS: {
			DEV_ASSERT(tclass);
			r_output += "extends " + tclass->text + "\n";
		} break;
		case ASTType::DECLARATION_FUNCTION: {
			DEV_ASSERT(tclass);
			r_output += "\nfunc " + tclass->text + "(";
			DEV_ASSERT(node.children.size() == 2);
			AST_OUT_CHILD(0);
			r_output += "):\n";
			AST_OUT_CHILD_INDENT(1);
			return;

		} break;
		case ASTType::EXPRESSION_FUNCTION_CALL: {
			DEV_ASSERT(node.children.size() == 2);
			AST_OUT_CHILD(0);
			r_output += "(";
			AST_OUT_CHILD(1);
			r_output += ")\n";
			return;
		} break;
		case ASTType::DECLARATION_FUNCTION_PARAMETERS: {
			for (uint32_t n = 0; n < node.children.size(); n++) {
				AST_OUT_CHILD(n);

				if (n != node.children.size() - 1) {
					r_output += ", ";
				}
			}
			return;
		} break;
		case ASTType::EXPRESSION_FUNCTION_PARAMETER: {
			for (uint32_t n = 0; n < node.children.size(); n++) {
				AST_OUT_CHILD(n);

				if (n != node.children.size() - 1) {
					r_output += ", ";
				}
			}
			return;
		} break;
		case ASTType::STATEMENT_RETURN: {
			if (node.children.size()) {
				r_output += "return  ";
				AST_OUT_CHILD(0);
				r_output += "\n";
			} else {
				r_output += "return\n";
			}
			return;
		} break;
		case ASTType::STATEMENT_PASS: {
			r_output += "pass\n";
		} break;
		case ASTType::STATEMENT_PRINT: {
			r_output += "print (";
			if (node.children.size()) {
				AST_OUT_CHILD(0);
			}
			r_output += ")\n";
			return;
		} break;
		case ASTType::STATEMENT_FOR: {
			r_output += "for ";
			DEV_ASSERT(tclass);
			r_output += tclass->text + " in range (";
			if (node.children.size()) {
				AST_OUT_CHILD(0);
			}
			r_output += "):\n";
			AST_OUT_CHILD_INDENT(1);
			return;
		} break;
		case ASTType::DECLARATION_VARIABLE: {
			DEV_ASSERT(tclass);
			r_output += "var " + tclass->text;
			if (node.children.size()) {
				r_output += " = ";
				AST_OUT_CHILD(0);
			}
			r_output += "\n";
			return;
		} break;
		case ASTType::EXPRESSION_BINARY: {
			AST_OUT_CHILD(0);
			DEV_ASSERT(tclass);
			r_output += " " + tclass->text + " ";
			AST_OUT_CHILD(1);
			return;
		} break;
		case ASTType::EXPRESSION_ASSIGNMENT: {
			AST_OUT_CHILD(0);
			DEV_ASSERT(tclass);
			r_output += " " + tclass->text + " ";
			AST_OUT_CHILD(1);
			r_output += "\n";
			return;
		} break;
		case ASTType::EXPRESSION_VARIABLE: {
			DEV_ASSERT(tclass);
			r_output += tclass->text;

			// Look up the variable.

		} break;
		case ASTType::LITERAL_STRING: {
			DEV_ASSERT(tclass);
			r_output += "\"" + tclass->text + "\"";
		} break;
		case ASTType::LITERAL_IDENTIFIER:
		case ASTType::LITERAL_INTEGER:
		case ASTType::LITERAL_FLOAT: {
			DEV_ASSERT(tclass);
			r_output += tclass->text;
		} break;
		case ASTType::EXPRESSION_INLINE_FUNCTION_CALL:
		case ASTType::BLOCK: {
			newline_children = true;
			tabbed_already = p_newline;
		} break;
	}

	if (newline_children) {
		for (uint32_t n = 0; n < node.children.size(); n++) {
			DEV_ASSERT(node.children[n] != p_node_id);

			if (tabbed_already && (n == 0)) {
				AST_OUT_CHILD(n);
			} else {
				AST_OUT_CHILD_NEWLINE(n);
			}
		}
	} else {
		for (uint32_t n = 0; n < node.children.size(); n++) {
			DEV_ASSERT(node.children[n] != p_node_id);
			AST_OUT_CHILD(n);
		}
	}
}

void GDScriptPreProcessor::debug_print_ast_node(ASTNodeID p_node_id, int32_t p_depth) {
	DEV_ASSERT(p_node_id.is_valid());
	String sz = String("\t").repeat(p_depth);

	const ASTNode &node = ast_get_node(p_node_id);

	sz += "ast: " + itos(p_node_id.id()) + " par: " + itos(node.parent.id()) + " : ";

	sz += String(ASTStrings[(int32_t)node.type]).to_lower();

	if (node.token_id.is_valid()) {
		const Token &token = data.tokens[node.token_id.id()];
		sz += " ... " + get_token_as_string(token);
	}
	print_line(sz);

	for (uint32_t n = 0; n < node.children.size(); n++) {
		DEV_ASSERT(node.children[n] != p_node_id);
		debug_print_ast_node(node.children[n], p_depth + 1);
	}
}

void GDScriptPreProcessor::debug_print_local_tokens() {
	int32_t num_tokens = data.tokens.size();
	int32_t curr_token = Data::current_token;

	int32_t start_token = curr_token;
	int32_t end_token = start_token + 6;
	start_token -= 3;
	start_token = MAX(start_token, 0);
	end_token = MIN(end_token, num_tokens);

	String string = " ... ";
	for (int32_t t = start_token; t < end_token; t++) {
		string += String("[ ") + itos(t) + " ] ";
		const Token &token_instance = data.tokens[t];
		String sz = get_token_as_string(token_instance);
		string += String(t == curr_token ? "* " : "") + sz + ", ";
	}
	print_line(string);
}

bool GDScriptPreProcessor::read_line_and_strip_comments(Line &r_line) {
	String &text = r_line.text;

	int32_t p = 0;
	int32_t l = text.length();

	while (p < l) {
		CharType c = text[p++];

		switch (c) {
			default:
				break;
			// Comment.
			case '#': {
				return false;
			} break;
			case ' ': {
				continue;
			} break;
			case '\t': {
				r_line.indent += 1;
				continue;
			} break;
		}

		// Process the line itself.
		p--;
		text = text.substr(p, l - p);
		p = 0;
		l = text.length();

		while (p < l) {
			if (text[p] == '#') {
				// Remove comment from end of line.
				text = text.substr(0, p);
				return true;
			}
			p++;
		}

		return true;
	}

	return false;
}

bool GDScriptPreProcessor::lex_line(Line &r_line) {
	String &text = r_line.text;

	print_line("Lexing line : " + text);

	int32_t p = 0;
	int32_t l = text.length();

	while (p < l) {
		CharType c = text[p++];

		switch (c) {
			default:
				break;
			// Comment.
			case '#': {
				return false;
			} break;
			case ' ': {
				continue;
			} break;
			case '\t': {
				r_line.indent += 1;
				continue;
			} break;
		}

		// Process the line itself.
		p--;
		text = text.substr(p, l - p);
		p = 0;
		l = text.length();

		while (p < l) {
			if (text[p] == '#') {
				// Remove comment from end of line.
				text = text.substr(0, p);

				lex_tokens(r_line);
				return true;
			}
			p++;
		}

		lex_tokens(r_line);
		return true;
	}

	// Blank line, might as well be a comment.
	return false;
}

void GDScriptPreProcessor::read_file() {
	const String &text = data.source;

	int32_t pos = 0;
	int32_t text_length = text.length();

	uint32_t read_line = 0;

	while (true) {
		if (pos >= text_length) {
			return;
		}

		int32_t line_start = pos;
		int32_t line_end = text.find("\n", pos);
		if (line_end == -1) {
			line_end = text_length;
		}

		Line l;
		l.text = text.substr(line_start, line_end - line_start);

		if (read_line_and_strip_comments(l)) {
			l.source_line = read_line;
			l.valid_line = data.lines.size();
			print_line("\tline " + itos(read_line) + " : " + l.text);

			// Maybe this line is a function definition.
			read_function(l);

			data.lines.push_back(l);
		}

		pos = line_end + 1;
		read_line++;
	}
}

bool GDScriptPreProcessor::read_function(const Line &p_line) {
	if (!p_line.text.begins_with("func "))
		return true;

	// Func and space.
	int32_t pos = 5;

	int32_t open_bracket = p_line.text.find("(", pos);
	if (open_bracket == -1) {
		// Parse error.
		print_line("Parse error in function definition, no '(' found, line" + itos(p_line.source_line));
		return false;
	}

	Func f;
	f.name = p_line.text.substr(pos, open_bracket - pos);

	f.source_line = p_line.source_line;
	f.valid_line = data.lines.size();

	print_line("Found function : " + f.name + " on source line " + itos(f.source_line) + ", valid line " + itos(f.valid_line));
	data.current_function = &f;

	// Pos from the function string only.
	pos = (open_bracket + 1);
	bool finished = false;

	while (!finished) {
		if (!read_param(p_line, f, pos, finished)) {
			break;
		}
	}

	data.funcs.push_back(f);
	return true;
}

GDScriptPreProcessor::ASTNodeID GDScriptPreProcessor::inline_add_var_declaration(ASTNodeID p_parent, String p_var_name, ASTNodeID p_assign_var, TokenID *r_token_id) {
	TokenClass tc;
	tc.text = p_var_name;
	tc.type = TT_IDENTIFIER;

	TokenLocation tl;
	TokenID tid(data.tokens.size());
	data.tokens.push_back(add_token(tl, tc));

	if (r_token_id) {
		*r_token_id = tid;
	}
	ASTNodeID decl_node = ast_create_node(p_parent, ASTType::DECLARATION_VARIABLE, tid);
	if (p_assign_var.is_valid()) {
		ast_get_node(decl_node).children.push_back(p_assign_var);
	}
	return decl_node;
}

void GDScriptPreProcessor::make_inline_params(const Func &p_source_func, ASTNodeID p_parent, InlineInfo &r_info) {
	// Parameters.
	for (uint32_t p = 0; p < p_source_func.params.size(); p++) {
		const FuncParameter &fp = p_source_func.params[p];
		String unique_identifier = "__" + fp.identifier + itos(data.unique_identifier_count++);
		InlineChange &change = r_info.changes[p];
		change.identifier_from = fp.identifier;
		change.identifier_to = unique_identifier;

		if (fp.is_written) {
			ASTNodeID decl_param_id = inline_add_var_declaration(p_parent, change.identifier_to, change.passed_expression, &change.token_id);
			ASTNode *parent = &ast_get_node(p_parent);
			parent->children.push_back(decl_param_id);
		}
	}
}

void GDScriptPreProcessor::make_inline(InlineCall p_call) {
	DEV_ASSERT(p_call.call.is_valid());
	const ASTNode &call_node = ast_get_node(p_call.call);
	DEV_ASSERT(call_node.type == ASTType::EXPRESSION_FUNCTION_CALL);

	DEV_ASSERT(call_node.children.size() > 1);
	const ASTNode &call_identifier = ast_get_node(call_node.children[0]);
	const String &func_name = get_token_class(call_identifier.token_id).text;

	uint32_t func_id = data.find_function(func_name);
	DEV_ASSERT(func_id != -1);
	const Func &source_func = data.funcs[func_id];

	// Translate the call parameters to inline parameters.
	InlineInfo info;
	const ASTNode &call_params = ast_get_node(call_node.children[1]);
	for (uint32_t n = 0; n < call_params.children.size(); n++) {
		InlineChange ic;
		ic.passed_expression = call_params.children[n];
		info.changes.push_back(ic);
		//const ASTNode &call_param = get_ast_node(ic.passed_expression);
	}

	//ASTNode inline_func;
	//inline_func.type = ASTType::EXPRESSION_INLINE_FUNCTION_CALL;
	//inline_func.token_id = call_identifier.token_id;
	ASTNodeID inline_func_id = ast_create_node(p_call.parent, ASTType::EXPRESSION_INLINE_FUNCTION_CALL, call_identifier.token_id);

	// Get the function declaration node's block.
	const ASTNode &func_decl = ast_get_node(source_func.ast_nodes[0]);
	ASTNodeID func_decl_block = func_decl.children[1];
	//const ASTNode &block = get_ast_node(func_decl.children[1]);

	// How to do this depends on whether the inline function has a return or not.
	// If there is a return, we want to insert the inline on the statement BEFORE
	// the current one, store the return value in a temp, then replace this in the current expression.
	// If there is no return value, we can insert the inline function directly.
	if (source_func.has_return_value) {
		// Find the parent block, and where to insert in the parent block.
		ASTNodeID prev_child_id = p_call.call;
		ASTNodeID parent_block_id;
		int64_t parent_block_insert_pos = 0;

		ASTNode *node = &ast_get_node(p_call.call);

		while (node->type != ASTType::BLOCK) {
			parent_block_id = node->parent;

			if (!parent_block_id.is_valid()) {
				// Something has gone wrong in the hierarchy, we can't inline.
				return;
			}

			node = &ast_get_node(parent_block_id);
			parent_block_insert_pos = node->children.find(prev_child_id);
			DEV_ASSERT(parent_block_insert_pos != -1);
			prev_child_id = parent_block_id;
		}

		// At this point, node should be the block,
		// and parent_block_insert_pos should be valid.
		DEV_ASSERT(p_call.parent == parent_block_id);
		make_inline_params(source_func, inline_func_id, info);

		duplicate_inline(func_decl_block, inline_func_id, source_func.ast_nodes[0], info);

		ASTNode *parent = &ast_get_node(parent_block_id);
		parent->children.insert(parent_block_insert_pos, inline_func_id);

		// Final step, replace the Call node with the temp return variable.
		//		ASTNode &old_node = get_ast_node(p_call.call);
		//		old_node.type = ASTType::EXPRESSION_VARIABLE;
		//		old_node.children.clear();
		//		old_node.token_id = data.inline_return_token;

		ast_replace_child(p_call.call, data.inline_return_expression);

	} else {
		// Create a block for the parameters.
		ASTNodeID param_block = ast_create_node(p_call.call, ASTType::GROUP);
		//parent = &get_ast_node(parent_block_id);
		//parent->children.insert(parent_block_insert_pos, param_block);
		make_inline_params(source_func, param_block, info);

		duplicate_inline(func_decl_block, inline_func_id, source_func.ast_nodes[0], info);

		// Final step, replace the Call node with the Inline node.
		ASTNode &old_node = ast_get_node(p_call.call);
		const ASTNode &new_node = ast_get_node(inline_func_id);
		old_node = new_node;

		old_node.children.insert(0, param_block);
	}
}

void GDScriptPreProcessor::ast_add_child(ASTNodeID p_parent, ASTNodeID p_child) {
	ASTNode &parent = ast_get_node(p_parent);
	parent.children.push_back(p_child);

	ASTNode &child = ast_get_node(p_child);
	child.parent = p_parent;
}

void GDScriptPreProcessor::ast_replace_child(ASTNodeID p_old_child, ASTNodeID p_new_child) {
	DEV_ASSERT(p_old_child.is_valid());
	//DEV_ASSERT(p_new_child.is_valid());

	ASTNodeID parent_id = ast_get_node(p_old_child).parent;
	DEV_ASSERT(parent_id.is_valid());

	ASTNode &parent = ast_get_node(parent_id);
	int64_t found = parent.children.find(p_old_child);
	DEV_ASSERT(found != -1);

	if (p_new_child.is_valid()) {
		parent.children[found] = p_new_child;
	} else {
		parent.children.remove(found);
	}
}

void GDScriptPreProcessor::duplicate_inline(ASTNodeID p_source_id, ASTNodeID p_dest_id, ASTNodeID p_dest_parent_id, InlineInfo &r_inline_info) {
	const ASTNode *source = &ast_get_node(p_source_id);
	ASTNode *dest = &ast_get_node(p_dest_id);

	if (dest->type != ASTType::EXPRESSION_INLINE_FUNCTION_CALL) {
		dest->type = source->type;
		dest->token_id = source->token_id;
	}

	bool save_children_as_return_expression = false;

	switch (dest->type) {
		default:
			break;
		case ASTType::DECLARATION_VARIABLE: {
			const TokenClass &tc = get_token_class(dest->token_id);

			InlineChange ic;
			ic.identifier_from = tc.text;
			ic.identifier_to = "__" + ic.identifier_from + itos(data.unique_identifier_count++);

			// Create a new token for this unique variable.
			// (location NYI).
			TokenClass tc_new;
			tc_new.text = ic.identifier_to;
			tc_new.type = TT_IDENTIFIER;

			TokenLocation tl;
			ic.token_id = data.tokens.size();
			data.tokens.push_back(add_token(tl, tc_new));

			r_inline_info.changes.push_back(ic);

			// Change the declaration to use this token.
			dest->token_id = ic.token_id;

		} break;
		case ASTType::EXPRESSION_VARIABLE: {
			// Translate if a parameter.

			//bool found = false;
			//			while (!found) {
			const TokenClass &tc = get_token_class(dest->token_id);

			for (uint32_t n = 0; n < r_inline_info.changes.size(); n++) {
				const InlineChange &change = r_inline_info.changes[n];

				if (tc.text == change.identifier_from) {
					if (change.passed_expression.is_valid()) {
						ast_replace_child(p_dest_id, change.passed_expression);
					} else {
						// We are replacing the variable with a unique variable.
						//							TokenClass tc_new;
						//							tc_new.text = change.identifier_to;
						//							tc_new.type = TT_IDENTIFIER;

						//							TokenLocation tl;
						//							TokenID tid(data.tokens.size());
						//							data.tokens.push_back(add_token(tl, tc_new));

						//							dest->token_id = tid;
						dest->token_id = change.token_id;
					}
					//found = true;
					break;
				}
			}
#if 0
				// If using a variable and not a parameter so far, add it to the changes,
				// with a unique version.
				if (!found) {
					InlineChange ic;
					ic.identifier_from = tc.text;
					ic.identifier_to = "__" + ic.identifier_from + itos(data.unique_identifier_count++);

					// Create a new token for this unique variable.
					// (location NYI).
					TokenClass tc_new;
					tc_new.text = ic.identifier_to;
					tc_new.type = TT_IDENTIFIER;

					TokenLocation tl;
					ic.token_id = data.tokens.size();
					data.tokens.push_back(add_token(tl, tc_new));

					r_inline_info.changes.push_back(ic);
				}
			} // while not found
#endif
		} break;
		case ASTType::STATEMENT_RETURN: {
#if 1
			//ASTNodeID return_expr = dest->children[0];
			//TokenID dummy;

			ASTNodeID decl_node_id = inline_add_var_declaration(p_dest_parent_id, "__return" + itos(data.unique_identifier_count++), ASTNodeID(), &data.inline_return_token);

			//ast_replace_child(p_dest_id, decl_node_id);
			ast_replace_child(p_dest_id, ASTNodeID());

			p_dest_id = decl_node_id;

			save_children_as_return_expression = true;
			//			source = &get_ast_node(p_source_id);
			//			LocalVector<ASTNodeID> children = source->children;
			//			DEV_ASSERT(children.size() == 1);
			//			data.inline_return_expression = children[0];

#else
			// Special case.. return via a temporary.
			dest->type = ASTType::EXPRESSION_ASSIGNMENT;

			// Make an equals assign token specially for this purpose.
			TokenClass tc;
			tc.text = "=";
			tc.type = TT_ASSIGN;
			int64_t assign_equal_token_id = data.token_classes.find(tc);
			DEV_ASSERT(assign_equal_token_id != -1);
			TokenLocation tl;
			TokenID tid(data.tokens.size());
			data.tokens.push_back(add_token(tl, tc));

			dest->token_id = tid;
			ASTNodeID return_temp_identifier = create_ast_node(p_dest_id, ASTType::EXPRESSION_VARIABLE, p_return_value_token_id);

			// Reget.. could be invalidated.
			dest = &get_ast_node(p_dest_id);
			dest->children.insert(0, return_temp_identifier);
			//return;
#endif
		} break;
	}

	// Children.
	// Copy by value, as these may be invalidated.
	source = &ast_get_node(p_source_id);
	LocalVector<ASTNodeID> children = source->children;

	for (uint32_t n = 0; n < children.size(); n++) {
		ASTNodeID old_child_id = children[n];
		const ASTNode &child = ast_get_node(old_child_id);
		ASTType child_type = child.type;
		TokenID child_token_id = child.token_id;

		ASTNodeID new_child_id = ast_create_node(p_dest_parent_id, child_type, child_token_id);
		ast_add_child(p_dest_id, new_child_id);

		duplicate_inline(old_child_id, new_child_id, p_dest_id, r_inline_info);
	}

	if (save_children_as_return_expression) {
		DEV_ASSERT(ast_get_node(p_dest_id).children.size() == 1);
		data.inline_return_expression = ast_get_node(p_dest_id).children[0];
	}
}

void GDScriptPreProcessor::create_inlines() {
	LocalVector<InlineCall> calls;

	Func &func = data.funcs[0];
	for (uint32_t n = 0; n < func.ast_nodes.size(); n++) {
		search_for_inlines(func.ast_nodes[n], ASTNodeID(), UINT32_MAX, calls);
	}

	for (uint32_t n = 0; n < calls.size(); n++) {
		make_inline(calls[n]);
	}
}

void GDScriptPreProcessor::search_for_inlines(ASTNodeID p_node_id, ASTNodeID p_parent_node_id, uint32_t p_parent_func_id, LocalVector<InlineCall> &r_calls) {
	DEV_ASSERT(p_node_id.is_valid());
	const ASTNode &node = ast_get_node(p_node_id);

	switch (node.type) {
		default:
			break;
		case ASTType::BLOCK: {
			p_parent_node_id = p_node_id;
		} break;

		case ASTType::EXPRESSION_FUNCTION_CALL: {
			// The first child is the function name, if it matches the parent function,
			// we don't want to inline recursive funcs.
			DEV_ASSERT(node.children.size() > 1);
			const ASTNode &call_identifier = ast_get_node(node.children[0]);
			const String &func_name = get_token_class(call_identifier.token_id).text;
			uint32_t func_id = data.find_function(func_name);
			// Not found is a syntax error, matching parent func is recursive, so reject.
			if ((func_id == -1) || (func_id == p_parent_func_id)) {
				return;
			}

			InlineCall ic;
			ic.call = p_node_id;
			ic.parent = p_parent_node_id;

			r_calls.push_back(ic);
			return;
		} break;
		case ASTType::DECLARATION_FUNCTION: {
			const String &func_name = get_token_class(node.token_id).text;
			print_line("Found parent function " + func_name);

			p_parent_func_id = data.find_function(func_name);

			// Probably a parse error? Maybe cannot happen.
			ERR_FAIL_COND(p_parent_func_id == UINT32_MAX);
		} break;
	}

	for (uint32_t n = 0; n < node.children.size(); n++) {
		search_for_inlines(node.children[n], p_parent_node_id, p_parent_func_id, r_calls);
	}
}

#if 0
void GDScriptPreProcessor::search_for_inlines(uint32_t p_func_id) {
	Func &func = data.funcs[p_func_id];

	for (int32_t l = 1; l < func.body.lines.size(); l++) {
		Line &line = func.body.lines[l];

		for (uint32_t f = 0; f < p_func_id; f++) {
			const String &inline_func = data.funcs[f].name;
			int32_t found = line.text.find(inline_func);
			if (found != -1) {
				print_line("Found " + inline_func + " in line :\n\t" + line.text);
			}
		}
	}
}
#endif

bool GDScriptPreProcessor::eat_whitespace(const String &p_string, int32_t &r_pos) {
	while (r_pos < p_string.length()) {
		if (p_string[r_pos] == ' ') {
			r_pos++;
		} else {
			return true;
		}
	}
	return false;
}

bool GDScriptPreProcessor::read_token(const String &p_string, int32_t &r_pos, String &r_token, CharType &r_delimiter, const String &p_disallow_start, const String &p_delimiters) {
	r_token = "";

	if (p_disallow_start.find_char(p_string[r_pos]) != -1) {
		r_delimiter = p_string[r_pos++];
		return false;
	}

	// Might be a space after a comma...
	if (!eat_whitespace(p_string, r_pos)) {
		return false;
	}

	while (r_pos < p_string.length()) {
		CharType c = p_string[r_pos++];

		// Delimiter?
		int32_t found = p_delimiters.find_char(c);
		if (found != -1) {
			r_delimiter = c;
			return r_token.length() != 0;
		}

		r_token += c;
	}

	return false;
}

bool GDScriptPreProcessor::read_param(const Line &p_line, Func &r_func, int32_t &r_pos, bool &r_finished) {
	//	if (!r_func.body.lines.size()) {
	//		return false;
	//	}

	const String &text = p_line.text;
	//	if (text.substr(r_pos, 4) == String("var ")) {
	//		r_pos += 4;
	//	} else {
	//		return false;
	//	}

	String token;
	CharType delimiter;
	r_finished = false;

	while (true) {
		if (!read_token(text, r_pos, token, delimiter, ")", ",) ")) {
			return false;
		}
		if (token != "var") {
			break;
		}
	}

	print_line("\tparam is " + token);

	r_finished = delimiter == ')';
	return true;
}

bool GDScriptPreProcessor::lex_file() {
	data.current_function = nullptr;

	uint32_t next_func_id = 0;
	uint32_t next_func_valid_line = UINT32_MAX;

	if (data.funcs.size()) {
		next_func_valid_line = data.funcs[0].valid_line;
	}

	for (uint32_t l = 0; l < data.lines.size(); l++) {
		if (l == next_func_valid_line) {
			data.current_function = &data.funcs[next_func_id];
			next_func_id++;

			if (next_func_id < data.funcs.size()) {
				next_func_valid_line = data.funcs[next_func_id].valid_line;
			} else {
				next_func_id = UINT32_MAX;
				next_func_valid_line = UINT32_MAX;
			}
		}

		if (!lex_line(data.lines[l]))
			return false;
	}
	return true;
}

bool GDScriptPreProcessor::parse_file() {
	data.current_function = &data.funcs[0];

	DEV_ASSERT(!data.parse_finished());

	bool result = true;

	while (!data.parse_finished() && !data.error_found) {
		ASTNodeID id = parse_block(ASTNodeID(), 0);
		if (id.is_valid()) {
			data.funcs[0].ast_nodes.push_back(id);
		} else {
			result = false;
			break;
		}
	}

	// Remove previous function scope?
	if (data.scope_stack.size() > 1) {
		scope_stack_pop();
	}

	if (data.error_found)
		return false;

	return result;
}

String GDScriptPreProcessor::create_output() {
	// Debug print AST tree.
	print_line("\nAST Tree\n**********\n");

	String output;
	for (uint32_t f = 0; f < 1; f++) {
		//	for (uint32_t f = 0; f < data.funcs.size(); f++) {
		//for (uint32_t n = 0; n < data.function_nodes.size(); n++) {

		const Func &func = data.funcs[f];
		for (uint32_t n = 0; n < func.ast_nodes.size(); n++) {
			debug_print_ast_node(func.ast_nodes[n], n == 0 ? 0 : 1);
			output_ast_node(func.ast_nodes[n], output, 0, true);
		}
		//debug_print_ast_node(data.function_nodes[n], 0);
	}

	print_line("\nReconstructed\n**********\n" + output);
	return output;
}

#if 0
bool GDScriptPreProcessor::parse_function(uint32_t p_function) {
	Func &func = data.funcs[p_function];
	data.current_function = &func;
	print_line("\nPARSING FUNCTION : " + func.name);

	return parse_body(func.body);
}
#endif

String GDScriptPreProcessor::process(const String &p_source) {
	return p_source;
	data.source = p_source;

	read_file();

	lex_file();
	if (!parse_file()) {
		return p_source;
	}

	create_inlines();

	String output = create_output();
	//return output;

#if 0
	// Attempt to inline in each function.
	for (uint32_t n = 0; n < data.funcs.size(); n++) {
		search_for_inlines(n);
	}

	// Final output
	String final;
	print_line("\nFINAL OUTPUT:\n****************\n");

	for (uint32_t l = 0; l < data.lines.size(); l++) {
		const Line &line = data.lines[l];
		final += String("\t").repeat(line.indent);
		final += line.text + "\n";
	}
	final += "\n";

	print_line(final);
#endif

	return p_source;
}

void GDScriptPreProcessor::scope_stack_push(TokenClassID p_func_identifier_token_id) {
	DEV_ASSERT(p_func_identifier_token_id.is_valid());
	const TokenClass &token = data.token_classes[p_func_identifier_token_id.id()];

	data.scope_stack.resize(data.scope_stack.size() + 1);
	get_current_scope().name = token.text;

	print_line("Parsing function : " + token.text);

	uint32_t func_id = data.find_function(token.text);
	DEV_ASSERT(func_id != UINT32_MAX);

	data.current_function = &data.funcs[func_id];

	//get_current_scope().indent = p_indent;
}

void GDScriptPreProcessor::scope_stack_pop() {
	DEV_ASSERT(data.scope_stack.size() > 1);

	// Save any variable info to parameters.
	Scope &scope = get_current_scope();
	for (uint32_t n = 0; n < scope.vars.get_num_vars(); n++) {
		const Variable &var = scope.vars.get(n);
		if (var.is_parameter()) {
			FuncParameter &fp = data.current_function->params[var.parameter_id];
			fp.is_written = var.is_written;
			fp.is_used = var.is_used;

			print_line("scope_stack_pop param " + fp.identifier + ", used : " + String(Variant(fp.is_used)) + ", written : " + String(Variant(fp.is_written)));
		}
	}

	data.scope_stack.resize(data.scope_stack.size() - 1);
}

bool GDScriptPreProcessor::scope_stack_variable_exists(uint32_t p_token_id) const {
	uint32_t scope_id, var_id;
	return scope_stack_find_variable(p_token_id, scope_id, var_id);
}

bool GDScriptPreProcessor::scope_stack_find_variable(uint32_t p_token_id, uint32_t &r_scope_id, uint32_t &r_var_id) const {
	for (int32_t s = data.scope_stack.size() - 1; s >= 0; s--) {
		const Scope &scope = data.scope_stack[s];
		if (scope.vars.find(p_token_id, r_var_id)) {
			r_scope_id = s;
			return true;
		}
	}

	return false;
}

GDScriptPreProcessor::GDScriptPreProcessor() {
	data.scope_stack.resize(1);
	Scope &scope = get_current_scope();
	scope.name = "global";

	Func f;
	f.name = "global_scope";
	data.funcs.push_back(f);
}
