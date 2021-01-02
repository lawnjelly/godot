//#define BVH_USE_LOCAL_PAIRS
public:

// note .. maybe this can be attached to another node structure?
// depends which works best for cache.
struct ItemPairs
{
#ifdef BVH_USE_LOCAL_PAIRS
	enum {MAX_PAIRS_PER_ITEM = 32};
#endif

	struct Link
	{
		void set(BVHHandle h, void * ud) {handle = h; userdata = ud;}
		BVHHandle handle;
		void * userdata;
	};

	void clear() {num_pairs = 0; extended_pairs.reset();}
	
#ifdef BVH_USE_LOCAL_PAIRS
	bool extended() const {return extended_pairs.size() != 0;}
#endif

	AABB expanded_aabb;
	
	// in most cases the in built list will be long enough,
	// if not use the vector
	int32_t num_pairs;
#ifdef BVH_USE_LOCAL_PAIRS
	Link pairs[MAX_PAIRS_PER_ITEM];
#endif
	LocalVector<Link> extended_pairs;

	void add_pair_to(BVHHandle h, void * p_userdata)
	{
#ifdef BVH_USE_LOCAL_PAIRS
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
#endif
		Link temp;
		temp.set(h, p_userdata);

		extended_pairs.push_back(temp);
		num_pairs++;
	}

	uint32_t find_pair_to(BVHHandle h) const
	{
#ifdef BVH_USE_LOCAL_PAIRS
		if (!extended())
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (pairs[n].handle == h)
				{
					return n;
				}
			}
		}
		else
#endif
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (extended_pairs[n].handle == h)
				{
					return n;
				}
			}
		} // extended
		return -1;
		
	}
	
	bool contains_pair_to(BVHHandle h) const
	{
		return find_pair_to(h) != -1;
	}

	// return success
	void * remove_pair_to(BVHHandle h)
	{
//		int num_pairs_before = num_pairs;
		void * userdata = nullptr;

#ifdef BVH_USE_LOCAL_PAIRS
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
					break;
				}
			}
		}
		else
#endif
		{
			for (int n=0; n<num_pairs; n++)
			{
				if (extended_pairs[n].handle == h)
				{
					userdata = extended_pairs[n].userdata;
					extended_pairs.remove_unordered(n);
					num_pairs--;
					break;
				}
			}
			
		} // extended

		//return num_pairs < num_pairs_before;
		return userdata;
	}

	void update()
	{
#ifdef BVH_USE_LOCAL_PAIRS
		_deextend();
#endif
	}
#ifdef BVH_USE_LOCAL_PAIRS
	
private:
	void _extend()
	{
		//print_line("_extend()");
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
		// call AFTER processing
		
		// deextend?
		if (num_pairs > (MAX_PAIRS_PER_ITEM / 2))
			return;
		
		if (extended())
		{
			//print_line("_deextend()");
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
#endif
};


/*
void pairs_reset_from_item(BVHHandle p_handle)
{
	ItemPairs &p_from = _pairs[p_handle.id()];

	// remove from pairing list for every partner
	if (!p_from.extended())
	{
		for (int n=0; n<p_from.num_pairs; n++)
		{
			uint32_t to = p_from.pairs[n].id();
			_pairs[to].remove_pair_to(p_handle);
		}
	}
	else
	{
		for (int n=0; n<p_from.extended_pairs.size(); n++)
		{
			uint32_t to = p_from.extended_pairs[n].id();
			_pairs[to].remove_pair_to(p_handle);
		}
	}

	p_from.clear();
}
*/
