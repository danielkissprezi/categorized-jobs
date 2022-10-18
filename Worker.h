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
	std::condition_variable cv_;

	// yuck
	size_t size_ = 0;
	size_t capacity_;
	void* jobData_;
	std::mutex bufM_;

public:
	Worker(uint16_t categoryMask, JobsQueue* q);

	~Worker() {
		done_.store(true, std::memory_order_release);
		cv_.notify_one();
		t_.join();
	}

	Worker(Worker const&) = delete;
	Worker(Worker&&) = delete;
};
