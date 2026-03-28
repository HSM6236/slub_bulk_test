# slub_bulk_test
Workflow

1.Build slub_bulk_bench as a kernel module, and copy the generated slub_bulk_bench.ko into the same directory as the test scripts.

2.Run:
./test_before.sh
./test_after.sh
to generate the benchmark log files.

3.Run:
./summarize_bench.py ./
to summarize the benchmark results.
