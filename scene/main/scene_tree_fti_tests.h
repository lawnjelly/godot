#pragma once

class Spatial;
class Node;
class Transform;
class SceneTreeFTI;

class SceneTreeFTITests {
	SceneTreeFTI &_fti;

	void debug_verify_failed(const Spatial *p_spatial, const Transform &p_test);

public:
	void update_dirty_spatials(Node *p_node, uint32_t p_current_half_frame, float p_interpolation_fraction, bool p_active, const Transform *p_parent_global_xform = nullptr, int p_depth = 0);
	void frame_update(Node *p_root, uint32_t p_half_frame, float p_interpolation_fraction);

	SceneTreeFTITests(SceneTreeFTI &p_fti);
};
