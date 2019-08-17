// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR scene/main/

#include SCU_PATH(SCU_DIR,canvas_layer.cpp)
#include SCU_PATH(SCU_DIR,http_request.cpp)
#include SCU_PATH(SCU_DIR,instance_placeholder.cpp)
#include SCU_PATH(SCU_DIR,node.cpp)
#include SCU_PATH(SCU_DIR,resource_preloader.cpp)
#include SCU_PATH(SCU_DIR,scene_tree.cpp)
#include SCU_PATH(SCU_DIR,timer.cpp)
#include SCU_PATH(SCU_DIR,viewport.cpp)
