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
	std::chrono::system_clock::time_point startTime;
	std::vector<std::unique_ptr<Job>> jobs;
	int n = 0;

	AppData(std::unique_ptr<JobSystem> jobsystem) : js(std::move(jobsystem)) {
		jobs.reserve(4096);
	}
};

namespace {
void StartTest(int n);

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
	std::cout << "Done n=" << data->n << " time_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(now - data->startTime).count()
			  << "ms\n";
	delete data;
	return EM_FALSE;
}

void StartTest(int n) {
	auto data = new AppData(std::make_unique<JobSystem>(n));
	data->startTime = std::chrono::system_clock::now();
	data->n = n;
	for (int i = 0; i < 1'000'000; i++) {
		data->jobs.emplace_back(std::make_unique<Job>(Category::kApp, Priority::kDefault, [] {}));
		data->js->Dispatch(*data->jobs.back());
	}

	puts("initial dispatch done");
	emscripten_request_animation_frame_loop(Tick, data);
}

}  // namespace

int main() {
	StartTest(16);
}
