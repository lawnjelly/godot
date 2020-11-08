BVHHandle item_add(T *p_userdata, const AABB &p_aabb, int32_t p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
{
#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("\nitem_add BEFORE");
//	_recursive_print_tree();
	VERBOSE_PRINT("\n");
#endif

	ABB abb;
	abb.from(p_aabb);

	// handle to be filled with the new item ref
	BVHHandle handle;

	// ref id easier to pass around than handle
	uint32_t ref_id;

	// this should never fail
	ItemRef * ref = _refs.request(ref_id);

	// the extra data should be parallel list to the references
	uint32_t extra_id;
	ItemExtra * extra = _extra.request(extra_id);
	CRASH_COND(extra_id != ref_id);
	extra->subindex = p_subindex;
	extra->userdata = p_userdata;

	if (USE_PAIRS)
	{
		extra->pairable_mask = p_pairable_mask;
		extra->pairable_type = p_pairable_type;
		extra->pairable = p_pairable;
	}

	// assign to handle to return
	handle.id = ref_id;

	// ignore bound
#ifdef BVH_DEBUG_DRAW
	if (ref_id == 1)
		return handle;
#endif

	create_root_node();

	// we must choose where to add to tree
	ref->tnode_id = recursive_choose_item_add_node(_root_node_id, abb);

	_node_add_item(ref->tnode_id, ref_id, abb);

//		refit_all();
	refit_upward(ref->tnode_id);

//	_try_exchange_cousin(ref_id);

#ifdef BVH_VERBOSE
	// memory use
	int mem = _refs.estimate_memory_use();
	mem += _nodes.estimate_memory_use();

	String sz = _aabb_to_string(abb);
	VERBOSE_PRINT("item_add [" + itos(ref_id) + "] " + itos(_refs.size()) + " refs,\t" + itos(_nodes.size()) + " nodes " + sz);
	VERBOSE_PRINT("mem use : " + itos(mem) + ", num nodes : " + itos(_nodes.size()));

#endif

#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("item_add AFTER");
	_recursive_print_tree();
#endif

	return handle;
}


void item_move(BVHHandle p_handle, const AABB &p_aabb)
{
	uint32_t ref_id = p_handle.id;

#ifdef BVH_DEBUG_DRAW
	// ignore bound
	if (ref_id == 1)
		return;
#endif

	ABB abb;
	abb.from(p_aabb);

	// get the reference
	ItemRef &ref = _refs[ref_id];

	CRASH_COND(ref.tnode_id == -1);

	TNode &tnode = _nodes[ref.tnode_id];

//		int id = tnode.find_item(p_ref_id.id);
//		CRASH_COND(id == -1)

	Item &item = tnode.get_item(ref.item_id);
	item.aabb = abb;

	// does it fit within the current aabb?
	if (tnode.aabb.is_within(abb))
	{
		// no nothing .. fast path .. not moved enough to need refit
		return;
	}

	// remove and reinsert
	node_remove_item(ref_id);

	// we must choose where to add to tree
	ref.tnode_id = recursive_choose_item_add_node(_root_node_id, abb);
	_node_add_item(ref.tnode_id, ref_id, abb);
	refit_upward(ref.tnode_id);

//	refit_upward(ref.tnode_id);
}

void item_remove(BVHHandle p_handle)
{
	uint32_t ref_id = p_handle.id;

#ifdef BVH_DEBUG_DRAW
	// ignore bound
	if (ref_id == 1)
		return;
#endif

	// get the reference
	//ItemRef &ref = refs[ref_id];

	VERBOSE_PRINT("item_remove [" + itos(ref_id) + "] ");

	// remove the item from the node
	node_remove_item(ref_id);

	// remove the item reference
	_refs.free(ref_id);
	_extra.free(ref_id);

	refit_all();

#ifdef BVH_VERBOSE_TREE
	_recursive_print_tree();
#endif

//		refit_upward(ref.tnode_id);
}
