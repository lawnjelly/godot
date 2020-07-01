#include "lqmc.h"

namespace LM
{

void QMC::Create(int num_samples)
{
	m_CurrentVariation = 0;
	m_Group.m_Samples.resize(num_samples);

	for (int n=0; n<num_samples; n++)
	{
		GenerateSample(m_Group, n);
	}

}


void QMC::RandomUnitDir(Vector3 &dir) const
{
	while (true)
	{
		dir.x = Math::random(-1.0f, 1.0f);
		dir.y = Math::random(-1.0f, 1.0f);
		dir.z = Math::random(-1.0f, 1.0f);

		float l = dir.length();
		if (l > 0.001f)
		{
			dir /= l;
			return;
		}
	}
}

void QMC::GenerateSample(Group &group, int sample)
{
	const int nTestSamples = 8;
	Vector3 tests[nTestSamples];

	for (int var=0; var<NUM_VARIATIONS; var++)
	{
		// choose some random test directions
		for (int n=0; n<nTestSamples; n++)
		{
			RandomUnitDir(tests[n]);
		}

		// find the best
		int best_t = 0;
		float best_dot = 1.0f;

		// compare to all the rest
		for (int n=0; n<sample-1; n++)
		{
			const Vector3 &existing_dir = group.m_Samples[n].dir[var];

			for (int t=0; t<nTestSamples; t++)
			{
				float dot = existing_dir.dot(tests[t]);
				if (dot < best_dot)
				{
					best_dot = dot;
					best_t = t;
				}
			}
		}

		// now we know the best, add it to the list
		group.m_Samples[sample].dir[var] = tests[best_t];
	} // for variation
}

void QMC::QMCRandomUnitDir(Vector3 &dir, int count)
{
	dir = m_Group.m_Samples[count].dir[m_CurrentVariation];
	m_CurrentVariation++;

	// wraparound
	if (m_CurrentVariation >= NUM_VARIATIONS)
		m_CurrentVariation = 0;
}


} // namespace
