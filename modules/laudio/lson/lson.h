#pragma once

#include "core/local_vector.h"
#include "core/os/file_access.h"
#include "core/ustring.h"
#include <stdint.h>

namespace LSon {

struct Node {
	String name;
	enum Type {
		//		NT_U32,
		//		NT_S32,
		NT_UNKNOWN,
		NT_U64,
		NT_S64,
		//		NT_F32,
		//		NT_F64,
		NT_STRING,
		NT_ARRAY,
	};
	Type type = NT_UNKNOWN;
	union {
		uint64_t u64 = 0;
		int64_t s64;
		//		float f32;
		//		double f64;
	} val;
	String string;
	LocalVector<Node *> children;

	Node *get_child(uint32_t p_child) {
		ERR_FAIL_COND_V(p_child >= children.size(), nullptr);
		return children[p_child];
	}

	//	uint64_t get_u64() const
	//	{
	//		return val.u64;
	//	}

	void set_u64(uint64_t p_val) {
		val.u64 = p_val;
		type = NT_U64;
	}
	void set_s64(int64_t p_val) {
		val.s64 = p_val;
		type = NT_S64;
	}
	void set_array(String p_string = "") {
		string = p_string;
		type = NT_ARRAY;
	}
	void set_string(String p_string) {
		string = p_string;
		type = NT_STRING;
	}
	void set_name(String p_name) {
		name = p_name;
	}

	Node *request_child() {
		uint32_t id = children.size();
		children.resize(id + 1);
		children[id] = memnew(Node);
		return children[id];
	}

	bool load(FileAccess *p_file, int32_t p_depth = 0);
	bool save(FileAccess *p_file, int32_t p_depth = 0);
	bool load(String p_filename);
	bool save(String p_filename);

	Node *find_node(String p_name);

	~Node();

private:
	enum TokenType {
		TT_UNKNOWN,
		TT_U64,
		TT_S64,
		TT_STRING,
		TT_OPEN_CHILDREN,
		TT_CLOSE_CHILDREN,
		TT_OPEN_ARRAY,
		TT_CLOSE_ARRAY,
		TT_COMMA,
		TT_COLON,
		TT_EOF,
	};

	struct Token {
		TokenType type;
		String string;
		Token(TokenType p_type, const char *p_string = nullptr) {
			type = p_type;
			if (p_string) {
				string = String::utf8(p_string);
			}
		}
	};

	bool _load_children(FileAccess *p_file, int32_t p_depth);
	String _tabs(int32_t p_num_tabs);
	Token _load_token(FileAccess *p_file, bool rewind_if_not_close_bracket = false);
	bool _eat_whitespace(FileAccess *p_file, CharType &r_char);
};

} //namespace LSon
