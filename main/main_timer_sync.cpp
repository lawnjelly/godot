/*************************************************************************/
/*  main_timer_sync.cpp                                                  */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "main_timer_sync.h"

MainTimerSync::DeltaSmoother::DeltaSmoother()
{
	const int default_delta = 16666;

	for (int n=0; n<MAX_RECORDS; n++)
		m_iRecords[n] = default_delta;

	m_iRunningTotal = MAX_RECORDS * default_delta;
	m_iCurrentRecord = 0;
}


int MainTimerSync::DeltaSmoother::SmoothDelta(int delta)
{
	const int ciMaxDelta = 1000000 / 24;
	if (delta > ciMaxDelta)
		return delta;

	// cap at 1 second, prevent 32 bit integer overflow
	if (delta > 1000000) delta = 1000000;

	// keep running total (remove old record)
	m_iRunningTotal -= m_iRecords[m_iCurrentRecord];

	// store in the records
	m_iRecords[m_iCurrentRecord] = delta;

	// keep running total (add new record)
	m_iRunningTotal += delta;

	// increment the record pointer
	m_iCurrentRecord++;

	// wraparound circular buffer
	if (m_iCurrentRecord == MAX_RECORDS)
		m_iCurrentRecord = 0;

	// find the average
	int average = m_iRunningTotal / MAX_RECORDS;

	// turn off smoothing when the average is below a certain point
	// i.e. dropped frames are happening too often to deal with.
	const int ciMaxAverage = 1000000 / 40;
	if (average > ciMaxAverage)
		return delta;
	// Note that both the constants in this function could possibly be adjusted at runtime
	// according to hardware.

	return average;
}


////////////////////////////

MainTimerSync2::DeltaSmoother::DeltaSmoother()
{
	// just start by filling with values for 60 frames per second ...
	// this isn't crucial as it only affects the first few frames before there is data filled.
	const int default_delta = 16666;

	for (int n=0; n<MAX_RECORDS; n++)
		m_iRecords[n] = default_delta;

	// set up the initial running total of all the records
	m_iRunningTotal = MAX_RECORDS * default_delta;
	m_iCurrentRecord = 0;
	m_iNumRecords = MAX_RECORDS;
}


void MainTimerSync2::DeltaSmoother::UpdateNumRecords()
{
	int nRecords = Engine::get_singleton()->get_delta_smoothing();

	// sanitize
	if (nRecords < 0)
		nRecords = 0;
	if (nRecords > MAX_RECORDS)
		nRecords = MAX_RECORDS;

	if (nRecords == m_iNumRecords)
		return;

	// reset to the new number of records, and recalculate the running total
	m_iNumRecords = nRecords;

	m_iRunningTotal = 0;
	for (int n=0; n<m_iNumRecords; n++)
		m_iRunningTotal += m_iRecords[n];

	m_iCurrentRecord = 0;

	print_line ("num records is now " + itos(m_iNumRecords));
}


// References:
// https://medium.com/@alen.ladavac/the-elusive-frame-timing-168f899aec92
// http://bitsquid.blogspot.com/2010/10/time-step-smoothing.html
// https://www.gamedev.net/blogs/entry/2265460-fixing-your-timestep-and-evaluating-godot/
// http://frankforce.com/?p=2636

// Average smoothing is really to deal with variation in delta caused by the OS.
// Variation due to dropped frames is not easy to deal with as yet,
// it may be to deal with in future APIs.
int MainTimerSync2::DeltaSmoother::SmoothDelta(int delta)
{
//	if (delta <= 0)
//		print_line("delta is negative : " + itos(delta));

	// make sure the number of records used is up to date with project settings
	UpdateNumRecords();

	// if switched off
	if (m_iNumRecords == 0)
		return delta;


	// If the number of dropped frames is changing often, the smoothing will not help.
	// Turn off smoothing at real low frame rates where the game is struggling to keep up.
	// 24 fps is a reasonable cutoff as lowest refresh rate display devices are probably around 50fps (PAL tv standard)
	// and this is more than 1 dropped frame at that rate.
	const int ciMaxDelta = 1000000 / 24;
	if (delta > ciMaxDelta)
	{
		if (!Engine::get_singleton()->is_editor_hint())
			print_line("delta too high");
		return delta;
	}


	// cap at 1 second, prevent 32 bit integer overflow
	if (delta > 1000000) delta = 1000000;

	// keep running total (remove old record)
	m_iRunningTotal -= m_iRecords[m_iCurrentRecord];

	// store in the records
	m_iRecords[m_iCurrentRecord] = delta;

	// keep running total (add new record)
	m_iRunningTotal += delta;

	// increment the record pointer
	m_iCurrentRecord++;

	// wraparound circular buffer
	if (m_iCurrentRecord == m_iNumRecords)
	{
		m_iCurrentRecord = 0;
		// some debug stuff
//		if (!Engine::get_singleton()->is_editor_hint())
//		{
//			String str;
//			const int COLS = MAX_RECORDS / 8;
//			int count = 0;
//			for (int n=0; n<COLS; n++)
//			{
//				for (int m=0; m< 8; m++)
//				{
//					str += itos(m_iRecords[count++] / 100);
//					str += ", ";
//				}
//				print_line(str);
//			}
//			print_line("Average : " + itos(m_iRunningTotal / MAX_RECORDS));
//		} // if not editor
	}

	// find the average
	int average = m_iRunningTotal / m_iNumRecords;

	// turn off smoothing when the average is below a certain point
	// i.e. dropped frames are happening too often to deal with.
	const int ciMaxAverage = 1000000 / 40;
	if (average > ciMaxAverage)
	{
		if (!Engine::get_singleton()->is_editor_hint())
			print_line("average too high");
		return delta;
	}
	// Note that both the constants in this function could possibly be adjusted at runtime
	// according to hardware.
//	print_line("delta " + itos(delta) + "\taverage " + itos(average));

	return average;
}



//////////////////////////////////////////////

void MainFrameTime::clamp_idle(float min_idle_step, float max_idle_step) {
	if (idle_step < min_idle_step) {
		idle_step = min_idle_step;
	} else if (idle_step > max_idle_step) {
		idle_step = max_idle_step;
	}
}

/////////////////////////////////

// returns the fraction of p_frame_slice required for the timer to overshoot
// before advance_core considers changing the physics_steps return from
// the typical values as defined by typical_physics_steps
float MainTimerSync::get_physics_jitter_fix() {
	return Engine::get_singleton()->get_physics_jitter_fix();
}

// gets our best bet for the average number of physics steps per render frame
// return value: number of frames back this data is consistent
int MainTimerSync::get_average_physics_steps(float &p_min, float &p_max) {
	p_min = typical_physics_steps[0];
	p_max = p_min + 1;

	for (int i = 1; i < CONTROL_STEPS; ++i) {
		const float typical_lower = typical_physics_steps[i];
		const float current_min = typical_lower / (i + 1);
		if (current_min > p_max)
			return i; // bail out of further restrictions would void the interval
		else if (current_min > p_min)
			p_min = current_min;
		const float current_max = (typical_lower + 1) / (i + 1);
		if (current_max < p_min)
			return i;
		else if (current_max < p_max)
			p_max = current_max;
	}

	return CONTROL_STEPS;
}

// advance physics clock by p_idle_step, return appropriate number of steps to simulate
MainFrameTime MainTimerSync::advance_core(float p_frame_slice, int p_iterations_per_second, float p_idle_step) {
	MainFrameTime ret;

	ret.idle_step = p_idle_step;

	// simple determination of number of physics iteration
	time_accum += ret.idle_step;
	ret.physics_steps = floor(time_accum * p_iterations_per_second);

	int min_typical_steps = typical_physics_steps[0];
	int max_typical_steps = min_typical_steps + 1;

	// given the past recorded steps and typical steps to match, calculate bounds for this
	// step to be typical
	bool update_typical = false;

	for (int i = 0; i < CONTROL_STEPS - 1; ++i) {
		int steps_left_to_match_typical = typical_physics_steps[i + 1] - accumulated_physics_steps[i];
		if (steps_left_to_match_typical > max_typical_steps ||
				steps_left_to_match_typical + 1 < min_typical_steps) {
			update_typical = true;
			break;
		}

		if (steps_left_to_match_typical > min_typical_steps)
			min_typical_steps = steps_left_to_match_typical;
		if (steps_left_to_match_typical + 1 < max_typical_steps)
			max_typical_steps = steps_left_to_match_typical + 1;
	}

	// try to keep it consistent with previous iterations
	if (ret.physics_steps < min_typical_steps) {
		const int max_possible_steps = floor((time_accum)*p_iterations_per_second + get_physics_jitter_fix());
		if (max_possible_steps < min_typical_steps) {
			ret.physics_steps = max_possible_steps;
			update_typical = true;
		} else {
			ret.physics_steps = min_typical_steps;
		}
	} else if (ret.physics_steps > max_typical_steps) {
		const int min_possible_steps = floor((time_accum)*p_iterations_per_second - get_physics_jitter_fix());
		if (min_possible_steps > max_typical_steps) {
			ret.physics_steps = min_possible_steps;
			update_typical = true;
		} else {
			ret.physics_steps = max_typical_steps;
		}
	}

	time_accum -= ret.physics_steps * p_frame_slice;

	// keep track of accumulated step counts
	for (int i = CONTROL_STEPS - 2; i >= 0; --i) {
		accumulated_physics_steps[i + 1] = accumulated_physics_steps[i] + ret.physics_steps;
	}
	accumulated_physics_steps[0] = ret.physics_steps;

	if (update_typical) {
		for (int i = CONTROL_STEPS - 1; i >= 0; --i) {
			if (typical_physics_steps[i] > accumulated_physics_steps[i]) {
				typical_physics_steps[i] = accumulated_physics_steps[i];
			} else if (typical_physics_steps[i] < accumulated_physics_steps[i] - 1) {
				typical_physics_steps[i] = accumulated_physics_steps[i] - 1;
			}
		}
	}

	return ret;
}

// calls advance_core, keeps track of deficit it adds to animaption_step, make sure the deficit sum stays close to zero
MainFrameTime MainTimerSync::advance_checked(float p_frame_slice, int p_iterations_per_second, float p_idle_step) {
	if (fixed_fps != -1)
		p_idle_step = 1.0 / fixed_fps;

	// compensate for last deficit
	p_idle_step += time_deficit;

	MainFrameTime ret = advance_core(p_frame_slice, p_iterations_per_second, p_idle_step);

	// we will do some clamping on ret.idle_step and need to sync those changes to time_accum,
	// that's easiest if we just remember their fixed difference now
	const double idle_minus_accum = ret.idle_step - time_accum;

	// first, least important clamping: keep ret.idle_step consistent with typical_physics_steps.
	// this smoothes out the idle steps and culls small but quick variations.
	{
		float min_average_physics_steps, max_average_physics_steps;
		int consistent_steps = get_average_physics_steps(min_average_physics_steps, max_average_physics_steps);
		if (consistent_steps > 3) {
			ret.clamp_idle(min_average_physics_steps * p_frame_slice, max_average_physics_steps * p_frame_slice);
		}
	}

	// second clamping: keep abs(time_deficit) < jitter_fix * frame_slise
	float max_clock_deviation = get_physics_jitter_fix() * p_frame_slice;
	ret.clamp_idle(p_idle_step - max_clock_deviation, p_idle_step + max_clock_deviation);

	// last clamping: make sure time_accum is between 0 and p_frame_slice for consistency between physics and idle
	ret.clamp_idle(idle_minus_accum, idle_minus_accum + p_frame_slice);

	// restore time_accum
	time_accum = ret.idle_step - idle_minus_accum;

	// track deficit
	time_deficit = p_idle_step - ret.idle_step;

	// we will try and work out what is the interpolation fraction
	ret.interpolation_fraction = time_accum / (1.0f / p_iterations_per_second);

	return ret;
}

// determine wall clock step since last iteration
float MainTimerSync::get_cpu_idle_step() {
	uint64_t cpu_ticks_elapsed = current_cpu_ticks_usec - last_cpu_ticks_usec;
	last_cpu_ticks_usec = current_cpu_ticks_usec;

	cpu_ticks_elapsed = m_Smoother.SmoothDelta(cpu_ticks_elapsed);



	return cpu_ticks_elapsed / 1000000.0;
}

MainTimerSync::MainTimerSync() :
		last_cpu_ticks_usec(0),
		current_cpu_ticks_usec(0),
		time_accum(0),
		time_deficit(0),
		fixed_fps(0) {
	for (int i = CONTROL_STEPS - 1; i >= 0; --i) {
		typical_physics_steps[i] = i;
		accumulated_physics_steps[i] = i;
	}
}

// start the clock
void MainTimerSync::init(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = last_cpu_ticks_usec = p_cpu_ticks_usec;
}

// set measured wall clock time
void MainTimerSync::set_cpu_ticks_usec(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = p_cpu_ticks_usec;
}

void MainTimerSync::set_fixed_fps(int p_fixed_fps) {
	fixed_fps = p_fixed_fps;
}

// advance one frame, return timesteps to take
MainFrameTime MainTimerSync::advance(float p_frame_slice, int p_iterations_per_second) {
	float cpu_idle_step = get_cpu_idle_step();

	return advance_checked(p_frame_slice, p_iterations_per_second, cpu_idle_step);
}



///////////////////////////////////

// calls advance_core, keeps track of deficit it adds to animaption_step, make sure the deficit sum stays close to zero
// advance physics clock by p_idle_step, return appropriate number of steps to simulate
//MainFrameTime MainTimerSync2::advance_checked(float p_sec_per_tick, int p_iterations_per_second, float p_idle_step) {

//	MainFrameTime ret;

//	ret.idle_step = p_idle_step;

//	// safety, might not be needed
//	if (p_iterations_per_second <= 0)
//		p_iterations_per_second = 1;

////	float sec_per_frame = 1.0f / p_iterations_per_second;
////	float sec_per_tick = p_frame_slice;

//	float time_available = m_fTimeLeftover + p_idle_step;

//	ret.physics_steps = floor(time_available * p_iterations_per_second);

//	m_fTimeLeftover = time_available - (ret.physics_steps * p_sec_per_tick);

//	ret.interpolation_fraction = m_fTimeLeftover / p_sec_per_tick;
////	m_fInterpolationFraction = m_fTimeLeftover / sec_per_frame;
//	//print_line("MainTimerSync2::advance_core fraction " + String(Variant(m_fInterpolationFraction)));

//	return ret;
//}

// determine wall clock step since last iteration
float MainTimerSync2::get_cpu_idle_step() {
	uint64_t delta = current_cpu_ticks_usec - last_cpu_ticks_usec;
	last_cpu_ticks_usec = current_cpu_ticks_usec;

	// smoothing
	//print_line("input delta " + itos(delta));

	delta = m_Smoother.SmoothDelta(delta);


	return delta / 1000000.0;
}

MainTimerSync2::MainTimerSync2() :
		last_cpu_ticks_usec(0),
		current_cpu_ticks_usec(0),
		fixed_fps(0)
		{
			m_fTimeLeftover = 0.0f;
		}

// start the clock
void MainTimerSync2::init(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = last_cpu_ticks_usec = p_cpu_ticks_usec;
}

// set measured wall clock time
void MainTimerSync2::set_cpu_ticks_usec(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = p_cpu_ticks_usec;

	//ERR_FAIL_COND(p_cpu_ticks_usec < current_cpu_ticks_usec);

	// delta smoothed
//	uint64_t delta = p_cpu_ticks_usec - current_cpu_ticks_usec;
//	print_line("input delta " + itos(delta));
//	delta = m_Smoother.SmoothDelta(delta);
//	current_cpu_ticks_usec += delta;

	// fixed
//	current_cpu_ticks_usec += 16666;
}

void MainTimerSync2::set_fixed_fps(int p_fixed_fps) {
	fixed_fps = p_fixed_fps;
}

// advance one frame, return timesteps to take
MainFrameTime MainTimerSync2::advance(float p_sec_per_tick, int p_iterations_per_second) {
	float gap;
	if (fixed_fps <= 0)
	{
		gap = get_cpu_idle_step();
	}
	else
	{
		gap = 1.0f / fixed_fps;
	}

	MainFrameTime ret;

	ret.idle_step = gap;

	// safety, might not be needed
	if (p_iterations_per_second <= 0)
		p_iterations_per_second = 1;

//	float sec_per_frame = 1.0f / p_iterations_per_second;
//	float sec_per_tick = p_frame_slice;

	float time_available = m_fTimeLeftover + gap;

	ret.physics_steps = floor(time_available * p_iterations_per_second);

	m_fTimeLeftover = time_available - (ret.physics_steps * p_sec_per_tick);

	ret.interpolation_fraction = m_fTimeLeftover / p_sec_per_tick;

	return ret;


	//return advance_checked(p_sec_per_tick, p_iterations_per_second, cpu_idle_step);
}
