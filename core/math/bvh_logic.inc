
// for slow incremental optimization, we will periodically remove each
// item from the tree and reinsert, to give it a chance to find a better position
void _logic_item_remove_and_reinsert(uint32_t p_ref_id)
{
	// get the reference
	ItemRef &ref = _refs[p_ref_id];
	
	// special case of debug draw
	if (ref.item_id == -1)
		return;

	BVH_ASSERT(ref.tnode_id != -1);	

	// some overlay elaborate way to find out which tree the node is in!
	BVHHandle temp_handle;
	temp_handle.set_id(p_ref_id);
	_current_tree = _handle_get_tree_id(temp_handle);

	// remove and reinsert
	BVH_ABB abb;
	node_remove_item(p_ref_id, &abb);

	// we must choose where to add to tree
	ref.tnode_id = _logic_recursive_choose_item_add_node(_root_node_id[_current_tree], abb);
	_node_add_item(ref.tnode_id, p_ref_id, abb);
	
	refit_upward_and_balance(ref.tnode_id);


#ifdef BVH_VERBOSE_TREE
	//VERBOSE_PRINT("Remove_And_Reinsert " + itos(p_ref_id));
	//_recursive_print_tree(_current_tree);
#endif
	
}

// from randy gaul balance function
BVH_ABB _logic_abb_merge(const BVH_ABB &a, const BVH_ABB &b)
{
	BVH_ABB c = a;
	c.merge(b);
	return c;
}

int32_t _logic_balance(int32_t iA)
{
//	return iA;
	
	TNode *A = &_nodes[iA];

	if ( A->is_leaf( ) || A->height == 1 )
		return iA;

	/*      A
	      /   \
	     B     C
	    / \   / \
	   D   E F   G
	*/

	CRASH_COND(A->num_children != 2);
	int32_t iB = A->children[0];
	int32_t iC = A->children[1];
	TNode *B = &_nodes[iB];
	TNode *C = &_nodes[iC];

	int32_t balance = C->height - B->height;

	// C is higher, promote C
	if ( balance > 1 )
	{
		int32_t iF = C->children[0];
		int32_t iG = C->children[1];
		TNode *F = &_nodes[iF];
		TNode *G = &_nodes[iG];

		// grandParent point to C
		if ( A->parent_id != -1 )
		{
			if(_nodes[ A->parent_id ].children[0] == iA)
				_nodes[ A->parent_id ].children[0] = iC;

			else
				_nodes[ A->parent_id].children[1] = iC;
		}
		else
		{
			// check this .. seems dodgy
			change_root_node(iC);
		}

		// Swap A and C
		C->children[0] = iA;
		C->parent_id = A->parent_id;
		A->parent_id = iC;

		// Finish rotation
		if ( F->height > G->height )
		{
			C->children[1] = iF;
			A->children[1] = iG;
			G->parent_id = iA;
			A->aabb = B->aabb; A->aabb.merge(G->aabb);
			C->aabb = A->aabb; C->aabb.merge(F->aabb);

			A->height = 1 + MAX( B->height, G->height );
			C->height = 1 + MAX( A->height, F->height );
		}

		else
		{
			C->children[1] = iG;
			A->children[1] = iF;
			F->parent_id = iA;
			A->aabb = _logic_abb_merge( B->aabb, F->aabb );
			C->aabb = _logic_abb_merge( A->aabb, G->aabb );

			A->height = 1 + MAX( B->height, F->height );
			C->height = 1 + MAX( A->height, G->height );
		}

		return iC;
	}

	// B is higher, promote B
	else if ( balance < -1 )
	{
		int32_t iD = B->children[0];
		int32_t iE = B->children[1];
		TNode *D = &_nodes[iD];
		TNode *E = &_nodes[iE];

		// grandParent point to B
		if ( A->parent_id != -1 )
		{
			if(_nodes[ A->parent_id ].children[0] == iA)
				_nodes[ A->parent_id ].children[0] = iB;
			else
				_nodes[ A->parent_id ].children[1] = iB;
		}

		else
		{
			// check this .. seems dodgy
			change_root_node(iB);
		}

		// Swap A and B
		B->children[1] = iA;
		B->parent_id = A->parent_id;
		A->parent_id = iB;

		// Finish rotation
		if ( D->height > E->height )
		{
			B->children[0] = iD;
			A->children[0] = iE;
			E->parent_id = iA;
			A->aabb = _logic_abb_merge( C->aabb, E->aabb );
			B->aabb = _logic_abb_merge( A->aabb, D->aabb );

			A->height = 1 + MAX( C->height, E->height );
			B->height = 1 + MAX( A->height, D->height );
		}

		else
		{
			B->children[0] = iE;
			A->children[0] = iD;
			D->parent_id = iA;
			A->aabb = _logic_abb_merge( C->aabb, D->aabb );
			B->aabb = _logic_abb_merge( A->aabb, E->aabb );

			A->height = 1 + MAX( C->height, D->height );
			B->height = 1 + MAX( A->height, E->height );
		}

		return iB;
	}

	return iA;	
}


// either choose an existing node to add item to, or create a new node and return this
uint32_t _logic_recursive_choose_item_add_node(uint32_t p_node_id, const BVH_ABB &p_aabb)
{
	BVH_ASSERT(p_node_id != -1);
	TNode &tnode = _nodes[p_node_id];

	if (tnode.is_leaf())
	{
		// if a leaf, and non full, use this to add to
		if (!node_is_leaf_full(tnode))
			return p_node_id;
		
		// else split the leaf, and use one of the children to add to
		return split_leaf(p_node_id, p_aabb);
	}

#define BVH_SELECT_BULLET_METHOD
#ifndef BVH_SELECT_BULLET_METHOD
	// else if a node, choose the best of the children
	float best_goodness_fit = FLT_MAX;
	int best_child = -1;

	// find the best child and recurse into that
	for (int n=0; n<tnode.num_children; n++)
	{
		TNode &child = _nodes[tnode.children[n]];

		float fit = _goodness_of_fit_merge(p_aabb, child.aabb);
		if (fit < best_goodness_fit)
		{
			best_goodness_fit = fit;
			best_child = tnode.children[n];
		}
	}
	CRASH_COND(best_child == -1);
	
	return _logic_recursive_choose_item_add_node(best_child, p_aabb);
#else
	// this should not happen???
	if (tnode.num_children == 1)
	{
		WARN_PRINT_ONCE("BVH::recursive_choose_item_add_node, node with 1 child, recovering");
		return _logic_recursive_choose_item_add_node(tnode.children[0], p_aabb);
	}
	
	// bullet method
	BVH_ASSERT(tnode.num_children == 2);
	TNode &childA = _nodes[tnode.children[0]];
	TNode &childB = _nodes[tnode.children[1]];
	int which = p_aabb.select_by_proximity(childA.aabb, childB.aabb);

	
	return _logic_recursive_choose_item_add_node(tnode.children[which], p_aabb);
#endif

	/*
	if (tnode.num_items && !p_aabb.intersects(tnode.aabb))
	{
		if (!tnode.is_leaf() || !tnode.is_full_of_children())
		{
		}
		else
		{
			// create sibling to this node
			return _create_sibling(p_node_id, p_aabb);
		}
	}

	// if not a leaf node
	if (!tnode.is_leaf())
	{
		// first choice is, if there are not max children, create another child node
		if (!tnode.is_full_of_children())
		{
			return _create_another_child(p_node_id, p_aabb);
		}

		// there are children already
		float best_goodness_fit = FLT_MAX;
		int best_child = -1;

		// find the best child and recurse into that
		for (int n=0; n<tnode.num_items; n++)
		{

					float fit = _goodness_of_fit_merge(p_aabb, tnode.get_item(n).aabb);
					if (fit < best_goodness_fit)
					{
						best_goodness_fit = fit;
						best_child = n;
					}
		}

		CRASH_COND(best_child == -1);
		return recursive_choose_item_add_node(tnode.get_item(best_child).item_ref_id, p_aabb);
	} // if not a leaf

	// if we got to here, must be a leaf

	// is it full?
	if (!tnode.is_full_of_items())
		return p_node_id;

	// if it is full, and a leaf node, we will split the leaf into children, then
	// recurse into the children
	//split_leaf(p_node_id);
	return split_leaf(p_node_id, p_aabb);

	return recursive_choose_item_add_node(p_node_id, p_aabb);
	*/
}
