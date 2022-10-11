#include "api.h"
#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

int main(int argc, char* argv[]) {
	Scheduler s;

	for (int j = 0; j < 10'000; j++) {
		puts("tick");
		// bunch of app tasks
		for (int i = 0; i < 100; i++) {
			s.Dispatch(std::make_unique<Job>(Category::kApp, Priority::kDefault, [] {
				puts("app");
				//*
				std::this_thread::sleep_for(100ms);
				//*/
			}));
		}
		// render job generator job
		s.Dispatch(std::make_unique<Job>(Category::kRender, Priority::kDefault, [&s] {
			puts("render dispatch");
			for (int i = 0; i < 10; i++) {
				s.Dispatch(std::make_unique<Job>(Category::kRender, Priority::kLow, [] {
					puts("low prio render");
					//*
					std::this_thread::sleep_for(100ms);
					//*/
				}));
			}
		}));
		// fast lane is submitted last
		s.Dispatch(std::make_unique<Job>(Category::kFastLane, Priority::kDefault, [] { puts("fast"); }));
		// simulate a little work
		std::this_thread::sleep_for(10ms);
	}
	// bandaid to wait for jobs' termination
	// TODO: fix scheduler shutdown
	std::this_thread::sleep_for(1000ms);
}
