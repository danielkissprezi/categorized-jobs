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
	// ????????????
	void BringToFront(Job& t);

private:
	// map is sorted by Category, Vec<Job*> is a heap, sorted by Job priority
	std::map<Category, QueueData> queues;
	std::mutex qm_;
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
