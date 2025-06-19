#include "count_cast_tos.h"
#include "core/reference.h"
#include "core/resource.h"
#include "main/input_default.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/3d/area.h"
#include "scene/3d/camera.h"
#include "scene/3d/collision_shape.h"
#include "scene/3d/light.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/physics_body.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/tree.h"
#include "scene/main/viewport.h"
#include <cstdio>

CountCast g_count_cast;

struct CastCounts {
	uint32_t references = 0;
	uint32_t spatials = 0;
	uint32_t visual_instances = 0;
	uint32_t canvas_items = 0;
	uint32_t resources = 0;
	uint32_t nodes = 0;
	uint32_t controls = 0;
	uint32_t input_defaults = 0;

	uint32_t node_2ds = 0;
	uint32_t bone_2ds = 0;
	uint32_t area_2ds = 0;
	uint32_t skeleton_2ds = 0;
	uint32_t animation_players = 0;
	uint32_t collision_object_2ds = 0;
	uint32_t tree_items = 0;
	uint32_t viewports = 0;

	uint32_t scripts = 0;
	uint32_t areas = 0;
	uint32_t cameras = 0;
	uint32_t collision_objects = 0;
	uint32_t collision_shapes = 0;
	uint32_t rigid_bodies = 0;
	uint32_t physics_bodies = 0;
	uint32_t static_bodies = 0;
	uint32_t skeletons = 0;
	uint32_t textures = 0;
	uint32_t materials = 0;
	uint32_t mesh_instances = 0;
	uint32_t geometry_instances = 0;
	uint32_t directional_lights = 0;
	uint32_t omni_lights = 0;
	uint32_t spot_lights = 0;

} g_cast_counts;

void CountCast::count_cast(const Object *p_obj) {
#define IMPL_COUNT_CAST(T, VAR)           \
	if (Object::cast_to<T>(p_obj, false)) \
		g_cast_counts.VAR++;

	IMPL_COUNT_CAST(Reference, references);
	IMPL_COUNT_CAST(CanvasItem, canvas_items);
	IMPL_COUNT_CAST(Spatial, spatials);
	IMPL_COUNT_CAST(VisualInstance, visual_instances);
	IMPL_COUNT_CAST(Resource, resources);
	IMPL_COUNT_CAST(Node, nodes);
	IMPL_COUNT_CAST(Control, controls);
	IMPL_COUNT_CAST(InputDefault, input_defaults);

	IMPL_COUNT_CAST(Node2D, node_2ds);
	IMPL_COUNT_CAST(Bone2D, bone_2ds);
	IMPL_COUNT_CAST(Area2D, area_2ds);
	IMPL_COUNT_CAST(Skeleton2D, skeleton_2ds);
	IMPL_COUNT_CAST(AnimationPlayer, animation_players);
	//IMPL_COUNT_CAST(AnimationTrackEdit, animation_track_edits);
	IMPL_COUNT_CAST(CollisionObject2D, collision_object_2ds);
	IMPL_COUNT_CAST(TreeItem, tree_items);
	IMPL_COUNT_CAST(Viewport, viewports);

	IMPL_COUNT_CAST(Script, scripts);
	IMPL_COUNT_CAST(Area, areas);
	IMPL_COUNT_CAST(Camera, cameras);
	IMPL_COUNT_CAST(CollisionObject, collision_objects);
	IMPL_COUNT_CAST(CollisionShape, collision_shapes);
	IMPL_COUNT_CAST(RigidBody, rigid_bodies);
	IMPL_COUNT_CAST(PhysicsBody, physics_bodies);
	IMPL_COUNT_CAST(StaticBody, static_bodies);
	IMPL_COUNT_CAST(Skeleton, skeletons);
	IMPL_COUNT_CAST(Texture, textures);
	//IMPL_COUNT_CAST(Material3D, material_3ds);
	IMPL_COUNT_CAST(Material, materials);
	IMPL_COUNT_CAST(MeshInstance, mesh_instances);
	IMPL_COUNT_CAST(GeometryInstance, geometry_instances);
	IMPL_COUNT_CAST(DirectionalLight, directional_lights);
	IMPL_COUNT_CAST(OmniLight, omni_lights);
	IMPL_COUNT_CAST(SpotLight, spot_lights);
}

CountCast::~CountCast() {
	//fprintf(stderr, "ERROR: %s\n   at: %s (%s:%i)\n", err_details, p_function, p_file, p_line);

#define IMPL_COUNT_LOG(VAR) printf(#VAR " : %i\n", g_cast_counts.VAR)

	IMPL_COUNT_LOG(references);
	IMPL_COUNT_LOG(canvas_items);
	IMPL_COUNT_LOG(spatials);
	IMPL_COUNT_LOG(visual_instances);
	IMPL_COUNT_LOG(resources);
	IMPL_COUNT_LOG(nodes);
	IMPL_COUNT_LOG(controls);
	IMPL_COUNT_LOG(input_defaults);

	IMPL_COUNT_LOG(node_2ds);
	IMPL_COUNT_LOG(bone_2ds);
	IMPL_COUNT_LOG(area_2ds);
	IMPL_COUNT_LOG(skeleton_2ds);
	IMPL_COUNT_LOG(animation_players);
	IMPL_COUNT_LOG(collision_object_2ds);
	IMPL_COUNT_LOG(tree_items);
	IMPL_COUNT_LOG(viewports);

	IMPL_COUNT_LOG(scripts);
	IMPL_COUNT_LOG(areas);
	IMPL_COUNT_LOG(cameras);
	IMPL_COUNT_LOG(collision_objects);
	IMPL_COUNT_LOG(collision_shapes);
	IMPL_COUNT_LOG(rigid_bodies);
	IMPL_COUNT_LOG(physics_bodies);
	IMPL_COUNT_LOG(static_bodies);
	IMPL_COUNT_LOG(skeletons);
	IMPL_COUNT_LOG(textures);
	IMPL_COUNT_LOG(materials);
	IMPL_COUNT_LOG(mesh_instances);
	IMPL_COUNT_LOG(geometry_instances);
	IMPL_COUNT_LOG(directional_lights);
	IMPL_COUNT_LOG(omni_lights);
	IMPL_COUNT_LOG(spot_lights);
}
