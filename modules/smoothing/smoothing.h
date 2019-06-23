/* smoothing.h */
#ifndef SMOOTHING_H
#define SMOOTHING_H

#include "scene/3d/spatial.h"
//#include "core/reference.h"

class Smoothing : public Spatial {
	GDCLASS(Smoothing, Spatial);

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

	Transform m_trParent_prev;
	Transform m_trParent_curr;
	bool m_bEnabled;




//    int count;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
//    void add(int value);
   // void reset();
	//int get_total() const;
	void SetEnabled(bool bEnable);
	bool IsEnabled() const;

    Smoothing();

private:
	void FixedUpdate();
	void FrameUpdate();
};

#endif
