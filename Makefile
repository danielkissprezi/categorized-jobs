CXX?=clang++
SRC=main.cpp JobsQueue.cpp Worker.cpp JobSystem.cpp

run: build
	time ./a.out

build:
	${CXX} -std=c++17 -Wno-c++98-compat -o a.out ${SRC}
