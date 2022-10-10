#include "api.h"

int main(int argc, char* argv[]) {
	puts("hello world");

	Scheduler s;

	s.Dispatch(std::make_unique<Job>(Category::kFastLane, Priority::kDefault, [] { puts("fast"); }));
	s.Dispatch(std::make_unique<Job>(Category::kApp, Priority::kDefault, [] { puts("app"); }));
}
