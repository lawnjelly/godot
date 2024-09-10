#pragma once

#include "navphysics_typedefs.h"

namespace NavPhysics {
class String;

enum ErrorHandlerType {
	NP_ERR_HANDLER_ERROR,
	NP_ERR_HANDLER_WARNING,
	NP_ERR_HANDLER_SCRIPT,
	NP_ERR_HANDLER_SHADER,
};

void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, ErrorHandlerType p_type = NP_ERR_HANDLER_ERROR);
void _NP_ERR_print_index_error(const char *p_function, const char *p_file, int p_line, i64 p_index, i64 p_size, const char *p_index_str, const char *p_size_str, const char *p_message = "", bool fatal = false);
void _NP_ERR_print_index_error(const char *p_function, const char *p_file, int p_line, i64 p_index, i64 p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool fatal = false);
void _NP_ERR_flush_stdout();

#ifndef _STR
#define _STR(m_x) #m_x
#define _MKSTR(m_x) _STR(m_x)
#endif

#define _FNL __FILE__ ":"

/** An index has failed if m_index<0 or m_index >=m_size, the function exits */

#ifdef __GNUC__
//#define FUNCTION_STR __PRETTY_FUNCTION__ - too annoying
#define FUNCTION_STR __FUNCTION__
#else
#define FUNCTION_STR __FUNCTION__
#endif

// Don't use this directly; instead, use any of the NP_CRASH_* macros
#ifdef _MSC_VER
#define GENERATE_TRAP                       \
	__debugbreak();                         \
	/* Avoid warning about control paths */ \
	for (;;) {                              \
	}
#else
#define GENERATE_TRAP __builtin_trap();
#endif

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`, prints a generic
 * error message and returns from the function. This macro should be preferred to
 * `NP_NP_ERR_FAIL_COND` for bounds checking.
 */
#define NP_NP_ERR_FAIL_INDEX(m_index, m_size)                                                                      \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                             \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size)); \
		return;                                                                                                    \
	} else                                                                                                         \
		((void)0)

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`, prints a custom
 * error message and returns from the function. This macro should be preferred to
 * `NP_NP_ERR_FAIL_COND_MSG` for bounds checking.
 */
#define NP_NP_ERR_FAIL_INDEX_MSG(m_index, m_size, m_msg)                                                                  \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                                    \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), m_msg); \
		return;                                                                                                           \
	} else                                                                                                                \
		((void)0)

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`,
 * prints a generic error message and returns the value specified in `m_retval`.
 * This macro should be preferred to `NP_NP_ERR_FAIL_COND_V` for bounds checking.
 */
#define NP_NP_ERR_FAIL_INDEX_V(m_index, m_size, m_retval)                                                          \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                             \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size)); \
		return m_retval;                                                                                           \
	} else                                                                                                         \
		((void)0)

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`,
 * prints a custom error message and returns the value specified in `m_retval`.
 * This macro should be preferred to `NP_NP_ERR_FAIL_COND_V_MSG` for bounds checking.
 */
#define NP_NP_ERR_FAIL_INDEX_V_MSG(m_index, m_size, m_retval, m_msg)                                                      \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                                    \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), m_msg); \
		return m_retval;                                                                                                  \
	} else                                                                                                                \
		((void)0)

/**
 * If `m_index` is greater than or equal to `m_size`,
 * prints a generic error message and returns the value specified in `m_retval`.
 * This macro should be preferred to `NP_NP_ERR_FAIL_COND_V` for unsigned bounds checking.
 */
#define NP_NP_ERR_FAIL_UNSIGNED_INDEX(m_index, m_size)                                                             \
	if (navphysics_unlikely((m_index) >= (m_size))) {                                                              \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size)); \
		return;                                                                                                    \
	} else                                                                                                         \
		((void)0)

/**
 * If `m_index` is greater than or equal to `m_size`,
 * prints a generic error message and returns the value specified in `m_retval`.
 * This macro should be preferred to `NP_NP_ERR_FAIL_COND_V` for unsigned bounds checking.
 */
#define NP_NP_ERR_FAIL_UNSIGNED_INDEX_V(m_index, m_size, m_retval)                                                 \
	if (navphysics_unlikely((m_index) >= (m_size))) {                                                              \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size)); \
		return m_retval;                                                                                           \
	} else                                                                                                         \
		((void)0)

/**
 * If `m_index` is greater than or equal to `m_size`,
 * prints a custom error message and returns the value specified in `m_retval`.
 * This macro should be preferred to `NP_NP_ERR_FAIL_COND_V_MSG` for unsigned bounds checking.
 */
#define NP_NP_ERR_FAIL_UNSIGNED_INDEX_V_MSG(m_index, m_size, m_retval, m_msg)                                             \
	if (navphysics_unlikely((m_index) >= (m_size))) {                                                                     \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), m_msg); \
		return m_retval;                                                                                                  \
	} else                                                                                                                \
		((void)0)

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`,
 * crashes the engine immediately with a generic error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 * This macro should be preferred to `NP_CRASH_COND` for bounds checking.
 */
#define NP_CRASH_BAD_INDEX(m_index, m_size)                                                                                  \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                                       \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), "", true); \
		_NP_ERR_flush_stdout();                                                                                              \
		GENERATE_TRAP                                                                                                        \
	} else                                                                                                                   \
		((void)0)

/**
 * If `m_index` is less than 0 or greater than or equal to `m_size`,
 * crashes the engine immediately with a custom error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 * This macro should be preferred to `NP_CRASH_COND` for bounds checking.
 */
#define NP_CRASH_BAD_INDEX_MSG(m_index, m_size, m_msg)                                                                          \
	if (navphysics_unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                                          \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), m_msg, true); \
		_NP_ERR_flush_stdout();                                                                                                 \
		GENERATE_TRAP                                                                                                           \
	} else                                                                                                                      \
		((void)0)

/**
 * If `m_index` is greater than or equal to `m_size`,
 * crashes the engine immediately with a generic error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 * This macro should be preferred to `NP_CRASH_COND` for bounds checking.
 */
#define NP_CRASH_BAD_UNSIGNED_INDEX(m_index, m_size)                                                                         \
	if (navphysics_unlikely((m_index) >= (m_size))) {                                                                        \
		_NP_ERR_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size), "", true); \
		_NP_ERR_flush_stdout();                                                                                              \
		GENERATE_TRAP                                                                                                        \
	} else                                                                                                                   \
		((void)0)

/**
 * If `m_param` is `null`, prints a generic error message and returns from the function.
 */
#define NP_NP_ERR_FAIL_NULL(m_param)                                                                       \
	if (navphysics_unlikely(!m_param)) {                                                                   \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null."); \
		return;                                                                                            \
	} else                                                                                                 \
		((void)0)

/**
 * If `m_param` is `null`, prints a custom error message and returns from the function.
 */
#define NP_NP_ERR_FAIL_NULL_MSG(m_param, m_msg)                                                                   \
	if (navphysics_unlikely(!m_param)) {                                                                          \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		return;                                                                                                   \
	} else                                                                                                        \
		((void)0)

/**
 * If `m_param` is `null`, prints a generic error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_NULL_V(m_param, m_retval)                                                           \
	if (navphysics_unlikely(!m_param)) {                                                                   \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null."); \
		return m_retval;                                                                                   \
	} else                                                                                                 \
		((void)0)

/**
 * If `m_param` is `null`, prints a custom error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_NULL_V_MSG(m_param, m_retval, m_msg)                                                       \
	if (navphysics_unlikely(!m_param)) {                                                                          \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		return m_retval;                                                                                          \
	} else                                                                                                        \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a generic error message and returns from the function.
 */
#define NP_NP_ERR_FAIL_COND(m_cond)                                                                       \
	if (navphysics_unlikely(m_cond)) {                                                                    \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true."); \
		return;                                                                                           \
	} else                                                                                                \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a custom error message and returns from the function.
 */
#define NP_NP_ERR_FAIL_COND_MSG(m_cond, m_msg)                                                                   \
	if (navphysics_unlikely(m_cond)) {                                                                           \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		return;                                                                                                  \
	} else                                                                                                       \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, crashes the engine immediately with a generic error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 */
#define NP_CRASH_COND(m_cond)                                                                                    \
	if (navphysics_unlikely(m_cond)) {                                                                           \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: Condition \"" _STR(m_cond) "\" is true."); \
		_NP_ERR_flush_stdout();                                                                                  \
		GENERATE_TRAP                                                                                            \
	} else                                                                                                       \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, crashes the engine immediately with a custom error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 */
#define NP_CRASH_COND_MSG(m_cond, m_msg)                                                                                \
	if (navphysics_unlikely(m_cond)) {                                                                                  \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		_NP_ERR_flush_stdout();                                                                                         \
		GENERATE_TRAP                                                                                                   \
	} else                                                                                                              \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a generic error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_COND_V(m_cond, m_retval)                                                                                     \
	if (navphysics_unlikely(m_cond)) {                                                                                              \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returned: " _STR(m_retval)); \
		return m_retval;                                                                                                            \
	} else                                                                                                                          \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a custom error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                 \
	if (navphysics_unlikely(m_cond)) {                                                                                                     \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returned: " _STR(m_retval), m_msg); \
		return m_retval;                                                                                                                   \
	} else                                                                                                                                 \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a custom error message and continues the loop the macro is located in.
 */
#define NP_ERR_CONTINUE(m_cond)                                                                                       \
	if (navphysics_unlikely(m_cond)) {                                                                                \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Continuing."); \
		continue;                                                                                                     \
	} else                                                                                                            \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a custom error message and continues the loop the macro is located in.
 */
#define NP_ERR_CONTINUE_MSG(m_cond, m_msg)                                                                                   \
	if (navphysics_unlikely(m_cond)) {                                                                                       \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Continuing.", m_msg); \
		continue;                                                                                                            \
	} else                                                                                                                   \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a generic error message and breaks from the loop the macro is located in.
 */
#define NP_ERR_BREAK(m_cond)                                                                                        \
	if (navphysics_unlikely(m_cond)) {                                                                              \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Breaking."); \
		break;                                                                                                      \
	} else                                                                                                          \
		((void)0)

/**
 * If `m_cond` evaluates to `true`, prints a custom error message and breaks from the loop the macro is located in.
 */
#define NP_ERR_BREAK_MSG(m_cond, m_msg)                                                                                    \
	if (navphysics_unlikely(m_cond)) {                                                                                     \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Breaking.", m_msg); \
		break;                                                                                                             \
	} else                                                                                                                 \
		((void)0)

/**
 * Prints a generic error message and returns from the function.
 */
#define NP_ERR_FAIL()                                                            \
	if (true) {                                                                  \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method failed."); \
		return;                                                                  \
	} else                                                                       \
		((void)0)

/**
 * Prints a custom error message and returns from the function.
 */
#define NP_NP_ERR_FAIL_MSG(m_msg)                                                       \
	if (true) {                                                                         \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method failed.", m_msg); \
		return;                                                                         \
	} else                                                                              \
		((void)0)

/**
 * Prints a generic error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_V(m_retval)                                                                           \
	if (true) {                                                                                              \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method failed. Returning: " __STR(m_retval)); \
		return m_retval;                                                                                     \
	} else                                                                                                   \
		((void)0)

/**
 * Prints a custom error message and returns the value specified in `m_retval`.
 */
#define NP_NP_ERR_FAIL_V_MSG(m_retval, m_msg)                                                                       \
	if (true) {                                                                                                     \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method failed. Returning: " __STR(m_retval), m_msg); \
		return m_retval;                                                                                            \
	} else                                                                                                          \
		((void)0)

/**
 * Crashes the engine immediately with a generic error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 */
#define NP_CRASH_NOW()                                                                  \
	if (true) {                                                                         \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: Method failed."); \
		_NP_ERR_flush_stdout();                                                         \
		GENERATE_TRAP                                                                   \
	} else                                                                              \
		((void)0)

/**
 * Crashes the engine immediately with a custom error message.
 * Only use this if there's no sensible fallback (i.e. the error is unrecoverable).
 */
#define NP_CRASH_NOW_MSG(m_msg)                                                                \
	if (true) {                                                                                \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: Method failed.", m_msg); \
		_NP_ERR_flush_stdout();                                                                \
		GENERATE_TRAP                                                                          \
	} else                                                                                     \
		((void)0)

/**
 * Prints an error message without returning.
 */
#define NP_ERR_PRINT(m_string) \
	_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, m_string)

/**
 * Prints an error message without returning, but only do so once in the application lifecycle.
 * This can be used to avoid spamming the console with error messages.
 */
#define NP_ERR_PRINT_ONCE(m_string)                                          \
	if (true) {                                                              \
		static bool first_print = true;                                      \
		if (first_print) {                                                   \
			_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, m_string); \
			first_print = false;                                             \
		}                                                                    \
	} else                                                                   \
		((void)0)

/**
 * Prints a warning message without returning. To warn about deprecated usage,
 * use `NP_WARN_DEPRECATED` or `NP_WARN_DEPRECATED_MSG` instead.
 */
#define NP_WARN_PRINT(m_string) \
	_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, m_string, NP_ERR_HANDLER_WARNING)

/**
 * Prints a warning message without returning, but only do so once in the application lifecycle.
 * This can be used to avoid spamming the console with warning messages.
 */
#define NP_WARN_PRINT_ONCE(m_string)                                                                 \
	if (true) {                                                                                      \
		static bool first_print = true;                                                              \
		if (first_print) {                                                                           \
			_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, m_string, NP_ERR_HANDLER_WARNING); \
			first_print = false;                                                                     \
		}                                                                                            \
	} else                                                                                           \
		((void)0)

/**
 * Prints a generic deprecation warning message without returning.
 * This should be preferred to `NP_WARN_PRINT` for deprecation warnings.
 */
#define NP_WARN_DEPRECATED                                                                                                                                       \
	if (true) {                                                                                                                                                  \
		static SafeFlag warning_shown;                                                                                                                           \
		if (!warning_shown.is_set()) {                                                                                                                           \
			_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "This method has been deprecated and will be removed in the future.", NP_ERR_HANDLER_WARNING); \
			warning_shown.set();                                                                                                                                 \
		}                                                                                                                                                        \
	} else                                                                                                                                                       \
		((void)0)

/**
 * Prints a custom deprecation warning message without returning.
 * This should be preferred to `NP_WARN_PRINT` for deprecation warnings.
 */
#define NP_WARN_DEPRECATED_MSG(m_msg)                                                                                                                                   \
	if (true) {                                                                                                                                                         \
		static SafeFlag warning_shown;                                                                                                                                  \
		if (!warning_shown.is_set()) {                                                                                                                                  \
			_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "This method has been deprecated and will be removed in the future.", m_msg, NP_ERR_HANDLER_WARNING); \
			warning_shown.set();                                                                                                                                        \
		}                                                                                                                                                               \
	} else                                                                                                                                                              \
		((void)0)

/**
 * This should be a 'free' assert for program flow and should not be needed in any releases,
 *  only used in dev builds.
 */
#ifdef NP_DEV_ENABLED
#define NP_DEV_ASSERT(m_cond)                                                                                                 \
	if (navphysics_unlikely(!(m_cond))) {                                                                                     \
		_NP_ERR_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: NP_DEV_ASSERT failed  \"" _STR(m_cond) "\" is false."); \
		_NP_ERR_flush_stdout();                                                                                               \
		GENERATE_TRAP                                                                                                         \
	} else                                                                                                                    \
		((void)0)
#else
#define NP_DEV_ASSERT(m_cond)
#endif

/**
 * These should be 'free' checks for program flow and should not be needed in any releases,
 *  only used in dev builds.
 */
#ifdef NP_DEV_ENABLED
#define NP_DEV_CHECK(m_cond)                                                 \
	if (navphysics_unlikely(!(m_cond))) {                                    \
		NP_ERR_PRINT("NP_DEV_CHECK failed  \"" _STR(m_cond) "\" is false."); \
	} else                                                                   \
		((void)0)
#else
#define NP_DEV_CHECK(m_cond)
#endif

#ifdef NP_DEV_ENABLED
#define NP_DEV_CHECK_ONCE(m_cond)                                                      \
	if (navphysics_unlikely(!(m_cond))) {                                              \
		NP_ERR_PRINT_ONCE("NP_DEV_CHECK_ONCE failed  \"" _STR(m_cond) "\" is false."); \
	} else                                                                             \
		((void)0)
#else
#define NP_DEV_CHECK_ONCE(m_cond)
#endif

} //namespace NavPhysics
