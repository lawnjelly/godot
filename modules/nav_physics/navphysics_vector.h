#pragma once

#include "navphysics_allocator.h"
#include "navphysics_error.h"
#include "navphysics_sort_array.h"
#include "navphysics_typedefs.h"

#include <type_traits>

namespace NavPhysics {

template <class T, class U = u32, bool force_trivial = false>
class Vector {
private:
	U count = 0;
	U capacity = 0;
	T *data = nullptr;

public:
	T *ptr() {
		return data;
	}

	const T *ptr() const {
		return data;
	}
	void push_back(T p_elem) {
		if (count == capacity) {
			if (capacity == 0) {
				capacity = 1;
			} else {
				capacity <<= 1;
			}
			data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
			NP_CRASH_COND_MSG(!data, "Out of memory");
		}

		//		if (!std::is_trivially_constructible<T>::value && !force_trivial)
		//		{
		//			memnew_placement(&data[count++], T(p_elem));
		//		}
		//		else
		{
			data[count++] = p_elem;
		}
	}

	void remove(U p_index) {
		NP_NP_ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (U i = p_index; i < count; i++) {
			data[i] = data[i + 1];
		}
		//		if (!std::is_trivially_destructible<T>::value && !force_trivial)
		//		{
		//			data[count].~T();
		//		}
	}

	// Removes the item copying the last value into the position of the one to
	// remove. It's generally faster than `remove`.
	void remove_unordered(U p_index) {
		NP_NP_ERR_FAIL_INDEX(p_index, count);
		count--;
		if (count > p_index) {
			data[p_index] = data[count];
		}
		if (!std::is_trivially_destructible<T>::value && !force_trivial) {
			data[count].~T();
		}
	}

	void erase(const T &p_val) {
		i64 idx = find(p_val);
		if (idx >= 0) {
			remove(idx);
		}
	}

	U erase_multiple_unordered(const T &p_val) {
		U from = 0;
		U count = 0;
		while (true) {
			i64 idx = find(p_val, from);

			if (idx == -1) {
				break;
			}
			remove_unordered(idx);
			from = idx;
			count++;
		}
		return count;
	}

	void invert() {
		for (U i = 0; i < count / 2; i++) {
			SWAP(data[i], data[count - i - 1]);
		}
	}

	void clear() { resize(0); }
	void reset() {
		clear();
		if (data) {
			ALLOCATOR::free(data);
			data = nullptr;
			capacity = 0;
		}
	}
	bool empty() const { return count == 0; }
	U get_capacity() const { return capacity; }

	void reserve(U p_size, bool p_allow_shrink = false) {
		p_size = nearest_power_of_2_templated(p_size);
		if (!p_allow_shrink ? p_size > capacity : ((p_size >= count) && (p_size != capacity))) {
			capacity = p_size;
			data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
			NP_CRASH_COND_MSG(!data, "Out of memory");
		}
	}

	U size() const { return count; }
	void resize(U p_size) {
		if (p_size < count) {
			count = p_size;
		} else if (p_size > count) {
			if (p_size > capacity) {
				if (capacity == 0) {
					capacity = 1;
				}
				while (capacity < p_size) {
					capacity <<= 1;
				}
				data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
				NP_CRASH_COND_MSG(!data, "Out of memory");
			}
			count = p_size;
		}
	}
	const T &operator[](U p_index) const {
		NP_CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}
	T &operator[](U p_index) {
		NP_CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	void fill(T p_val) {
		for (U i = 0; i < count; i++) {
			data[i] = p_val;
		}
	}

	void insert(U p_pos, T p_val) {
		NP_NP_ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		if (p_pos == count) {
			push_back(p_val);
		} else {
			resize(count + 1);
			for (U i = count - 1; i > p_pos; i--) {
				data[i] = data[i - 1];
			}
			data[p_pos] = p_val;
		}
	}

	i64 find(const T &p_val, U p_from = 0) const {
		for (U i = p_from; i < count; i++) {
			if (data[i] == p_val) {
				return i64(i);
			}
		}
		return -1;
	}

	template <class C>
	void sort_custom() {
		U len = count;
		if (len == 0) {
			return;
		}

		SortArray<T, C> sorter;
		sorter.sort(data, len);
	}

	void sort() {
		sort_custom<_DefaultComparator<T>>();
	}

	void ordered_insert(T p_val) {
		U i;
		for (i = 0; i < count; i++) {
			if (p_val < data[i]) {
				break;
			}
		}
		insert(i, p_val);
	}

	Vector() {}
	Vector(const Vector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}
	Vector &operator=(const Vector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
		return *this;
	}

	~Vector() {
		if (data) {
			reset();
		}
	}
};

} // namespace NavPhysics
