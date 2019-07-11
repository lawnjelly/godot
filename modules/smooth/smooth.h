/* smooth.h */
#ifndef SMOOTH_H
#define SMOOTH_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"


// Smooth node allows fixed timestep interpolation without having to write any code.
// It requires a proxy node (which is moved on physics tick), e.g. a rigid body or manually moved spatial..
// and instead of having MeshInstance as a child of this, you add Smooth node to another part of the scene graph,
// make the MeshInstance a child of the smooth node, then choose the proxy as the target for the smooth node.

// Note that in the special case of manually moving the proxy to a completely new location, you should call
// 'teleport' on the smooth node after setting the proxy node transform. This will ensure that the current AND
// previous transform records are reset, so it moves instantaneously.

class Smooth : public Spatial {
	GDCLASS(Smooth, Spatial);

// custom
private:
	class STransform
	{
	public:
		Transform m_Transform;
		Quat m_qtRotate;
		Vector3 m_ptScale;
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
private:
	void LerpBasis(const Basis &from, const Basis &to, Basis &res, float f) const;
};

VARIANT_ENUM_CAST(Smooth::eMode);
VARIANT_ENUM_CAST(Smooth::eMethod);

#endif
