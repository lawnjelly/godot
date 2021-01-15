#pragma once

#include "lvector.h"
#include "pitem.h"

class PStatics
{
public:
	PStatic &get_static(unsigned int ui) {return _statics[ui];}
	const PStatic &get_static(unsigned int ui) const {return _statics[ui];}
	
	PStatic * request(Node * p_node, const AABB &p_aabb);
	void clear() {_statics.clear();}
	
private:
	Lawn::LVector<PStatic> _statics;
};
