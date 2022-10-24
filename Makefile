CXX?=clang++
SRC=src/JobsQueue.cpp src/Worker.cpp src/JobSystem.cpp

run: build
	time ./a.out

build:
	@${CXX} -O3 -std=c++17 -Wno-c++98-compat -o a.out ${SRC} src/main.cpp

run-wasm: wasm
	emrun index.html --browser firefox

wasm:
	@emcc -Os -std=c++17 -s PROXY_TO_PTHREAD -s ASSERTIONS=1 -s PTHREAD_POOL_SIZE=16 -s ALLOW_MEMORY_GROWTH=1  -pthread -o index.html ${SRC} src/wasm-main.cpp 
