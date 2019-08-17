// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/physics_2d/

#include SCU_PATH(SCU_DIR,area_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,area_pair_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,body_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,body_pair_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_2d_basic.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_2d_hash_grid.cpp)
#include SCU_PATH(SCU_DIR,broad_phase_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,collision_object_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,collision_solver_2d_sat.cpp)
#include SCU_PATH(SCU_DIR,collision_solver_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,joints_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,physics_2d_server_sw.cpp)
#include SCU_PATH(SCU_DIR,physics_2d_server_wrap_mt.cpp)
#include SCU_PATH(SCU_DIR,shape_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,space_2d_sw.cpp)
#include SCU_PATH(SCU_DIR,step_2d_sw.cpp)
