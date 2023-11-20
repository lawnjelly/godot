#include "lanalyzer.h"
#include "lfft.h"

void LAnalyzer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_sample", "filename"), &LAnalyzer::load_sample);
	ClassDB::bind_method(D_METHOD("process_sample", "pos"), &LAnalyzer::process_sample);
	ClassDB::bind_method(D_METHOD("finalize"), &LAnalyzer::finalize);

	ClassDB::bind_method(D_METHOD("get_sample_length"), &LAnalyzer::get_sample_length);
	ClassDB::bind_method(D_METHOD("get_sample", "pos"), &LAnalyzer::get_sample);

	ClassDB::bind_method(D_METHOD("get_num_waves"), &LAnalyzer::get_num_waves);
	ClassDB::bind_method(D_METHOD("get_wave_start", "wave"), &LAnalyzer::get_wave_start);
	ClassDB::bind_method(D_METHOD("get_wave_length", "wave"), &LAnalyzer::get_wave_length);
}

void LAnalyzer::log(String p_sz) {
	print_line(p_sz);
}

int32_t LAnalyzer::get_sample_length() {
	return data.source_sample.get_format().num_samples;
}

float LAnalyzer::get_sample(int32_t p_pos) {
	ERR_FAIL_COND_V(p_pos >= (int32_t)data.source_sample.get_format().num_samples, 0.0f);
	return data.source_sample.safe_get_f(p_pos);
}

int32_t LAnalyzer::get_num_waves() {
	return data.waves.size();
}

int32_t LAnalyzer::get_wave_start(int32_t p_wave) {
	ERR_FAIL_COND_V(p_wave >= (int32_t)data.waves.size(), 0);
	return data.waves[p_wave].start;
}

int32_t LAnalyzer::get_wave_length(int32_t p_wave) {
	ERR_FAIL_COND_V(p_wave >= (int32_t)data.waves.size(), 0);
	return data.waves[p_wave].length;
}

int32_t LAnalyzer::process_sample(int32_t p_pos) {
	log("\n");
	log("PROCESS SAMPLE from " + itos(p_pos));
	LSample &sample = data.source_sample;
	const LAudioFormat &format = sample.get_format();

	uint32_t pos = p_pos;

	// End
	if (pos >= format.num_samples) {
		return -1;
	}

	Wave wave;
	pos = find_next_crossing(sample, pos, wave);

	// No crossing found
	if (pos == UINT32_MAX) {
		return -1;
	}
	log("BEST CROSSING " + itos(pos));

	wave.start = pos;
	wave.ramp = 0;

	bool correlated = autocorrelate(sample, pos, UINT32_MAX, wave);

	// More than 1 wave?
	if (correlated && wave.num_waves > 1) {
		// blank
		uint32_t first_wave_length = wave.length / wave.num_waves;
		// add a bit
		first_wave_length *= 3;
		first_wave_length /= 2;
		//wave = Wave();

		correlated = autocorrelate(sample, pos, first_wave_length, wave);
	}

	if (correlated) {
		log("found crossing " + itos(pos) + ", loop " + itos(wave.length));
		data.waves.push_back(wave);

		//  move on to close to the end of the autocorrelate
		if (wave.length > 4) {
			pos += wave.length - 4;
		} else {
			pos++;
		}
	} else {
		log("\tautocorrelate failed " + itos(pos));
		pos++;
	}

	log("\tbest_error " + String(Variant(data.best_fit)) + ", best_window " + itos(data.best_window));

	return pos;
}

bool LAnalyzer::load_sample(String p_filename) {
	//create_dummy_data();
	//return true;

	LSample &sample = data.source_sample;

	if (!sample.load_wav(p_filename)) {
		return false;
	}

	sample.convert_to(4);
	sample.normalize();

	//sample.resize(sample.get_format().num_samples / 3);

	LFFT fft;
	fft.run(sample);
	return true;

	//	int32_t n=0;
	//	while (n != -1)
	//	{
	//		n = process_sample(n);
	//	}

	return true;
}

void LAnalyzer::finalize() {
	create_wavedata();
	//output_waves();
	reconstruct();
}

void LAnalyzer::create_dummy_data() {
	data.source_sample.create(4, 1, 48000, 48000);

	uint32_t num_samples = data.source_sample.get_format().num_samples;

	for (uint32_t n = 0; n < num_samples; n++) {
		float f = Math::sin(n * 442 * 2 * Math_PI / (float)48000);
		data.source_sample.set_f(n, 0, f);
	}

	//data.source_sample.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/dummy_sourcedata.wav");

	Wave w;
	w.start = 0;
	w.length = 48000 / 442.0f;
	data.waves.push_back(w);

	w.start = w.length;
	w.length = w.length * 3;
	w.num_waves = 3;
	data.waves.push_back(w);

	create_wavedata();

	// edit the second wave
	data.waves[1].length *= 3;
	data.waves[1].length /= 2;

	reconstruct();
}

void LAnalyzer::count_waves(Wave &r_wave) {
	bool crossed = true;
	r_wave.num_waves = 0;

	float threshold = r_wave.loudest_sample * 0.5f;

	for (uint32_t n = 0; n < r_wave.wavedata_length; n++) {
		uint32_t sample_id = n + r_wave.wavedata_start;
		float s = data.wavedata.get_f(sample_id);

		if (crossed) {
			if (s > threshold) {
				r_wave.num_waves += 1;
				crossed = false;
			}
		} else {
			if (s < 0) {
				crossed = true;
			}
		}
	}

	print_line("\tcounted " + itos(r_wave.num_waves) + " waves.");
}

void LAnalyzer::count_waves_from_source(const LSample &p_sample, Wave &r_wave) {
	bool crossed = true;
	r_wave.num_waves = 0;
	r_wave.loudest_sample = 0;

	// first find loudest
	for (uint32_t n = 0; n < r_wave.length; n++) {
		uint32_t sample_id = n + r_wave.start;
		float s = p_sample.safe_get_f(sample_id);
		r_wave.loudest_sample = MAX(r_wave.loudest_sample, Math::absf(s));
	}

	float threshold = r_wave.loudest_sample * 0.5f;

	for (uint32_t n = 0; n < r_wave.length; n++) {
		uint32_t sample_id = n + r_wave.start;
		float s = p_sample.safe_get_f(sample_id);

		if (crossed) {
			if (s > threshold) {
				r_wave.num_waves += 1;
				crossed = false;
			}
		} else {
			if (s < 0) {
				crossed = true;
			}
		}
	}

	print_line("\tcounted from source " + itos(r_wave.num_waves) + " waves.");
}

void LAnalyzer::create_wavedata() {
	uint32_t count = 0;
	for (uint32_t w = 0; w < data.waves.size(); w++) {
		//const Wave &wave = data.waves[w];
		count++;
	}

	const int32_t WAVEDATA_LENGTH = 256;
	const int32_t SPACER_LENGTH = 128;
	data.wavedata.create(4, 1, 48000, (WAVEDATA_LENGTH + SPACER_LENGTH) * count);

	uint32_t i = 0;
	for (uint32_t w = 0; w < data.waves.size(); w++) {
		Wave &wave = data.waves[w];
		wave.wavedata_start = i;
		wave.wavedata_length = WAVEDATA_LENGTH;

		for (uint32_t n = 0; n < WAVEDATA_LENGTH; n++) {
			DEV_ASSERT(i < data.wavedata.get_format().num_samples);
			float f = n / (float)WAVEDATA_LENGTH;
			float s = get_source_wave_f(wave, f);

			data.wavedata.set_f(i++, 0, s);
			wave.loudest_sample = MAX(wave.loudest_sample, Math::absf(s));
		}

		count_waves(wave);

		i += SPACER_LENGTH;
	}

	data.wavedata.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/wavedata.wav");
}

void LAnalyzer::output_waves() {
	LSample &sample = data.source_sample;
	const LAudioFormat &format = sample.get_format();

	LSample out;
	out.create(4, 1, format.sample_rate, format.num_samples * 2);

	uint32_t num_out_samples = out.get_format().num_samples;
	uint32_t n = 0;

	for (uint32_t w = 0; w < data.waves.size() - 1; w++) {
		const Wave &wave = data.waves[w];

		for (uint32_t i = 0; i < wave.length; i++) {
			if (n >= num_out_samples) {
				break;
			}

			float s = sample.get_f(wave.start + i);
			out.set_f(n, 0, s);
			n++;
		}

		for (uint32_t i = 0; i < 200; i++) {
			if (n >= num_out_samples) {
				break;
			}
			out.set_f(n, 0, 0.0f);
			n++;
		}
	}
	out.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/waves.wav");
}

void LAnalyzer::reconstruct() {
	//	LSample &wd = data.wavedata;
	//	const LAudioFormat &format = wd.get_format();

	LSample out;
	out.create(4, 1, data.source_sample.get_format().sample_rate, data.source_sample.get_format().sample_rate * 2);

	uint32_t num_out_samples = out.get_format().num_samples;
	uint32_t n = 0;

	//	for (uint32_t n = 0; n < num_out_samples; n++) {
	for (uint32_t w = 0; w < data.waves.size() - 1; w++) {
		const Wave &wave_a = data.waves[w];
		const Wave &wave_b = data.waves[w + 1];

		float phase = 0;
		float phases = (wave_a.num_waves + wave_b.num_waves) * 4;

		//		const float multiplier = 16.0f;
		//		uint32_t overall_length = wave_a.length + wave_b.length;
		//		overall_length *= multiplier;

		float phase_a = wave_a.get_frequency();
		float phase_b = wave_b.get_frequency();

		print_line("phase " + itof(phase_a) + " to " + itof(phase_b));

		//phase_a = 0.017;
		//phase_b = 0.017;

		while (phase < phases) {
			if (n >= num_out_samples) {
				break;
			}

			float s0 = wave_a.get_f(data.wavedata, phase);
			float s1 = wave_b.get_f(data.wavedata, phase);

			float f = phase / phases;
			float s = s0 + ((s1 - s0) * f);

			out.set_f(n, 0, s);
			n++;

			// phase increment interpolates between the two
			float phase_increment = phase_a + ((phase_b - phase_a) * f);

			phase += phase_increment;
		}

		if (n >= num_out_samples) {
			break;
		}
	}
	/*
	//		for (uint32_t repeat = 0; repeat < 3; repeat++) {

	uint32_t sample_rate = data.source_sample.get_format().sample_rate;

	float freq_a = wave_a.get_frequency();
	float freq_b = wave_b.get_frequency();

//		freq_a = 1.0 / wave_a.wavedata_length;
//		freq_b = 1.0 / wave_a.wavedata_length;

	float diff_freq = freq_b - freq_a;
	float freq_change_per_sample = diff_freq / overall_length;

	float freq = freq_a;
	float f = 0.0f;

	for (uint32_t i = 0; i < overall_length; i++) {
		if (n >= num_out_samples) {
			break;
		}

		//float s = sample.get_f_interpolated(wave.start + i, wave.start_offset);
		//				float fract = i / (float)wave.length;
		//				float ramp = (1.0f * (1.0f - fract)) + (fract * wave.ramp);
		//				s *= ramp;

		float relative_volume = i / (float)overall_length;
		//float f = relative_volume * multiplier * 2.0f;

		f += freq;
		f = Math::fmod(f, 1.0f);
		freq += freq_change_per_sample;


		//relative_volume = relative_volume < 0.5f ? 0.0f : 1.0f;

		float s = mix_waves(wave_a, wave_b, f, relative_volume);

		out.set_f(n, 0, s);
		n++;
	}

	if (false) {
		for (uint32_t i = 0; i < 200; i++) {
			if (n >= num_out_samples) {
				break;
			}
			out.set_f(n, 0, 0.0f);
			n++;
		}
	}

	//		}
}
//	}
*/

	out.save_wav("/media/baron/Blue/Home/Pel/Godot/Other/LAudio_Synth_Output/out.wav");
}

float LAnalyzer::mix_waves(const Wave &p_wave_a, const Wave &p_wave_b, float p_fraction, float p_relative_volume) {
	float s0 = get_wave_f(p_wave_a, p_fraction);
	float s1 = get_wave_f(p_wave_b, p_fraction);

	return s0 + ((s1 - s0) * p_relative_volume);
}

float LAnalyzer::get_wave_f(const Wave &p_wave, float p_fraction) {
	LSample &d = data.wavedata;
	float f = p_wave.wavedata_length * p_fraction;
	uint32_t pos = f;
	f -= pos;

	return d.get_f_interpolated(p_wave.wavedata_start + pos, f);
}

float LAnalyzer::get_source_wave_f(const Wave &p_wave, float p_fraction) {
	LSample &sample = data.source_sample;

	DEV_ASSERT(p_wave.length);
	float p = p_fraction * p_wave.length;
	int32_t p_int = p;
	float f = p - p_int;

	float s = sample.get_f_interpolated(p_wave.start + p_int, f);
	//float ramp = (1.0f * (1.0f - p_fraction)) + (p_fraction * p_wave.ramp);
	float ramp = 1.0f;
	return s * ramp;
}

float LAnalyzer::calculate_error(const Wave &p_wave, const LSample &p_sample, uint32_t p_start) {
	DEV_ASSERT(p_wave.length);

	float error = 0;
	for (uint32_t n = 0; n < p_wave.length; n++) {
		float s0 = p_sample.safe_get_f(p_wave.start + n);
		float s1 = p_sample.safe_get_f(p_start + n);

		error += Math::absf(s1 - s0);
	}

	error /= p_wave.length;
	return error;
}

bool LAnalyzer::does_crossing_phase_match_existing(const LSample &p_sample, uint32_t p_start) {
	if (!data.waves.size()) {
		return true;
	}

	// Compare to previous wave
	const Wave &wave = data.waves[data.waves.size() - 1];

	float orig_error = calculate_error(wave, p_sample, p_start);

	if (orig_error > 0.4f) {
		return false;
	}

	// Try all possible phases.
	uint32_t phase_start = p_start;
	float s0, s1;
	while (true) {
		phase_start = _find_next_crossing_flip(p_sample, phase_start, s0, s1);

		// Tried all the phases, none were better.
		if (phase_start >= p_start + wave.length) {
			return true;
		}

		float phase_error = calculate_error(wave, p_sample, phase_start);

		// Phased is better?
		if (phase_error < orig_error) {
			return false;
		}

		phase_start++;
	}

	return true;
}

uint32_t LAnalyzer::_find_next_crossing_flip(const LSample &p_sample, uint32_t p_start, float &r_s0, float &r_s1, float p_threshold) {
	uint32_t end = p_sample.get_format().num_samples - 1;

	while (p_start < end) {
		r_s0 = p_sample.safe_get_f(p_start);
		r_s1 = p_sample.safe_get_f(p_start + 1);

		if ((r_s0 < 0) && (r_s1 > 0)) {
			float diff = r_s1 - r_s0;
			if (diff >= p_threshold) {
				return p_start;
			}
		}

		p_start++;
	}

	return UINT32_MAX;
}

uint32_t LAnalyzer::find_next_crossing(const LSample &p_sample, uint32_t p_start, Wave &r_wave) {
	const LAudioFormat &format = p_sample.get_format();
	DEV_ASSERT(format.num_samples);

	uint32_t n = p_start;
	float s0, s1;

	while (true) {
		//n = _find_next_crossing_flip(p_sample, n, s0, s1, 0.01f);
		n = _find_next_crossing_flip(p_sample, n, s0, s1, 0.01f);
		if (n == UINT32_MAX) {
			break;
		}

		if (true) {
			//		if (does_crossing_phase_match_existing(p_sample, n)) {
			float diff = s1 - s0;
			r_wave.start_offset = -s0 / diff;
			return n;
		}

		// Not enough difference, try the next crossing.
		n++;
	}

	return UINT32_MAX;
}

bool LAnalyzer::autocorrelate_find_end_offset(const LSample &p_sample, uint32_t p_start, uint32_t p_length, Wave &r_wave) {
	uint32_t p_end = p_start + p_length;

	// attempt, shift the end point back and forth
	for (uint32_t a = 0; a < 64; a++) {
		float s0 = p_sample.safe_get_f(p_end - 1);
		float s1 = p_sample.safe_get_f(p_end);

		// Both below 0, move end point forward
		if ((s0 < 0) && (s1 < 0)) {
			p_end++;
			r_wave.length += 1;
			continue;
		}
		if ((s0 > 0) && (s1 > 0)) {
			p_end--;
			r_wave.length -= 1;
			continue;
		}

		float diff = s1 - s0;

		if ((s0 > 0) || (s1 < 0) || (diff < 0.0001f)) {
			// something went wrong, this loop point is not good
			log("\tautocorrelate_find_end_offset loop point no good");
			return false;
		}

		r_wave.end_offset = -s0 / (s1 - s0);
		return true;
	}

	log("\tautocorrelate_find_end_offset FAILED for best_window " + itos(p_length));
	return false;
}

bool LAnalyzer::autocorrelate(const LSample &p_sample, uint32_t p_start, uint32_t p_max_length, Wave &r_wave) {
	const LAudioFormat &format = p_sample.get_format();

	uint32_t best_window = UINT32_MAX;
	//float best_error = FLT_MAX;
	float best_fit = 0;
	float best_fit_averaged_by_volume = 0;
	data.best_fit = 0;
	data.best_window = 0;

	// use the previous wave as a guide
	uint32_t win_min = 32;
	uint32_t win_max = (format.sample_rate / 10);

	if (false) {
		//	if (data.waves.size()) {
		const Wave &last_wave = data.waves[data.waves.size() - 1];
		//win_min = (last_wave.length * 3) / 4;
		//win_max = (last_wave.length * 4) / 3;
		win_min = (last_wave.length / 2);
		win_max = (last_wave.length * 2);
	}

	win_max = MIN(p_max_length, win_max);

	for (uint32_t window = win_min; window < win_max; window++) {
		float fit = 0;

		if ((p_start + (window * 2)) >= format.num_samples) {
			// no more windows to try
			break;
		}

		//		const float vol_threshold = 0.1f;
		//		if ((volume0 < vol_threshold) || (volume1 < vol_threshold)) {
		//			// ignore this window, too low volume
		//			continue;
		//		}

		for (uint32_t n = 0; n < window; n++) {
			float s0 = p_sample.get_f(p_start + n);
			float s1 = p_sample.get_f(p_start + n + window);

			fit += (s0 * s1);
			//fit += Math::absf(s1 - s0) * ramp;
		}

		fit /= window;

		//		float average_volume = volume0 / window;
		//		fit /= average_volume;

		//		if (fit > data.best_fit) {
		//			data.best_fit = fit;
		//			data.best_window = window;
		//		}

		if (fit > best_fit) {
			float volume0 = 0;
			float volume1 = 0;

			// calculate volume ramp between the two windows
			for (uint32_t n = 0; n < window; n++) {
				float s0 = p_sample.get_f(p_start + n);
				float s1 = p_sample.get_f(p_start + n + window);

				volume0 += Math::absf(s0);
				volume1 += Math::absf(s1);
			}

			float average_volume = volume0 / window;
			best_fit_averaged_by_volume = fit / average_volume;

			if (best_fit_averaged_by_volume >= 0.5f) {
				float ramp = volume1 / volume0;

				//print_line("error before " + String(Variant(error)));

				//			error = 0;
				//			for (uint32_t n=0; n<window; n++)
				//			{
				//				float s0 = p_sample.get_f(p_start + n);
				//				float s1 = p_sample.get_f(p_start + n + window);
				//				error += Math::absf(s1 - s0) * ramp;
				//			}
				//			error /= window;
				//print_line("error after " + String(Variant(error)));

				best_fit = fit;
				best_window = window;
				data.best_fit = fit;
				data.best_window = window;
				r_wave.ramp = ramp;
			}
		}
	}

	r_wave.length = best_window;

	if (best_window != UINT32_MAX) {
		if (true) {
			//		if (autocorrelate_find_end_offset(p_sample, r_wave.start, r_wave.length, r_wave)) {
			//log("\tbest_error " + String(Variant(best_error)));
			count_waves_from_source(p_sample, r_wave);
			return true;
		}
	} else {
		//log("\tno best_window found, best error " + String(Variant(best_error_overall)));
		log("\tno best_window found");
	}
	return false;
}
