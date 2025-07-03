#pragma once

#include "core/local_vector.h"
#include "core/ustring.h"

class GDScriptPreProcessor {
	enum TokenType {
		TT_UNKNOWN,
		TT_STRING,
		TT_INT,
		TT_FLOAT,
		TT_FUNC,
		TT_COMPARISON,
		TT_ASSIGN,
		TT_KEYWORD,
		TT_EXTENDS,
		TT_VAR,
		TT_OPEN_BRACKET,
		TT_CLOSE_BRACKET,
		TT_COLON,
		TT_COMMA,
		TT_IDENTIFIER,
		//TT_IDENTIFIER_FUNC,
		//TT_IDENTIFIER_VAR,

		TT_AND,
		TT_OR,
		TT_NOT,

		TT_MATH,

		TT_IF,
		TT_ELSE,
		TT_FOR,
		TT_WHILE,
		TT_BREAK,
		TT_CONTINUE,
		TT_PASS,
		TT_RETURN,
		TT_MATCH,
		TT_IS,
		TT_CONST,
		TT_ASSERT,
		TT_YIELD,
		TT_SIGNAL,
		TT_IN,
		TT_RANGE,
		TT_PRINT,

	};

	enum class ASTType {
		UNKNOWN,
		BLOCK,
		GROUP,
		DECLARATION_FUNCTION,
		DECLARATION_FUNCTION_PARAMETERS,
		DECLARATION_VARIABLE,
		STATEMENT_EXPRESSION,
		STATEMENT_EXTENDS,
		STATEMENT_IF,
		STATEMENT_FOR,
		STATEMENT_PASS,
		STATEMENT_RETURN,
		STATEMENT_BREAK,
		STATEMENT_CONTINUE,
		STATEMENT_MATCH,
		STATEMENT_PRINT,
		EXPRESSION_LITERAL,
		EXPRESSION_VARIABLE,
		EXPRESSION_BINARY,
		EXPRESSION_UNARY,
		EXPRESSION_ASSIGNMENT,
		EXPRESSION_FUNCTION_CALL,
		EXPRESSION_INLINE_FUNCTION_CALL,
		EXPRESSION_FUNCTION_PARAMETER,
		EXPRESSION_MEMBER,
		EXPRESSION_TERNARY,
		EXPRESSION_SELF,
		EXPRESSION_AWAIT,
		EXPRESSION_YIELD,
		LITERAL_INTEGER,
		LITERAL_FLOAT,
		LITERAL_STRING,
		LITERAL_BOOL,
		LITERAL_NULL,
		LITERAL_ARRAY,
		LITERAL_DOLLAR_NODEPATH,
		LITERAL_IDENTIFIER,
		CLAUSE_MATCH_CASE,
		CLAUSE_MATCH_DEFAULT,
	};

	class IDBase {
		uint32_t _id;

	public:
		bool operator==(const IDBase &p_o) const { return _id == p_o._id; }
		bool operator!=(const IDBase &p_o) const { return !(*this == p_o); }
		bool is_valid() const { return _id != UINT32_MAX; }
		uint32_t id() const { return _id; }
		IDBase(uint32_t p_id) { _id = p_id; }
		IDBase() { _id = UINT32_MAX; }
	};

	class TokenID : public IDBase {
		using IDBase::IDBase;
	};
	class TokenClassID : public IDBase {
		using IDBase::IDBase;
	};
	class ASTNodeID : public IDBase {
		using IDBase::IDBase;
	};

	struct ASTNode {
		ASTType type = ASTType::UNKNOWN;
		TokenID token_id;
		ASTNodeID self;
		ASTNodeID parent;
		//private:
		LocalVector<ASTNodeID> children;
	};

	static const char *TokenStrings[];
	static const char *ASTStrings[];

	struct TokenLocation {
		uint32_t valid_line = 0;
		uint32_t source_line = 0;
		uint32_t source_column = 0;
	};

	struct TokenClass {
		TokenType type;
		String text;
		bool operator==(const TokenClass &p_o) const { return (type == p_o.type) && (text == p_o.text); }
	};

	struct Token {
		TokenClassID class_id;
		TokenLocation location;
	};

	struct Variable {
		TokenClassID token_class_id;

		// If a variable in a function is not written to inside a function,
		// then it can directly be substituted when inlining.
		// Otherwise we need to make a copy.
		bool is_written = false;
		bool is_used = false;

		uint32_t parameter_id = UINT32_MAX;
		bool is_parameter() const { return parameter_id != UINT32_MAX; }
	};

	struct Variables {
		Variable &get_var(TokenClassID p_class_id);
		Variable &get(uint32_t p_id) { return list[p_id]; }
		const Variable &get(uint32_t p_id) const { return list[p_id]; }
		bool find(TokenClassID p_class_id, uint32_t &r_var_id) const;
		bool already_exists(TokenClassID p_class_id) {
			uint32_t id;
			return find(p_class_id, id);
		}
		bool add_variable(TokenClassID p_class_id, uint32_t p_parameter_id);

		uint32_t get_num_vars() const { return list.size(); }

	private:
		LocalVector<Variable> list;
	};

	struct InlineChange {
		ASTNodeID passed_expression;
		String identifier_from;
		String identifier_to;
		TokenID token_id;
	};

	struct InlineInfo {
		LocalVector<InlineChange> changes;
	};

	struct Line {
		int32_t indent = 0;
		uint32_t source_line = 0;
		uint32_t valid_line = 0;
		String text;
	};

	struct Scope {
		String name;
		uint32_t indent = 0;
		Variables vars;
	};

	struct Body {
	};

	struct FuncParameter {
		TokenID token_id;
		TokenClassID token_class_id;
		String identifier;
		bool is_written = false;
		bool is_used = false;
	};

	struct Func {
		String name;
		Body body;
		uint32_t source_line = 0;
		uint32_t valid_line = 0;
		LocalVector<FuncParameter> params;
		LocalVector<ASTNodeID> ast_nodes;
		bool has_return_value = false;
		//ASTNodeID declaration_ast_node;
	};

	struct Data {
		LocalVector<Func> funcs;
		LocalVector<TokenClass> token_classes;
		LocalVector<Scope> scope_stack;

		LocalVector<Line> lines;

		// Abstract Syntax Tree
		LocalVector<ASTNode> nodes;
		LocalVector<ASTNodeID> function_nodes;

		String source;
		uint32_t source_line = 0;
		uint32_t source_column = 0;

		// Parsing state.
		Func *current_function = nullptr;
		bool error_found = false;
		static uint32_t current_token;

		// Inlining state.
		uint32_t unique_identifier_count = 0;
		TokenID inline_return_token;
		ASTNodeID inline_return_expression;

		LocalVector<Token> tokens;

		bool parse_advance();
		bool parse_finished() const { return current_token >= tokens.size(); }
		uint32_t find_function(String p_func_name);
	} data;

	// Scopes
	Scope &get_current_scope() { return data.scope_stack[data.scope_stack.size() - 1]; }
	const Scope &get_current_scope() const { return data.scope_stack[data.scope_stack.size() - 1]; }

	void scope_stack_push(TokenClassID p_func_identifier_token_id);
	void scope_stack_pop();
	bool scope_stack_variable_exists(uint32_t p_token_id) const;
	bool scope_stack_find_variable(uint32_t p_token_id, uint32_t &r_scope_id, uint32_t &r_var_id) const;
	////////////////////////////////////////////////////////////////////

	ASTNodeID ast_create_node(ASTNodeID p_parent, ASTType p_type, TokenID p_token_id = TokenID(), ASTNodeID p_left_child = ASTNodeID(), ASTNodeID p_right_child = ASTNodeID());
	ASTNode &ast_get_node(ASTNodeID p_node_id);
	void ast_replace_child(ASTNodeID p_old_child, ASTNodeID p_new_child);
	void ast_add_child(ASTNodeID p_parent, ASTNodeID p_child);
	////////////////////////////////////////////////////////////////////

	// Parser
	const Token &get_token(TokenID p_id) const { return data.tokens[p_id.id()]; }
	Token &get_token(TokenID p_id) { return data.tokens[p_id.id()]; }
	const TokenClass &get_token_class(TokenID p_id) const { return data.token_classes[get_token(p_id).class_id.id()]; }
	TokenID get_current_token_id() const;
	TokenID get_previous_token_id() const;
	const Token &get_current_token() const;
	const Token &get_previous_token() const;
	const TokenClass *get_current_token_class() const;
	const TokenClass *get_previous_token_class() const;

	bool add_previous_token_as_local_variable(bool p_error_on_existing, bool p_is_parameter);

	bool is_parse_finished() const { return data.parse_finished(); }

	ASTNodeID parse_accept_variable(ASTNodeID p_parent, bool p_flag_not_found = false);
	ASTNodeID parse_accept_identifier_function(ASTNodeID p_parent);

	bool accept(TokenType p_token_type);
	bool expect(TokenType p_token_type);
	void backup() { data.current_token -= 1; }

	ASTNodeID parse_declare_func_params(ASTNodeID p_parent);
	ASTNodeID parse_func_params(ASTNodeID p_parent);

	ASTNodeID syntax_error(const char *p_message = nullptr);

	ASTNodeID parse_factor(ASTNodeID p_parent);
	ASTNodeID parse_term(ASTNodeID p_parent);
	ASTNodeID parse_expression(ASTNodeID p_parent);
	ASTNodeID parse_statement(ASTNodeID p_parent, uint32_t p_depth);
	ASTNodeID parse_block(ASTNodeID p_parent, uint32_t p_depth);
	ASTNodeID parse_block_loop(ASTNodeID p_parent, uint32_t p_depth);

	bool parse_file();
	String create_output();
	////////////////////////////////////////////////////////////////////

	// Strings
	void debug_print_local_tokens();
	void debug_print_ast_node(ASTNodeID p_node_id, int32_t p_depth);

	void output_ast_node(ASTNodeID p_node_id, String &r_output, uint32_t p_indent, bool p_newline);

	String get_token_class_as_string(const TokenClass &p_class);
	String get_token_as_string(const Token &p_token);

	////////////////////////////////////////////////////////////////////
	// Lexer
	bool _is_char(CharType c) const;
	bool _is_num(CharType c) const;
	bool _is_delimiter(CharType c, String p_delimiters) const;

	Token add_token(const TokenLocation &p_token_location, const TokenClass &p_token);
	Token add_predefined_token(const TokenLocation &p_token_location, TokenType p_tt);

	bool lex_token_string(const TokenLocation &p_token_location, const String &p_text, int32_t &r_pos, int32_t p_length);
	bool lex_identifier(const TokenLocation &p_token_location, const String &p_text, int32_t &r_pos, int32_t p_length);

	bool is_token_number(const String &p_text, int32_t &r_pos, int32_t p_length, GDScriptPreProcessor::TokenType &r_token_type, String &r_num_string);

	bool lex_predefined_token(const TokenLocation &p_token_location, int32_t &r_pos, const String &p_text, const String &p_searchstring, TokenType p_type, bool p_store_search_string);
	bool lex_predefined_keyword(const TokenLocation &p_token_location, int32_t &r_pos, const String &p_text, const String &p_searchstring, TokenType p_type, int32_t p_text_length, bool p_store_search_string);

	bool lex_tokens(Line &r_line);
	bool lex_line(Line &r_line);

	//int32_t find_func(int32_t p_pos);

	bool lex_file();
	bool lex_function(uint32_t p_function);
	//void prefind_functions();
	////////////////////////////////////////////////////////////////////

	bool eat_whitespace(const String &p_string, int32_t &r_pos);

	bool read_token(const String &p_string, int32_t &r_pos, String &r_token, CharType &r_delimiter, const String &p_disallow_start, const String &p_delimiters);

	bool read_line_and_strip_comments(Line &r_line);
	//void read_body(Body &r_body, const String &p_text);
	bool read_param(const Line &p_line, Func &r_func, int32_t &r_pos, bool &r_finished);
	bool read_function(const Line &p_line);
	void read_file();

	////////////////////////////////////////////////////////////////////

	//void search_for_inlines(uint32_t p_func_id);
	struct InlineCall {
		ASTNodeID call;
		ASTNodeID parent;
	};

	ASTNodeID inline_add_var_declaration(ASTNodeID p_parent, String p_var_name, ASTNodeID p_assign_var = ASTNodeID(), TokenID *r_token_id = nullptr);

	void search_for_inlines(ASTNodeID p_node_id, ASTNodeID p_parent_node_id, uint32_t p_parent_func_id, LocalVector<InlineCall> &r_calls);
	void create_inlines();
	void make_inline(InlineCall p_call);
	void duplicate_inline(ASTNodeID p_source_id, ASTNodeID p_dest_id, ASTNodeID p_dest_parent_id, InlineInfo &r_inline_info);

	void make_inline_params(const Func &p_source_func, ASTNodeID p_parent, InlineInfo &r_info);

public:
	String process(const String &p_source);
	GDScriptPreProcessor();
};

#if 0
An Abstract Syntax Tree (AST) represents the syntactic structure of source code in a hierarchical, tree-like form. The specific node types in an AST depend on the programming language, but there are common node types that appear across many languages due to shared programming constructs. Below is a list of typical AST node types found in many general-purpose programming languages (e.g., C, JavaScript, Python, Java, etc.):Common AST Node TypesProgram/SourceFile: The root node representing the entire program or source file.Contains a list of top-level nodes (e.g., statements, declarations).

Statements:ExpressionStatement: A statement that evaluates an expression (e.g., x = 5;).
BlockStatement: A block of statements enclosed in braces {}.
IfStatement: Represents an if conditional with condition, consequent, and optional alternate (else) branches.
LoopStatement:ForLoop: Represents a for loop with initialization, condition, update, and body.
WhileLoop: Represents a while loop with a condition and body.
DoWhileLoop: Represents a do-while loop.

ReturnStatement: A statement that returns a value from a function.
BreakStatement: Exits a loop or switch.
ContinueStatement: Skips to the next iteration of a loop.
SwitchStatement: Represents a switch statement with cases and a default branch.
TryCatchStatement: Represents exception handling with try, catch, and optional finally blocks.

Declarations:VariableDeclaration: Declares a variable (e.g., let x = 5;).May include const, let, var in languages like JavaScript.

FunctionDeclaration: Declares a function with a name, parameters, and body.
ClassDeclaration: Declares a class with properties and methods (in object-oriented languages).
ImportDeclaration: Represents import statements for modules (e.g., import x from 'y';).

Expressions:Literal: Represents constant values like numbers, strings, booleans, or null (e.g., 42, "hello", true).
Identifier: Represents variable names or references (e.g., x in x = 5).
BinaryExpression: Represents binary operations (e.g., a + b, x * y).
UnaryExpression: Represents unary operations (e.g., -x, !flag).
AssignmentExpression: Assigns a value to a variable (e.g., x = 5).
CallExpression: Represents a function call (e.g., foo(1, 2)).
MemberExpression: Represents access to an object property or array element (e.g., obj.prop, arr[0]).
ConditionalExpression: Represents a ternary operator (e.g., x ? y : z).
ArrowFunctionExpression: Represents an arrow function in languages like JavaScript (e.g., x => x + 1).
NewExpression: Represents object creation with a constructor (e.g., new Date()).

Literals and Constants:NumericLiteral: Represents a number (e.g., 42, 3.14).
StringLiteral: Represents a string (e.g., "hello").
BooleanLiteral: Represents true or false.
NullLiteral: Represents null or equivalent.
ArrayLiteral: Represents an array (e.g., [1, 2, 3]).
ObjectLiteral: Represents an object (e.g., { key: value }).

Control Structures:CaseClause: Represents a case in a switch statement.
CatchClause: Represents a catch block in a try-catch statement.
DefaultClause: Represents the default case in a switch statement.

Function and Class Related:Parameter: Represents a function parameter.
MethodDefinition: Represents a method within a class or object.
Property: Represents a key-value pair in an object literal.
Super: Represents a reference to a parent class (e.g., super() in JavaScript).

Module-Related:ExportNamedDeclaration: Exports specific variables or functions from a module.
ExportDefaultDeclaration: Exports a default value from a module.
ImportSpecifier: Specifies imported bindings in an import statement.

Miscellaneous:ThisExpression: Represents the this keyword.
SpreadElement: Represents spread/rest syntax (e.g., ...args in JavaScript).
TemplateLiteral: Represents template strings (e.g., `hello ${name}` in JavaScript).
TaggedTemplateExpression: Represents a tagged template literal.
AwaitExpression: Represents an await operation in asynchronous code.
YieldExpression: Represents a yield operation in generator functions.

Language-Specific VariationsJavaScript (ESTree Spec): Includes nodes like ArrowFunctionExpression, SpreadElement, JSXElement (for JSX in React), and TemplateLiteral.
Python (ast module): Includes nodes like Module, Name, Call, Attribute, List, Dict, and Comprehension.
Java (Eclipse JDT): Includes nodes like CompilationUnit, MethodDeclaration, SingleVariableDeclaration, and InfixExpression.
C/C++ (Clang AST): Includes nodes like TranslationUnitDecl, FunctionDecl, CompoundStmt, and BinaryOperator.

NotesGranularity: Some ASTs are more fine-grained (e.g., separating BinaryOperator into Add, Subtract, etc.), while others are more abstract.
Extensions: Languages with unique features (e.g., JSX in JavaScript, list comprehensions in Python) introduce specialized node types.
Tooling: The exact node names and structure depend on the parser or compiler (e.g., ESTree for JavaScript, Clang for C/C++, Python’s ast module).
#endif
