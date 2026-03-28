#!/bin/bash

# ============================================
# SLUB Bulk Allocation Benchmark Script
# ============================================
# Description:
#   This script evaluates the performance of the SLUB allocator
#   under different object sizes (16 to 512 bytes).
#   Each configuration is tested for 20 iterations.
#
# Procedure:
#   1. Clear kernel message buffer using 'dmesg -c'
#   2. Load the kernel module (slub_bulk_bench.ko) with parameters:
#        - obj_size: object size in bytes
#        - total_mb: total allocated memory in MB
#        - cpu: CPU core binding
#   3. Extract benchmark results from kernel logs
#   4. Unload the kernel module
#
# Output:
#   Results are appended to log files (after-XX.log),
#   where XX represents the object size.
#
# Note:
#   Root privileges are required to execute this script.
# ============================================

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=16 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-16.log
    rmmod slub_bulk_bench
done

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=32 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-32.log
    rmmod slub_bulk_bench
done

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=64 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-64.log
    rmmod slub_bulk_bench
done

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=128 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-128.log
    rmmod slub_bulk_bench
done

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=256 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-256.log
    rmmod slub_bulk_bench
done

for i in $(seq 1 20); do
    dmesg -c
    insmod ./slub_bulk_bench.ko obj_size=512 total_mb=256 cpu=0
    dmesg | grep 'slub_bulk_bench:' | tail -n1 >> after-512.log
    rmmod slub_bulk_bench
done