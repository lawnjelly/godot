#pragma once

#include "core/math/vector3.h"
#include "lvector.h"

namespace LM {

class QMC {
	enum {
		NUM_VARIATIONS = 1024
	};

	struct Sample {
		Vector3 dir[NUM_VARIATIONS];
	};

	struct Group {
		LVector<Sample> samples;
	};

public:
	void create(int num_samples);
	void QMC_random_unit_dir(Vector3 &dir, int count, int variation) const {
		dir = _group.samples[count].dir[variation];
	}
	int get_next_variation(int previous) const;
	int random_variation() const;

private:
	void generate_variation(Group &group, int var);

	int _current_variation;
	Group _group;
};

} //namespace LM
