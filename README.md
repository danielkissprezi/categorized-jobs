# Categorized job system example

Example implementation of a job system using both categories and priorities for jobs.

[api.h](api.h) contains an example API.

[main.cpp](main.cpp) has, a somewhat contrived, example of using the API.

## Dependencies

- C++17 compatible compiler
- Make

## Build

```sh
make build
```

## Usage

The example program will schedule jobs that sleep for a bit, imitating long running, but compute in-intensive tasks.
The first command line argument is used to determine the target number of worker threads.

```
./a 1 # start with 1 shared worker
./a 7 # start with 7 workers
./a   # start with as many workers as hyper-threads available in the system

