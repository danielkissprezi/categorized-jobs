CXX?=clang++
SRC=src/JobsQueue.cpp src/Worker.cpp src/JobSystem.cpp

run: build
	time ./a.out

build:
	${CXX} -O3 -std=c++17 -Wno-c++98-compat -o a.out ${SRC} src/main.cpp

wasm:
	emcc -O3 -std=c++17 ${SRC}
