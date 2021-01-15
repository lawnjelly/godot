#include "pstatics.h"

PStatic * PStatics::request(Node * p_node, const AABB &p_aabb)
{
	PStatic * s = _statics.request();
	s->create(p_node, p_aabb);
	return s;
}
