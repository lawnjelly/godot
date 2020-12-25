
public:
struct ItemRef
{
	uint32_t tnode_id; // -1 is invalid
	uint32_t item_id; // in the tnode
};

// extra info kept in separate parallel list to the references,
// as this is less used as keeps cache better
struct ItemExtra
{
	uint32_t last_updated_tick;
	uint32_t pairable;
	uint32_t pairable_mask;
	uint32_t pairable_type;
	T * userdata;
	int32_t subindex;
};

// this is an item OR a child node depending on whether a leaf node
struct Item
{
	BVH_ABB aabb;
	uint32_t item_ref_id;
};

struct CullParams
{
	int result_count_overall; // both trees
	int result_count; // this tree only
	int result_max;
	T ** result_array;
	int * subindex_array;
	uint32_t mask;

	// optional components for different tests
	Vector3 point;
	BVH_ABB abb;
	typename BVH_ABB::ConvexHull hull;
	typename BVH_ABB::Segment segment;
};


struct ItemPairs
{
	enum {MAX_PAIRS_PER_ITEM = 8};

	struct Link
	{
		void set(BVHHandle h, void * ud) {handle = h; userdata = ud;}
		BVHHandle handle;
		void * userdata;
	};

	void clear() {num_pairs = 0; extended_pairs.reset();}
	bool extended() const {return extended_pairs.size() != 0;}

	// in most cases the in built list will be long enough,
	// if not use the vector
	int32_t num_pairs;
	Link pairs[MAX_PAIRS_PER_ITEM];
	LocalVector<Link> extended_pairs;

	void add_pair_to(BVHHandle h, void * p_userdata)
	{
		if (!extended())
		{
			// simplest case
			if (num_pairs < MAX_PAIRS_PER_ITEM)
			{
				pairs[num_pairs++].set(h, p_userdata);
				return;
			}

			// needs to become extended
			_extend();
		}
		Link temp;
		temp.set(h, p_userdata);

		extended_pairs.push_back(temp);
		num_pairs++;
	}

	bool contains_pair_to(BVHHandle h)
	{
		if (!extended())
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (pairs[n].handle == h)
				{
					return true;
				}
			}
		}
		else
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (extended_pairs[n].handle == h)
				{
					return true;
				}
			}
		} // extended
		return false;
	}

	// return success
	void * remove_pair_to(BVHHandle h)
	{
//		int num_pairs_before = num_pairs;
		void * userdata = nullptr;

		if (!extended())
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (pairs[n].handle == h)
				{
					userdata = pairs[n].userdata;
					// remove unordered
					num_pairs--;
					pairs[n] = pairs[num_pairs];
				}
			}
		}
		else
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (extended_pairs[n].handle == h)
				{
					userdata = extended_pairs[n].userdata;
					extended_pairs.remove_unordered(n);
					num_pairs--;
				}
			}
		} // extended

		//return num_pairs < num_pairs_before;
		return userdata;
	}

private:
	void _extend()
	{
		CRASH_COND(num_pairs > MAX_PAIRS_PER_ITEM);
		CRASH_COND(extended());
		extended_pairs.resize(num_pairs);
		for (int n=0; n<num_pairs; n++)
		{
			extended_pairs[n] = pairs[n];
		}
	}

	void _deextend()
	{
		if (extended())
		{
			if (num_pairs <= MAX_PAIRS_PER_ITEM)
			{
				// copy
				for (int n=0; n<num_pairs; n++)
				{
					pairs[n] = extended_pairs[n];
				}
				extended_pairs.reset();
			}
		}
	}
};

struct TLeaf
{
	uint32_t num_items;
	Item items[MAX_ITEMS];

	Item * request_item(uint32_t &id)
	{
		if (num_items < MAX_ITEMS)
		{
			id = num_items;
			return &items[num_items++];
		}
		return nullptr;
	}

	const Item &get_item(int id) const {return items[id];}
	Item &get_item(int id) {return items[id];}

	// ALSO NEEDS the reference changing in the tree, cannot be used on its own
	void remove_item_internal(uint32_t item_id)
	{
		CRASH_COND(item_id >= num_items);
		num_items--;
		// swap with last .. unordered
		items[item_id] = items[num_items];
	}
	
};

// tree node
struct TNode
{
	BVH_ABB aabb;
	// either number of children if positive
	// or leaf id if negative (leaf id 0 is disallowed)
	union {
		int32_t num_children;
		int32_t neg_leaf_id;
	};
	uint32_t parent_id; // or -1
	uint16_t children[MAX_CHILDREN];

	bool is_leaf() const {return num_children < 0;}
	void set_leaf_id(int id) {neg_leaf_id = -id;}

	void clear()
	{
		num_children = 0;
		parent_id = -1;

		// for safety set to improbable value
		aabb.set_to_max_opposite_extents();

		// other members are not blanked for speed .. they may be uninitialized
	}

//	bool is_full_of_items() const {return num_items >= MAX_ITEMS;}
	bool is_full_of_children() const {return num_children >= MAX_CHILDREN;}


	int find_child(uint32_t p_child_node_id)
	{
		CRASH_COND(is_leaf());

		for (int n=0; n<num_children; n++)
		{
			if (children[n] == p_child_node_id)
				return n;
		}

		// not found
		return -1;
	}


	void update_aabb_internal()
	{
		// should there even be nodes with no children?
		// the 'blank' aabb will screw up parent aabbs
		if (!num_items)
		{
#ifdef BVH_CHECKS
			WARN_PRINT("BVH_Tree::TNode AABB is undefined");
#endif
			aabb.set_to_max_opposite_extents();
			return;
		}

		// items
		aabb = items[0].aabb;
		for (int n=1; n<num_items; n++)
		{
			aabb.merge(items[n].aabb);
		}

		// we make the aabb larger than necessary, to allow most item
		// moves to be no-ops.
//		if (is_leaf())
//			aabb.grow_fractional(0.05f);
	}
};


// instead of using linked list we maintain
// item references (for quick lookup)
PooledList<ItemRef, true> _refs;
PooledList<ItemExtra, true> _extra;
PooledList<ItemPairs> _pairs;

PooledList<TNode, true> _nodes;

// instead of translating directly to the userdata output,
// we keep an intermediate list of hits as reference IDs, which can be used
// for pairing collision detection
LocalVector<uint32_t, uint32_t, true> _cull_hits;

// we now have multiple root nodes, allowing us to store
// more than 1 tree. This can be more efficient, while sharing the same
// common lists
enum {NUM_TREES = 2,};

int _root_node_id[NUM_TREES];
