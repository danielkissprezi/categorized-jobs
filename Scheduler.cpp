#include "api.h"

Scheduler::Scheduler() : queue(std::make_unique<JobsQueue>()) {
	const auto n = 1ULL;
	// const auto n = std::thread::hardware_concurrency();
	workers.reserve(n);

	workers.push_back(std::make_unique<Worker>(CategoryMask(Category::kFastLane), queue.get(), &cv, &cv_m_));
	for (int i = std::max(int(n) - 1, 1); i >= 0; i--) {
		workers.push_back(std::make_unique<Worker>(kAllCategories, queue.get(), &cv, &cv_m_));
	}
	jobs.reserve(1024);
}

void Scheduler::Dispatch(std::unique_ptr<Job> j) {
	std::lock_guard<std::mutex> l(m_);
	jobs.emplace_back(std::move(j));
	queue->Push(*jobs.back());
	// wake up threads, because workers have different filters we gotta wake up all
	cv.notify_all();
}
