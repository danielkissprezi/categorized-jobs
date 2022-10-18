#include "api.h"
#include <mutex>
#include <algorithm>
#include <chrono>

#define SENTINEL ~0ULL

unsigned JobsQueue::AddWorker(unsigned capacity, CategoryMask filter, decltype(WorkerData::consume) consume) {
	std::unique_lock<std::mutex> l(qm_);
	auto id = nextId++;
	workers.emplace_back(WorkerData{id, capacity, 0, filter, std::move(consume)});
	totalCapacity_ += capacity;
	return id;
}

void JobsQueue::Push(Job& t) {
	std::unique_lock<std::mutex> l(qm_);

	jobs.emplace_back(JobData{&t, SENTINEL});
	if (jobs.size() >= totalCapacity_) {
		l.unlock();
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
			if (workers[j].assinged < workers[j].capacity && (CategoryMask(job.j->category) & workers[j].jobFilter) != 0) {
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
		// groups are sorted by priority
		std::stable_sort(jobs.begin(), jobs.end(), [](auto&& a, auto&& b) { return a.workerIndex < b.workerIndex; });
		/*
		for (auto j : jobs) {
			printf("%zu ", j.workerIndex);
		}
		puts("");
		//*/

		auto workerIndex = jobs[0].workerIndex;
		auto i = 0;
		auto j = 0;

		for (; j < jobs.size() && workerIndex != SENTINEL; ++j) {
			auto wid = jobs[j].workerIndex;
			if (wid != workerIndex) {
				// last batch ended, send it
				workers[workerIndex].consume(&jobs[i], &jobs[j]);
				i = j;
			}
			workerIndex = wid;
		}
		// send last batch
		if (workerIndex != SENTINEL) {
			workers[workerIndex].consume(&jobs[i], &jobs[j]);
			jobs.clear();
		} else {
			auto it = std::remove_if(jobs.begin(), jobs.end(), [](auto&& j) { return j.workerIndex != SENTINEL; });
			jobs.erase(it, jobs.end());
		}
	}
}

void JobsQueue::SignalDone(unsigned workerId) {
	std::unique_lock<std::mutex> l(qm_);
	workers[workerId].assinged = 0;
	if (jobs.size() > 0) {
		l.unlock();
		Dispatch();
	}
}
