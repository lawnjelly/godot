#pragma once

#include <float.h>
#include <stdint.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;
using freal = f32;

#define navphysics_unlikely

#define NAVPHYSICS_OPERATOR_RET(T, OP)               \
	T operator OP(const T &p_v) const {              \
		T res;                                       \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {    \
			res.coord[n] = coord[n] OP p_v.coord[n]; \
		}                                            \
		return res;                                  \
	}

#define NAVPHYSICS_OPERATOR_IN_PLACE(T, OP)       \
	void operator OP(const T &p_v) {              \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) { \
			coord[n] OP p_v.coord[n];             \
		}                                         \
	}

#define NAVPHYSICS_OPERATOR_RET_COMPONENT(T, OP, COMP) \
	T operator OP(COMP p_v) const {                    \
		T res;                                         \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {      \
			res.coord[n] = coord[n] OP p_v;            \
		}                                              \
		return res;                                    \
	}

#define NAVPHYSICS_OPERATOR_IN_PLACE_COMPONENT(T, OP, COMP) \
	void operator OP(COMP p_v) {                            \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {           \
			coord[n] OP p_v;                                \
		}                                                   \
	}

#define NAVPHYSICS_DECLARE_OPERATORS(T, COMP)                       \
	NAVPHYSICS_OPERATOR_RET(T, +)                                   \
	NAVPHYSICS_OPERATOR_RET(T, -)                                   \
	NAVPHYSICS_OPERATOR_RET(T, *)                                   \
	NAVPHYSICS_OPERATOR_RET(T, /)                                   \
	NAVPHYSICS_OPERATOR_IN_PLACE(T, +=)                             \
	NAVPHYSICS_OPERATOR_IN_PLACE(T, -=)                             \
	NAVPHYSICS_OPERATOR_IN_PLACE(T, *=)                             \
	NAVPHYSICS_OPERATOR_IN_PLACE(T, /=)                             \
	NAVPHYSICS_OPERATOR_RET_COMPONENT(T, *, i32)                    \
	NAVPHYSICS_OPERATOR_RET_COMPONENT(T, *, freal)                  \
	NAVPHYSICS_OPERATOR_RET_COMPONENT(T, /, i32)                    \
	NAVPHYSICS_OPERATOR_RET_COMPONENT(T, /, freal)                  \
	NAVPHYSICS_OPERATOR_IN_PLACE_COMPONENT(T, *=, i32)              \
	NAVPHYSICS_OPERATOR_IN_PLACE_COMPONENT(T, *=, freal)            \
	NAVPHYSICS_OPERATOR_IN_PLACE_COMPONENT(T, /=, i32)              \
	NAVPHYSICS_OPERATOR_IN_PLACE_COMPONENT(T, /=, freal)            \
	T operator-() const {                                           \
		T res;                                                      \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {                   \
			res.coord[n] = -coord[n];                               \
		}                                                           \
		return res;                                                 \
	}                                                               \
	bool operator==(const T &p_v) const {                           \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {                   \
			if (coord[n] != p_v.coord[n])                           \
				return false;                                       \
		}                                                           \
		return true;                                                \
	}                                                               \
	bool operator!=(const T &p_v) const { return !(*this == p_v); } \
	void fill(COMP p_v) {                                           \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) {                   \
			coord[n] = p_v;                                         \
		}                                                           \
	}                                                               \
	void zero() { fill(0); }                                        \
	const COMP &operator[](u32 p_axis) const {                      \
		NP_DEV_ASSERT(p_axis < T::AXIS_COUNT);                      \
		return coord[p_axis];                                       \
	}                                                               \
	COMP &operator[](u32 p_axis) {                                  \
		NP_DEV_ASSERT(p_axis < T::AXIS_COUNT);                      \
		return coord[p_axis];                                       \
	}

#define NAVPHYSICS_DECLARE_OPERATORS_3(T, COMP)   \
	NAVPHYSICS_DECLARE_OPERATORS(T, COMP)         \
	void set(COMP p_x, COMP p_y, COMP p_z) {      \
		x = p_x;                                  \
		y = p_y;                                  \
		z = p_z;                                  \
	}                                             \
	static T make(COMP p_x, COMP p_y, COMP p_z) { \
		T v;                                      \
		v.set(p_x, p_y, p_z);                     \
		return v;                                 \
	}

#define NAVPHYSICS_DECLARE_OPERATORS_2(T, COMP) \
	NAVPHYSICS_DECLARE_OPERATORS(T, COMP)       \
	void set(COMP p_x, COMP p_y) {              \
		x = p_x;                                \
		y = p_y;                                \
	}                                           \
	static T make(COMP p_x, COMP p_y) {         \
		T v;                                    \
		v.set(p_x, p_y);                        \
		return v;                               \
	}

#ifndef ABS
#define ABS(m_v) (((m_v) < 0) ? (-(m_v)) : (m_v))
#endif

#define ABSDIFF(x, y) (((x) < (y)) ? ((y) - (x)) : ((x) - (y)))

#ifndef SGN
#define SGN(m_v) (((m_v) < 0) ? (-1.0f) : (+1.0f))
#endif

#ifndef MIN
#define MIN(m_a, m_b) (((m_a) < (m_b)) ? (m_a) : (m_b))
#endif

#ifndef MAX
#define MAX(m_a, m_b) (((m_a) > (m_b)) ? (m_a) : (m_b))
#endif

#ifndef CLAMP
#define CLAMP(m_a, m_min, m_max) (((m_a) < (m_min)) ? (m_min) : (((m_a) > (m_max)) ? m_max : m_a))
#endif

/** Generic swap template */
#ifndef SWAP

#define SWAP(m_x, m_y) __swap_tmpl((m_x), (m_y))
template <class T>
inline void __swap_tmpl(T &x, T &y) {
	T aux = x;
	x = y;
	y = aux;
}

#endif // swap
