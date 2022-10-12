#include "api.h"

Worker::Worker(uint16_t categoryMask, JobsQueue* q, std::condition_variable* cv, std::mutex* cv_m)
	: queue_(q), categoryMask_(categoryMask), cv_(cv), cv_m_(cv_m) {
	t_ = std::thread([this] {
		std::unique_lock<std::mutex> l(*cv_m_);
		l.unlock();
		while (!done_.load(std::memory_order_relaxed)) {
			auto* task = queue_->Pop(categoryMask_);
			if (task) {
				(task->f)();
				task->done.store(true, std::memory_order_release);
			} else {
				// nothing to do
				l.lock();
				cv_->wait_for(l, std::chrono::milliseconds(10));
				l.unlock();
			}
		}
	});
}
