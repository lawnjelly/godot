#pragma once

#include "navphysics_error.h"
#include "navphysics_typedefs.h"

namespace NavPhysics {

template <typename T, class U = uint32_t>
class Span {
	const T *_ptr = nullptr;
	U _len = 0;

public:
	static constexpr bool is_string = std::disjunction_v<
			std::is_same<T, char>,
			std::is_same<T, char16_t>,
			std::is_same<T, char32_t>,
			std::is_same<T, wchar_t>>;

	constexpr Span() = default;

	Span(const T *p_ptr, U p_len) :
			_ptr(p_ptr), _len(p_len) {
#ifdef NP_DEV_ENABLED
		if (_ptr == nullptr && _len > 0) {
			NP_ERR_PRINT("Internal bug, please report: Span was created from nullptr with size > 0. Recovering by using size = 0.");
			_len = 0;
		}
#endif
	}

	// Allows creating Span directly from C arrays and string literals.
	template <size_t N>
	constexpr Span(const T (&p_array)[N]) :
			_ptr(p_array), _len(N) {
		if constexpr (is_string) {
			// Cut off the \0 terminator implicitly added to string literals.
			if (N > 0 && p_array[N - 1] == '\0') {
				_len--;
			}
		}
	}

	constexpr U size() const { return _len; }
	constexpr bool is_empty() const { return _len == 0; }

	constexpr const T *ptr() const { return _ptr; }

	// NOTE: Span subscripts sanity check the bounds to avoid undefined behavior.
	//       This is slower than direct buffer access and can prevent autovectorization.
	//       If the bounds are known, use ptr() subscript instead.
	constexpr const T &operator[](U p_idx) const {
		NP_CRASH_COND(p_idx >= _len);
		return _ptr[p_idx];
	}

	constexpr const T *begin() const { return _ptr; }
	constexpr const T *end() const { return _ptr + _len; }

	template <typename T1>
	constexpr Span<T1> reinterpret() const {
		return Span<T1>(reinterpret_cast<const T1 *>(_ptr), _len * sizeof(T) / sizeof(T1));
	}

	// Algorithms.
	constexpr int64_t find(const T &p_val, U p_from = 0) const;
	constexpr int64_t find_sequence(const Span<T> &p_span, U p_from = 0) const;
	constexpr int64_t rfind(const T &p_val, U p_from) const;
	constexpr int64_t rfind(const T &p_val) const { return rfind(p_val, size() - 1); }
	constexpr int64_t rfind_sequence(const Span<T> &p_span, U p_from) const;
	constexpr int64_t rfind_sequence(const Span<T> &p_span) const { return rfind_sequence(p_span, size() - p_span.size()); }
	constexpr U count(const T &p_val) const;
	/// Find the index of the given value using binary search.
	/// Note: Assumes that elements in the span are sorted. Otherwise, use find() instead.
	template <typename Comparator = Comparator<T>>
	constexpr U bisect(const T &p_value, bool p_before, Comparator compare = Comparator()) const;

	/// The caller is responsible to ensure size() > 0.
	constexpr T max() const;
};

template <typename T, class U>
constexpr int64_t Span<T, U>::find(const T &p_val, U p_from) const {
	for (U i = p_from; i < size(); i++) {
		if (ptr()[i] == p_val) {
			return i;
		}
	}
	return -1;
}

template <typename T, class U>
constexpr int64_t Span<T, U>::find_sequence(const Span<T> &p_span, U p_from) const {
	for (U i = p_from; i <= size() - p_span.size(); i++) {
		bool found = true;
		for (U j = 0; j < p_span.size(); j++) {
			if (ptr()[i + j] != p_span.ptr()[j]) {
				found = false;
				break;
			}
		}
		if (found) {
			return i;
		}
	}

	return -1;
}

template <typename T, class U>
constexpr int64_t Span<T, U>::rfind(const T &p_val, U p_from) const {
	for (int64_t i = p_from; i >= 0; i--) {
		if (ptr()[i] == p_val) {
			return i;
		}
	}
	return -1;
}

template <typename T, class U>
constexpr int64_t Span<T, U>::rfind_sequence(const Span<T> &p_span, U p_from) const {
	for (int64_t i = p_from; i >= 0; i--) {
		bool found = true;
		for (U j = 0; j < p_span.size(); j++) {
			if (ptr()[i + j] != p_span.ptr()[j]) {
				found = false;
				break;
			}
		}
		if (found) {
			return i;
		}
	}

	return -1;
}

template <typename T, class U>
constexpr U Span<T, U>::count(const T &p_val) const {
	U amount = 0;
	for (U i = 0; i < size(); i++) {
		if (ptr()[i] == p_val) {
			amount++;
		}
	}
	return amount;
}

template <typename T, class U>
template <typename Comparator>
constexpr U Span<T, U>::bisect(const T &p_value, bool p_before, Comparator compare) const {
	U lo = 0;
	U hi = size();
	if (p_before) {
		while (lo < hi) {
			const U mid = (lo + hi) / 2;
			if (compare(ptr()[mid], p_value)) {
				lo = mid + 1;
			} else {
				hi = mid;
			}
		}
	} else {
		while (lo < hi) {
			const U mid = (lo + hi) / 2;
			if (compare(p_value, ptr()[mid])) {
				hi = mid;
			} else {
				lo = mid + 1;
			}
		}
	}
	return lo;
}

template <typename T, class U>
constexpr T Span<T, U>::max() const {
	NP_DEV_ASSERT(size() > 0);
	T max_val = _ptr[0];
	for (U i = 1; i < _len; ++i) {
		if (_ptr[i] > max_val) {
			max_val = _ptr[i];
		}
	}
	return max_val;
}

} //namespace NavPhysics
