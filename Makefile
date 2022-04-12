all: numa_latency_test

clean:
	rm -f numa_latency_test

numa_latency_test: numa_latency_test.cc
	@# not -std=c++11 for compatibility with GCC 4.4.7
	g++ -o numa_latency_test -std=c++0x -O2 -g numa_latency_test.cc -lnuma -lpmem -lpthread
