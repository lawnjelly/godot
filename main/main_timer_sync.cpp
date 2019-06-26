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

void MainFrameTime::clamp_idle(float min_idle_step, float max_idle_step) {
	if (idle_step < min_idle_step) {
		idle_step = min_idle_step;
	} else if (idle_step > max_idle_step) {
		idle_step = max_idle_step;
	}
}

/////////////////////////////////



// advance physics clock by p_idle_step, return appropriate number of steps to simulate
MainFrameTime MainTimerSync::advance_core(float p_frame_slice, int p_iterations_per_second, float p_idle_step) {
	MainFrameTime ret;

	ret.idle_step = p_idle_step;

	// safety, might not be needed
	if (p_iterations_per_second <= 0)
		p_iterations_per_second = 1;

	float sec_per_frame = 1.0f / p_iterations_per_second;

	float time_available = m_fTimeLeftover + p_idle_step;

	ret.physics_steps = floor(time_available * p_iterations_per_second);

	m_fTimeLeftover = time_available - (ret.physics_steps * sec_per_frame);

	ret.interpolation_fraction = m_fTimeLeftover / sec_per_frame;
//	m_fInterpolationFraction = m_fTimeLeftover / sec_per_frame;
	//print_line("MainTimerSync::advance_core fraction " + String(Variant(m_fInterpolationFraction)));

	return ret;
}

// calls advance_core, keeps track of deficit it adds to animaption_step, make sure the deficit sum stays close to zero
MainFrameTime MainTimerSync::advance_checked(float p_frame_slice, int p_iterations_per_second, float p_idle_step) {

	MainFrameTime ret = advance_core(p_frame_slice, p_iterations_per_second, p_idle_step);

	// the left over time as a fraction of a whole physics tick
//	interpolation_fraction = time_accum / p_frame_slice;

	return ret;
}

// determine wall clock step since last iteration
float MainTimerSync::get_cpu_idle_step() {
	uint64_t cpu_ticks_elapsed = current_cpu_ticks_usec - last_cpu_ticks_usec;
	last_cpu_ticks_usec = current_cpu_ticks_usec;

	return cpu_ticks_elapsed / 1000000.0;
}

MainTimerSync::MainTimerSync() :
		last_cpu_ticks_usec(0),
		current_cpu_ticks_usec(0)
		{
			//m_fInterpolationFraction = 0.0f;
			m_fTimeLeftover = 0.0f;
		}

// start the clock
void MainTimerSync::init(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = last_cpu_ticks_usec = p_cpu_ticks_usec;
}

// set measured wall clock time
void MainTimerSync::set_cpu_ticks_usec(uint64_t p_cpu_ticks_usec) {
	current_cpu_ticks_usec = p_cpu_ticks_usec;
//	current_cpu_ticks_usec += 16666;
}

void MainTimerSync::set_fixed_fps(int p_fixed_fps) {
//	fixed_fps = p_fixed_fps;
}

// advance one frame, return timesteps to take
MainFrameTime MainTimerSync::advance(float p_frame_slice, int p_iterations_per_second) {
	float cpu_idle_step = get_cpu_idle_step();

	return advance_checked(p_frame_slice, p_iterations_per_second, cpu_idle_step);
}
