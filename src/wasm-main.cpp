#include "api.h"

#include <emscripten.h>

#include <emscripten/html5.h>

#include <memory>
#include <cstdio>
#include <iostream>
#include <utility>

using namespace std::literals::chrono_literals;

// test data
struct AppData {
	std::unique_ptr<JobSystem> js;
	const std::chrono::system_clock::time_point startTime;
	std::vector<std::unique_ptr<Job>> jobs;

	AppData(std::unique_ptr<JobSystem> jobsystem, std::chrono::system_clock::time_point st) : js(std::move(jobsystem)), startTime(st) {
		jobs.reserve(4096);
	}
};

namespace {
void ScheduleBunch(JobSystem& s, std::vector<std::unique_ptr<Job>>& jobs, size_t n, std::chrono::microseconds d, Category c, Priority p) {
	for (int i = 0; i < n; i++) {
		jobs.emplace_back(std::make_unique<Job>(c, p, [d] {
			auto a = std::unique_ptr<char[]>(new char[1024]);
			auto b = std::unique_ptr<char[]>(new char[1024]);
			std::chrono::system_clock::time_point now;
			auto start = std::chrono::system_clock::now();
			do {
				memcpy(a.get(), b.get(), 1024);
				a.swap(b);
				*(char volatile*)a.get() = *b.get();
				now = std::chrono::system_clock::now();
			} while ((now - start) < d);
		}));
		s.Dispatch(*jobs.back());
	}
}

EM_BOOL Tick(double t, void* userData) {
	auto* data = (AppData*)userData;
	// Return true to keep the loop running.
	for (auto&& job : data->jobs) {
		if (!job->done.load(std::memory_order_relaxed)) {
			return EM_TRUE;
		}
	}
	auto now = std::chrono::system_clock::now();
	std::cout << "Done in: " << std::chrono::duration_cast<std::chrono::milliseconds>(now - data->startTime).count() << "ms\n";
	delete data;
	return EM_FALSE;
}

}  // namespace

int main() {
	auto now = std::chrono::system_clock::now();
	auto data = new AppData(std::make_unique<JobSystem>(), now);

	ScheduleBunch(*data->js, data->jobs, 30, 3400us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*data->js, data->jobs, 90, 4200us, Category::kRender, Priority::kHigh);
	ScheduleBunch(*data->js, data->jobs, 43, 5ms, Category::kApp, Priority::kHigh);
	ScheduleBunch(*data->js, data->jobs, 4, 6ms, Category::kApp, Priority::kHigh);
	ScheduleBunch(*data->js, data->jobs, 1175, 350us, Category::kFastLane, Priority::kDefault);
	ScheduleBunch(*data->js, data->jobs, 24, 1150us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*data->js, data->jobs, 5, 1700us, Category::kApp, Priority::kDefault);
	ScheduleBunch(*data->js, data->jobs, 32, 3ms, Category::kApp, Priority::kHigh);
	ScheduleBunch(*data->js, data->jobs, 1000, 3ms, Category::kApp, Priority::kHigh);

	puts("initial dispatch done");

	emscripten_request_animation_frame_loop(Tick, data);
}
