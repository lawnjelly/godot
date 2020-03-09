#pragma once

#include "batcher.h"

namespace Batch {

class Playback : public Batcher
{
protected:
	void Playback_Change_Item(const Batch &batch);
	void Playback_Change_ItemGroup(const Batch &batch);
	void Playback_Change_ItemGroup_End(const Batch &batch);
};


} // namespace
