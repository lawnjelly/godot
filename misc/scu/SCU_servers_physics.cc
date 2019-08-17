// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/physics/

#include SCU_PATH(SCU_DIR,area_pair_sw.cpp)
#include SCU_PATH(SCU_DIR,area_sw.cpp)
#include SCU_PATH(SCU_DIR,body_pair_sw.cpp)
#include SCU_PATH(SCU_DIR,body_sw.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_basic.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_octree.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_sw.cpp)
#include SCU_PATH(SCU_DIR,collision_object_sw.cpp)
#include SCU_PATH(SCU_DIR,collision_solver_sat.cpp)
#include SCU_PATH(SCU_DIR,collision_solver_sw.cpp)
#include SCU_PATH(SCU_DIR,gjk_epa.cpp)
#include SCU_PATH(SCU_DIR,physics_server_sw.cpp)
#include SCU_PATH(SCU_DIR,shape_sw.cpp)
#include SCU_PATH(SCU_DIR,space_sw.cpp)
#include SCU_PATH(SCU_DIR,step_sw.cpp)
