#include "navphysics_error.h"

#include "navphysics_log.h"

namespace NavPhysics {

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error);
}

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error);
}

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error + ", " + p_message);
}

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error + ", " + p_message);
}

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error + ", " + p_message);
}

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, ErrorHandlerType p_type) {
	log(String(p_function) + " " + p_file + " " + p_line + " " + p_error + ", " + p_message);
}

void _NP_ERR_print_index_error(const char *p_function, const char *p_file, int p_line, i64 p_index, i64 p_size, const char *p_index_str, const char *p_size_str, const char *p_message, bool fatal) {
	log(String(p_function) + " " + p_file + " " + p_line + " index error : " + p_index + ", size " + p_size);
}

void _NP_ERR_print_index_error(const char *p_function, const char *p_file, int p_line, i64 p_index, i64 p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool fatal) {
	log(String(p_function) + " " + p_file + " " + p_line + " index error : " + p_index + ", size " + p_size);
}

void _NP_ERR_flush_stdout() {
}

} //namespace NavPhysics
