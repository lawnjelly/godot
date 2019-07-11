#ifndef SMOOTH2D_H
#define SMOOTH2D_H

#include "scene/2d/node_2d.h"

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
