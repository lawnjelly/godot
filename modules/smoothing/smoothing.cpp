#include "smoothing.h"
#include "core/engine.h"


//void Smoothing::add(int value) {

//    count += value;
//}

//void Smoothing::reset() {

//    count = 0;
//}

//int Smoothing::get_total() const {

//    return count;
//}

void Smoothing::SetEnabled(bool bEnable)
{
	m_bEnabled = bEnable;
}

bool Smoothing::IsEnabled() const
{
	return m_bEnabled;
}


void Smoothing::_bind_methods() {

//    ClassDB::bind_method(D_METHOD("add", "value"), &Smoothing::add);
  //  ClassDB::bind_method(D_METHOD("reset"), &Smoothing::reset);
   // ClassDB::bind_method(D_METHOD("get_total"), &Smoothing::get_total);


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "SetEnabled", "IsEnabled");
}

Smoothing::Smoothing() {
	//count = 0;
	m_bEnabled = true;
	m_trParent_prev.origin.zero();
	m_trParent_curr.origin.zero();
}

void Smoothing::FixedUpdate()
{
	Spatial *parent = Object::cast_to<Spatial>(get_parent());
	if (!parent)
		return;

	// keep the data flowing...
	m_trParent_prev = m_trParent_curr;

	// new transform for this tick
	m_trParent_curr = parent->get_transform();

}

void Smoothing::FrameUpdate()
{
	Vector3 ptDiff = m_trParent_curr.get_origin() - m_trParent_prev.get_origin();

	// interpolation fraction
	float f = get_interpolation_fraction();

	//print_line("interpolation fraction " + itos(f * 1000.0f));

	// reverse interpolation
	f = 1.0f - f;

	Vector3 ptNew = (ptDiff * -f);
	set_translation(ptNew);
}


void Smoothing::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

			if (!Engine::get_singleton()->is_editor_hint() && m_bEnabled)
			{
				//set_process_internal(false);
				set_process(true);
				set_physics_process(true);
			}

		} break;
	case NOTIFICATION_PHYSICS_PROCESS: {
			if (!m_bEnabled)
				break;
			FixedUpdate();
		} break;
	case NOTIFICATION_PROCESS: {
			if (!m_bEnabled)
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
