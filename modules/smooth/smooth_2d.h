#ifndef SMOOTH2D_H
#define SMOOTH2D_H

#include "scene/2d/node_2d.h"

class Smooth2D : public Node2D
{
	GDCLASS(Smooth2D, Node2D);

	enum eSmoothFlags
	{
		SF_ENABLED = 1,
		SF_DIRTY = 2,
		SF_TRANSLATE = 4,
		SF_ROTATE = 8,
		SF_SCALE = 16,
	};

	class STransform
	{
	public:
		Point2 pos;
		float angle;
		Size2 scale;
	};

	STransform m_Curr;
	STransform m_Prev;

	Point2 m_ptPosDiff;

	// defined by eSmoothFlags
	int m_Flags;

	ObjectID m_ID_target;
	NodePath m_path_target;


	void ResolveTargetPath();
	void _set_target(const Object *p_target);
	void RemoveTarget();
	void FixedUpdate();
	void FrameUpdate();
	void RefreshTransform(Node2D * pTarget, bool bDebug = false);
	Node2D * GetTarget() const;
	void SetProcessing(bool bEnable);

	void ChangeFlags(int f, bool bSet) {if (bSet) {SetFlags(f);} else {ClearFlags(f);}}
	void SetFlags(int f) {m_Flags |= f;}
	void ClearFlags(int f) {m_Flags &= ~f;}
	bool TestFlags(int f) const {return (m_Flags & f) == f;}

	float LerpAngle(float from, float to, float weight) const;
	float ShortAngleDist(float from, float to) const;


protected:
	static void _bind_methods();
	void _notification(int p_what);


public:
	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_interpolate_translation(bool bTranslate);
	bool get_interpolate_translation() const;

	void set_interpolate_rotation(bool bRotate);
	bool get_interpolate_rotation() const;

	void set_interpolate_scale(bool bScale);
	bool get_interpolate_scale() const;

	void set_target(const Object *p_target);
	void set_target_path(const NodePath &p_path);
	NodePath get_target_path() const;

	void teleport();


	Smooth2D();
};


#endif
