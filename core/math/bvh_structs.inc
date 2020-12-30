
public:
struct ItemRef
{
	uint32_t tnode_id; // -1 is invalid
	uint32_t item_id; // in the leaf
};

// extra info kept in separate parallel list to the references,
// as this is less used as keeps cache better
struct ItemExtra
{
	uint32_t last_updated_tick;
	uint32_t pairable;
	uint32_t pairable_mask;
	uint32_t pairable_type;
	
	int32_t subindex;
	
	// the active reference is a separate list of which references
	// are active so that we can slowly iterate through it over many frames for
	// slow optimize.
	uint32_t active_ref_id;
	
	T * userdata;
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



struct TLeaf
{
	uint32_t num_items;
	//Item items[MAX_ITEMS];

	uint32_t item_ref_ids[MAX_ITEMS];
	// separate data orientated lists for faster SIMD traversal
	BVH_ABB aabbs[MAX_ITEMS];
	
	BVH_ABB &get_aabb(uint32_t p_id) {return aabbs[p_id];}
	const BVH_ABB &get_aabb(uint32_t p_id) const {return aabbs[p_id];}

	uint32_t &get_item_ref_id(uint32_t p_id) {return item_ref_ids[p_id];}
	const uint32_t &get_item_ref_id(uint32_t p_id) const {return item_ref_ids[p_id];}
	
	void remove_item_unordered(uint32_t p_id)
	{
		CRASH_COND(p_id >= num_items);
		num_items--;
		aabbs[p_id] = aabbs[num_items];
		item_ref_ids[p_id] = item_ref_ids[num_items];
	}
	
	void clear() {num_items = 0;}
	
	bool is_full() const {return num_items >= MAX_ITEMS;}
	
	uint32_t request_item()
	{
		if (num_items < MAX_ITEMS)
		{
			uint32_t id = num_items;
			num_items++;
			return id;
		}
		return -1;
	}

	//const Item &get_item(int id) const {return items[id];}
	//Item &get_item(int id) {return items[id];}

	// ALSO NEEDS the reference changing in the tree, cannot be used on its own
//	void remove_item_internal(uint32_t item_id)
//	{
//		CRASH_COND(item_id >= num_items);
//		num_items--;
//		// swap with last .. unordered
//		items[item_id] = items[num_items];
//	}
	
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

	// height in the tree, where leaves are 0, and all above are 1+
	// (or the highest where there is a tie off)
	int32_t height;

	bool is_leaf() const {return num_children < 0;}
	void set_leaf_id(int id) {neg_leaf_id = -id;}
	int get_leaf_id() const {return -neg_leaf_id;}
	
	void clear()
	{
		num_children = 0;
		parent_id = -1;
		height = 0;

		// for safety set to improbable value
		aabb.set_to_max_opposite_extents();

		// other members are not blanked for speed .. they may be uninitialized
	}

//	bool is_full_of_items() const {return num_items >= MAX_ITEMS;}
	bool is_full_of_children() const {return num_children >= MAX_CHILDREN;}

	void remove_child_internal(uint32_t child_num)
	{
		children[child_num] = children[num_children-1];
		num_children--;
	}

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
};


// instead of using linked list we maintain
// item references (for quick lookup)
PooledList<ItemRef, true> _refs;
PooledList<ItemExtra, true> _extra;
PooledList<ItemPairs> _pairs;

// these 2 are not in sync .. nodes != leaves!
PooledList<TNode, true> _nodes;
PooledList<TLeaf, true> _leaves;

// we can maintain an un-ordered list of which references are active,
// in order to do a slow incremental optimize of the tree over each frame.
// This will work best if dynamic objects and static objects are in a different tree.
LocalVector<uint32_t, uint32_t, true> _active_refs;
uint32_t _current_active_ref = 0;

// instead of translating directly to the userdata output,
// we keep an intermediate list of hits as reference IDs, which can be used
// for pairing collision detection
LocalVector<uint32_t, uint32_t, true> _cull_hits;

// we now have multiple root nodes, allowing us to store
// more than 1 tree. This can be more efficient, while sharing the same
// common lists
enum {NUM_TREES = 2,};

int _root_node_id[NUM_TREES];
int _current_tree = 0;
