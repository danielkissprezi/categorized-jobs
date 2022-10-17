#include "api.h"
#include <mutex>
#include <algorithm>
#include <chrono>

auto cmp = [](auto* lhs, auto* rhs) { return lhs->priority < rhs->priority; };

namespace {
int64_t CategoryToWaitBudget(Category c) {
	return 2 * int64_t(c);
}

}  // namespace

unsigned JobsQueue::AddWorker(unsigned capacity, CategoryMask filter, decltype(WorkerData::consume) consume) {
	std::unique_lock<std::mutex> l(qm_);
	auto id = nextId++;
	workers.emplace_back(WorkerData{id, capacity, 0, filter, std::move(consume)});
	totalCapacity_ += capacity;
	return id;
}

void JobsQueue::Push(Job& t) {
	std::unique_lock<std::mutex> l(qm_);

	jobs.emplace_back(JobData{&t, 0});
	if (jobs.size() >= totalCapacity_) {
		Dispatch();
	}
}

void JobsQueue::Dispatch() {
	std::unique_lock<std::mutex> l(qm_);

	if (jobs.size() == 0) {
		return;	 // nothing to do
	}

	assert(workers.size() > 0 && "Can't dispatch without workers");

	// sort by category and priority
	std::sort(jobs.begin(), jobs.end(), [](auto&& a, auto&& b) {
		auto acat = a.j->category, bcat = b.j->category;
		return (acat == bcat) ? a.j->priority < b.j->priority : acat < bcat;
	});

	// basic round robin
	for (auto&& w : workers) {
		w.assinged = 0;
	}
	size_t len = workers.size();
	size_t i = len;
	for (auto&& job : jobs) {
		// try to find the next that can accept this job
		// if none is found then this job is skipped? UB? Error?
		auto next = (i + 1) % len;
		for (auto j = next; j != i; j = (j + 1) % len) {
			if (workers[j].assinged < workers[j].capacity) {
				job.workerIndex = j;
				++workers[j].assinged;
				break;
			}
		}
		i = next;
	}

	// group by workerId
	// and send batches
	{
		std::stable_sort(jobs.begin(), jobs.end(), [](auto&& a, auto&& b) { return a.workerIndex < b.workerIndex; });
		auto workerIndex = jobs[0].workerIndex;
		auto i = 0;
		auto j = 0;

		for (; j < jobs.size(); ++j) {
			auto wid = jobs[j].workerIndex;
			if (wid != workerIndex) {
				// last batch ended, send it
				workers[workerIndex].consume(&jobs[i], &jobs[j]);
				i = j;
			}
			workerIndex = wid;
		}
		// send last batch
		workers[workerIndex].consume(&jobs[i], &jobs[j]);
	}
	jobs.clear();
}

void JobsQueue::SignalDone(unsigned workerId) {
	std::unique_lock<std::mutex> l(qm_);
	workers[workerId].assinged = 0;
}
