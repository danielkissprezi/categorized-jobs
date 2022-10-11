#include "api.h"

Scheduler::Scheduler() : queue(std::make_unique<JobsQueue>()) {
	const auto n = std::thread::hardware_concurrency();
	workers.reserve(n);

	// dedicated workers
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kFastLane), queue.get(), &cv, &cv_m_));
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kBackground), queue.get(), &cv, &cv_m_));
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kRender), queue.get(), &cv, &cv_m_));
	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kApp), queue.get(), &cv, &cv_m_));

	// shared workers increase throughput, if there's available hardware
	for (int i = int(n) - int(workers.size()); i > 0; i--) {
		workers.push_back(std::make_unique<Worker>(kAllCategories, queue.get(), &cv, &cv_m_));
	}

	printf("initialized %lu workers, target n=%d\n", workers.size(), n);
	jobs.reserve(1024);
}

void Scheduler::Dispatch(std::unique_ptr<Job> j) {
	std::lock_guard<std::mutex> l(m_);
	jobs.emplace_back(std::move(j));
	queue->Push(*jobs.back());
	// wake up threads, because workers have different filters we gotta wake up all
	cv.notify_all();
}
