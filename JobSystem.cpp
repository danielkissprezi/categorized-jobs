#include "api.h"

constexpr CategoryMask kAllCategories = uint16_t(~0U);

JobSystem::JobSystem(size_t n) : queue(std::make_unique<JobsQueue>()) {
	workers.reserve(n);

	// dedicated workers
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kFastLane), queue.get()));
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kBackground), queue.get()));
	// shared workers increase throughput, if there's available hardware
	// make at least 1 so there's no category that gets stuck
	for (int i = std::max(int(n) - int(workers.size()), 1); i > 0; i--) {
		workers.push_back(std::make_unique<Worker>(kAllCategories, queue.get()));
	}

	printf("initialized n=%zu, %lu workers in total\n", n, workers.size());
}

void JobSystem::Dispatch(Job& j) {
	queue->Push(j);
	// TODO: call Dispatch on queue with some heuristic?
}

void JobSystem::Dispatch() {
	queue->Dispatch();
}
