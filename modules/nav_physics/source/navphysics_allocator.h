#pragma once

#include <stdlib.h>

namespace NavPhysics {

class DefaultAllocator {
public:
	template <class T>
	static T *newT() {
		T *t = (T *)DefaultAllocator::alloc(sizeof(T));
		t = new (t) T();
		return t;
	}

	template <class T>
	static void deleteT(T *t) {
		t->~T();
		DefaultAllocator::free(t);
	}

	static void *alloc(u32 p_bytes) {
		return ::malloc(p_bytes);
	}

	static void free(void *p_addr) {
		::free(p_addr);
	}

	static void *realloc(void *p_addr, u32 p_size) {
		return ::realloc(p_addr, p_size);
	}

	/*
template <class T>
void memnew(T *p_class) {

		p_class = alloc(sizeof(T));

	if (!std::is_trivially_constructible<T>::value) {
		p_class->T();
}

}


template <class T>
void memdelete(T *p_class) {
	if (!std::is_trivially_destructible<T>::value) {
		p_class->~T();
	}

	free(p_class);
}
*/
};

using ALLOCATOR = DefaultAllocator;

} // namespace NavPhysics
