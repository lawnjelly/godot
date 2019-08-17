// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR main/tests/

#include SCU_PATH(SCU_DIR,test_astar.cpp)
#include SCU_PATH(SCU_DIR,test_gdscript.cpp)
#include SCU_PATH(SCU_DIR,test_gui.cpp)
#include SCU_PATH(SCU_DIR,test_main.cpp)
#include SCU_PATH(SCU_DIR,test_math.cpp)
#include SCU_PATH(SCU_DIR,test_oa_hash_map.cpp)
#include SCU_PATH(SCU_DIR,test_ordered_hash_map.cpp)
#include SCU_PATH(SCU_DIR,test_physics.cpp)
#include SCU_PATH(SCU_DIR,test_physics_2d.cpp)
#include SCU_PATH(SCU_DIR,test_render.cpp)
#include SCU_PATH(SCU_DIR,test_shader_lang.cpp)
#include SCU_PATH(SCU_DIR,test_string.cpp)
