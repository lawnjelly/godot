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



void Smoothing::set_enabled(bool p_enabled)
{
	m_bEnabled = p_enabled;
}

bool Smoothing::is_enabled() const
{
	return m_bEnabled;
}

void Smoothing::set_interpolate_rotation(bool bRotate)
{
	m_bInterpolate_Rotation = bRotate;
}

bool Smoothing::get_interpolate_rotation() const
{
	return m_bInterpolate_Rotation;
}



void Smoothing::_bind_methods() {

//	ClassDB::bind_method(D_METHOD("get_quat_curr"), &Smoothing::get_quat_curr);
//	ClassDB::bind_method(D_METHOD("get_quat_prev"), &Smoothing::get_quat_prev);
//	ClassDB::bind_method(D_METHOD("get_fraction"), &Smoothing::get_fraction);

	ClassDB::bind_method(D_METHOD("teleport"), &Smoothing::teleport);

//    ClassDB::bind_method(D_METHOD("add", "value"), &Smoothing::add);
  //  ClassDB::bind_method(D_METHOD("reset"), &Smoothing::reset);
   // ClassDB::bind_method(D_METHOD("get_total"), &Smoothing::get_total);
	ClassDB::bind_method(D_METHOD("set_enabled"), &Smoothing::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &Smoothing::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interpolate_rotation"), &Smoothing::set_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("get_interpolate_rotation"), &Smoothing::get_interpolate_rotation);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interpolate rotation"), "set_interpolate_rotation", "get_interpolate_rotation");
}

Smoothing::Smoothing() {
	//count = 0;
	m_bEnabled = true;
	m_bInterpolate_Rotation = true;
	//m_ptCurr.zero();
	//m_ptPrev.zero();
	m_bDirty = false;
//	m_fFraction = 0.0f;
}


Spatial * Smoothing::GetProxy() const
{
	Node *pParent = get_parent();
	if (!pParent)
		return 0;

	// first child of parent
	Node * pChild = pParent->get_child(0);
	Spatial *pProxy = Object::cast_to<Spatial>(pChild);

	// cannot have ourself as the target of the smoothing
	if (pProxy == this)
		return 0;

	return pProxy;
}


void Smoothing::teleport()
{
	Spatial * pProxy = GetProxy();
	if (!pProxy)
	return;

	RefreshTransform(pProxy);

	// set previous equal to current
	m_ptPrev = m_ptCurr;
	m_ptDiff.zero();

	m_qtPrev = m_qtCurr;
}

void Smoothing::RefreshTransform(Spatial * pProxy)
{
	m_bDirty = false;

	// keep the data flowing...
	m_ptPrev = m_ptCurr;

	// new transform for this tick
	const Transform &trans = pProxy->get_transform();
	m_ptCurr = trans.origin;
	m_ptDiff = m_ptCurr - m_ptPrev;

	if (m_bInterpolate_Rotation)
	{
		m_qtPrev = m_qtCurr;
		m_qtCurr = trans.basis.get_quat();

		//if (m_qtPrev == m_qtCurr)
		//print_line("\tREFRESH TRANSFORM : qtPrev " + String(m_qtPrev) + "\tqtCurr " + String(m_qtCurr));
	}
	else
	{
		// directly set rotation to the proxy rotation
		// (not necessary to set the translate at same time but this is easy)
//		set_transform(trans);
	}

//	Variant pt = m_trParent_curr.origin;
//	print_line("translate " + String(pt));

}

void Smoothing::FixedUpdate()
{
	m_bDirty = true;
}

void Smoothing::FrameUpdate()
{
	Spatial * pProxy = GetProxy();
	if (!pProxy)
	return;

	if (m_bDirty)
		RefreshTransform(pProxy);

	// interpolation fraction
	float f = get_physics_interpolation_fraction();

	Vector3 ptNew = m_ptPrev + (m_ptDiff * f);


//	Variant vf = f;
	//print_line("fraction " + itos(f * 1000.0f) + "\tsetting translation " + String(ptNew));

	if (!m_bInterpolate_Rotation)
	{
		set_translation(ptNew);
	}
	else
	{
		// send as a transform
		Transform trans;
		trans.origin = ptNew;

		Quat qtRot = m_qtPrev.slerp(m_qtCurr, f);
		trans.basis.set_quat(qtRot);

		set_transform(trans);
	}
//	print_line("\tframe " + itos(f * 1000.0f) + " y is " + itos(y * 1000.0f) + " actual y " + itos(actual_y * 1000.0f));

}


void Smoothing::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

			if (!Engine::get_singleton()->is_editor_hint() && m_bEnabled)
			{
				set_process_internal(true);
				set_physics_process_internal(true);
				//set_process(true);
				//set_physics_process(true);
			}

		} break;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (!m_bEnabled)
				break;
			FixedUpdate();
		} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
			if (!m_bEnabled)
				break;
			FrameUpdate();

		} break;
	}
}
