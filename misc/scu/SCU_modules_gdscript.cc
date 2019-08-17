// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR modules/gdscript/

#include SCU_PATH(SCU_DIR,gdscript.cpp)
#include SCU_PATH(SCU_DIR,gdscript_compiler.cpp)
#include SCU_PATH(SCU_DIR,gdscript_editor.cpp)
#include SCU_PATH(SCU_DIR,gdscript_function.cpp)
#include SCU_PATH(SCU_DIR,gdscript_functions.cpp)
#include SCU_PATH(SCU_DIR,gdscript_parser.cpp)
#include SCU_PATH(SCU_DIR,gdscript_tokenizer.cpp)
#include SCU_PATH(SCU_DIR,register_types.cpp)
