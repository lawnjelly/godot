#pragma once

#include "navphysics_string.h"
#include "navphysics_typedefs.h"

#ifdef NP_DEV_ENABLED

#if 0
#define NP_LOG_ACTIVE
#endif

#if 0
#define NP_LLOG_ACTIVE
#endif

#endif

#ifdef NP_LOG_ACTIVE
#define NP_LOG(a) log(a)
#else
#define NP_LOG(a)
#endif

#ifdef NP_LLOG_ACTIVE
#define NP_LLOG(a) llog(a)
#else
#define NP_LLOG(a)
#endif

#ifdef NP_LLOG_ACTIVE
#define NP_LLOG2(a, b) llog(a, b)
#else
#define NP_LLOG2(a, b)
#endif

namespace NavPhysics {
typedef void (*np_log_callback)(const char *userdata);

void set_log_callback(np_log_callback p_callback);

void loga(String p_sz, u32 p_priority = 0);
void log(String p_sz, u32 p_depth = 0, u32 p_priority = 0);

void print_line(String p_sz);
String itos(i32 p_val);

struct IPoint2;
struct IPoint3;
struct FPoint2;
struct FPoint3;

String str(const IPoint2 &p_pt);
String str(const IPoint3 &p_pt);
String str(const FPoint2 &p_pt);
String str(const FPoint3 &p_pt);

} // namespace NavPhysics
