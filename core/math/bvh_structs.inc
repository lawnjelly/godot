#include "bvh_abb.inc"

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
	int32_t subindex;
	uint32_t pairable;
	uint32_t pairable_mask;
	uint32_t pairable_type;
	T * userdata;
};

struct ItemPairs
{
	enum {MAX_PAIRS_PER_ITEM = 7};
	void clear() {num_pairs = 0;}

	// in most cases the in built list will be long enough,
	// if not use the vector
	uint32_t num_pairs;
	// ref ids
	uint32_t pairs[MAX_PAIRS_PER_ITEM];
	LocalVector<uint32_t> extended_pairs;
};

// this is an item OR a child node depending on whether a leaf node
struct Item
{
	ABB aabb;
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
	ABB abb;
	typename ABB::ConvexHull hull;
	typename ABB::Segment segment;
};

// tree node
struct TNode
{
	uint32_t parent_tnode_id_p1;
	//		uint16_t parent_child_number; // which child this is of the parent node
private:
	uint16_t _leaf;
public:
	// items can be child nodes or items (if we are a leaf)
	uint16_t num_items;

	ABB aabb;
	Item items[MAX_ITEMS];

	bool is_leaf() const {return _leaf != 0;}
	void set_leaf(bool b) {_leaf = (uint16_t) b;}

	void clear(bool p_leaf = true)
	{
		num_items = 0;
		parent_tnode_id_p1 = 0;
		set_leaf(p_leaf);

		// for safety set to improbable value
		aabb.set_to_max_opposite_extents();

		// other members are not blanked for speed .. they may be uninitialized
	}

	bool is_full_of_items() const {return num_items >= MAX_ITEMS;}
	bool is_full_of_children() const {return num_items >= MAX_CHILDREN;}

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

	int find_child(uint32_t p_child_node_id)
	{
		CRASH_COND(is_leaf());

		for (int n=0; n<num_items; n++)
		{
			if (items[n].item_ref_id == p_child_node_id)
				return n;
		}

		// not found
		return -1;
	}

	// ALSO NEEDS the reference changing in the tree, cannot be used on its own
	void remove_item_internal(uint32_t item_id)
	{
		CRASH_COND(item_id >= num_items)
		num_items--;
		// swap with last .. unordered
		items[item_id] = items[num_items];
	}

	void update_aabb_internal()
	{
		// should there even be nodes with no children?
		// the 'blank' aabb will screw up parent aabbs
		if (!num_items)
		{
#ifdef BVH_CHECKS
			WARN_PRINT_ONCE("BVH_Tree::TNode AABB is undefined");
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
		aabb.grow_fractional(0.05f);
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
