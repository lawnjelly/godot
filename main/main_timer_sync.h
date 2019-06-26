/*************************************************************************/
/*  main_timer_sync.h                                                    */
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

#ifndef MAIN_TIMER_SYNC_H
#define MAIN_TIMER_SYNC_H

#include "core/engine.h"

struct MainFrameTime {
	float idle_step; // time to advance idles for (argument to process())
	int physics_steps; // number of times to iterate the physics engine
	float interpolation_fraction;

	void clamp_idle(float min_idle_step, float max_idle_step);
};

class MainTimerSync {
	// wall clock time measured on the main thread
	uint64_t last_cpu_ticks_usec;
	uint64_t current_cpu_ticks_usec;

	// for fixed time step interpolation, the interpolation fraction is calculated as how
	// far we are through the current physics tick on this frame
//	float m_fInterpolationFraction;
	float m_fTimeLeftover;


protected:
	// advance physics clock by p_idle_step, return appropriate number of steps to simulate
	MainFrameTime advance_core(float p_frame_slice, int p_iterations_per_second, float p_idle_step);

	// calls advance_core, keeps track of deficit it adds to animaption_step, make sure the deficit sum stays close to zero
	MainFrameTime advance_checked(float p_frame_slice, int p_iterations_per_second, float p_idle_step);

	// determine wall clock step since last iteration
	float get_cpu_idle_step();

public:
	MainTimerSync();

	// start the clock
	void init(uint64_t p_cpu_ticks_usec);
	// set measured wall clock time
	void set_cpu_ticks_usec(uint64_t p_cpu_ticks_usec);
	//set fixed fps
	void set_fixed_fps(int p_fixed_fps);

	// advance one frame, return timesteps to take
	MainFrameTime advance(float p_frame_slice, int p_iterations_per_second);

	// fraction through current physics tick on this frame
//	float get_physics_interpolation_fraction() const {return m_fInterpolationFraction;}
};

#endif // MAIN_TIMER_SYNC_H
