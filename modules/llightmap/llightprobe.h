#pragma once

#include "llighttypes.h"
#include "lvector.h"

namespace LM {

class LightMapper;

class LightProbe {
public:
	void save(FileAccess *f);
	void save_secondary(FileAccess *f);
	struct Contribution {
		uint32_t light_id;
		float power; // linear
	};

	//	LightProbe()
	//	{
	//		m_fLight = 0.0f;
	//	}
	//	float m_fLight;
	LVector<Contribution> contributions;
	FColor color_indirect;
};

class LightProbes {
public:
	int create(LightMapper &lm);
	void process(int stage);
	void save();

private:
	void normalize_indirect();
	void save(String pszFilename);
	void calculate_probe(const Vec3i &pt);
	void calculate_probe_old(const Vec3i &pt);
	float calculate_power(const LightMapper_Base::LLight &light, float dist, const Vector3 &pos) const;

	void save_Vector3(FileAccess *f, const Vector3 &v);

	LightProbe *get_probe(const Vec3i &pt) {
		unsigned int ui = get_probe_num(pt);
		if (ui < data.probes.size()) {
			return &data.probes[ui];
		}
		return nullptr;
	}

	void get_probe_position(const Vec3i &pt, Vector3 &pos) const {
		pos = data.pt_min;
		pos.x += pt.x * data.voxel_size.x;
		pos.y += pt.y * data.voxel_size.y;
		pos.z += pt.z * data.voxel_size.z;
	}

	unsigned int get_probe_num(const Vec3i &pt) const {
		return (pt.z * data.dim_x_times_y) + (pt.y * data.dims.x) + pt.x;
	}

	struct Data {
		// number of probes on each axis
		Vec3i dims;
		int dim_x_times_y;

		// world size of each voxel separating probes
		Vector3 voxel_size;

		// bottom left probe point, can be used to derive the other points using the voxel size
		Vector3 pt_min;

		LVector<LightProbe> probes;
		LightMapper *light_mapper;
	} data;
};

} //namespace LM
