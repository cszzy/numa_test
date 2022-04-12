#include <thread>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <libpmem.h>
#include <numa.h>
#include <atomic>
#include <mutex>

#define SIZE (100UL << 20) // 100MB
#define ITERATIONS 8
#define IO_SIZE 64
#define THREADS 1
#define CACHELINE_SIZE 64

bool write_test = false;
bool pmem_test = false;
char work_space[128];
thread_local float thread_elapse = 0.0;
bool remote_test = false;

float total_latency = 0.0;
std::mutex lock;

void thread_func(void *p_ptr, void *v_ptr) {
    if (numa_run_on_node(0) != 0) {
		printf("failed to run on node: %s\n", strerror(errno));
		return;
	}

    volatile uint64_t *p_data = (volatile uint64_t *)p_ptr;
    volatile uint64_t *v_data = (volatile uint64_t *)v_ptr;

    auto start = std::chrono::high_resolution_clock::now();

    if(write_test) {
        /* Use 8-byte stores for each IO and flush the CPU cache */
        for (int i = 0; i < SIZE / IO_SIZE; ++i) {
            for (int j = 0; j < IO_SIZE >> 3; ++j) {
                *(p_data + j + (i * (IO_SIZE >> 3))) = *(v_data + j);
            }
            if(pmem_test)
                pmem_persist((void *)p_data, IO_SIZE);
        }
    } else {
        /* Use 8-byte loads for each IO */
        for (int i = 0; i < SIZE / IO_SIZE; ++i) {
            for (int j = 0; j < IO_SIZE >> 3; ++j) {
                *(p_data + j + (i * (IO_SIZE >> 3)));
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    lock.lock();
    total_latency += std::chrono::duration<float>(end - start).count() * 1000000000 / (SIZE / IO_SIZE);
    lock.unlock();
}

int main(int argc, char **argv) {
    std::vector<std::thread*> th_arr(THREADS);
    char ch;
    uint64_t total_cycles = 0;

    volatile uint64_t *p_data = NULL;
    volatile uint64_t *v_data = NULL;
    
    while ((ch = getopt(argc, argv, "t::w::pr")) != -1) {
		switch (ch) {
        case 't': // is write test
            write_test = atoi(optarg);
            break;
        case 'w': // workspace
            strcpy(work_space, optarg);
            break;
		case 'p': // pmem test
			pmem_test = true;
			break;
        case 'r': // is remote test?
            remote_test = true;
            break;
		default:
			printf("invalid param\n");
			exit(1);
		}
	}

    void *p_ptr = NULL;
    int numanodes = numa_max_node() + 1;
    printf("numa node nums : %d\n", numanodes);
    
    if(pmem_test) {
        p_ptr = pmem_map_file(work_space, SIZE, PMEM_FILE_CREATE, 0666, NULL, NULL);
        if (p_ptr == NULL) {
            printf("ERROR: Failed to map file %s\n", work_space);
            return 1;
        }
    } else {
        p_ptr = numa_alloc_onnode(SIZE, remote_test ? 1 :0);
        if (p_ptr == NULL) {
			printf("ERROR: failed to allocate memory\n");
			return 1;
		}
    }

    void *v_ptr;
    posix_memalign(&v_ptr, CACHELINE_SIZE, IO_SIZE);

    printf("WorkSpace:%s, Testing %s Latency\n", work_space, write_test ? "Write" : "Read");
    printf("IO Size: %d\n", IO_SIZE);

    for(int k = 0; k < ITERATIONS; ++k) {
        p_data = (volatile uint64_t *)p_ptr;
        v_data = (volatile uint64_t *)v_ptr;

        /* First, make all cache lines in PMEM area dirty */
        for (int i = 0; i < SIZE / IO_SIZE; ++i) {
            for (int j = 0; j < IO_SIZE >> 3; ++j) {
                *(p_data + j) = *(v_data + j);
            }
            p_data += (IO_SIZE >> 3);
        }

        /* Flush all cache lines to clear CPU cache */
        p_data = (volatile uint64_t *)p_ptr;
        pmem_persist((void *)p_data, SIZE);

        for(int i = 0; i < THREADS; ++i) {
            th_arr[i] = new std::thread(thread_func, p_ptr, v_ptr);
        }

        for(auto t : th_arr) {
            t->join();
            delete t;
        }
    }

    printf("%s %s, latency : %.4fns\n", pmem_test ? "pmem" : "dram", 
        write_test ? "write" : "read", total_latency / (THREADS * ITERATIONS));

    return 0;
}