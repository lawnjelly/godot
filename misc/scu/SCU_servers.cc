// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/

#include SCU_PATH(SCU_DIR,arvr_server.cpp)
#include SCU_PATH(SCU_DIR,audio_server.cpp)
#include SCU_PATH(SCU_DIR,camera_server.cpp)
#include SCU_PATH(SCU_DIR,physics_2d_server.cpp)
#include SCU_PATH(SCU_DIR,physics_server.cpp)
#include SCU_PATH(SCU_DIR,register_server_types.cpp)
#include SCU_PATH(SCU_DIR,visual_server.cpp)
