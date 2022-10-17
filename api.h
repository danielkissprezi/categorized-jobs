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
	// Onceenqueued, category and priority must only be changed via public JobSystem APIs!
	Category category;
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
	std::vector<std::unique_ptr<Worker>> workers;

public:
	JobSystem(size_t n = std::thread::hardware_concurrency());

	// dispatch remaining work
	void Dispatch();
	void Dispatch(Job& j);
};
//////////////////////////////////// </public API> ////////////////////////////////////

class JobsQueue {
public:
	struct JobData {
		Job* j;
		size_t workerIndex = 0;
	};

	struct WorkerData {
		unsigned workerId;
		unsigned capacity;
		unsigned assinged = 0;
		CategoryMask jobFilter;

		/// consume must not call the queue
		/// consume must copy all data to the worker
		std::function<void(JobData* begin, JobData* end)> consume;
	};

	JobsQueue() = default;
	JobsQueue(JobsQueue const&) = delete;
	~JobsQueue() = default;

	void Push(Job& t);
	unsigned AddWorker(unsigned capacity, CategoryMask filter, decltype(WorkerData::consume) consume);
	void Dispatch();

	void SignalDone(unsigned workerId);

private:
	unsigned nextId = 0;
	std::vector<JobData> jobs;
	std::vector<WorkerData> workers;
	size_t totalCapacity_ = 0;
	std::mutex qm_;
};
