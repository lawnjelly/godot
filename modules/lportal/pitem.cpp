#include "pitem.h"

void PHidable::show(bool p_show)
{
	
}

void PHidable::hidable_create(Node * p_node)
{
	CRASH_COND(!p_node);
	_node = p_node;
	_parent_node = _node->get_parent();
	_show = true;
}


void PStatic::create(Node * p_node, const AABB &p_aabb)
{
	hidable_create(p_node);
	_aabb = p_aabb;
}


