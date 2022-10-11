#include "api.h"
#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

#define LONG_TASKS

int main(int argc, char* argv[]) {
	Scheduler s;

	for (int j = 0; j < 10'000; j++) {
		printf("tick %d\n", j);
		// bunch of app tasks
		for (int i = 0; i < 10; i++) {
			s.Dispatch(std::make_unique<Job>(Category::kApp, Priority::kDefault, [j, i] {
				printf("app %d %d\n", j, i);
#ifdef LONG_TASKS
				std::this_thread::sleep_for(10ms);
#endif
			}));
		}
		// render job generator job
		s.Dispatch(std::make_unique<Job>(Category::kRender, Priority::kDefault, [&s, j] {
			printf("render dispatch %d\n", j);
			for (int i = 0; i < 10; i++) {
				s.Dispatch(
					std::make_unique<Job>(Category::kBackground, Priority::kLow, [i, j] { printf("low prio render %d %d\n", j, i); }));
			}
			for (int i = 0; i < 10; i++) {
				s.Dispatch(std::make_unique<Job>(Category::kBackground, Priority::kDefault, [i, j] {
					printf("background %d %d\n", j, i);
#ifdef LONG_TASKS
					std::this_thread::sleep_for(100ms);
#endif
				}));
			}
		}));
		// fast lane is submitted last
		s.Dispatch(std::make_unique<Job>(Category::kFastLane, Priority::kDefault, [j] { printf("fast %d\n", j); }));
		// simulate a little work
		std::this_thread::sleep_for(10ms);
	}
	// bandaid to wait for jobs' termination
	// TODO: fix scheduler shutdown
	std::this_thread::sleep_for(1000ms);
}
