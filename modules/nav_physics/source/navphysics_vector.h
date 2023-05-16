#pragma once

#include "navphysics_allocator.h"
#include "navphysics_error.h"
#include "navphysics_sort_array.h"
#include "navphysics_typedefs.h"

#include <type_traits>

namespace NavPhysics {

template <class T, class U = u32, bool force_trivial = false, bool USE_EXTERNAL_DATA = false>
class Vector {
private:
	U count = 0;
	U capacity = 0;
	T *data = nullptr;

public:
	void setup_external(T *p_data, U p_capacity) {
		NP_DEV_ASSERT(USE_EXTERNAL_DATA);
		data = p_data;
		capacity = p_capacity;
	}

	T *ptr() {
		return data;
	}

	const T *ptr() const {
		return data;
	}

	bool check_and_push_back(T p_elem) {
		if (find(p_elem) == -1) {
			push_back(p_elem);
			return true;
		}
		return false;
	}

	// Only really valid for external vectors.
	bool is_full() const {
		return count == capacity;
	}

	void push_back(T p_elem) {
		if (is_full()) {
			if (USE_EXTERNAL_DATA) {
				NP_ERR_FAIL_MSG("External Vector full.");
			}

			if (capacity == 0) {
				capacity = 1;
			} else {
				capacity <<= 1;
			}
			data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
			NP_CRASH_COND_MSG(!data, "Out of memory");
		}

		if (!std::is_trivially_constructible<T>::value && !force_trivial) {
			new (&data[count++]) T(p_elem);
		} else {
			data[count++] = p_elem;
		}
	}

	bool pop_back(T &r_t) {
		const T *l = get_last();
		NP_ERR_FAIL_NULL_V(l, false);
		r_t = *l;
		resize(size() - 1);
		return true;
	}

	T &request() {
		if (USE_EXTERNAL_DATA) {
			NP_ERR_FAIL_COND_V(is_full(), data[0]);
		}
		u32 s = size();
		resize(s + 1);
		return data[s];
	}

	const T *get_first() const {
		if (!size()) {
			return nullptr;
		}
		return &data[0];
	}

	const T *get_last() const {
		if (!size()) {
			return nullptr;
		}
		return &data[size() - 1];
	}

	T *get_last() {
		if (!size()) {
			return nullptr;
		}
		return &data[size() - 1];
	}

	const T &last(U p_from_last = 0) const {
		NP_DEV_ASSERT(size());
		NP_DEV_ASSERT(p_from_last <= (size() - 1));
		return data[size() - 1 - p_from_last];
	}

	T &last(U p_from_last = 0) {
		NP_DEV_ASSERT(size());
		NP_DEV_ASSERT(p_from_last <= (size() - 1));
		return data[size() - 1 - p_from_last];
	}

	void remove(U p_index) {
		NP_ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (U i = p_index; i < count; i++) {
			data[i] = data[i + 1];
		}
		if (!std::is_trivially_destructible<T>::value && !force_trivial) {
			data[count].~T();
		}
	}

	// Removes the item copying the last value into the position of the one to
	// remove. It's generally faster than `remove`.
	void remove_unordered(U p_index) {
		NP_ERR_FAIL_INDEX(p_index, count);
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
		if (!USE_EXTERNAL_DATA && data) {
			ALLOCATOR::free(data);
			data = nullptr;
			capacity = 0;
		}
	}
	bool is_empty() const { return count == 0; }
	U get_capacity() const { return capacity; }

	void reserve(U p_size, bool p_allow_shrink = false) {
		NP_ERR_FAIL_COND(USE_EXTERNAL_DATA);

		p_size = np_nearest_power_of_2_templated(p_size);
		if (!p_allow_shrink ? p_size > capacity : ((p_size >= count) && (p_size != capacity))) {
			capacity = p_size;
			data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
			NP_CRASH_COND_MSG(!data, "Out of memory");
		}
	}

	U size() const { return count; }
	bool resize(U p_size) {
		if (p_size < count) {
			if (!std::is_trivially_destructible<T>::value && !force_trivial) {
				for (U i = p_size; i < count; i++) {
					data[i].~T();
				}
			}
			count = p_size;
		} else if (p_size > count) {
			if (p_size > capacity) {
				if (USE_EXTERNAL_DATA) {
					NP_ERR_FAIL_COND_V(p_size > capacity, false);
				}

				if (capacity == 0) {
					capacity = 1;
				}
				while (capacity < p_size) {
					capacity <<= 1;
				}
				data = (T *)ALLOCATOR::realloc(data, capacity * sizeof(T));
				NP_CRASH_COND_MSG(!data, "Out of memory");
			}
			if (!std::is_trivially_constructible<T>::value && !force_trivial) {
				for (U i = count; i < p_size; i++) {
					new (&data[i]) T;
				}
			}
			count = p_size;
		}
		return true;
	}
	const T &operator[](U p_index) const {
		NP_DEV_ASSERT(p_index < count);
		// NP_CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}
	T &operator[](U p_index) {
		NP_DEV_ASSERT(p_index < count);
		// NP_CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	void fill(T p_val) {
		for (U i = 0; i < count; i++) {
			data[i] = p_val;
		}
	}

	void insert(U p_pos, T p_val) {
		NP_ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		if (p_pos == count) {
			push_back(p_val);
		} else {
			NP_ERR_FAIL_COND(resize(count + 1) == false);
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

	bool contains(const T &p_val, U p_from = 0) const {
		return find(p_val, p_from) != -1;
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

	void insert_multiple(const T *p_source, U p_num_items, U p_pos) {
		if (!p_num_items) {
			return;
		}
		NP_ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		NP_ERR_FAIL_NULL(p_source);

		i32 old_size = size();
		u32 new_size = old_size + p_num_items;

		NP_ERR_FAIL_COND(resize(new_size) == false);

		// First move any existing items AFTER the insertion.
		i32 num_move = old_size - p_pos;
		for (i32 n = 0; n < num_move; n++) {
			(*this)[new_size - 1 - n] = (*this)[old_size - 1 - n];
		}

		// Now move the source into the vector.
		for (u32 n = 0; n < p_num_items; n++) {
			(*this)[p_pos + n] = p_source[n];
		}
	}

	Vector() {}
	Vector(const Vector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}

	Vector(Vector &&p_from) {
		data = p_from.data;
		count = p_from.count;
		capacity = p_from.capacity;

		p_from.data = nullptr;
		p_from.count = 0;
		p_from.capacity = 0;
	}

	Vector &operator=(const Vector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
		return *this;
	}

	void operator=(Vector &&p_from) {
		if (this == &p_from) {
			return;
		}
		reset();
		NP_ERR_FAIL_COND(USE_EXTERNAL_DATA);

		data = p_from.data;
		count = p_from.count;
		capacity = p_from.capacity;

		p_from.data = nullptr;
		p_from.count = 0;
		p_from.capacity = 0;
	}

	~Vector() {
		if (data) {
			reset();
		}
	}
};

// Constructed vector.
template <class T, class U = u32, bool USE_EXTERNAL_DATA = false>
class TVector : public Vector<T, U, true, USE_EXTERNAL_DATA> {};

template <class T, class U = u32>
class StackVector : public Vector<T, U, false, true> {};

} // namespace NavPhysics
