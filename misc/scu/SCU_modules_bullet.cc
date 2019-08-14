// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR modules/bullet/

#include SCU_PATH(SCU_DIR,area_bullet.cpp)
#include SCU_PATH(SCU_DIR,btRayShape.cpp)
#include SCU_PATH(SCU_DIR,bullet_physics_server.cpp)
#include SCU_PATH(SCU_DIR,bullet_types_converter.cpp)
#include SCU_PATH(SCU_DIR,collision_object_bullet.cpp)
#include SCU_PATH(SCU_DIR,cone_twist_joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,constraint_bullet.cpp)
#include SCU_PATH(SCU_DIR,generic_6dof_joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,godot_collision_configuration.cpp)
#include SCU_PATH(SCU_DIR,godot_collision_dispatcher.cpp)
#include SCU_PATH(SCU_DIR,godot_ray_world_algorithm.cpp)
#include SCU_PATH(SCU_DIR,godot_result_callbacks.cpp)
#include SCU_PATH(SCU_DIR,hinge_joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,pin_joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,register_types.cpp)
#include SCU_PATH(SCU_DIR,rigid_body_bullet.cpp)
#include SCU_PATH(SCU_DIR,shape_bullet.cpp)
#include SCU_PATH(SCU_DIR,shape_owner_bullet.cpp)
#include SCU_PATH(SCU_DIR,slider_joint_bullet.cpp)
#include SCU_PATH(SCU_DIR,soft_body_bullet.cpp)
#include SCU_PATH(SCU_DIR,space_bullet.cpp)
