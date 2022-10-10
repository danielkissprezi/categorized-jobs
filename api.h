#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <map>
#include <vector>
#include <memory>
#include <atomic>

// lower category comes first
enum class Category : uint16_t { kFastLane = 1, kRender = 1 << 1, kApp = 1 << 2 };
// lower priority comes first
enum class Priority : uint16_t { kHigh = 0, kDefault, kLow };

// category masks use OR relation
using CategoryMask = uint16_t;
constexpr CategoryMask kAllCategories = uint16_t(~0U);

struct Job {
	std::function<void()> f;
	// category stays the same
	const Category category;
	// Priority may change, but should only be changed via Queue APIs once enqueued!
	Priority priority;

	Job(Category c, Priority p, std::function<void()> fun) : f(std::move(fun)), category(c), priority(p) {
	}
};

class JobsQueue {
	// map is sorted by Category, tasks are a heap, sorted by Task priority
	std::map<Category, std::vector<Job*>> queues;
	std::mutex qm_;

public:
	JobsQueue() = default;
	JobsQueue(JobsQueue const&) = delete;
	~JobsQueue() = default;

	void Push(Job& t);
	Job* Pop(CategoryMask categoryMask);
	// ????????????
	void BringToFront(Job& t);
};

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

class Scheduler {
	// init/deinit order matters!
	std::vector<std::unique_ptr<Job>> jobs;
	std::unique_ptr<JobsQueue> queue;
	std::condition_variable cv;
	std::mutex cv_m_;
	std::vector<std::unique_ptr<Worker>> workers;
	std::mutex m_;

public:
	Scheduler();

	void Dispatch(std::unique_ptr<Job> j);
};
