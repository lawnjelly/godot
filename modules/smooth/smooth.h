/* smooth.h */
#ifndef SMOOTH_H
#define SMOOTH_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"


class Smooth : public Spatial {
	GDCLASS(Smooth, Spatial);

// custom
private:
	class STransform
	{
	public:
		Transform m_Transform;
		//Vector3 m_ptTranslate;
		Quat m_qtRotate;
		Vector3 m_ptScale;
		//Basis m_Basis;
	};

	Vector3 m_ptTranslateDiff;

public:
	enum eMethod
	{
		METHOD_SLERP,
		METHOD_LERP,
	};

#define SMOOTHNODE Spatial
#include "smooth_header.inl"
#undef SMOOTHNODE



// specific
public:
	Smooth();

	void set_method(eMethod p_method);
	eMethod get_method() const;
//	void set_lerp(bool bLerp);
//	bool get_lerp() const;
private:
	void LerpBasis(const Basis &from, const Basis &to, Basis &res, float f) const;
};

VARIANT_ENUM_CAST(Smooth::eMode);
VARIANT_ENUM_CAST(Smooth::eMethod);

#endif
