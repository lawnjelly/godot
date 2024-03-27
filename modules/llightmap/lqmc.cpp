#include "lqmc.h"
#include "llighttypes.h"

namespace LM {

void QMC::create(int num_samples) {
	_current_variation = 0;
	_group.samples.resize(num_samples);

	for (int n = 0; n < NUM_VARIATIONS; n++) {
		generate_variation(_group, n);
	}
}

void QMC::generate_variation(Group &group, int var) {
	int nSamples = _group.samples.size();

	const int nTestSamples = 8;
	Vector3 tests[nTestSamples];
	float dots[nTestSamples];

	for (int s = 0; s < nSamples; s++) {
		// choose some random test directions
		for (int n = 0; n < nTestSamples; n++) {
			generate_random_unit_dir(tests[n]);
		}

		// find the best
		int best_t = 0;

		// blank dots
		for (int d = 0; d < nTestSamples; d++) {
			dots[d] = -1.0f;
		}

		// compare to all the rest
		for (int n = 0; n < s - 1; n++) {
			const Vector3 &existing_dir = group.samples[n].dir[var];

			for (int t = 0; t < nTestSamples; t++) {
				float dot = existing_dir.dot(tests[t]);

				if (dot > dots[t]) {
					dots[t] = dot;
				}
			}
		}

		// find the best test
		for (int t = 0; t < nTestSamples; t++) {
			if (dots[t] < dots[best_t]) {
				best_t = t;
			}
		}

		// now we know the best, add it to the list
		group.samples[s].dir[var] = tests[best_t];
	} // for variation
}

int QMC::random_variation() const {
	return Math::rand() % NUM_VARIATIONS;
}

int QMC::get_next_variation(int previous) const {
	int next = previous + 1;
	if (next >= NUM_VARIATIONS)
		next = 0;

	return next;
}

//void QMC::QMCRandomUnitDir(Vector3 &dir, int count, int variation)
//{
//	if (count == 0)
//	{
//		m_CurrentVariation++;

//		// wraparound
//		if (m_CurrentVariation >= NUM_VARIATIONS)
//			m_CurrentVariation = 0;
//	}

//	dir = m_Group.m_Samples[count].dir[m_CurrentVariation];
//}

} //namespace LM
