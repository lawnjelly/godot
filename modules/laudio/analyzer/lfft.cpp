#include "lfft.h"
#include "../sample/lsample.h"
#include "core/math/math_funcs.h"

typedef std::complex<double> cplx;

// IDFT function
void LFFT::idft(LocalVector<Harmonic> &p_harmonics, LocalVector<float> &r_result, uint32_t p_num_source_samples, uint32_t p_num_samples) {
	int num_harmonics = p_harmonics.size();

	//	int num_samples = num_harmonics;
	//	//	int extended_frame = (num_samples * 2) / 3;
	//	int extended_frame = num_samples;

	r_result.resize(p_num_samples);

	LocalVector<float> phases_a;
	LocalVector<float> phases_b;
	LocalVector<float> phase_interpolators;
	phases_a.resize(num_harmonics);
	phases_b.resize(num_harmonics);
	phase_interpolators.resize(num_harmonics);

	// Phases in over 1 length of the input samples
	const float phase_interpolator_increment = 1.0f / p_num_source_samples;

	for (int32_t n = 0; n < num_harmonics; n++) {
		phases_a[n] = Math::randf() * 2 * M_PI;
		phases_b[n] = Math::randf() * 2 * M_PI;
		phase_interpolators[n] = Math::randf() * 3;
	}

	for (int j = 0; j < (int)p_num_samples; j++) {
		cplx sum(0, 0);
		for (int k = 0; k < num_harmonics; k++) {
			const Harmonic &h = p_harmonics[k];

			double angle = 2 * M_PI * h.h * j / p_num_source_samples;

			// Decide on the phase. This is the cunning bit.
			double phase_offset = 0;
			{
				float &phase_interpolator = phase_interpolators[k];
				float &phase_a = phases_a[k];
				float &phase_b = phases_b[k];

				//float old_phase_interpolator = phase_interpolator;
				phase_interpolator += phase_interpolator_increment;

				// Wrap
				if (phase_interpolator >= 3.0f) {
					phase_interpolator -= 3.0f;
					phase_a = phase_b;

					// random new phase target
					phase_b = Math::randf() * 2 * M_PI;
				}

				// Interpolate
				if (phase_interpolator < 1.0f) {
					float f = phase_interpolator;
					phase_offset = phase_a + ((phase_b - phase_a) * f);
				} else {
					phase_offset = phase_b;
				}
			}

			// phase modification
			angle += phase_offset;

			//double angle = 2 * M_PI * k * j / num_records;
			sum += h.c * cplx(cos(angle), -sin(angle));
		}
		sum /= p_num_source_samples;
		r_result[j] = real(sum);
	}
}

void LFFT::dft(LocalVector<cplx> &x, LocalVector<cplx> &X) {
	int num_samples = x.size();
	//	int extended_frame = (num_samples * 2) / 3;
	int extended_frame = num_samples;

	// For through frequencies
	for (int k = 0; k < num_samples; ++k) {
		cplx sum = 0;
		if ((k % 100) == 0) {
			print_line("dft " + itos(k));
		}

		// Check each sample
		for (int j = 0; j < num_samples; ++j) {
			double angle = -2 * M_PI * k * j / extended_frame;
			//double angle = -2 * M_PI * k * j / num_samples;
			sum += x[j] * cplx(cos(angle), sin(angle));

			//sum += x[j] * exp(cplx(0, -2.0 * M_PI * k * j / n));
		}
		X[k] = sum;
	}
}

void LFFT::process(uint32_t p_start, uint32_t p_length) {
	//p_length = MIN(p_length, 1024);

	// Fill data
	LocalVector<cplx> x;
	x.resize(p_length);
	for (uint32_t n = 0; n < p_length; n++) {
		uint32_t sample_id = p_start + n;
		float s = _sample->safe_get_f(sample_id);
		x[n] = cplx(s, 0);
	}

	LocalVector<cplx> X;
	X.resize(p_length);
	dft(x, X);

	// Shrink the harmonics
	LocalVector<Harmonic> harmonics;
	// Print the magnitude of the DFT
	uint32_t present = 0;
	uint32_t removed = 0;

	for (uint32_t k = 0; k < X.size(); ++k) {
		float magnitude = abs(X[k]);
		print_line("magnitude " + String(Variant(magnitude)));
		//		if (false)
		if (magnitude < 10.0) {
			//X[k] = cplx(0, 0);
			removed++;
		} else {
			Harmonic h;
			h.h = k;
			h.c = X[k];
			harmonics.push_back(h);
			present++;
		}
	}

	print_line("DFT present " + itos(present) + ", removed " + itos(removed));

	// recreate
	LocalVector<float> result;
	idft(harmonics, result, X.size(), X.size() * 4);

	LSample sresult;
	sresult.create(4, 1, _sample->get_format().sample_rate, p_length);

	for (uint32_t n = 0; n < p_length; n++) {
		sresult.set_f(n, 0, _sample->safe_get_f(n));
	}
	sresult.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/dft.wav");

	uint32_t extended_length = result.size() * 1;
	sresult.create(4, 1, _sample->get_format().sample_rate, extended_length);

	for (uint32_t n = 0; n < extended_length; n++) {
		sresult.set_f(n, 0, result[n % result.size()]);
	}

	sresult.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/idft.wav");
}

void LFFT::_run(uint32_t p_start, uint32_t p_length) {
	//	uint32_t length = 1 << p_power;
	//	uint32_t max_stride = length / 4;

	for (uint32_t h = 0; h < _num_harmonics; h++) {
		float total_cos = 0;
		float total_sin = _run_freq(p_start, p_length, h, total_cos);

		float wavelength = p_length / (float)(h + 1);
		float freq = _sample->get_format().sample_rate / wavelength;

		if (total_sin > 0) {
			print_line("start " + itos(p_start) + ", harmonic " + itos(h) + ", freq " + itos(freq) + "hz , sin " + String(Variant(total_sin)));
		}
		if (total_cos > 0) {
			print_line("start " + itos(p_start) + ", harmonic " + itos(h) + ", freq " + itos(freq) + "hz , cos " + String(Variant(total_cos)));
		}
	}
}

float LFFT::_run_freq(uint32_t p_start, uint32_t p_length, uint32_t p_harmonic, float &r_total_cos) {
	float total_sin = 0;
	float total_cos = 0;

	uint32_t i = p_harmonic * p_length;

	for (uint32_t n = 0; n < p_length; n++) {
		uint32_t sample_id = p_start + n;
		float s = _sample->safe_get_f(sample_id);

		float sins = s * _sin_window[i];
		total_sin += sins;

		float coss = s * _cos_window[i];
		total_cos += coss;

		i++;
	}

	r_total_cos = total_cos;
	return total_sin;
}

void LFFT::_construct_window(uint32_t p_length) {
	_sin_window.resize(p_length * _num_harmonics);
	_cos_window.resize(p_length * _num_harmonics);

	uint32_t i = 0;
	for (uint32_t h = 0; h < _num_harmonics; h++) {
		for (uint32_t n = 0; n < p_length; n++) {
			double f = n / (double)p_length;

			// which harmonic
			f *= (h + 1);
			//print_line("f "+ String(Variant(f)));

			f *= Math_PI * 2.0;

			float s = Math::sin(f);
			_sin_window[i] = s;

			//print_line("\th " + itos(h) + ", n " + itos(n) + " : " + String(Variant(s)));

			s = Math::cos(f);
			_cos_window[i] = s;
			i++;
		}
	}
}

void LFFT::run(const LSample &p_sample) {
	_sample = &p_sample;

	//	uint32_t max_power = 9;
	//uint32_t window = 1024;
	uint32_t window = p_sample.get_format().num_samples;
	_num_harmonics = 31;

	//_construct_window(window);

	for (uint32_t n = 0; n < 1; n++) {
		//_run(n, window);
		process(n, window);
	}

	//	for (uint32_t n=0; n<p_sample.get_format().num_samples; n++)
	//	{

	//	}
}

void LFFT::idft_simple(LocalVector<cplx> X, LocalVector<float> &r_result) {
	int n = X.size();
	r_result.resize(n);

	for (int j = 0; j < n; j++) {
		cplx sum(0, 0);
		for (int k = 0; k < n; k++) {
			double angle = 2 * M_PI * k * j / n;
			sum += X[k] * cplx(cos(angle), -sin(angle));
		}
		sum /= n;
		r_result[j] = real(sum);
	}
}

void LFFT::dft_simple(LocalVector<cplx> &x, LocalVector<cplx> &X) {
	int n = x.size();
	// For through frequencies
	for (int k = 0; k < n; ++k) {
		cplx sum = 0;

		// Check each sample
		for (int j = 0; j < n; ++j) {
			double angle = -2 * M_PI * k * j / n;
			sum += x[j] * cplx(cos(angle), sin(angle));
			//sum += x[j] * exp(cplx(0, -2.0 * M_PI * k * j / n));
		}
		X[k] = sum;
	}
}

// IDFT function
void LFFT::idft_complex(LocalVector<cplx> X, LocalVector<float> &r_result, uint32_t p_num_samples) {
	int num_records = X.size();
	//vector<double> x(n);

	int num_samples = num_records;
	//	int extended_frame = (num_samples * 2) / 3;
	int extended_frame = num_samples;

	if (p_num_samples != UINT32_MAX) {
		num_samples = p_num_samples;
	}
	r_result.resize(num_samples);

	LocalVector<float> phases_a;
	LocalVector<float> phases_b;
	LocalVector<float> phase_interpolators;
	phases_a.resize(num_records);
	phases_b.resize(num_records);
	phase_interpolators.resize(num_records);
	const float phase_interpolator_increment = 1.0f / num_records;

	for (int32_t n = 0; n < num_records; n++) {
		phases_a[n] = Math::randf() * 2 * M_PI;
		phases_b[n] = Math::randf() * 2 * M_PI;
		phase_interpolators[n] = Math::randf() * 3;
	}

	for (int j = 0; j < num_samples; j++) {
		cplx sum(0, 0);
		for (int k = 0; k < num_records; k++) {
			double angle = 2 * M_PI * k * j / extended_frame;

			// Decide on the phase. This is the cunning bit.
			double phase_offset = 0;
			{
				float &phase_interpolator = phase_interpolators[k];
				float &phase_a = phases_a[k];
				float &phase_b = phases_b[k];

				//float old_phase_interpolator = phase_interpolator;
				phase_interpolator += phase_interpolator_increment;

				// Change of state
				//				if ((int) old_phase_interpolator != (int) phase_interpolator)
				//				{
				//				}

				// Wrap
				if (phase_interpolator >= 3.0f) {
					phase_interpolator -= 3.0f;
					phase_a = phase_b;

					// random new phase target
					phase_b = Math::randf() * 2 * M_PI;
				}

				// Interpolate
				if (phase_interpolator < 1.0f) {
					float f = phase_interpolator;
					phase_offset = phase_a + ((phase_b - phase_a) * f);
				} else {
					phase_offset = phase_b;
				}
			}

			// phase modification
			angle += phase_offset;

			//double angle = 2 * M_PI * k * j / num_records;
			sum += X[k] * cplx(cos(angle), -sin(angle));

			/*
			double prev_angle = 2 * M_PI * k * (j % num_records) / num_records;

			 float diff = prev_angle - angle;
			 if (Math::absf(diff) > 0.01f)
			 {
				 //print_line("angle diff " + itof(diff));
			 }

			  if (k == (num_records-1))
			  {
				  print_line("sample is " + itos(j) + ", angle is " + itof(angle));

			   }
   */
		}
		sum /= num_records;
		r_result[j] = real(sum);

		//		if (j >= num_records)
		//		{
		//			float diff = r_result[j] - r_result[j-num_records];
		//			if (Math::absf(diff) > 0.01f)
		//			{
		//				print_line("diff " + itof(diff));
		//			}
		//		}
	}

	//return x;
}
