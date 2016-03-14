#ifndef __HAZ_PTR_HPP__
#define __HAZ_PTR_HPP__

#include <functional>
#include <atomic>
#include <cstddef>

typedef void *	raw_ptr_t;
static const std::size_t MAX_THREADPTR_COUNT = 16;

void releaseRawPtr(raw_ptr_t ptr, std::function<void(raw_ptr_t)> deleter);
std::atomic<raw_ptr_t> *acquireRawHazardPtr(int idx);

template <typename T>
struct HazardPtr {
	std::atomic<raw_ptr_t> *hp;

	HazardPtr(std::atomic<raw_ptr_t> *hp) : hp(hp) { }
	~HazardPtr() { set(nullptr); }

	void set(T *ptr) { hp->store(reinterpret_cast<raw_ptr_t>(ptr)); }
	T *get() { return reinterpret_cast<T *>(hp->load()); }
};

template <typename T>
void releasePtr(T *ptr)
{
	std::function<void(raw_ptr_t)> deleter = [](raw_ptr_t ptr) {
		delete reinterpret_cast<T *>(ptr);
	};
	releaseRawPtr(reinterpret_cast<raw_ptr_t>(ptr), deleter);
}

#endif /*__HAZ_PTR_HPP__*/
