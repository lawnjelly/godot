// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR editor/import/

#include SCU_PATH(SCU_DIR,editor_import_collada.cpp)
#include SCU_PATH(SCU_DIR,editor_import_plugin.cpp)
#include SCU_PATH(SCU_DIR,editor_scene_importer_gltf.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_bitmask.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_csv_translation.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_image.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_layered_texture.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_obj.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_scene.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_texture.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_texture_atlas.cpp)
#include SCU_PATH(SCU_DIR,resource_importer_wav.cpp)
