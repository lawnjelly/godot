#include "mod/GoModCompile.cpp"

#pragma once

#include "mod/gamelogger/GoGameLogger.h"

#undef GO_GDSCRIPT_CUSTOM_NAMES
#undef GO_GDSCRIPT_CUSTOM_CALL
#undef GO_GDSCRIPT_CUSTOM_GET_INFO

#define GO_GDSCRIPT_CUSTOM_NAMES "log_size",\
"log_get",\
"log_clear",

#define GO_GDSCRIPT_CUSTOM_CALL case LOG_SIZE:\
{\
VALIDATE_ARG_COUNT(0);\
r_ret = Pel::g_GameLogger.Size();\
}\
break;\
case LOG_CLEAR:\
{\
VALIDATE_ARG_COUNT(0);\
Pel::g_GameLogger.Clear();\
r_ret = Variant();\
}\
break;\
case LOG_GET:\
{\
VALIDATE_ARG_COUNT(1);\
VALIDATE_ARG_NUM(0);\
r_ret = Pel::g_GameLogger.Get((int) *p_args[0]);\
}\
break;

#define GO_GDSCRIPT_CUSTOM_GET_INFO case LOG_SIZE: {\
MethodInfo mi("log_size");\
mi.return_val.type = Variant::INT;\
return mi;\
} break;\
case LOG_CLEAR: {\
MethodInfo mi("log_clear");\
mi.return_val.type = Variant::NIL;\
return mi;\
} break;\
case LOG_GET: {\
MethodInfo mi("log_get", PropertyInfo(Variant::INT, "which"));\
mi.return_val.type = Variant::STRING;\
return mi;\
} break;
