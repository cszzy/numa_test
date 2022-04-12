# Disable hw prefetching
disable_prefetch/intel-prefetch-disable > /dev/null 2>&1
sleep 2

##local dram test
# rm -f /dev/shm/test
# echo "-------------local dram test-------------"
# ./numa_latency_test -t0 -w/dev/shm/test
# sleep 2
# ./numa_latency_test -t1 -w/dev/shm/test
# sleep 2


##remote dram test
# echo "-------------remote dram test-------------"
# rm -f /dev/shm/test
# ./numa_latency_test -t0 -w/dev/shm/test -r
# sleep 2
# ./numa_latency_test -t1 -w/dev/shm/test -r
# sleep 2

# # local pmem test
# echo "-------------local pmem test-------------"
# rm -f /mnt/AEP0/test
# ./numa_latency_test -t0 -p -w/mnt/AEP0/test
# sleep 2
# ./numa_latency_test -t1 -p -w/mnt/AEP0/test
# sleep 2

# remote pmem test
# echo "-------------remote pmem test-------------"
rm -f /mnt/AEP1/test
./numa_latency_test -t0 -p -w/mnt/AEP1/test
sleep 2
./numa_latency_test -t1 -p -w/mnt/AEP1/test
sleep 2

# Enable hw prefetching
disable_prefetch/intel-prefetch-disable -e > /dev/null 2>&1
sleep 2