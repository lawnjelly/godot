#include "smoothing.h"
#include "core/engine.h"


void Smoothing::add(int value) {

    count += value;
}

void Smoothing::reset() {

    count = 0;
}

int Smoothing::get_total() const {

    return count;
}

void Smoothing::_bind_methods() {

    ClassDB::bind_method(D_METHOD("add", "value"), &Smoothing::add);
    ClassDB::bind_method(D_METHOD("reset"), &Smoothing::reset);
    ClassDB::bind_method(D_METHOD("get_total"), &Smoothing::get_total);
}

Smoothing::Smoothing() {
	count = 0;
	enabled = true;
	parent_transform_prev.origin.zero();
	parent_transform_curr.origin.zero();
}

void Smoothing::FixedUpdate()
{
	Spatial *parent = Object::cast_to<Spatial>(get_parent());
	if (!parent)
		return;

	// keep the data flowing...
	parent_transform_prev = parent_transform_curr;

	// new transform for this tick
	parent_transform_curr = parent->get_transform();

}

void Smoothing::FrameUpdate()
{
	Vector3 ptDiff = parent_transform_curr.get_origin() - parent_transform_prev.get_origin();

	// interpolation fraction
	float f = get_interpolation_fraction();

	//print_line("interpolation fraction " + itos(f * 1000.0f));

	// reverse interpolation
	f = 1.0f - f;

	Vector3 ptNew = get_translation() + (ptDiff * f);
	set_translation(ptNew);
}


void Smoothing::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

			if (!Engine::get_singleton()->is_editor_hint() && enabled)
			{
				//set_process_internal(false);
				set_process(true);
				set_physics_process(true);
			}

		} break;
	case NOTIFICATION_PHYSICS_PROCESS: {
			if (!enabled)
				break;
			FixedUpdate();
		} break;
	case NOTIFICATION_PROCESS: {
			if (!enabled)
				break;
			FrameUpdate();
			/*
			if (has_node(target)) {

				Spatial *node = Object::cast_to<Spatial>(get_node(target));
				if (!node)
					break;

				float delta = speed * get_process_delta_time();
				Transform target_xform = node->get_global_transform();
				Transform local_transform = get_global_transform();
				local_transform = local_transform.interpolate_with(target_xform, delta);
				set_global_transform(local_transform);
				Camera *cam = Object::cast_to<Camera>(node);
				if (cam) {

					if (cam->get_projection() == get_projection()) {

						float new_near = Math::lerp(get_znear(), cam->get_znear(), delta);
						float new_far = Math::lerp(get_zfar(), cam->get_zfar(), delta);

						if (cam->get_projection() == PROJECTION_ORTHOGONAL) {

							float size = Math::lerp(get_size(), cam->get_size(), delta);
							set_orthogonal(size, new_near, new_far);
						} else {

							float fov = Math::lerp(get_fov(), cam->get_fov(), delta);
							set_perspective(fov, new_near, new_far);
						}
					}
				}
			}
			*/

		} break;
	}
}
