#include "smooth_2d.h"
#include "core/engine.h"


void Smooth2D::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("teleport"), &Smooth2D::teleport);

	ClassDB::bind_method(D_METHOD("set_enabled"), &Smooth2D::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &Smooth2D::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interpolate_translation"), &Smooth2D::set_interpolate_translation);
	ClassDB::bind_method(D_METHOD("get_interpolate_translation"), &Smooth2D::get_interpolate_translation);
	ClassDB::bind_method(D_METHOD("set_interpolate_rotation"), &Smooth2D::set_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("get_interpolate_rotation"), &Smooth2D::get_interpolate_rotation);
	ClassDB::bind_method(D_METHOD("set_interpolate_scale"), &Smooth2D::set_interpolate_scale);
	ClassDB::bind_method(D_METHOD("get_interpolate_scale"), &Smooth2D::get_interpolate_scale);


	ClassDB::bind_method(D_METHOD("set_target", "target"), &Smooth2D::set_target);
	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &Smooth2D::set_target_path);
	ClassDB::bind_method(D_METHOD("get_target_path"), &Smooth2D::get_target_path);


	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target"), "set_target_path", "get_target_path");


	ADD_GROUP("Components", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "translation"), "set_interpolate_translation", "get_interpolate_translation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotation"), "set_interpolate_rotation", "get_interpolate_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scale"), "set_interpolate_scale", "get_interpolate_scale");
//	ADD_GROUP("Method", "");
//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lerp"), "set_lerp", "get_lerp");

}


void Smooth2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
				SetProcessing(TestFlags(SF_ENABLED));
				// we can't translate string name of Target to a node until we are in the tree
				ResolveTargetPath();
		} break;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			FixedUpdate();
		} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
			FrameUpdate();
		} break;
	}

}


void Smooth2D::set_enabled(bool p_enabled)
{
	if (TestFlags(SF_ENABLED) == p_enabled)
		return;

	ChangeFlags(SF_ENABLED, p_enabled);

	SetProcessing (p_enabled);
}

bool Smooth2D::is_enabled() const
{
	return TestFlags(SF_ENABLED);
}

void Smooth2D::set_interpolate_translation(bool bTranslate)
{
	ChangeFlags(SF_TRANSLATE, bTranslate);
}

bool Smooth2D::get_interpolate_translation() const
{
	return TestFlags(SF_TRANSLATE);
}


void Smooth2D::set_interpolate_rotation(bool bRotate)
{
	ChangeFlags(SF_ROTATE, bRotate);
}

bool Smooth2D::get_interpolate_rotation() const
{
	return TestFlags(SF_ROTATE);
}

void Smooth2D::set_interpolate_scale(bool bScale)
{
	ChangeFlags(SF_SCALE, bScale);
}

bool Smooth2D::get_interpolate_scale() const
{
	return TestFlags(SF_SCALE);
}


void Smooth2D::set_target(const Object *p_target)
{
	// handle null
	if (p_target == NULL)
	{
		print_line("SCRIPT set_Target NULL");
		RemoveTarget();
		return;
	}
	print_line("SCRIPT set_Target");

	// is it a node2d?
	const Node2D * pNode = Object::cast_to<Node2D>(p_target);

	if (!pNode)
	{
		WARN_PRINT("Target must be a Node2D.");
		RemoveTarget();
		return;
	}

	NodePath path = get_path_to(pNode);
	set_target_path(path);

}

void Smooth2D::set_target_path(const NodePath &p_path)
{
	print_line("set_Target_path " + p_path);
	m_path_target = p_path;
	ResolveTargetPath();
}

NodePath Smooth2D::get_target_path() const
{
	return m_path_target;
}

void Smooth2D::teleport()
{
	Node2D * pTarget = GetTarget();
	if (!pTarget)
	return;

	RefreshTransform(pTarget);

	// set previous equal to current
	m_Prev = m_Curr;
	m_ptPosDiff.x = 0.0f;
	m_ptPosDiff.y = 0.0f;


}


void Smooth2D::ResolveTargetPath()
{
	print_line("resolve_Target_path " + m_path_target);
	if (has_node(m_path_target))
	{
		print_line("has_node");
		Node2D * pNode = Object::cast_to<Node2D>(get_node(m_path_target));
		if (pNode)
		{
			print_line("node_was Node2D");
			_set_target(pNode);
			return;
		}
	}

	// default to setting to null
	_set_target(NULL);
}

void Smooth2D::_set_target(const Object *p_target)
{
	if (p_target)
		print_line("\t_set_Target");
	else
		print_line("\t_set_Target NULL");

	m_ID_target = 0;

	if (p_target)
	{
		// check is spatial
		const Node2D *pNode = Object::cast_to<Node2D>(p_target);

		if (pNode)
		{
			m_ID_target = pNode->get_instance_id();
			print_line("\t\tTarget was Node2D ID " + itos(m_ID_target));
		}
		else
		{
			print_line("\t\tTarget was not Node2D!");
		}
	}

}

void Smooth2D::RemoveTarget()
{
	NodePath pathnull;
	set_target_path(pathnull);
}

void Smooth2D::FixedUpdate()
{
	// has more than one physics tick occurred before a frame? if so
	// refresh the transform to the last physics tick. In most cases this will not occur,
	// except with tick rates higher than frame rate
	if (TestFlags(SF_DIRTY))
	{
		Node2D * pTarget = GetTarget();
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

void Smooth2D::FrameUpdate()
{
	Node2D * pTarget = GetTarget();
	if (!pTarget)
	return;

	if (TestFlags(SF_DIRTY))
		RefreshTransform(pTarget);

	// interpolation fraction
	float f = Engine::get_singleton()->get_physics_interpolation_fraction();

	if (TestFlags(SF_TRANSLATE))
	{
		Point2 ptNew = m_Prev.pos + (m_ptPosDiff * f);
		set_position(ptNew);
	}

	if (TestFlags(SF_ROTATE))
	{
		// need angular interp
		float r = LerpAngle(m_Prev.angle, m_Curr.angle, f);
		set_rotation(r);
	}

	if (TestFlags(SF_SCALE))
	{
		Size2 s = m_Prev.scale + ((m_Curr.scale - m_Prev.scale) * f);
		set_scale(s);
	}

}

float Smooth2D::LerpAngle(float from, float to, float weight) const
{
    return from + ShortAngleDist(from, to) * weight;
}

float Smooth2D::ShortAngleDist(float from, float to) const
{
	float max_angle = (3.14159265358979323846264 * 2.0);
    float difference = fmod(to - from, max_angle);
    return fmod(2.0f * difference, max_angle) - difference;
}

void Smooth2D::RefreshTransform(Node2D * pTarget, bool bDebug)
{
	ClearFlags(SF_DIRTY);

	// keep the data flowing...
	m_Prev = m_Curr;

	// new transform for this tick
	if (TestFlags(SF_TRANSLATE))
	{
		m_Curr.pos = pTarget->get_position();
		m_ptPosDiff = m_Curr.pos - m_Prev.pos;
	}


	if (TestFlags(SF_ROTATE))
		m_Curr.angle = pTarget->get_rotation();

	if (TestFlags(SF_SCALE))
		m_Curr.scale = pTarget->get_scale();

}

Node2D * Smooth2D::GetTarget() const
{
	if (m_ID_target == 0)
		return 0;

	Object *obj = ObjectDB::get_instance(m_ID_target);
	return (Node2D *) obj;
}

void Smooth2D::SetProcessing(bool bEnable)
{
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_process_internal(bEnable);
	set_physics_process_internal(bEnable);
}


Smooth2D::Smooth2D()
{
	m_Flags = 0;
	SetFlags(SF_ENABLED | SF_TRANSLATE);
}

