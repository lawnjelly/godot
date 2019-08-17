// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR scene/animation/

#include SCU_PATH(SCU_DIR,animation_blend_space_1d.cpp)
#include SCU_PATH(SCU_DIR,animation_blend_space_2d.cpp)
#include SCU_PATH(SCU_DIR,animation_blend_tree.cpp)
#include SCU_PATH(SCU_DIR,animation_cache.cpp)
#include SCU_PATH(SCU_DIR,animation_node_state_machine.cpp)
#include SCU_PATH(SCU_DIR,animation_player.cpp)
#include SCU_PATH(SCU_DIR,animation_tree.cpp)
#include SCU_PATH(SCU_DIR,animation_tree_player.cpp)
#include SCU_PATH(SCU_DIR,root_motion_view.cpp)
#include SCU_PATH(SCU_DIR,skeleton_ik.cpp)
#include SCU_PATH(SCU_DIR,tween.cpp)
