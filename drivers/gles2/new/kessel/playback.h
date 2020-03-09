#pragma once

#include "batcher_state.h"

namespace Batch {

class Playback : public BatcherState
{
public:
	Playback() {m_bDryRun = true;}

protected:
	void Playback_SetDryRun(bool b) {m_bDryRun = b;}

	void Playback_Change_Item(const Batch &batch);
	void Playback_Change_ItemGroup(const Batch &batch);
	void Playback_Change_ItemGroup_End(const Batch &batch);

private:
	// when filling we do a dry run and don't change GL states,
	// we have to repeat the same logic to fill
	bool m_bDryRun;
};


} // namespace
