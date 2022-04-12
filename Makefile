all: numa_test

clean:
	rm -f numa_test

numa_test: numa_test.cc
	@# not -std=c++11 for compatibility with GCC 4.4.7
	g++ -o numa_test -std=c++0x -O2 -g numa_test.cc -lnuma -lpmem -lpthread
