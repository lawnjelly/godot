BVH_Handle item_add(const AABB &p_aabb)
{
#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("\nitem_add BEFORE");
//	_recursive_print_tree();
	VERBOSE_PRINT("\n");
#endif

	// handle to be filled with the new item ref
	BVH_Handle handle;

	// ref id easier to pass around than handle
	uint32_t ref_id;

	// this should never fail
	ItemRef * ref = _refs.request(ref_id);

	// assign to handle to return
	handle.id = ref_id;

	// ignore bound
	if (ref_id == 1)
		return handle;

	create_root_node();

	// we must choose where to add to tree
	ref->tnode_id = recursive_choose_item_add_node(_root_node_id, p_aabb);

	_node_add_item(ref->tnode_id, ref_id, p_aabb);

//		refit_all();
	refit_upward(ref->tnode_id);

#ifdef BVH_VERBOSE
	String sz = _aabb_to_string(p_aabb);
	VERBOSE_PRINT("item_add [" + itos(ref_id) + "] " + itos(_refs.size()) + " refs,\t" + itos(_nodes.size()) + " nodes " + sz);
#endif

#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("item_add AFTER");
	_recursive_print_tree();
#endif

	return handle;
}


void item_move(BVH_Handle p_handle, const AABB &p_aabb)
{
	uint32_t ref_id = p_handle.id;

	// ignore bound
	if (ref_id == 1)
		return;


	// get the reference
	ItemRef &ref = _refs[ref_id];

	CRASH_COND(ref.tnode_id == -1);

	TNode &tnode = _nodes[ref.tnode_id];

//		int id = tnode.find_item(p_ref_id.id);
//		CRASH_COND(id == -1)

	Item &item = tnode.get_item(ref.item_id);
	item.aabb = p_aabb;

	//refit_all();
	refit_upward(ref.tnode_id);
}

void item_remove(BVH_Handle p_handle)
{
	uint32_t ref_id = p_handle.id;

	// ignore bound
	if (ref_id == 1)
		return;

	// get the reference
	//ItemRef &ref = refs[ref_id];

	VERBOSE_PRINT("item_remove [" + itos(ref_id) + "] ");

	// remove the item from the node
	node_remove_item(ref_id);

	// remove the item reference
	_refs.free(ref_id);

	refit_all();

#ifdef BVH_VERBOSE_TREE
	_recursive_print_tree();
#endif

//		refit_upward(ref.tnode_id);
}
