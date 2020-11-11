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
