#include "api.h"
#include <mutex>
#include <algorithm>
#include <chrono>

auto cmp = [](auto* lhs, auto* rhs) { return lhs->priority < rhs->priority; };

int64_t CategoryToWaitBudget(Category c) {
	return 10 * int64_t(c);
}

void JobsQueue::Push(Job& t) {
	std::unique_lock<std::mutex> l(qm_);

	auto it = queues.find(t.category);
	if (it == queues.end()) {
		// If category q does not exist, then we lazily initialize the heap
		//
		// another approach might be to pre-initialize categories and return an error in this case
		//
		std::vector<Job*> v;
		v.reserve(128);
		v.push_back(&t);

		// TODO: budget should be computed from the category
		auto data = QueueData{v, 0, CategoryToWaitBudget(t.category)};
		queues.insert(std::make_pair(t.category, data));
	} else {
		it->second.heap.push_back(&t);
		std::push_heap(it->second.heap.begin(), it->second.heap.end(), cmp);
	}
}

namespace {
Job* Pop(JobsQueue::QueueData& qdata, int64_t now) {
	std::pop_heap(qdata.heap.begin(), qdata.heap.end(), cmp);
	Job* result = qdata.heap.back();
	qdata.heap.pop_back();
	qdata.lastRun = now;
	return result;
}
}  // namespace

Job* JobsQueue::Pop(CategoryMask taskMask) {
	std::unique_lock<std::mutex> l(qm_);

    // ms
	const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	// first walk backwards and see if a category has exhausted it's wait budget
	//
	for (auto it = queues.rbegin(); it != queues.rend(); ++it) {
		auto& qdata = it->second;
		auto& heap = qdata.heap;
		auto elapsed = now - qdata.lastRun;

		if ((elapsed > qdata.waitBudget) && (((CategoryMask(it->first) & taskMask) != 0 && heap.size() != 0))) {
			return ::Pop(qdata, now);
		}
	}
	// then walk forwards and try to pop the most urgent task
	//
	for (auto&& [category, qdata] : queues) {
		auto& heap = qdata.heap;
		if (((CategoryMask(category) & taskMask) != 0 && heap.size() != 0)) {
			return ::Pop(qdata, now);
		}
	}
	// not found
	return nullptr;
}
