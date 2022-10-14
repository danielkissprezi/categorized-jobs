#include "api.h"

JobSystem::JobSystem(size_t n) : queue(std::make_unique<JobsQueue>()) {
	workers.reserve(n);

	// dedicated workers
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kFastLane), queue.get(), &cv, &cv_m_));
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kBackground), queue.get(), &cv, &cv_m_));
	// shared workers increase throughput, if there's available hardware
	// make at least 1 so there's no category that gets stuck
	for (int i = std::max(int(n) - int(workers.size()), 1); i > 0; i--) {
		workers.push_back(std::make_unique<Worker>(kAllCategories, queue.get(), &cv, &cv_m_));
	}

	printf("initialized n=%zu, %lu workers in total\n", n, workers.size());
}

void JobSystem::Dispatch(Job& j) {
	std::lock_guard<std::mutex> l(m_);
	queue->Push(j);
	// wake up threads, because workers have different filters we gotta wake up all
	cv.notify_all();
}
