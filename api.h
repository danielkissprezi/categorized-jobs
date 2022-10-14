#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <map>
#include <vector>
#include <memory>
#include <atomic>

#include "Worker.h"

//////////////////////////////////// <public API> ////////////////////////////////////

// lower category comes first
enum class Category : uint16_t {
	kFastLane = 1,
	kRender = 1 << 1,
	kApp = 1 << 2,
	kBackground = 1 << 3,
};
// higher priority comes first
enum class Priority : uint16_t {
	kLow = 0,
	kDefault,
	kHigh,
};

// Masks can match multiple categories
using CategoryMask = uint16_t;

struct Job {
	std::function<void()> f;
	// category stays the same
	const Category category;
	// Priority may change, but should only be changed via Queue APIs once enqueued!
	Priority priority;
	std::atomic_bool done{false};

	Job(Category c, Priority p, std::function<void()> fun) : f(std::move(fun)), category(c), priority(p) {
	}
};

class JobsQueue;
class JobSystem {
	// init/deinit order matters!
	std::unique_ptr<JobsQueue> queue;
	std::condition_variable cv;
	std::mutex cv_m_;
	std::vector<std::unique_ptr<Worker>> workers;
	std::mutex m_;

public:
	JobSystem(size_t n = std::thread::hardware_concurrency());

	void Dispatch(Job& j);
};
//////////////////////////////////// </public API> ////////////////////////////////////

class JobsQueue {
public:
	struct QueueData {
		std::vector<Job*> heap;
		int64_t lastPop;
		// how long this queue may wait before popping one
		const int64_t waitBudget;
	};

	JobsQueue() = default;
	JobsQueue(JobsQueue const&) = delete;
	~JobsQueue() = default;

	void Push(Job& t);
	Job* Pop(CategoryMask categoryMask);

private:
	// map is sorted by Category, Vec<Job*> is a heap, sorted by Job priority
	std::map<Category, QueueData> queues;
	std::mutex qm_;
};

