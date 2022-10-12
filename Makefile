CXX?=clang++
SRC=main.cpp JobsQueue.cpp Worker.cpp Scheduler.cpp

run: build
	time ./a.out

build:
	${CXX} -O3 -std=c++17 -Wno-c++98-compat -o a.out ${SRC}
