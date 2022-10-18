#include "Worker.h"
#include "api.h"

#include <algorithm>
#include <atomic>

static_assert(std::is_trivially_destructible<JobsQueue::JobData>(), "assume POD");
static_assert(std::is_trivially_copyable<JobsQueue::JobData>(), "assume POD");

Worker::Worker(uint16_t categoryMask, JobsQueue* q)
	: queue_(q), categoryMask_(categoryMask), capacity_(16), jobData_(malloc(sizeof(JobsQueue::JobData) * capacity_)) {
	// register this worker in the queue
	auto id = q->AddWorker(capacity_, categoryMask_, [this](JobsQueue::JobData* begin, JobsQueue::JobData* end) {
		// FIXME: really shouldn't block here bro
		std::unique_lock<std::mutex> l(bufM_);
		size_ = std::distance(begin, end);
		assert(size_ <= capacity_);
		memcpy(jobData_, begin, sizeof(JobsQueue::JobData) * size_);

		cv_.notify_one();
	});

	// worker thread
	t_ = std::thread([this, id] {
		std::unique_lock<std::mutex> l(bufM_);
		l.unlock();
		while (!done_.load(std::memory_order_relaxed)) {
			if (size_) {
				printf("workerId=%u is working on job batch of size=%zu\n", id, size_);
				auto* jobs = (JobsQueue::JobData*)jobData_;
				l.lock();
				for (int i = 0; i < size_; i++) {
					assert(jobs[i].j);
					assert(jobs[i].j->f);
					(jobs)[i].j->f();
					(jobs)[i].j->done.store(true, std::memory_order_release);
				}
				// clear job queue
				size_ = 0;
				l.unlock();
				queue_->SignalDone(id);
			} else {
				l.lock();
				cv_.wait_for(l, std::chrono::milliseconds(10));
				l.unlock();
			}
		}
	});
}
