// mm/slub_bulk_bench.c

/*
 * slub_bulk_bench.c
 *
 * A Linux kernel module for benchmarking SLUB bulk allocation performance.
 *
 * This module creates a dedicated kmem_cache with a configurable object size
 * and measures the latency of repeated kmem_cache_alloc_bulk() operations.
 * The benchmark is intended to evaluate the allocation-side performance of
 * the SLUB allocator under a fixed total allocation size.
 *
 * Main features:
 * - Creates an isolated slab cache for the target object size
 * - Optionally pins the benchmark task to a specific CPU
 * - Derives the bulk allocation batch size from the slab layout, unless
 *   explicitly overridden by the user
 * - Performs several warm-up rounds before entering the measured phase
 * - Measures only the allocation window in the main benchmark
 * - Reports timing statistics through kernel log messages
 *
 * Measured output includes:
 * - object size
 * - batch size
 * - slab order
 * - number of bulk operations
 * - total allocated memory
 * - total allocation time
 * - average time per bulk allocation
 * - average time per object allocation
 *
 * Module parameters:
 * - obj_size:
 *     Size of each object in bytes
 * - total_mb:
 *     Total memory volume to allocate during the measured phase, in MB
 * - warmups:
 *     Number of warm-up rounds executed before measurement
 * - cpu:
 *     CPU core to which the benchmark task should be pinned
 * - batch_override:
 *     User-specified batch size; if not set, the number of objects per slab
 *     is used as the batch size
 *
 * Benchmark flow:
 * 1. Pin current task to the selected CPU (if possible)
 * 2. Create a private slab cache
 * 3. Compute batch size and number of bulk allocation rounds
 * 4. Allocate pointer storage for returned objects
 * 5. Run warm-up allocation/free rounds
 * 6. Measure the time of bulk allocations only
 * 7. Free all allocated objects after measurement
 * 8. Destroy the slab cache and release temporary memory
 *
 * This module is useful for studying allocator behavior, slab layout effects,
 * and the performance impact of bulk allocation under different object sizes.
 */
 
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include "slab.h"

MODULE_LICENSE("GPL");

static unsigned int obj_size = 128;
module_param(obj_size, uint, 0444);
static unsigned int total_mb = 256;
module_param(total_mb, uint, 0444);
static unsigned int warmups = 3;
module_param(warmups, uint, 0444);
static unsigned int cpu = 0;
module_param(cpu, uint, 0444);
static unsigned int batch_override;
module_param(batch_override, uint, 0444);


/* get slab objects */
static inline unsigned int oo_objects(struct kmem_cache_order_objects oo)
{
    return oo.x & ((1 << 16) - 1);
}

/* get slab order */
static inline unsigned int oo_order(struct kmem_cache_order_objects oo)
{
    return oo.x >> 16;
}

static int __init slub_bulk_bench_init(void)
{
    struct kmem_cache *s;
    unsigned int batch;
    size_t bulks, nr_ptrs, i;
    void **objs;
    u64 start, end;
    int ret;

    ret = set_cpus_allowed_ptr(current, cpumask_of(cpu));
    if (ret)
        pr_warn("slub_bulk_bench: pin cpu%u failed: %d\n", cpu, ret);

    s = kmem_cache_create("slub_whole_bench", obj_size, 0, SLAB_NO_MERGE, NULL);
    if (!s)
        return -ENOMEM;

    batch = batch_override ? batch_override : oo_objects(s->oo);
    bulks = max_t(size_t, 1, ((size_t)total_mb << 20) / (batch * obj_size));
    nr_ptrs = bulks * batch;

    objs = vzalloc(array_size(nr_ptrs, sizeof(*objs)));
    if (!objs) {
        kmem_cache_destroy(s);
        return -ENOMEM;
    }

    /* Warmup */
    while (warmups--) {
        for (i = 0; i < bulks; i++) {
            if (!kmem_cache_alloc_bulk(s, GFP_KERNEL, batch, &objs[i * batch])) {
                pr_err("slub_bulk_bench: warmup alloc failed at bulk %zu\n", i);
                goto out;
            }
        }
        for (i = 0; i < bulks; i++)
            kmem_cache_free_bulk(s, batch, &objs[i * batch]);
    }

    /* Main measured window */
    start = ktime_get_ns();
    for (i = 0; i < bulks; i++) {
        if (!kmem_cache_alloc_bulk(s, GFP_KERNEL, batch, &objs[i * batch])) {
            pr_err("slub_bulk_bench: alloc failed at bulk %zu\n", i);
            goto out;
        }
    }
    end = ktime_get_ns();

    pr_info("slub_bulk_bench: obj=%u batch=%u order=%u bulks=%zu total_mb=%u "
            "alloc_ns=%llu ns_per_bulk=%llu ns_per_obj=%llu\n",
            obj_size, batch, oo_order(s->oo), bulks, total_mb,
            end - start, div64_u64(end - start, bulks),
            div64_u64(end - start, bulks * batch));

    /* Free outside measured window */
    for (i = 0; i < bulks; i++)
        kmem_cache_free_bulk(s, batch, &objs[i * batch]);

out:
    kmem_cache_destroy(s);
    vfree(objs);
    return 0;
}

static void __exit slub_bulk_bench_exit(void) {}

module_init(slub_bulk_bench_init);
module_exit(slub_bulk_bench_exit);