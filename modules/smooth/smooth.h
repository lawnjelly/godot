/* smooth.h */
#ifndef SMOOTH_H
#define SMOOTH_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"
//#include "core/reference.h"


class Smooth : public Spatial {
	GDCLASS(Smooth, Spatial);

	/*
	struct Data {

		//mutable Vector3 rotation;

		//mutable int dirty;

		//int children_lock;
		//Spatial *parent;
		//List<Spatial *> children;
		//List<Spatial *>::Element *C;

		//bool disable_scale;

#ifdef TOOLS_ENABLED
		//Ref<SpatialGizmo> gizmo;
		//bool gizmo_dirty;
#endif

	} data;
*/
public:
	enum eMode
	{
		MODE_LOCAL,
		MODE_GLOBAL,
	};
private:
	enum eSmoothFlags
	{
		SF_ENABLED = 1,
		SF_DIRTY = 2,
		SF_TRANSLATE = 4,
		SF_ROTATE = 8,
		SF_SCALE = 16,
		SF_GLOBAL = 32,
		SF_LERP = 64,
	};

	class STransform
	{
	public:
		Transform m_Transform;
		//Vector3 m_ptTranslate;
		Quat m_qtRotate;
		Vector3 m_ptScale;
		//Basis m_Basis;
	};

	STransform m_Curr;
	STransform m_Prev;


	Vector3 m_ptTranslateDiff;

	// defined by eSmoothFlags
	int m_Flags;
//	eMode m_Mode;

	//WeakRef m_refTarget;
	ObjectID m_ID_target;
	NodePath m_path_target;

//	bool m_bEnabled;
//	bool m_bInterpolate_Rotation;
//	bool m_bInterpolate_Scale;

	// use lerping instead of slerping and scale
	// (faster but only suitable for high tick rates, and may
	// introduce visual artefacts / skewing)
//	bool m_bLerp;

	// whether the transform needs to be refreshed from the parent
	// (a physics tick has occurred)
//	bool m_bDirty;

//    int count;

	// for debugging
//	float m_fFraction;
protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
//    void add(int value);
   // void reset();
	//int get_total() const;

	void set_mode(eMode p_mode);
	eMode get_mode() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

//	Quat get_quat_curr() const {return m_qtCurr;}
//	Quat get_quat_prev() const {return m_qtPrev;}
//	float get_fraction() const {return m_fFraction;}

	void set_interpolate_translation(bool bTranslate);
	bool get_interpolate_translation() const;

	void set_interpolate_rotation(bool bRotate);
	bool get_interpolate_rotation() const;

	void set_interpolate_scale(bool bScale);
	bool get_interpolate_scale() const;

	void set_lerp(bool bLerp);
	bool get_lerp() const;

	void set_target(const Object *p_target);
	void set_target_path(const NodePath &p_path);
	NodePath get_target_path() const;

	void teleport();

    Smooth();

private:
	void _set_target(const Object *p_target);
	void RemoveTarget();
	void FixedUpdate();
	void FrameUpdate();
	void RefreshTransform(Spatial * pTarget, bool bDebug = false);
	Spatial * GetTarget() const;
	void SetProcessing(bool bEnable);

	void ChangeFlags(int f, bool bSet) {if (bSet) {SetFlags(f);} else {ClearFlags(f);}}
	void SetFlags(int f) {m_Flags |= f;}
	void ClearFlags(int f) {m_Flags &= ~f;}
	bool TestFlags(int f) const {return (m_Flags & f) == f;}

	void LerpBasis(const Basis &from, const Basis &to, Basis &res, float f) const;
	void ResolveTargetPath();
};

VARIANT_ENUM_CAST(Smooth::eMode);

#endif
