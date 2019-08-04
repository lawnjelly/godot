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
	void SetDefaults();

	float frame_delta; // time to advance idles for (argument to process())
	int physics_steps; // number of times to iterate the physics engine
	float interpolation_fraction;

	// for semi fixed and frame based methods, we can
	// optionally have the last physics step with a variable delta to pass
	// to physics
	bool physics_variable_step;
	float physics_variable_step_delta;
	float physics_fixed_step_delta;

	void clamp_idle(float min_idle_step, float max_idle_step);
};

///////////////////////////////////////////////////////////

class MainTimerSync_JitterFix {
	// logical game time since last physics timestep
	float time_accum;

	// current difference between wall clock time and reported sum of idle_steps
	float time_deficit;

	// number of frames back for keeping accumulated physics steps roughly constant.
	// value of 12 chosen because that is what is required to make 144 Hz monitors
	// behave well with 60 Hz physics updates. The only worse commonly available refresh
	// would be 85, requiring CONTROL_STEPS = 17.
	static const int CONTROL_STEPS = 12;

	// sum of physics steps done over the last (i+1) frames
	int accumulated_physics_steps[CONTROL_STEPS];

	// typical value for accumulated_physics_steps[i] is either this or this plus one
	int typical_physics_steps[CONTROL_STEPS];

protected:
	// returns the fraction of p_frame_slice required for the timer to overshoot
	// before advance_core considers changing the physics_steps return from
	// the typical values as defined by typical_physics_steps
	float get_physics_jitter_fix();

	// gets our best bet for the average number of physics steps per render frame
	// return value: number of frames back this data is consistent
	int get_average_physics_steps(float &p_min, float &p_max);

	// advance physics clock by p_idle_step, return appropriate number of steps to simulate
	MainFrameTime advance_core(float p_frame_slice, float p_iterations_per_second, float p_idle_step);

	// calls advance_core, keeps track of deficit it adds to animaption_step, make sure the deficit sum stays close to zero
	MainFrameTime advance_checked(float p_frame_slice, float p_iterations_per_second, float p_idle_step);

public:
	MainTimerSync_JitterFix();

	// advance one frame, return timesteps to take
	MainFrameTime advance(float cpu_idle_step, float p_frame_slice, float p_iterations_per_second);
};

class MainTimerSync_Fixed {
	float m_fTimeLeftover;

public:
	MainTimerSync_Fixed();

	// advance one frame, return timesteps to take
	MainFrameTime advance(float delta, float p_sec_per_tick, float p_iterations_per_second);
};

/////////////////////////////////////////////////////////

class MainTimerSync_SemiFixed {
public:
	// advance one frame, return timesteps to take
	MainFrameTime advance(float delta, float p_sec_per_tick, float p_iterations_per_second);
};

/////////////////////////////////////////////////////////

//class MainTimerSync_FrameBased
//{
//public:
//	// advance one frame, return timesteps to take
//	MainFrameTime advance(float delta, float p_sec_per_tick, int p_iterations_per_second);
//};

/////////////////////////////////////////////////////////

class MainTimerSync {
	enum eMethod {
		//	M_DEFAULT, // default will be same as jitterfix... for now
		M_FIXED,
		M_SEMIFIXED,
		//		M_FRAMEBASED,
		M_JITTERFIX,
	};

	class DeltaSmoother {
	public:
		DeltaSmoother();

		// pass the recorded delta, returns a smoothed delta
		int SmoothDelta(int delta);

	private:
		void UpdateNumRecords();

		// records of the delta on previous main loop iterations
		static const int MAX_RECORDS = 256;
		int m_iRecords[MAX_RECORDS];
		int m_iNumRecords;

		// circular buffer of records
		int m_iCurrentRecord;

		// save on computation by keeping a running total to calculate the average delta
		int m_iRunningTotal;
	};

	// wall clock time measured on the main thread
	uint64_t last_cpu_ticks_usec;
	uint64_t current_cpu_ticks_usec;

	int fixed_fps;

	// just for debugging
	float m_fTimeScale;
	String ftos(float f);

	DeltaSmoother m_Smoother;

	//int m_iDeltaSmooth_ObjectID;
	//String m_szDeltaSmooth_Func;

	eMethod m_eMethod;
	String m_szCurrentMethod;

	MainTimerSync_Fixed m_TSFixed;
	MainTimerSync_SemiFixed m_TSSemiFixed;
	//	MainTimerSync_FrameBased m_TSFrameBased;
	MainTimerSync_JitterFix m_TSJitterFix;

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

	// user delta smoothing function
	//void set_delta_smoothing_func(int objID, String szFunc);

	// advance one frame, return timesteps to take
	MainFrameTime advance(int p_iterations_per_second);
};

#endif // MAIN_TIMER_SYNC_H
