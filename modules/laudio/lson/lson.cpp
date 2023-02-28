#include "lson.h"

namespace LSon {

Node::~Node() {
	for (uint32_t n = 0; n < children.size(); n++) {
		memdelete(children[n]);
	}
	children.clear();
}

Node *Node::find_node(String p_name) {
	if (name == p_name) {
		return this;
	}

	for (uint32_t n = 0; n < children.size(); n++) {
		Node *found = children[n]->find_node(p_name);
		if (found) {
			return found;
		}
	}

	return nullptr;
}

bool Node::load(FileAccess *p_file, int32_t p_depth) {
	Token t = _load_token(p_file);
	switch (t.type) {
		case TT_EOF: {
			// should never encounter EOF here unless there is an error
			return false;
		} break;
		case TT_OPEN_ARRAY: {
			type = NT_ARRAY;
			return _load_children(p_file, p_depth + 1);
		} break;
		case TT_OPEN_CHILDREN: {
			return _load_children(p_file, p_depth + 1);
		} break;
		default: {
		} break;
	}

	Token t2 = _load_token(p_file);
	if (t2.type == TT_EOF) {
		// should never encounter EOF here unless there is an error
		return false;
	}

	// if a string, either single or double
	if ((t.type == TT_STRING) && (t2.type == TT_COLON)) {
		// the first string is the name
		set_name(t.string);
		print_line(_tabs(p_depth) + "loading node name: " + name);

		// recursive call this routine
		return load(p_file, p_depth);
	}

	// load the value in t
	switch (t.type) {
		case TT_STRING: {
			set_string(t.string);
			print_line(_tabs(p_depth) + "string is " + string);
		} break;
		case TT_U64: {
			Variant v = t.string;
			set_u64(v);
			print_line(_tabs(p_depth) + "u64 is " + itos(val.u64));
		} break;
		case TT_S64: {
			Variant v = t.string;
			set_s64(v);
			print_line(_tabs(p_depth) + "s64 is " + itos(val.s64));
		} break;
			//		case TT_OPEN_ARRAY:
			//		case TT_OPEN_CHILDREN: {
			//			// there is no value to load, but we are loading children
			//			t2 = t;
			//		} break;

		default: {
			// ERROR
			DEV_ASSERT(0);
			return false;
		} break;
	}

	// the node either has children, starts array children,
	// or has a comma
	switch (t2.type) {
		case TT_COMMA: {
			// loaded ok, no children
			return true;
		} break;
		case TT_OPEN_CHILDREN:
		case TT_OPEN_ARRAY: {
			return _load_children(p_file, p_depth + 1);
		} break;

		default: {
			// ERROR
			DEV_ASSERT(0);
			return false;
		} break;
	}

	//	while (true) {
	//		sz = "";
	//		TokenType tt = _load_token(p_file, sz);
	//		if (tt == TT_EOF) {
	//			break;
	//		}

	//		print_line("Token : " + sz);
	//	}

	return true;
}

bool Node::_load_children(FileAccess *p_file, int32_t p_depth) {
	print_line(_tabs(p_depth) + "start children");
	while (true) {
		Token t3 = _load_token(p_file, true);
		switch (t3.type) {
			case TT_CLOSE_CHILDREN:
			case TT_CLOSE_ARRAY: {
				print_line(_tabs(p_depth) + "end children");
				return true;
			} break;
			case TT_EOF: {
				// ERROR
				DEV_ASSERT(0);
				return false;
			} break;
			default: {
				// a child
				Node *child = request_child();
				if (!child->load(p_file, p_depth)) {
					return false;
				}
			} break;
		} // switch t3 type

	} // while

	return true;
}

bool Node::_eat_whitespace(FileAccess *p_file, CharType &r_char) {
	r_char = p_file->get_8();
	while (!p_file->eof_reached()) {
		switch (r_char) {
			case ' ':
			case '\t':
			case '\n': {
			} break;
			default: {
				// not whitespace
				//p_file->seek(p_file->get_position() - 1);
				return true;
			} break;
		}
		r_char = p_file->get_8();
	}

	return false;
}

Node::Token Node::_load_token(FileAccess *p_file, bool rewind_if_not_close_bracket) {
	uint64_t old_pos = p_file->get_position();

	CharType ch;
	if (!_eat_whitespace(p_file, ch)) {
		return Token(TT_EOF);
	}

	// the token depends on the first char
	switch (ch) {
		case ',': {
			return Token(TT_COMMA);
		} break;
		case ':': {
			return Token(TT_COLON);
		} break;
		case '{': {
			return Token(TT_OPEN_CHILDREN);
		} break;
		case '}': {
			return Token(TT_CLOSE_CHILDREN);
		} break;
		case '[': {
			return Token(TT_OPEN_ARRAY);
		} break;
		case ']': {
			return Token(TT_CLOSE_ARRAY);
		} break;
		case 'u': {
			Token temp = _load_token(p_file);
			temp.type = (temp.type == TT_STRING) ? TT_U64 : TT_UNKNOWN;
			if (rewind_if_not_close_bracket) {
				p_file->seek(old_pos);
			}
			return temp;
		} break;
		case '#': {
			Token temp = _load_token(p_file);
			temp.type = (temp.type == TT_STRING) ? TT_S64 : TT_UNKNOWN;
			if (rewind_if_not_close_bracket) {
				p_file->seek(old_pos);
			}
			return temp;
		} break;
		case '"': {
			// pass through, it is a string starting
		} break;
		default: {
			return TT_UNKNOWN;
		} break;
	}

	CharString token;
	CharType c = p_file->get_8();
	while (!p_file->eof_reached()) {
		if (c == '"') {
			if (rewind_if_not_close_bracket) {
				p_file->seek(old_pos);
			}
			return Token(TT_STRING, token.get_data());
		}
		token += c;

		c = p_file->get_8();
	}

	if (rewind_if_not_close_bracket) {
		p_file->seek(old_pos);
	}
	return Token(TT_UNKNOWN);
}

//			// read through whitespace
//			String token = p_file->get_token();
//			print_line("token : " + token);
//			return TT_UNKNOWN;
//		}

String Node::_tabs(int32_t p_num_tabs) {
	String line;
	for (int32_t n = 0; n < p_num_tabs; n++) {
		line += "\t";
	}
	return line;
}

bool Node::load(String p_filename) {
	FileAccess *file = FileAccess::open(p_filename, FileAccess::READ);
	if (file) {
		bool result = load(file);
		memdelete(file);
		return result;
	}
	return false;
}

bool Node::save(String p_filename) {
	FileAccess *file = FileAccess::open(p_filename, FileAccess::WRITE);
	if (file) {
		bool result = save(file);
		memdelete(file);
		file = nullptr;
		return result;
	}
	return false;
}

bool Node::save(FileAccess *p_file, int32_t p_depth) {
	String line = _tabs(p_depth);
	if (name != "") {
		line += "\"" + name + "\": ";
	}

	bool needs_spacer = false;

	switch (type) {
		case NT_U64: {
			line += "u\"" + itos(val.u64) + "\"";
			needs_spacer = true;
		} break;
		case NT_S64: {
			line += "#\"" + itos(val.s64) + "\"";
			needs_spacer = true;
		} break;
		case NT_STRING: {
			line += "\"" + string + "\"";
			needs_spacer = true;
		} break;
		case NT_ARRAY: {
			if (string != "") {
				line += "\"" + string + "\"";
				needs_spacer = true;
			}
		} break;
		case NT_UNKNOWN: {
			;
			//			return false;
		} break;

		default:
			DEV_ASSERT(0);
			break;
	}

	bool ok = true;

	if (children.size()) {
		if (needs_spacer) {
			line += " ";
		}
		if (type != NT_ARRAY) {
			line += "{\n";
		} else {
			line += "[\n";
		}
		p_file->store_string(line);

		for (uint32_t n = 0; n < children.size(); n++) {
			DEV_ASSERT(children[n]);
			if (!children[n]->save(p_file, p_depth + 1)) {
				ok = false;
			}
		}

		line = _tabs(p_depth);
		if (type != NT_ARRAY) {
			line += "}\n";
		} else {
			line += "]\n";
		}
		p_file->store_string(line);
	} else {
		p_file->store_string(line + ",\n");
	}

	return ok;
}

} //namespace LSon
