#include "smooth.h"
#include "core/engine.h"


//void Smooth::add(int value) {

//    count += value;
//}

//void Smooth::reset() {

//    count = 0;
//}

//int Smooth::get_total() const {

//    return count;
//}

void Smooth::SetProcessing(bool bEnable)
{
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_process_internal(bEnable);
	set_physics_process_internal(bEnable);
	//set_process(true);
	//set_physics_process(true);
}


void Smooth::set_enabled(bool p_enabled)
{
	if (TestFlags(SF_ENABLED) == p_enabled)
		return;

	ChangeFlags(SF_ENABLED, p_enabled);

	SetProcessing (p_enabled);
}

bool Smooth::is_enabled() const
{
	return TestFlags(SF_ENABLED);
}

void Smooth::set_interpolate_translation(bool bTranslate)
{
	ChangeFlags(SF_TRANSLATE, bTranslate);
}

bool Smooth::get_interpolate_translation() const
{
	return TestFlags(SF_TRANSLATE);
}


void Smooth::set_interpolate_rotation(bool bRotate)
{
	ChangeFlags(SF_ROTATE, bRotate);
}

bool Smooth::get_interpolate_rotation() const
{
	return TestFlags(SF_ROTATE);
}

void Smooth::set_interpolate_scale(bool bScale)
{
	ChangeFlags(SF_SCALE, bScale);
}

bool Smooth::get_interpolate_scale() const
{
	return TestFlags(SF_SCALE);
}

void Smooth::set_lerp(bool bLerp)
{
	ChangeFlags(SF_LERP, bLerp);
}

bool Smooth::get_lerp() const
{
	return TestFlags(SF_LERP);
}


//void Smooth::set_mode(eMode p_mode)
//{
//	m_Mode = p_mode;
//}

//Smooth::eMode Smooth::get_mode() const
//{
//	return m_Mode;
//}


void Smooth::RemoveTarget()
{
	NodePath pathnull;
	set_target_path(pathnull);
}

void Smooth::set_target(const Object *p_target)
{
	// handle null
	if (p_target == NULL)
	{
		print_line("SCRIPT set_Target NULL");
		RemoveTarget();
		return;
	}
	print_line("SCRIPT set_Target");

	// is it a spatial?
	const Spatial * pSpatial = Object::cast_to<Spatial>(p_target);

	if (!pSpatial)
	{
		WARN_PRINT("Target must be a spatial.");
		RemoveTarget();
		return;
	}

	NodePath path = get_path_to(pSpatial);
	set_target_path(path);
}

void Smooth::_set_target(const Object *p_target)
{
//	m_refTarget.set_obj(pTarget);
	if (p_target)
		print_line("\t_set_Target");
	else
		print_line("\t_set_Target NULL");

	m_ID_target = 0;

	if (p_target)
	{
		// check is spatial
		const Spatial *pSpatial = Object::cast_to<Spatial>(p_target);

		if (pSpatial)
		{
			m_ID_target = pSpatial->get_instance_id();
			print_line("\t\tTarget was spatial ID " + itos(m_ID_target));
		}
		else
		{
			print_line("\t\tTarget was not spatial!");
		}
	}

}

void Smooth::ResolveTargetPath()
{
	print_line("resolve_Target_path " + m_path_target);
	if (has_node(m_path_target))
	{
		print_line("has_node");
		Spatial * pNode = Object::cast_to<Spatial>(get_node(m_path_target));
		if (pNode)
		{
			print_line("node_was spatial");
			_set_target(pNode);
			return;
		}
	}

	// default to setting to null
	_set_target(NULL);
}


void Smooth::set_target_path(const NodePath &p_path)
{
	print_line("set_Target_path " + p_path);
	m_path_target = p_path;

	ResolveTargetPath();
}

NodePath Smooth::get_target_path() const
{
	return m_path_target;
}


void Smooth::_bind_methods() {

//	ClassDB::bind_method(D_METHOD("get_quat_curr"), &Smooth::get_quat_curr);
//	ClassDB::bind_method(D_METHOD("get_quat_prev"), &Smooth::get_quat_prev);
//	ClassDB::bind_method(D_METHOD("get_fraction"), &Smooth::get_fraction);

//	BIND_ENUM_CONSTANT(MODE_AUTO);
//	BIND_ENUM_CONSTANT(MODE_MANUAL);


	ClassDB::bind_method(D_METHOD("teleport"), &Smooth::teleport);

//    ClassDB::bind_method(D_METHOD("add", "value"), &Smooth::add);
  //  ClassDB::bind_method(D_METHOD("reset"), &Smooth::reset);
   // ClassDB::bind_method(D_METHOD("get_total"), &Smooth::get_total);
	ClassDB::bind_method(D_METHOD("set_enabled"), &Smooth::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &Smooth::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interpolate_translation"), &Smooth::set_interpolate_translation);
	ClassDB::bind_method(D_METHOD("get_interpolate_translation"), &Smooth::get_interpolate_translation);
	ClassDB::bind_method(D_METHOD("set_interpolate_rotation"), &Smooth::set_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("get_interpolate_rotation"), &Smooth::get_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("set_interpolate_scale"), &Smooth::set_interpolate_scale);
	ClassDB::bind_method(D_METHOD("get_interpolate_scale"), &Smooth::get_interpolate_scale);
	ClassDB::bind_method(D_METHOD("set_lerp"), &Smooth::set_lerp);
	ClassDB::bind_method(D_METHOD("get_lerp"), &Smooth::get_lerp);

//	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &Smooth::set_mode);
//	ClassDB::bind_method(D_METHOD("get_mode"), &Smooth::get_mode);

	ClassDB::bind_method(D_METHOD("set_target", "target"), &Smooth::set_target);
	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &Smooth::set_target_path);
	ClassDB::bind_method(D_METHOD("get_target_path"), &Smooth::get_target_path);


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

//	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Auto,Manual"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target"), "set_target_path", "get_target_path");


	ADD_GROUP("Components", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "translation"), "set_interpolate_translation", "get_interpolate_translation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotation"), "set_interpolate_rotation", "get_interpolate_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scale"), "set_interpolate_scale", "get_interpolate_scale");
	ADD_GROUP("Method", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lerp"), "set_lerp", "get_lerp");

}

Smooth::Smooth() {
	//count = 0;
	m_Flags = 0;
	SetFlags(SF_ENABLED | SF_TRANSLATE | SF_ROTATE);
//	m_Mode = MODE_AUTO;

//	m_bEnabled = true;
//	m_bInterpolate_Rotation = true;
//	m_bInterpolate_Scale = false;
//	m_bLerp = false;
	//m_ptCurr.zero();
	//m_ptPrev.zero();
//	m_bDirty = false;
//	m_fFraction = 0.0f;
}


Spatial * Smooth::GetTarget() const
{
//	if (m_Mode == MODE_MANUAL)
//	{
		if (m_ID_target == 0)
			return 0;

		Object *obj = ObjectDB::get_instance(m_ID_target);
		return (Spatial *) obj;
//	}

//	Node *pParent = get_parent();
//	if (!pParent)
//		return 0;

//	// first child of parent
//	Node * pChild = pParent->get_child(0);
//	Spatial *pTarget = Object::cast_to<Spatial>(pChild);

//	// cannot have ourself as the target of the smoothing
//	if (pTarget == this)
//		return 0;

//	return pTarget;
}




void Smooth::teleport()
{
	Spatial * pTarget = GetTarget();
	if (!pTarget)
	return;

	RefreshTransform(pTarget);

	// set previous equal to current
	m_Prev = m_Curr;
	m_ptTranslateDiff.zero();

}

void Smooth::RefreshTransform(Spatial * pTarget, bool bDebug)
{
	ClearFlags(SF_DIRTY);

	// keep the data flowing...
	// translate..
	m_Prev.m_Transform.origin = m_Curr.m_Transform.origin;

	// new transform for this tick
	const Transform &trans = pTarget->get_transform();
	m_Curr.m_Transform.origin = trans.origin;
	m_ptTranslateDiff = m_Curr.m_Transform.origin - m_Prev.m_Transform.origin;


	// lerp? keep the basis
	if (TestFlags(SF_LERP))
	{
		m_Prev.m_Transform.basis = m_Curr.m_Transform.basis;
		m_Curr.m_Transform.basis = trans.basis;
	}
	else
	{
		if (TestFlags(SF_ROTATE))
		{
			m_Prev.m_qtRotate = m_Curr.m_qtRotate;
			m_Curr.m_qtRotate = trans.basis.get_rotation_quat();
			//m_qtCurr = trans.basis.get_quat();

	//		if (bDebug && (m_qtPrev == m_qtCurr))
	//		if (bDebug)
	//			print_line("\tREFRESH TRANSFORM : qtPrev " + String(m_qtPrev) + "\tqtCurr " + String(m_qtCurr));
		}
	//	else
	//	{
			// directly set rotation to the Target rotation
			// (not necessary to set the translate at same time but this is easy)
	//		set_transform(trans);
	//	}

		if (TestFlags(SF_SCALE))
		{
			m_Prev.m_ptScale = m_Curr.m_ptScale;
			m_Curr.m_ptScale = trans.basis.get_scale();
		}

	} // if not lerp



//	Variant pt = m_trParent_curr.origin;
//	print_line("translate " + String(pt));

}

void Smooth::FixedUpdate()
{
	// has more than one physics tick occurred before a frame? if so
	// refresh the transform to the last physics tick. In most cases this will not occur,
	// except with tick rates higher than frame rate
	if (TestFlags(SF_DIRTY))
	{
		Spatial * pTarget = GetTarget();
		if (pTarget)
		{
			RefreshTransform(pTarget, true);
		}
	}

	// refresh the transform on the next frame
	// because the Target transform may not be updated until AFTER the fixed update!
	// (this gets around an order of operations issue)
	SetFlags(SF_DIRTY);
}

void Smooth::FrameUpdate()
{
	Spatial * pTarget = GetTarget();
	if (!pTarget)
	return;

	if (TestFlags(SF_DIRTY))
		RefreshTransform(pTarget);

	// interpolation fraction
	float f = Engine::get_singleton()->get_physics_interpolation_fraction();

	Vector3 ptNew = m_Prev.m_Transform.origin + (m_ptTranslateDiff * f);


//	Variant vf = f;
	//print_line("fraction " + itos(f * 1000.0f) + "\tsetting translation " + String(ptNew));

	// simplified, only using translate
	if (m_Flags == (SF_ENABLED | SF_TRANSLATE))
	{
		set_translation(ptNew);
	}
	else
	{
		// send as a transform, the whole kabunga
		Transform trans;
		trans.origin = ptNew;

		// lerping
		if (TestFlags(SF_LERP))
		{
			//trans.basis = m_Prev.m_Basis.slerp(m_Curr.m_Basis, f);
			LerpBasis(m_Prev.m_Transform.basis, m_Curr.m_Transform.basis, trans.basis, f);
		}
		else
		{
			// slerping
			Quat qtRot = m_Prev.m_qtRotate.slerp(m_Curr.m_qtRotate, f);
			trans.basis.set_quat(qtRot);

			if (TestFlags(SF_SCALE))
			{
				Vector3 ptScale = ((m_Curr.m_ptScale - m_Prev.m_ptScale) * f) + m_Prev.m_ptScale;
				trans.basis.scale(ptScale);
			}
		}


		set_transform(trans);
	}
//	print_line("\tframe " + itos(f * 1000.0f) + " y is " + itos(y * 1000.0f) + " actual y " + itos(actual_y * 1000.0f));

}


void Smooth::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

//			if (!Engine::get_singleton()->is_editor_hint() && m_bEnabled)
	//		{
				SetProcessing(TestFlags(SF_ENABLED));

				// we can't translate string name of Target to a node until we are in the tree
				ResolveTargetPath();
		//	}

		} break;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
//			if (!m_bEnabled)
	//			break;
			FixedUpdate();
		} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		//	if (!m_bEnabled)
			//	break;
			FrameUpdate();

		} break;
	}
}

void Smooth::LerpBasis(const Basis &from, const Basis &to, Basis &res, float f) const
{
	res = from;

	for (int n=0; n<3; n++)
		res.elements[n] = from.elements[n].linear_interpolate(to.elements[n], f);
}

