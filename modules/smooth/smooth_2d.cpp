#include "smooth_2d.h"
#include "core/engine.h"


#define SMOOTHCLASS Smooth2D
#define SMOOTHNODE Node2D
#include "smooth_body.inl"

// finish the bind with custom stuff
}

#undef SMOOTHCLASS
#undef SMOOTHNODE


////////////////////////////////

Smooth2D::Smooth2D()
{
	m_Flags = 0;
	SetFlags(SF_ENABLED | SF_TRANSLATE);
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

	// global or local?
	bool bGlobal = TestFlags(SF_GLOBAL_IN);

	if (bGlobal)
	{
		// new transform for this tick
		if (TestFlags(SF_TRANSLATE))
		{
			m_Curr.pos = pTarget->get_global_position();
			m_ptTranslateDiff = m_Curr.pos - m_Prev.pos;
		}


		if (TestFlags(SF_ROTATE))
			m_Curr.angle = pTarget->get_global_rotation();

		if (TestFlags(SF_SCALE))
			m_Curr.scale = pTarget->get_global_scale();
	}
	else
	{
		// new transform for this tick
		if (TestFlags(SF_TRANSLATE))
		{
			m_Curr.pos = pTarget->get_position();
			m_ptTranslateDiff = m_Curr.pos - m_Prev.pos;
		}


		if (TestFlags(SF_ROTATE))
			m_Curr.angle = pTarget->get_rotation();

		if (TestFlags(SF_SCALE))
			m_Curr.scale = pTarget->get_scale();
	}

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


	// global or local?
	bool bGlobal = TestFlags(SF_GLOBAL_OUT);

	if (bGlobal)
	{
		if (TestFlags(SF_TRANSLATE))
		{
			Point2 ptNew = m_Prev.pos + (m_ptTranslateDiff * f);
			set_global_position(ptNew);
		}

		if (TestFlags(SF_ROTATE))
		{
			// need angular interp
			float r = LerpAngle(m_Prev.angle, m_Curr.angle, f);
			set_global_rotation(r);
		}

		if (TestFlags(SF_SCALE))
		{
			Size2 s = m_Prev.scale + ((m_Curr.scale - m_Prev.scale) * f);
			set_global_scale(s);
		}
	}
	else
	{
		if (TestFlags(SF_TRANSLATE))
		{
			Point2 ptNew = m_Prev.pos + (m_ptTranslateDiff * f);
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



}

void Smooth2D::teleport()
{
	Node2D * pTarget = GetTarget();
	if (!pTarget)
	return;

	RefreshTransform(pTarget);

	// set previous equal to current
	m_Prev = m_Curr;
	m_ptTranslateDiff.x = 0.0f;
	m_ptTranslateDiff.y = 0.0f;
}
