#pragma once
#include <thread>
#include <atomic>
#include <cstdint>

class JobsQueue;

class Worker {
	std::thread t_;
	JobsQueue* queue_;
	const uint16_t categoryMask_;
	std::atomic_bool done_{false};
	std::condition_variable* cv_;
	std::mutex* cv_m_;

public:
	Worker(uint16_t categoryMask, JobsQueue* q, std::condition_variable* cv, std::mutex* cv_m);

	~Worker() {
		done_.store(true, std::memory_order_release);
		cv_->notify_all();
		t_.join();
	}

	Worker(Worker const&) = delete;
	Worker(Worker&&) = delete;
};
