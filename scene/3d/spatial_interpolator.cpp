#include "spatial.h"
#include "spatial_interpolator.h"
#include "core/engine.h"

void SpatialInterpolator::FixedUpdate(const Transform &trans)
{
	m_Prev = m_Curr;

	// new transform for this tick
	m_Curr.m_ptTranslate = trans.origin;
	m_ptTranslate_Diff = m_Curr.m_ptTranslate - m_Prev.m_ptTranslate;

	m_Curr.m_qtRotate = trans.basis.get_quat();
}

Transform SpatialInterpolator::GetInterpolatedTransform()
{
	// interpolation fraction
	float f = Engine::get_singleton()->get_physics_interpolation_fraction();

	Vector3 ptNew = m_Prev.m_ptTranslate + (m_ptTranslate_Diff * f);


//	Variant vf = f;
	//print_line("fraction " + itos(f * 1000.0f) + "\tsetting translation " + String(ptNew));

//	if (!m_bInterpolate_Rotation)
//	{
//		set_translation(ptNew);
//	}
//	else
//	{
		// send as a transform
		Transform trans;
		trans.origin = ptNew;

		Quat qtRot = m_Prev.m_qtRotate.slerp(m_Curr.m_qtRotate, f);
		trans.basis.set_quat(qtRot);

//		set_transform(trans);
//	}
	return trans;

}


