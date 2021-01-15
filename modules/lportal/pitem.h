#pragma once

#include "scene/3d/spatial.h"


class PHidable
{
public:
	void show(bool p_show);
	void hidable_create(Node * p_node);
	
private:
	// separate flag so we don't have to touch the godot lookup
	bool _show;
	
	// new .. can be separated from the scene tree to cull
	Node * _node;
	Node * _parent_node;
	
	ObjectID _godot_ID; // godot object
};

// static object
class PStatic : public PHidable
{
public:
	void create(Node * p_node, const AABB &p_aabb);

private:	
	AABB _aabb; // world space
};

// dynamic object
class PDob : public PHidable
{
public:
	
private:
	float _radius;
	int32_t _room_ID;
};
