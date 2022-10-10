CXX?=clang++
SRC=main.cpp JobsQueue.cpp Worker.cpp Scheduler.cpp

run: build
	./a.out

build:
	${CXX} -std=c++17 -Wall -Weverything -Wpedantic -Wno-c++98-compat -o a.out ${SRC}
	chmod +x a.out
