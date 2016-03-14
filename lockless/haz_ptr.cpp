#include "haz_ptr.hpp"
#include <cassert>
#include <vector>
#include <set>

static const std::size_t MAX_THREAD_COUNT = 1024;
static const std::size_t MAX_HAZARD_COUNT = MAX_THREAD_COUNT * MAX_THREADPTR_COUNT;
static std::atomic<int> next_tid;
static std::atomic<raw_ptr_t> hazard[MAX_HAZARD_COUNT];

static int getThreadId()
{
	static thread_local int tid = -1;

	if (tid == -1)
		tid = next_tid.fetch_add(1);
	return tid;
}

std::atomic<raw_ptr_t> *acquireRawHazardPtr(int idx)
{
	assert(idx < MAX_THREADPTR_COUNT);
	const int tid = getThreadId();
	return &hazard[tid * MAX_THREADPTR_COUNT + idx];
}

struct RetireNode {
	std::function<void(raw_ptr_t)> deleter;
	raw_ptr_t ptr;
};

static void collectGarbage(std::vector<RetireNode> &retireList)
{
	std::set<raw_ptr_t> plist;
	for (std::size_t i = 0; i != MAX_HAZARD_COUNT; ++i) {
		raw_ptr_t ptr = hazard[i].load();

		if (ptr != nullptr)
			plist.insert(ptr);
	}

	std::vector<RetireNode> retire;
	retire.swap(retireList);
	for (std::size_t i = 0; i != retire.size(); ++i) {
		if (plist.count(retire[i].ptr))
			retireList.push_back(retire[i]);
		else
			retire[i].deleter(retire[i].ptr);
	}
}

void releaseRawPtr(raw_ptr_t ptr, std::function<void(raw_ptr_t)> deleter)
{
	static thread_local std::vector<RetireNode> retireList;
	static const size_t RETIRE_THRESHOLD = 10; // should be much bigger

	RetireNode node = { deleter, ptr };
	retireList.push_back(node);
	if (retireList.size() > RETIRE_THRESHOLD)
		collectGarbage(retireList);
}
