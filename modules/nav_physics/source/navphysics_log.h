#pragma once

#include "navphysics_string.h"
#include "navphysics_typedefs.h"

namespace NavPhysics {
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
