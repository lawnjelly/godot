/* smoothing.h */
#ifndef SMOOTHING_H
#define SMOOTHING_H

#include "scene/3d/spatial.h"
//#include "core/reference.h"

class Smoothing : public Spatial {
	GDCLASS(Smoothing, Spatial);

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

	Vector3 m_ptCurr;
	Vector3 m_ptPrev;
	Vector3 m_ptDiff;

	Quat m_qtCurr;
	Quat m_qtPrev;

	bool m_bEnabled;

	bool m_bInterpolate_Rotation;
//	bool m_bInterpolate_Scale;

	// whether the transform needs to be refreshed from the parent
	// (a physics tick has occurred)
	bool m_bDirty;

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
	void set_enabled(bool p_enabled);
	bool is_enabled() const;

//	Quat get_quat_curr() const {return m_qtCurr;}
//	Quat get_quat_prev() const {return m_qtPrev;}
//	float get_fraction() const {return m_fFraction;}

	void set_interpolate_rotation(bool bRotate);
	bool get_interpolate_rotation() const;

	void teleport();

    Smoothing();

private:
	void FixedUpdate();
	void FrameUpdate();
	void RefreshTransform(Spatial * pProxy);
	Spatial * GetProxy() const;
};

#endif
