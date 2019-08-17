// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/visual/

#include SCU_PATH(SCU_DIR,rasterizer.cpp)
#include SCU_PATH(SCU_DIR,shader_language.cpp)
#include SCU_PATH(SCU_DIR,shader_types.cpp)
#include SCU_PATH(SCU_DIR,visual_server_canvas.cpp)
#include SCU_PATH(SCU_DIR,visual_server_globals.cpp)
#include SCU_PATH(SCU_DIR,visual_server_light_baker.cpp)
#include SCU_PATH(SCU_DIR,visual_server_raster.cpp)
#include SCU_PATH(SCU_DIR,visual_server_scene.cpp)
#include SCU_PATH(SCU_DIR,visual_server_viewport.cpp)
#include SCU_PATH(SCU_DIR,visual_server_wrap_mt.cpp)
