// where smoothclass is defined as the class
// and smoothnode is defined as the node

//void SMOOTHCLASS::add(int value) {

//    count += value;
//}

//void SMOOTHCLASS::reset() {

//    count = 0;
//}

//int SMOOTHCLASS::get_total() const {

//    return count;
//}

void SMOOTHCLASS::SetProcessing(bool bEnable)
{
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_process_internal(bEnable);
	set_physics_process_internal(bEnable);
	//set_process(true);
	//set_physics_process(true);
}


void SMOOTHCLASS::set_enabled(bool p_enabled)
{
	if (TestFlags(SF_ENABLED) == p_enabled)
		return;

	ChangeFlags(SF_ENABLED, p_enabled);

	SetProcessing (p_enabled);
}

bool SMOOTHCLASS::is_enabled() const
{
	return TestFlags(SF_ENABLED);
}

void SMOOTHCLASS::set_interpolate_translation(bool bTranslate)
{
	ChangeFlags(SF_TRANSLATE, bTranslate);
}

bool SMOOTHCLASS::get_interpolate_translation() const
{
	return TestFlags(SF_TRANSLATE);
}


void SMOOTHCLASS::set_interpolate_rotation(bool bRotate)
{
	ChangeFlags(SF_ROTATE, bRotate);
}

bool SMOOTHCLASS::get_interpolate_rotation() const
{
	return TestFlags(SF_ROTATE);
}

void SMOOTHCLASS::set_interpolate_scale(bool bScale)
{
	ChangeFlags(SF_SCALE, bScale);
}

bool SMOOTHCLASS::get_interpolate_scale() const
{
	return TestFlags(SF_SCALE);
}


void SMOOTHCLASS::set_input_mode(eMode p_mode)
{
	if (p_mode == MODE_GLOBAL)
		SetFlags(SF_GLOBAL_IN);
	else
		ClearFlags(SF_GLOBAL_IN);
}

SMOOTHCLASS::eMode SMOOTHCLASS::get_input_mode() const
{
	if (TestFlags(SF_GLOBAL_IN))
		return MODE_GLOBAL;

	return MODE_LOCAL;
}

void SMOOTHCLASS::set_output_mode(eMode p_mode)
{
	if (p_mode == MODE_GLOBAL)
		SetFlags(SF_GLOBAL_OUT);
	else
		ClearFlags(SF_GLOBAL_OUT);
}

SMOOTHCLASS::eMode SMOOTHCLASS::get_output_mode() const
{
	if (TestFlags(SF_GLOBAL_OUT))
		return MODE_GLOBAL;

	return MODE_LOCAL;
}


void SMOOTHCLASS::RemoveTarget()
{
	NodePath pathnull;
	set_target_path(pathnull);
}

void SMOOTHCLASS::set_target(const Object *p_target)
{
	// handle null
	if (p_target == NULL)
	{
		print_line("SCRIPT set_Target NULL");
		RemoveTarget();
		return;
	}
	print_line("SCRIPT set_Target");

	// is it a SMOOTHNODE?
	const SMOOTHNODE * pSMOOTHNODE = Object::cast_to<SMOOTHNODE>(p_target);

	if (!pSMOOTHNODE)
	{
		WARN_PRINT("Target must be a SMOOTHNODE.");
		RemoveTarget();
		return;
	}

	NodePath path = get_path_to(pSMOOTHNODE);
	set_target_path(path);
}

void SMOOTHCLASS::_set_target(const Object *p_target)
{
//	m_refTarget.set_obj(pTarget);
	if (p_target)
		print_line("\t_set_Target");
	else
		print_line("\t_set_Target NULL");

	m_ID_target = 0;

	if (p_target)
	{
		// check is SMOOTHNODE
		const SMOOTHNODE *pSMOOTHNODE = Object::cast_to<SMOOTHNODE>(p_target);

		if (pSMOOTHNODE)
		{
			m_ID_target = pSMOOTHNODE->get_instance_id();
			print_line("\t\tTarget was SMOOTHNODE ID " + itos(m_ID_target));
		}
		else
		{
			print_line("\t\tTarget was not SMOOTHNODE!");
		}
	}

}

void SMOOTHCLASS::ResolveTargetPath()
{
	print_line("resolve_Target_path " + m_path_target);
	if (has_node(m_path_target))
	{
		print_line("has_node");
		SMOOTHNODE * pNode = Object::cast_to<SMOOTHNODE>(get_node(m_path_target));
		if (pNode)
		{
			print_line("node_was SMOOTHNODE");
			_set_target(pNode);
			return;
		}
	}

	// default to setting to null
	_set_target(NULL);
}


void SMOOTHCLASS::set_target_path(const NodePath &p_path)
{
	print_line("set_Target_path " + p_path);
	m_path_target = p_path;
	ResolveTargetPath();
}

NodePath SMOOTHCLASS::get_target_path() const
{
	return m_path_target;
}





SMOOTHNODE * SMOOTHCLASS::GetTarget() const
{
//	if (m_Mode == MODE_MANUAL)
//	{
		if (m_ID_target == 0)
			return 0;

		Object *obj = ObjectDB::get_instance(m_ID_target);
		return (SMOOTHNODE *) obj;
//	}

//	Node *pParent = get_parent();
//	if (!pParent)
//		return 0;

//	// first child of parent
//	Node * pChild = pParent->get_child(0);
//	SMOOTHNODE *pTarget = Object::cast_to<SMOOTHNODE>(pChild);

//	// cannot have ourself as the target of the smoothing
//	if (pTarget == this)
//		return 0;

//	return pTarget;
}






void SMOOTHCLASS::FixedUpdate()
{
	// has more than one physics tick occurred before a frame? if so
	// refresh the transform to the last physics tick. In most cases this will not occur,
	// except with tick rates higher than frame rate
	if (TestFlags(SF_DIRTY))
	{
		SMOOTHNODE * pTarget = GetTarget();
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



void SMOOTHCLASS::_notification(int p_what) {

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


void SMOOTHCLASS::_bind_methods() {

//	ClassDB::bind_method(D_METHOD("get_quat_curr"), &SMOOTHCLASS::get_quat_curr);
//	ClassDB::bind_method(D_METHOD("get_quat_prev"), &SMOOTHCLASS::get_quat_prev);
//	ClassDB::bind_method(D_METHOD("get_fraction"), &SMOOTHCLASS::get_fraction);

//	BIND_ENUM_CONSTANT(MODE_AUTO);
//	BIND_ENUM_CONSTANT(MODE_MANUAL);


	ClassDB::bind_method(D_METHOD("teleport"), &SMOOTHCLASS::teleport);

//    ClassDB::bind_method(D_METHOD("add", "value"), &SMOOTHCLASS::add);
  //  ClassDB::bind_method(D_METHOD("reset"), &SMOOTHCLASS::reset);
   // ClassDB::bind_method(D_METHOD("get_total"), &SMOOTHCLASS::get_total);
	ClassDB::bind_method(D_METHOD("set_enabled"), &SMOOTHCLASS::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &SMOOTHCLASS::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interpolate_translation"), &SMOOTHCLASS::set_interpolate_translation);
	ClassDB::bind_method(D_METHOD("get_interpolate_translation"), &SMOOTHCLASS::get_interpolate_translation);
	ClassDB::bind_method(D_METHOD("set_interpolate_rotation"), &SMOOTHCLASS::set_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("get_interpolate_rotation"), &SMOOTHCLASS::get_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("set_interpolate_scale"), &SMOOTHCLASS::set_interpolate_scale);
	ClassDB::bind_method(D_METHOD("get_interpolate_scale"), &SMOOTHCLASS::get_interpolate_scale);

	ClassDB::bind_method(D_METHOD("set_input_mode", "mode"), &SMOOTHCLASS::set_input_mode);
	ClassDB::bind_method(D_METHOD("get_input_mode"), &SMOOTHCLASS::get_input_mode);
	ClassDB::bind_method(D_METHOD("set_output_mode", "mode"), &SMOOTHCLASS::set_output_mode);
	ClassDB::bind_method(D_METHOD("get_output_mode"), &SMOOTHCLASS::get_output_mode);

	ClassDB::bind_method(D_METHOD("set_target", "target"), &SMOOTHCLASS::set_target);
	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &SMOOTHCLASS::set_target_path);
	ClassDB::bind_method(D_METHOD("get_target_path"), &SMOOTHCLASS::get_target_path);


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target"), "set_target_path", "get_target_path");


	ADD_GROUP("Components", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "translation"), "set_interpolate_translation", "get_interpolate_translation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotation"), "set_interpolate_rotation", "get_interpolate_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scale"), "set_interpolate_scale", "get_interpolate_scale");
	ADD_GROUP("Coords", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "input", PROPERTY_HINT_ENUM, "Local,Global"), "set_input_mode", "get_input_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output", PROPERTY_HINT_ENUM, "Local,Global"), "set_output_mode", "get_output_mode");
//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lerp"), "set_lerp", "get_lerp");
//}
