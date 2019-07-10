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

void Smoothing::SetProcessing(bool bEnable)
{
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_process_internal(bEnable);
	set_physics_process_internal(bEnable);
	//set_process(true);
	//set_physics_process(true);
}


void Smoothing::set_enabled(bool p_enabled)
{
	if (TestFlags(SF_ENABLED) == p_enabled)
		return;

	ChangeFlags(SF_ENABLED, p_enabled);

	SetProcessing (p_enabled);
}

bool Smoothing::is_enabled() const
{
	return TestFlags(SF_ENABLED);
}

void Smoothing::set_interpolate_rotation(bool bRotate)
{
	ChangeFlags(SF_ROTATE, bRotate);
}

bool Smoothing::get_interpolate_rotation() const
{
	return TestFlags(SF_ROTATE);
}

void Smoothing::set_interpolate_scale(bool bScale)
{
	ChangeFlags(SF_SCALE, bScale);
}

bool Smoothing::get_interpolate_scale() const
{
	return TestFlags(SF_SCALE);
}

void Smoothing::set_lerp(bool bLerp)
{
	ChangeFlags(SF_LERP, bLerp);
}

bool Smoothing::get_lerp() const
{
	return TestFlags(SF_LERP);
}


//void Smoothing::set_mode(eMode p_mode)
//{
//	m_Mode = p_mode;
//}

//Smoothing::eMode Smoothing::get_mode() const
//{
//	return m_Mode;
//}


void Smoothing::RemoveProxy()
{
	NodePath pathnull;
	set_proxy_path(pathnull);
}

void Smoothing::set_proxy(const Object *p_proxy)
{
	// handle null
	if (p_proxy == NULL)
	{
		print_line("SCRIPT set_proxy NULL");
		RemoveProxy();
		return;
	}
	print_line("SCRIPT set_proxy");

	// is it a spatial?
	const Spatial * pSpatial = Object::cast_to<Spatial>(p_proxy);

	if (!pSpatial)
	{
		WARN_PRINT("Proxy must be a spatial.");
		RemoveProxy();
		return;
	}

	NodePath path = get_path_to(pSpatial);
	set_proxy_path(path);
}

void Smoothing::_set_proxy(const Object *p_proxy)
{
//	m_refProxy.set_obj(pProxy);
	if (p_proxy)
		print_line("\t_set_proxy");
	else
		print_line("\t_set_proxy NULL");

	m_ID_Proxy = 0;

	if (p_proxy)
	{
		// check is spatial
		const Spatial *pSpatial = Object::cast_to<Spatial>(p_proxy);

		if (pSpatial)
		{
			m_ID_Proxy = pSpatial->get_instance_id();
			print_line("\t\tproxy was spatial ID " + itos(m_ID_Proxy));
		}
		else
		{
			print_line("\t\tproxy was not spatial!");
		}
	}

}

void Smoothing::ResolveProxyPath()
{
	print_line("resolve_proxy_path " + m_path_Proxy);
	if (has_node(m_path_Proxy))
	{
		print_line("has_node");
		Spatial * pNode = Object::cast_to<Spatial>(get_node(m_path_Proxy));
		if (pNode)
		{
			print_line("node_was spatial");
			_set_proxy(pNode);
			return;
		}
	}

	// default to setting to null
	_set_proxy(NULL);
}


void Smoothing::set_proxy_path(const NodePath &p_path)
{
	print_line("set_proxy_path " + p_path);
	m_path_Proxy = p_path;

	ResolveProxyPath();
}

NodePath Smoothing::get_proxy_path() const
{
	return m_path_Proxy;
}


void Smoothing::_bind_methods() {

//	ClassDB::bind_method(D_METHOD("get_quat_curr"), &Smoothing::get_quat_curr);
//	ClassDB::bind_method(D_METHOD("get_quat_prev"), &Smoothing::get_quat_prev);
//	ClassDB::bind_method(D_METHOD("get_fraction"), &Smoothing::get_fraction);

//	BIND_ENUM_CONSTANT(MODE_AUTO);
//	BIND_ENUM_CONSTANT(MODE_MANUAL);


	ClassDB::bind_method(D_METHOD("teleport"), &Smoothing::teleport);

//    ClassDB::bind_method(D_METHOD("add", "value"), &Smoothing::add);
  //  ClassDB::bind_method(D_METHOD("reset"), &Smoothing::reset);
   // ClassDB::bind_method(D_METHOD("get_total"), &Smoothing::get_total);
	ClassDB::bind_method(D_METHOD("set_enabled"), &Smoothing::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &Smoothing::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interpolate_rotation"), &Smoothing::set_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("get_interpolate_rotation"), &Smoothing::get_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("set_interpolate_scale"), &Smoothing::set_interpolate_scale);
	ClassDB::bind_method(D_METHOD("get_interpolate_scale"), &Smoothing::get_interpolate_scale);
	ClassDB::bind_method(D_METHOD("set_lerp"), &Smoothing::set_lerp);
	ClassDB::bind_method(D_METHOD("get_lerp"), &Smoothing::get_lerp);

//	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &Smoothing::set_mode);
//	ClassDB::bind_method(D_METHOD("get_mode"), &Smoothing::get_mode);

	ClassDB::bind_method(D_METHOD("set_proxy", "proxy"), &Smoothing::set_proxy);
	ClassDB::bind_method(D_METHOD("set_proxy_path", "path"), &Smoothing::set_proxy_path);
	ClassDB::bind_method(D_METHOD("get_proxy_path"), &Smoothing::get_proxy_path);


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

//	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Auto,Manual"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "proxy"), "set_proxy_path", "get_proxy_path");


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotation"), "set_interpolate_rotation", "get_interpolate_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scale"), "set_interpolate_scale", "get_interpolate_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lerp"), "set_lerp", "get_lerp");

}

Smoothing::Smoothing() {
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


Spatial * Smoothing::GetProxy() const
{
//	if (m_Mode == MODE_MANUAL)
//	{
		if (m_ID_Proxy == 0)
			return 0;

		Object *obj = ObjectDB::get_instance(m_ID_Proxy);
		return (Spatial *) obj;
//	}

//	Node *pParent = get_parent();
//	if (!pParent)
//		return 0;

//	// first child of parent
//	Node * pChild = pParent->get_child(0);
//	Spatial *pProxy = Object::cast_to<Spatial>(pChild);

//	// cannot have ourself as the target of the smoothing
//	if (pProxy == this)
//		return 0;

//	return pProxy;
}




void Smoothing::teleport()
{
	Spatial * pProxy = GetProxy();
	if (!pProxy)
	return;

	RefreshTransform(pProxy);

	// set previous equal to current
	m_Prev = m_Curr;
	m_ptTranslateDiff.zero();

}

void Smoothing::RefreshTransform(Spatial * pProxy, bool bDebug)
{
	ClearFlags(SF_DIRTY);

	// keep the data flowing...
	m_Prev.m_ptTranslate = m_Curr.m_ptTranslate;

	// new transform for this tick
	const Transform &trans = pProxy->get_transform();
	m_Curr.m_ptTranslate = trans.origin;
	m_ptTranslateDiff = m_Curr.m_ptTranslate - m_Prev.m_ptTranslate;


	// lerp? keep the basis
	if (TestFlags(SF_LERP))
	{
		m_Prev.m_Basis = m_Curr.m_Basis;
		m_Curr.m_Basis = trans.basis;
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
			// directly set rotation to the proxy rotation
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

void Smoothing::FixedUpdate()
{
	// has more than one physics tick occurred before a frame? if so
	// refresh the transform to the last physics tick. In most cases this will not occur,
	// except with tick rates higher than frame rate
	if (TestFlags(SF_DIRTY))
	{
		Spatial * pProxy = GetProxy();
		if (pProxy)
		{
			RefreshTransform(pProxy, true);
		}
	}

	// refresh the transform on the next frame
	// because the proxy transform may not be updated until AFTER the fixed update!
	// (this gets around an order of operations issue)
	SetFlags(SF_DIRTY);
}

void Smoothing::FrameUpdate()
{
	Spatial * pProxy = GetProxy();
	if (!pProxy)
	return;

	if (TestFlags(SF_DIRTY))
		RefreshTransform(pProxy);

	// interpolation fraction
	float f = Engine::get_singleton()->get_physics_interpolation_fraction();

	Vector3 ptNew = m_Prev.m_ptTranslate + (m_ptTranslateDiff * f);


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
			LerpBasis(m_Prev.m_Basis, m_Curr.m_Basis, trans.basis, f);
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


void Smoothing::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {

//			if (!Engine::get_singleton()->is_editor_hint() && m_bEnabled)
	//		{
				SetProcessing(TestFlags(SF_ENABLED));

				// we can't translate string name of proxy to a node until we are in the tree
				ResolveProxyPath();
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

void Smoothing::LerpBasis(const Basis &from, const Basis &to, Basis &res, float f) const
{
	res = from;

	for (int n=0; n<3; n++)
		res.elements[n] = from.elements[n].linear_interpolate(to.elements[n], f);
}

