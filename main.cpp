#include "api.h"
#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

#define LONG_TASKS

void ScheduleBunch(Scheduler& s, std::vector<std::unique_ptr<Job>>& jobs, size_t n, std::chrono::microseconds d, Category c, Priority p) {
	for (int i = 0; i < n; i++) {
		jobs.emplace_back(std::make_unique<Job>(c, p, [d] {
			// simulate waiting
			std::this_thread::sleep_for(d);
		}));
		s.Dispatch(*jobs.back());
	}
}

int main(int argc, char* argv[]) {
	std::vector<std::unique_ptr<Job>> jobs;
	jobs.reserve(20'000);
	std::unique_ptr<Scheduler> s = nullptr;
	if (argc > 1) {
		char* c;
		const auto n = strtol(argv[1], &c, 10);
		if (n > 0) {
			s = std::make_unique<Scheduler>(n);
		} else {
			s = std::make_unique<Scheduler>();
		}
	} else {
		s = std::make_unique<Scheduler>();
	}
	ScheduleBunch(*s, jobs, 30, 3400us, Category::kApp, Priority::kHigh);
	ScheduleBunch(*s, jobs, 90, 4200us, Category::kApp, Priority::kHigh);
	ScheduleBunch(*s, jobs, 43, 5ms, Category::kApp, Priority::kHigh);
	ScheduleBunch(*s, jobs, 4, 6ms, Category::kApp, Priority::kHigh);

	ScheduleBunch(*s, jobs, 1175, 350us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*s, jobs, 24, 1150us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*s, jobs, 5, 1700us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*s, jobs, 32, 3000us, Category::kApp, Priority::kHigh);

	// wait for all jobs to finish
	for (;;) {
		std::this_thread::yield();
		for (auto&& job : jobs) {
			if (!job->done.load(std::memory_order_relaxed)) goto cont;
		}
		break;
	cont:;
	}
}
