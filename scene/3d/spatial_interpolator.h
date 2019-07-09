#pragma once

class SpatialInterpolator
{
public:
	void FixedUpdate(const Transform &trans);
	Transform GetInterpolatedTransform();

private:
	class ITransform
	{
	public:
		Vector3 m_ptTranslate;
		Quat m_qtRotate;
	};

	ITransform m_Curr;
	ITransform m_Prev;
	
	Vector3 m_ptTranslate_Diff;

	// cache the interpolated transform, according to the frame
	//Transform m_trRender;
	//int m_iCurrentFrame;
};
