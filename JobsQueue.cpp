#include "api.h"
#include <mutex>
#include <algorithm>

auto cmp = [](auto* lhs, auto* rhs) { return lhs->priority < rhs->priority; };

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
		queues.insert(std::make_pair(t.category, v));
	} else {
		it->second.push_back(&t);
		std::push_heap(it->second.begin(), it->second.end(), cmp);
	}
}

Job* JobsQueue::Pop(CategoryMask taskMask) {
	std::unique_lock<std::mutex> l(qm_);

	for (auto&& [category, heap] : queues) {
		// TODO: port the de-starvation code from engine here
		if ((CategoryMask(category) & taskMask) != 0 && heap.size() != 0) {
			std::pop_heap(heap.begin(), heap.end(), cmp);
			Job* result = heap.back();
			heap.pop_back();
			return result;
		}
	}
	// not found
	return nullptr;
}
