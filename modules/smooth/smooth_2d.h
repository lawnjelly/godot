#ifndef SMOOTH2D_H
#define SMOOTH2D_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/2d/node_2d.h"

// Smooth node allows fixed timestep interpolation without having to write any code.
// It requires a proxy node (which is moved on physics tick), e.g. a rigid body or manually moved node2d..
// and instead of having MeshInstance as a child of this, you add Smooth node to another part of the scene graph,
// make the MeshInstance a child of the smooth node, then choose the proxy as the target for the smooth node.

// Note that in the special case of manually moving the proxy to a completely new location, you should call
// 'teleport' on the smooth node after setting the proxy node transform. This will ensure that the current AND
// previous transform records are reset, so it moves instantaneously.

class Smooth2D : public Node2D
{
	GDCLASS(Smooth2D, Node2D);

	// custom
	class STransform
	{
	public:
		Point2 pos;
		float angle;
		Size2 scale;
	};

	Point2 m_ptTranslateDiff;

#define SMOOTHNODE Node2D
#include "smooth_header.inl"
#undef SMOOTHNODE

	// specific
public:
	Smooth2D();
private:
	float LerpAngle(float from, float to, float weight) const;
	float ShortAngleDist(float from, float to) const;
};

VARIANT_ENUM_CAST(Smooth2D::eMode);


#endif
