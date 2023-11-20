#pragma once

#include "core/local_vector.h"
#include <complex>

class LSample;

class LFFT {
	const LSample *_sample = nullptr;
	LocalVector<float> _sin_window;
	LocalVector<float> _cos_window;
	uint32_t _num_harmonics = 0;

	struct Harmonic {
		uint32_t h;
		std::complex<double> c;
	};

	void _run(uint32_t p_start, uint32_t p_length);
	float _run_freq(uint32_t p_start, uint32_t p_length, uint32_t p_harmonic, float &r_total_cos);
	void _construct_window(uint32_t p_length);

	void dft(LocalVector<std::complex<double>> &x, LocalVector<std::complex<double>> &X);
	void idft(LocalVector<Harmonic> &p_harmonics, LocalVector<float> &r_result, uint32_t p_num_source_samples, uint32_t p_num_samples);

	void process(uint32_t p_start, uint32_t p_length);

	void dft_simple(LocalVector<std::complex<double>> &x, LocalVector<std::complex<double>> &X);
	void idft_simple(LocalVector<std::complex<double>> X, LocalVector<float> &r_result);
	void idft_complex(LocalVector<std::complex<double>> X, LocalVector<float> &r_result, uint32_t p_num_samples = UINT32_MAX);

	String itof(float f) const { return String(Variant(f)); }

public:
	void run(const LSample &p_sample);
};
