public:

BVHHandle item_add(T *p_userdata, const AABB &p_aabb, int32_t p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask, bool p_invisible = false)
{
#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("\nitem_add BEFORE");
//	_recursive_print_tree();
//	VERBOSE_PRINT("\n");
#endif

	BVH_ABB abb;
	abb.from(p_aabb);
	abb.expand();

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

	// pairs info
	if (USE_PAIRS)
	{
		uint32_t pairs_id;
		ItemPairs * pairs = _pairs.request(pairs_id);
		pairs->clear();
		CRASH_COND(pairs_id != ref_id);
	}

	extra->subindex = p_subindex;
	extra->userdata = p_userdata;
	extra->last_updated_tick = 0;

	if (USE_PAIRS)
	{
		extra->pairable_mask = p_pairable_mask;
		extra->pairable_type = p_pairable_type;
		extra->pairable = p_pairable;
	}
	else
	{
		// just for safety, in case this gets queried etc
		extra->pairable = 0;
		p_pairable = false;
	}

	// assign to handle to return
	handle.set_id(ref_id);

	// ignore bound
#ifdef BVH_DEBUG_DRAW
	if (ref_id == 1)
		return handle;
#endif

	_current_tree = 0;
	if (p_pairable)
		_current_tree = 1;

	create_root_node(_current_tree);

	// we must choose where to add to tree
	ref->tnode_id = recursive_choose_item_add_node(_root_node_id[_current_tree], abb);

	_node_add_item(ref->tnode_id, ref_id, abb);

//	refit_upward(ref->tnode_id);
	refit_upward_and_balance(ref->tnode_id);
		
//	_try_exchange_cousin(ref_id);

#ifdef BVH_VERBOSE
	// memory use
	int mem = _refs.estimate_memory_use();
	mem += _nodes.estimate_memory_use();

	String sz = _aabb_to_string(abb);
	VERBOSE_PRINT("\titem_add [" + itos(ref_id) + "] " + itos(_refs.size()) + " refs,\t" + itos(_nodes.size()) + " nodes " + sz);
//	VERBOSE_PRINT("mem use : " + itos(mem) + ", num nodes : " + itos(_nodes.size()));

#endif

#ifdef BVH_VERBOSE_TREE
//	VERBOSE_PRINT("item_add AFTER");
//	if (ref_id == 16)
		_recursive_print_tree(_current_tree);
#endif

//	return handle;
	return handle;
}


// returns false if noop
bool item_move(BVHHandle p_handle, const AABB &p_aabb)
{
	uint32_t ref_id = p_handle.id();

#ifdef BVH_DEBUG_DRAW
	// ignore bound
	if (ref_id == 1)
		return false;
#endif

	BVH_ABB abb;
	abb.from(p_aabb);

	// get the reference
	ItemRef &ref = _refs[ref_id];
	CRASH_COND(ref.tnode_id == -1);

	TNode &tnode = _nodes[ref.tnode_id];

	// does it fit within the current aabb?
	if (tnode.aabb.is_within(abb))
	{
		// no nothing .. fast path .. not moved enough to need refit
		return true;
	}
	
	// expand bound
	abb.expand();
	
	// remove from old leaf, and reinsert
	
//		int id = tnode.find_item(p_ref_id.id);
//		CRASH_COND(id == -1)

//	Item &item = tnode.get_item(ref.item_id);

//	// noop
//	if (abb == item.aabb)
//		return false;

	//item.aabb = abb;


	_current_tree = _get_tree_id(p_handle);

	// remove and reinsert
	node_remove_item(ref_id);

	// we must choose where to add to tree
	ref.tnode_id = recursive_choose_item_add_node(_root_node_id[_current_tree], abb);
	_node_add_item(ref.tnode_id, ref_id, abb);
	
	refit_upward(ref.tnode_id);


#ifdef BVH_VERBOSE_TREE
	VERBOSE_PRINT("Move " + itos(ref_id));
//	if (ref_id == 16)
		_recursive_print_tree(_current_tree);
#endif

//	refit_upward(ref.tnode_id);
	return true;
}

void item_remove(BVHHandle p_handle)
{
	uint32_t ref_id = p_handle.id();

	_current_tree = _get_tree_id(p_handle);

#ifdef BVH_DEBUG_DRAW
	// ignore bound
//	if (ref_id == 1)
//		return;
#endif

	// get the reference
	//ItemRef &ref = refs[ref_id];

	VERBOSE_PRINT("item_remove [" + itos(ref_id) + "] ");

#ifdef BVH_DEBUG_DRAW
	if (p_handle.id() == 1)
//	if (p_handle.is_invisible())
	{

	}
	else
#else
	{
		// remove the item from the node
		node_remove_item(ref_id);
	}
#endif

	// remove the item reference
	_refs.free(ref_id);
	_extra.free(ref_id);
	if (USE_PAIRS)
	{
		_pairs.free(ref_id);
	}

	refit_all(_current_tree);

#ifdef BVH_VERBOSE_TREE
//	_recursive_print_tree(tree_id);
#endif

//		refit_upward(ref.tnode_id);
}

void item_set_pairable(const BVHHandle &p_handle, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
{
	// change tree?
	uint32_t ref_id = p_handle.id();

	ItemExtra &ex = _extra[ref_id];
	ItemRef &ref = _refs[ref_id];

	ex.pairable_type = p_pairable_type;
	ex.pairable_mask = p_pairable_mask;

	if (ex.pairable != p_pairable)
	{
		// record abb
		TNode &tnode = _nodes[ref.tnode_id];
		Item &item = tnode.get_item(ref.item_id);
		BVH_ABB abb = item.aabb;

		// remove from old tree
		node_remove_item(ref_id);

		ex.pairable = p_pairable;

		// add to new tree
		int tree_id = _get_tree_id(p_handle);

		// we must choose where to add to tree
		ref.tnode_id = recursive_choose_item_add_node(_root_node_id[tree_id], abb);
		_node_add_item(ref.tnode_id, ref_id, abb);
		refit_upward(ref.tnode_id);
	}

}
