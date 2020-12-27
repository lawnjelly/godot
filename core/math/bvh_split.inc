void _split_leaf_sort_groups(int &num_a, int &num_b, uint16_t * group_a, uint16_t * group_b, const BVH_ABB * temp_bounds)
{
	BVH_ABB groupb_aabb;
	groupb_aabb.set_to_max_opposite_extents();
	for (int n=0; n<num_b; n++)
	{
		int which = group_b[n];
		groupb_aabb.merge(temp_bounds[which]);
	}
	BVH_ABB groupb_aabb_new;

	BVH_ABB rest_aabb;

	float best_size = FLT_MAX;
	int best_candidate = -1;

	// find most likely from a to move into b
	for (int check = 0; check < num_a; check++)
	{
		rest_aabb.set_to_max_opposite_extents();
		groupb_aabb_new = groupb_aabb;

		// find aabb of all the rest
		for (int rest=0; rest< num_a; rest++)
		{
			if (rest == check)
				continue;

			int which = group_a[rest];
			rest_aabb.merge(temp_bounds[which]);
		}

		groupb_aabb_new.merge(temp_bounds[group_a[check]]);

		// now compare the sizes
		float size = groupb_aabb_new.get_area() + rest_aabb.get_area();
		if (size < best_size)
		{
			best_size = size;
			best_candidate = check;
		}
	}

	// we should now have the best, move it from group a to group b
	group_b[num_b++] = group_a[best_candidate];

	// remove best candidate from group a
	num_a--;
	group_a[best_candidate] = group_a[num_a];
}


// aabb is the new inserted node
uint32_t split_leaf(uint32_t p_node_id, const BVH_ABB &p_added_item_aabb)
{
	VERBOSE_PRINT("split_leaf");

	// note the tnode before and AFTER splitting may be a different address
	// in memory because the vector could get relocated. So we need to reget
	// the tnode after the split
	TNode &tnode_orig = _nodes[p_node_id];
	CRASH_COND(!tnode_orig.is_leaf());
	
	// first create child leaf nodes
	uint32_t * child_ids = (uint32_t *) alloca(sizeof (uint32_t) * MAX_CHILDREN);

	for (int n=0; n<MAX_CHILDREN; n++)
	{
		// create node children
		TNode * child_node = _nodes.request(child_ids[n]);
		
		child_node->clear();

		// back link to parent
		child_node->parent_id = p_node_id;
		
		// make each child a leaf node
		uint32_t child_leaf_id;
		TLeaf * child_leaf = _leaves.request(child_leaf_id);
		child_leaf->clear();
		CRASH_COND(child_leaf_id == 0);
		child_node->neg_leaf_id = -child_leaf_id;
	}

	// don't get any leaves or nodes till AFTER the split
	TNode &tnode = _nodes[p_node_id];
	uint32_t orig_leaf_id = tnode.get_leaf_id();
	const TLeaf &orig_leaf = _leaves[orig_leaf_id];

	// store the final child ids
	for (int n=0; n<MAX_CHILDREN; n++)
	{
		tnode.children[n] = child_ids[n];
	}
	
	// mark as no longer a leaf node
	tnode.num_children = MAX_CHILDREN;

	// 2 groups, A and B, and assign children to each to split equally
	int max_children = orig_leaf.num_items +1;  // plus 1 for the wildcard .. the item being added
	//CRASH_COND(max_children > MAX_CHILDREN);

	uint16_t * group_a = (uint16_t *) alloca(sizeof (uint16_t) * max_children);
	uint16_t * group_b = (uint16_t *) alloca(sizeof (uint16_t) * max_children);

	// we are copying the ABBs. This is ugly, but we need one extra for the inserted item...
	BVH_ABB * temp_bounds = (BVH_ABB *) alloca(sizeof (BVH_ABB) * max_children);

	int num_a = max_children;
	int num_b = 0;

	// setup - start with all in group a
	for (int n=0; n<orig_leaf.num_items; n++)
	{
		group_a[n] = n;
		temp_bounds[n] = orig_leaf.get_item(n).aabb;
	}
	// wildcard
	int wildcard = orig_leaf.num_items;

	group_a[wildcard] = wildcard;
	temp_bounds[wildcard] = p_added_item_aabb;

	// we can choose here either an equal split, or just 1 in the new leaf
//#define BVH_TREE_SPLIT_EQUAL
#ifdef BVH_TREE_SPLIT_EQUAL
	int num_moves = num_a / 2;
#else
	int num_moves = 1;
#endif
	for (int m=0; m<num_moves; m++)
	{
		_split_leaf_sort_groups(num_a, num_b, group_a, group_b, temp_bounds);
	}

	uint32_t wildcard_node = -1;

	// now there should be equal numbers in both groups
	for (int n=0; n<num_a; n++)
	{
		int which = group_a[n];

		if (which != wildcard)
		{
			const Item &source_item = orig_leaf.get_item(which);
			_node_add_item(tnode.children[0], source_item.item_ref_id, source_item.aabb);
		}
		else
		{
			wildcard_node = tnode.children[0];
		}
	}
	for (int n=0; n<num_b; n++)
	{
		int which = group_b[n];

		if (which != wildcard)
		{
			const Item &source_item = orig_leaf.get_item(which);
			_node_add_item(tnode.children[1], source_item.item_ref_id, source_item.aabb);
		}
		else
		{
			wildcard_node = tnode.children[1];
		}
	}

	/*
	// move each item to a child node
	for (int n=0; n<tnode.num_items; n++)
	{
		int child_node_id = child_ids[n % MAX_CHILDREN];
		const Item &source_item = tnode.get_item(n);
		_node_add_item(child_node_id, source_item.item_ref_id, source_item.aabb);
	}
	*/

	// now remove all items from the parent and replace with the child nodes
	_leaves.free(orig_leaf_id);
	
	
//		tnode.num_items = MAX_CHILDREN;
//		for (int n=0; n<MAX_CHILDREN; n++)
//		{
//			// store the child node ids, but not the AABB as not calculated yet
//			tnode.items[n].item_ref_id = child_ids[n];
//		}

	// change this to make more efficient
	node_update_aabb(_nodes[tnode.children[0]]);
	node_update_aabb(_nodes[tnode.children[1]]);
	refit_upward(p_node_id);

	CRASH_COND(wildcard_node == -1);
	return wildcard_node;
}
