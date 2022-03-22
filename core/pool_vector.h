/*************************************************************************/
/*  pool_vector.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifndef POOL_VECTOR_H
#define POOL_VECTOR_H

#include "core/local_vector.h"
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/safe_refcount.h"
#include "core/ustring.h"

// Notes on use.
//////////////////////////////
// For efficiency, if you are intending to read or write more than one element,
// You should create a Read or Write and access through that, because it will
// create one lock for multiple elements, versus individual access via PoolVector
// will do a lock per element.

// Adds locking. This adds some memory overhead and possibly
// some performance penalty, but is assumed to be present.
// It may not actually be necessary when using single thread rendering etc
// according to limited tests.
#define GODOT_POOL_VECTOR_THREAD_SAFE

#ifdef GODOT_POOL_VECTOR_THREAD_SAFE
// We have a choice of two methods of locking, either per Alloc
// (which uses more memory and is fine grained)
// or a global Lock (uses less memory, more likely to be contended).
// The global lock in addition only locks for resizing, and does not prevent
// e.g. reading and writing at the same time.
// Define this in order to lock per alloc, or else it will use a global lock.
#define GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC

#ifndef GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC
#define GODOT_POOL_VECTOR_USE_GLOBAL_LOCK
#ifdef DEV_ENABLED
#define GODOT_POOL_VECTOR_USE_CONTENTION_MONITOR
#endif
#endif // ndef GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC

#endif // GODOT_POOL_VECTOR_THREAD_SAFE

// Uncomment this to turn on leak reporting.
// Only intended for internal testing, it is not very efficient..
// and once the reference counting is working correctly, it should
// just work (TM), unless you for instance create a PoolVector and forget to delete it.
// #define GODOT_POOL_VECTOR_REPORT_LEAKS

// These are used purely for debug statistics
struct MemoryPool {
#ifdef GODOT_POOL_VECTOR_REPORT_LEAKS
	static void report_alloc(void *p_alloc, int p_line);
	static void report_free(void *p_alloc);
	static void report_leaks();
#endif

#ifdef GODOT_POOL_VECTOR_USE_GLOBAL_LOCK
	static Mutex global_mutex;
#endif

	static Mutex counter_mutex;
	static uint32_t allocs_used;
	static size_t total_memory;
	static size_t max_memory;
};

template <class T>
class PoolVector {
	// The actual allocation which contains the vector
	// is separate from the PoolVector - as it may be shared between
	// multiple PoolVectors using COW.
	// It is thus reference counted and can be locked.
	struct Alloc {
		// Each PoolVector that references an Alloc adds a count.
		// When this reaches zero through dereferencing, the Alloc can be deleted.
		SafeRefCount refcount;

#ifdef GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC
		// Mutex is possibly 80 bytes per alloc, so there is a cost here.
		// We could also possibly use some smaller locking method.
		// In addition deleting a locked mutex is undefined behaviour, so we need to
		// watch for this.
		Mutex mutex;
#endif

#ifdef GODOT_POOL_VECTOR_USE_CONTENTION_MONITOR
		SafeNumeric<uint32_t> contention_lock;
		Alloc() :
				contention_lock(0) {}
#endif

		LocalVector<T> vector;

		// Use either a global lock or local lock, depending which is compiled in.
		void super_lock() {
#ifdef GODOT_POOL_VECTOR_USE_GLOBAL_LOCK
			MemoryPool::global_mutex.lock();
#endif
			lock();
		}

		// Use either a global lock or local lock, depending which is compiled in.
		void super_unlock() {
#ifdef GODOT_POOL_VECTOR_USE_GLOBAL_LOCK
			MemoryPool::global_mutex.unlock();
#endif
			unlock();
		}

		void lock(bool p_report_simultaneous = false) {
#ifdef GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC
			// Can uncomment this version to show when contention is occurring.
			// if (mutex.try_lock() != OK) {
			// WARN_PRINT("Contended poolvector lock.");
			// mutex.lock();
			// }
			mutex.lock();
#endif

#ifdef GODOT_POOL_VECTOR_USE_CONTENTION_MONITOR
			if ((contention_lock.postincrement() != 0) && p_report_simultaneous) {
				WARN_PRINT_ONCE("Poolvector simultaneous access detected, possible race condition.");
			}
#endif
		}
		void unlock() {
#ifdef GODOT_POOL_VECTOR_USE_LOCK_PER_ALLOC
			mutex.unlock();
#endif

#ifdef GODOT_POOL_VECTOR_USE_CONTENTION_MONITOR
			contention_lock.postdecrement();
#endif
		}
	};

	Alloc *_alloc = nullptr;

	// Returns true if makes a copy.
	bool _copy_on_write() {
		if (!_alloc) {
			return false;
		}

		// Note: This comment is preserved from the old implementation, possibly indicates some past problem with locking here.
		// ERR_FAIL_COND(alloc->lock>0); should not be illegal to lock this for copy on write, as it's a copy on write after all

		// Refcount should not be zero, otherwise it's a misuse of COW
		uint32_t refcount = _alloc->refcount.get();
		if (refcount == 1) {
			return false; //nothing to do
		}
		DEV_ASSERT(refcount);

		Alloc *alloc = memnew(Alloc);
		alloc->refcount.init(1);

#ifdef GODOT_POOL_VECTOR_REPORT_LEAKS
		MemoryPool::report_alloc(alloc, __LINE__);
#endif

		// copy contents
		_alloc->super_lock();
		alloc->vector = _alloc->vector;
		_alloc->super_unlock();

#ifdef DEBUG_ENABLED
		MemoryPool::counter_mutex.lock();
		MemoryPool::allocs_used++;
		MemoryPool::total_memory += (alloc->vector.size() * sizeof(T)) + sizeof(Alloc);
		if (MemoryPool::total_memory > MemoryPool::max_memory) {
			MemoryPool::max_memory = MemoryPool::total_memory;
		}
		MemoryPool::counter_mutex.unlock();
#endif

		// release old alloc
		_unreference();

		// we are the new owner of this new allocation copy
		_alloc = alloc;

		return true;
	}

	void _reference(const PoolVector &p_pool_vector) {
		// Possible race condition. What happens if source_alloc is destroyed
		// or changed before we add a refcount?
		Alloc *source_alloc = p_pool_vector._alloc;

		if (_alloc == source_alloc) {
			return;
		}

		_unreference();

		// We referenced it okay
		if (source_alloc && source_alloc->refcount.ref()) {
			_alloc = source_alloc;
			return;
		}

		// Either source_alloc is NULL
		// This shouldn't happen in normal use, as we are attempting to make sure
		// any PoolVector always has an alloc (even if it is empty vector).

		// OR we are in the danger zone -
		// alloc is pending deletion and refcount is zero.
		// We therefore need to abort.
		if (!source_alloc) {
			ERR_PRINT("Attempting to reference from an empty PoolVector");
		}

		// Create a dummy, or else this may assert / crash later
		Alloc *alloc = memnew(Alloc);
		alloc->refcount.init(1);

#ifdef DEBUG_ENABLED
		MemoryPool::counter_mutex.lock();
		MemoryPool::allocs_used++;
		MemoryPool::total_memory += sizeof(Alloc);
		if (MemoryPool::total_memory > MemoryPool::max_memory) {
			MemoryPool::max_memory = MemoryPool::total_memory;
		}
		MemoryPool::counter_mutex.unlock();
#endif

#ifdef GODOT_POOL_VECTOR_REPORT_LEAKS
		MemoryPool::report_alloc(alloc, __LINE__);
#endif
		_alloc = alloc;
	}

	void _unreference() {
		if (!_alloc) {
			return;
		}

		if (!_alloc->refcount.unref()) {
			_alloc = nullptr;
			return;
		}

		// Great terror lies here. At this point the alloc
		// has zero refcount and could possibly be incremented BEFORE
		// we delete it, so we would ideally like to detect this in areas that
		// increase the refcount and abort if incrementing from zero (as it is pending deletion).

#ifdef GODOT_POOL_VECTOR_REPORT_LEAKS
		MemoryPool::report_free(_alloc);
#endif

#ifdef DEBUG_ENABLED
		MemoryPool::counter_mutex.lock();
		MemoryPool::allocs_used--;
		MemoryPool::total_memory -= (_alloc->vector.size() * sizeof(T)) + sizeof(Alloc);
		MemoryPool::counter_mutex.unlock();
#endif
		memdelete(_alloc);
		_alloc = nullptr;
	}

public:
	class Access {
		friend class PoolVector;

	protected:
		Alloc *_alloc;

		void _ref(Alloc *p_alloc) {
			_alloc = p_alloc;
			if (_alloc) {
				_alloc->lock();
			}
		}

		_FORCE_INLINE_ void _unref() {
			if (_alloc) {
				_alloc->unlock();
				_alloc = nullptr;
			}
		}

		Access() {
			_alloc = nullptr;
		}

	public:
		~Access() {
			_unref();
		}

		void release() {
			_unref();
		}
	};

	class Read : public Access {
	public:
		// This will go down if _alloc is NULL, (e.g. after calling release).
		// There is no protection (perhaps for speed?)
		// This was the same in the old PoolVector.. we could check for this.
		_FORCE_INLINE_ const T &operator[](int p_index) const { return this->_alloc->vector.ptr()[p_index]; }
		_FORCE_INLINE_ const T *ptr() const {
			return this->_alloc ? this->_alloc->vector.ptr() : nullptr;
		}

		void operator=(const Read &p_read) {
			if (this->_alloc == p_read._alloc) {
				return;
			}
			this->_unref();
			this->_ref(p_read._alloc);
		}

		Read(const Read &p_read) {
			this->_ref(p_read._alloc);
		}

		Read() {}
	};

	class Write : public Access {
	public:
		// This will go down if _alloc is NULL, (e.g. after calling release).
		// There is no protection (perhaps for speed?)
		// This was the same in the old PoolVector.. we could check for this.
		_FORCE_INLINE_ T &operator[](int p_index) const { return this->_alloc->vector.ptr()[p_index]; }
		_FORCE_INLINE_ T *ptr() const {
			return this->_alloc ? this->_alloc->vector.ptr() : nullptr;
		}

		void operator=(const Write &p_read) {
			if (this->_alloc == p_read._alloc) {
				return;
			}
			this->_unref();
			this->_ref(p_read._alloc);
		}

		Write(const Write &p_read) {
			this->_ref(p_read._alloc);
		}

		// For a Write we override this function to allow reporting simultaneous accesses
		// for debugging purposes. Simultaneous access is not a problem for Reads.
		void _ref(Alloc *p_alloc) {
			this->_alloc = p_alloc;
			if (this->_alloc) {
				this->_alloc->lock(true);
			}
		}

		Write() {}
	};

	Read read() const {
		Read r;
		if (_alloc) {
			r._ref(this->_alloc);
		}
		return r;
	}
	Write write() {
		Write w;
		if (_alloc) {
			_copy_on_write(); //make sure there is only one being accessed
			w._ref(_alloc);
		}
		return w;
	}

	template <class MC>
	void fill_with(const MC &p_mc) {
		int c = p_mc.size();
		resize(c);
		Write w = write();
		int idx = 0;
		for (const typename MC::Element *E = p_mc.front(); E; E = E->next()) {
			w[idx++] = E->get();
		}
	}

	void remove(int p_index) {
		Write w = write();
		int s = size();
		ERR_FAIL_INDEX(p_index, s);
		for (int i = p_index; i < s - 1; i++) {
			w[i] = w[i + 1];
		};
		w = Write();
		resize(s - 1);
	}

	inline int size() const;
	inline bool empty() const;
	T get(int p_index) const;
	void set(int p_index, const T &p_val);
	void push_back(const T &p_val);
	void append(const T &p_val) { push_back(p_val); }
	void append_array(const PoolVector<T> &p_arr) {
		ERR_FAIL_COND(p_arr._alloc == _alloc);

		int ds = p_arr.size();
		if (ds == 0) {
			return;
		}
		int bs = size();
		resize(bs + ds);
		Write w = write();
		Read r = p_arr.read();
		for (int i = 0; i < ds; i++) {
			w[bs + i] = r[i];
		}
	}

	PoolVector<T> subarray(int p_from, int p_to) {
		if (p_from < 0) {
			p_from = size() + p_from;
		}
		if (p_to < 0) {
			p_to = size() + p_to;
		}

		ERR_FAIL_INDEX_V(p_from, size(), PoolVector<T>());
		ERR_FAIL_INDEX_V(p_to, size(), PoolVector<T>());

		PoolVector<T> slice;
		int span = 1 + p_to - p_from;
		slice.resize(span);
		Read r = read();
		Write w = slice.write();
		for (int i = 0; i < span; ++i) {
			w[i] = r[p_from + i];
		}

		return slice;
	}

	Error insert(int p_pos, const T &p_val) {
		int s = size();
		ERR_FAIL_INDEX_V(p_pos, s + 1, ERR_INVALID_PARAMETER);
		resize(s + 1);
		{
			Write w = write();
			for (int i = s; i > p_pos; i--) {
				w[i] = w[i - 1];
			}
			w[p_pos] = p_val;
		}

		return OK;
	}

	String join(String delimiter) {
		String rs = "";
		Read r = read();
		int s = size();
		for (int i = 0; i < s; i++) {
			rs += r[i] + delimiter;
		}
		rs.erase(rs.length() - delimiter.length(), delimiter.length());
		return rs;
	}

	inline T operator[](int p_index) const;

	Error resize(int p_size);

	void invert();

	void operator=(const PoolVector &p_pool_vector) { _reference(p_pool_vector); }
	PoolVector() {
		_alloc = memnew(Alloc);
		_alloc->refcount.init(1);

#ifdef DEBUG_ENABLED
		MemoryPool::counter_mutex.lock();
		MemoryPool::allocs_used++;
		MemoryPool::total_memory += sizeof(Alloc);
		if (MemoryPool::total_memory > MemoryPool::max_memory) {
			MemoryPool::max_memory = MemoryPool::total_memory;
		}
		MemoryPool::counter_mutex.unlock();
#endif

#ifdef GODOT_POOL_VECTOR_REPORT_LEAKS
		MemoryPool::report_alloc(_alloc, __LINE__);
#endif
	}
	PoolVector(const PoolVector &p_pool_vector) {
		_alloc = nullptr;
		_reference(p_pool_vector);
	}
	~PoolVector() {
		// This reduces the reference count but may not
		// delete the Alloc, if more than one PoolVector
		// references it.
		_unreference();
	}
};

template <class T>
int PoolVector<T>::size() const {
	DEV_ASSERT(_alloc);
	return _alloc->vector.size();
}

template <class T>
bool PoolVector<T>::empty() const {
	return size() == 0;
}

// Inefficient, prefer a Read for multiple elements.
template <class T>
T PoolVector<T>::get(int p_index) const {
	return operator[](p_index);
}

// Inefficient, prefer using a Write for multiple elements.
template <class T>
void PoolVector<T>::set(int p_index, const T &p_val) {
	ERR_FAIL_INDEX(p_index, size());

	Write w = write();
	w[p_index] = p_val;
}

// Inefficient, prefer using a resize to push_back multiple at once and a Write for setting.
template <class T>
void PoolVector<T>::push_back(const T &p_val) {
	resize(size() + 1);
	set(size() - 1, p_val);
}

// Inefficient, prefer a Read for multiple elements.
template <class T>
T PoolVector<T>::operator[](int p_index) const {
	CRASH_BAD_INDEX(p_index, size());

	Read r = read();
	return r[p_index];
}

template <class T>
Error PoolVector<T>::resize(int p_size) {
	ERR_FAIL_COND_V_MSG(p_size < 0, ERR_INVALID_PARAMETER, "Size of PoolVector cannot be negative.");

	// this should never happen, the logic should always have an alloc associated with a PoolVector
	DEV_ASSERT(_alloc);

	// no change
	if (size() == p_size) {
		return OK;
	}

#ifdef DEBUG_ENABLED
	MemoryPool::counter_mutex.lock();
	MemoryPool::total_memory += (p_size - size()) * sizeof(T);
	if (MemoryPool::total_memory > MemoryPool::max_memory) {
		MemoryPool::max_memory = MemoryPool::total_memory;
	}
	MemoryPool::counter_mutex.unlock();
#endif

	_copy_on_write(); // make it unique

	_alloc->super_lock();
	_alloc->vector.resize(p_size);
	_alloc->super_unlock();
	return OK;
}

template <class T>
void PoolVector<T>::invert() {
	T temp;
	Write w = write();
	int s = size();
	int half_s = s / 2;

	for (int i = 0; i < half_s; i++) {
		temp = w[i];
		w[i] = w[s - i - 1];
		w[s - i - 1] = temp;
	}
}

#endif // POOL_VECTOR_H
