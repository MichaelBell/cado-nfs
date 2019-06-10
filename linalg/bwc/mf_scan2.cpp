#include "cado.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <pthread.h>
#include <hwloc.h>
#include <mutex>
#include <atomic>
#include <omp.h>
#include "utils.h"
#include "ringbuf.h"

void mf_scan2_decl_usage(cxx_param_list & pl)
{
    param_list_usage_header(pl,
            "This program make one reading pass through a binary matrix, and produces\n"
            "the companion .rw and .cw files.\n"
            "Typical usage:\n"
            "\tmf_scan [<matrix file name> | options...]\n"
            );
    param_list_decl_usage(pl, "withcoeffs", "Handle DLP matrix, with coefficients\n");
    param_list_decl_usage(pl, "mfile", "Input matrix name (free form also accepted)");
    param_list_decl_usage(pl, "rwfile", "Name of the row weight file to write (defaults to auto-determine from matrix name)");
    param_list_decl_usage(pl, "cwfile", "Name of the col weight file to write (defaults to auto-determine from matrix name)");
    param_list_decl_usage(pl, "threads", "Number of threads to use (defaults to auto detect\n");
    param_list_decl_usage(pl, "io-memory", "Amount of RAM to use for rolling buffer memory (in GB, floating point allowed)");
    param_list_decl_usage(pl, "thread-private-count", "Number of columns for which a thread-private zone is used");
    param_list_decl_usage(pl, "thread-read-window", "Chunk size for consumer thread reads from rolling buffer");
    param_list_decl_usage(pl, "thread-write-window", "Chunk size for producer thread writes to rolling buffer");
}

size_t thread_private_count = 1UL << 20;

/* These two are in bytes as far as the default value is concerned, but
 * they're converted to number of uint32_t's when the program runs.
 */
size_t thread_read_window = 1UL << 13;
size_t thread_write_window = 1UL << 10;

class reporter {
    std::atomic<size_t> produced;
    std::atomic<size_t> consumed;
    double t0;
    double last_report;
    std::mutex m;
    void report(bool force = false) {
        std::lock_guard<std::mutex> dummy(m);
        double tt = wct_seconds();
        if (!force && tt < last_report + 1) return;
        char buf1[20];
        char buf2[20];
        printf("read %s, parsed %s, in %.1f s\n",
                size_disp(produced, buf1),
                size_disp(consumed.load(), buf2),
                (last_report = tt) - t0);
    }
    public:
    struct consumer_data {
        double last;
        size_t s = 0;
        consumer_data() : last(wct_seconds()) {}
    };
    /* tells wheter this consumer has reason to schedule a new report */
    void consumer_report(consumer_data & D, size_t s, bool force = false) {
        double tt = wct_seconds();
        if (!force && tt < D.last + 0.9) {
            D.s += s;
            return;
        }
        consumed += D.s + s;
        D.last = tt;
        D.s = 0;
        report();
    }
    void producer_report(size_t s, bool force = false) {
        produced += s;
        report(force);
    }
    void reset() { t0 = last_report = wct_seconds(); }
};

reporter report;

inline int get_segment_index(uint32_t c)
{
    return 64 - cado_clz64((uint64_t) c);
}
inline uint32_t get_segment_offset(int t) { return 1UL << (t-1); }
inline uint32_t get_segment_size(int t) { return 1UL << (t-1); }

struct segment {
    std::atomic<uint32_t> * data;
    static size_t segment_size(int t) { return get_segment_size(t); }
    segment(int t) {
        data = new std::atomic<uint32_t>[segment_size(t)];
    }
    ~segment() {
        delete[] data;
    }
    segment(segment const&) = delete;
    segment& operator=(segment const&) = delete;
    segment(segment &&) = delete;
    segment& operator=(segment &&) = delete;
    void incr(uint32_t c) {
        data[c]++;
    }
};

/* It might seem somewhat overkill to use std::atomic here. Some of the
 * associated fencing is quite probably overkill on x86. But I'm not too
 * sure.
 *
 * I've added some loose memory_order constraints below, that seem to
 * improve performance. But I'm on thin ice, I'm not sure of what I'm
 * doing.
 *
 * (the reassuring thing is that I _think_ that the worst that can happen
 * is a seg fault, which would be loud enough, and therefore fine).
 *
 * Some pointers:
 *
 * https://bartoszmilewski.com/2008/12/01/c-atomics-and-memory-ordering/
 * https://bartoszmilewski.com/2008/12/23/the-inscrutable-c-memory-model/
 * http://www.cplusplus.com/reference/atomic/memory_order/
 */
std::atomic<segment *> segments[64];
std::mutex segment_mutexes[64];

ringbuf R;

struct parser_thread {
    std::vector<uint32_t> cw;
    uint32_t colmax=0;
    parser_thread() : cw(thread_private_count, 0) {};
    void loop() {
        reporter::consumer_data D;
        uint32_t buffer[thread_read_window];
        for(size_t s ; (s = ringbuf_get(R, (char*) buffer, sizeof(buffer))) != 0 ; ) {
            report.consumer_report(D, s);
            uint32_t * v = (uint32_t *) buffer;
            ASSERT_ALWAYS(s % sizeof(uint32_t) == 0);
            size_t sv = s / sizeof(uint32_t);
            for(size_t i = 0 ; i < sv ; i++) {
                uint32_t c = v[i];
                colmax = MAX(colmax, c+1);
                if (c < thread_private_count) {
                    cw[c]++;
                } else {
                    /* Get the bit size */
                    unsigned int t = get_segment_index(c);
                    uint32_t c1 = c-get_segment_offset(t);
                    ASSERT_ALWAYS(c1 < get_segment_size(t));
                    segment * x;
                    {
                        x = segments[t].load(std::memory_order_relaxed);
                        if (!x) {
                            std::lock_guard<std::mutex> dummy(segment_mutexes[t]);
                            x = segments[t].load(std::memory_order_relaxed);
                            if (!x)
                                segments[t].store(x = new segment(t), std::memory_order_relaxed);
                        }
                    }
                    x->incr(c1);
                }
            }
        }
        report.consumer_report(D, 0, true);
    }
};

int main(int argc, char * argv[])
{
    char * argv0 = argv[0];

    cxx_param_list pl;
    const char * rwfile = NULL;
    const char * cwfile = NULL;
    const char * mfile = NULL;

    unsigned int wild =  0;
    int withcoeffs = 0;

    argv++,argc--;

    mf_scan2_decl_usage(pl);

    param_list_configure_switch(pl, "--withcoeffs", &withcoeffs);

    for(;argc;) {
        if (param_list_update_cmdline(pl, &argc, &argv)) continue;
        if (argv[0][0] != '-' && wild == 0) {
            mfile = argv[0];
            wild++;
            argv++,argc--;
            continue;
        }
        fprintf(stderr, "unknown option %s\n", argv[0]);
        param_list_print_usage(pl, argv0, stderr);
        exit(EXIT_FAILURE);
    }

    param_list_parse_size_t(pl, "thread-private-count", &thread_private_count);
    param_list_parse_size_t(pl, "thread-read-window", &thread_read_window);
    param_list_parse_size_t(pl, "thread-write-window", &thread_write_window);
    ASSERT_ALWAYS(thread_read_window % sizeof(uint32_t) == 0);
    ASSERT_ALWAYS(thread_write_window % sizeof(uint32_t) == 0);
    thread_read_window  /= sizeof(uint32_t);
    thread_write_window /= sizeof(uint32_t);

    const char * tmp;
    if ((tmp = param_list_lookup_string(pl, "mfile")) != NULL) {
        mfile = tmp;
    }
    if ((tmp = param_list_lookup_string(pl, "rwfile")) != NULL) {
        rwfile = tmp;
    }
    if ((tmp = param_list_lookup_string(pl, "cwfile")) != NULL) {
        cwfile = tmp;
    }

    if (!mfile) {
        param_list_print_usage(pl, argv0, stderr);
        exit(EXIT_FAILURE);
    }

    if (strlen(mfile) < 4 || strcmp(mfile + strlen(mfile) - 4, ".bin") != 0) {
        fprintf(stderr, "Warning: matrix file name should end in .bin\n");
    }

    if (!rwfile) {
        char * leakme;
        rwfile = leakme = derived_filename(mfile, "rw", ".bin");
    }

    if (!cwfile) {
        char * leakme;
        cwfile = leakme = derived_filename(mfile, "cw", ".bin");
    }

    if (withcoeffs) abort();    // not implemented yet

    /* Detect hardware */
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    int depth = hwloc_topology_get_depth(topology);
    int npu = hwloc_get_nbobjs_by_depth(topology, depth-1);
    hwloc_obj_t root = hwloc_get_root_obj(topology);
#if HWLOC_API_VERSION < 0x020000
    uint64_t ram = root->memory.total_memory;
#else
    uint64_t ram = root->total_memory;
#endif

    for(uint64_t x = ram >> 4; x ; x >>= 1) ram |= x;
    ram = ram + 1;

    int threads = npu;
    size_t ringbuf_size = ram / 4;

    param_list_parse_int(pl, "threads", &threads);
    {
        double r;
        if (param_list_parse_double(pl, "io-memory", &r)) {
            ringbuf_size = r * (1UL << 30);
        }
    }

    ringbuf_init(R, ringbuf_size);

    /* Start with the input */

    FILE * f_in = fopen(mfile, "rb");
    if (f_in == NULL) { perror(mfile); exit(EXIT_FAILURE); }
    FILE * f_rw = fopen(rwfile, "wb");
    if (f_rw == NULL) { perror(rwfile); exit(EXIT_FAILURE); }
    FILE * f_cw = fopen(cwfile, "wb");
    if (f_cw == NULL) { perror(cwfile); exit(EXIT_FAILURE); }

    ASSERT_ALWAYS(threads >= 2);

    int consumers = threads-1;
    parser_thread T[consumers];
    
    report.reset();

    omp_set_num_threads(threads);
#pragma omp parallel
    {
        int t = omp_get_thread_num();
        if (t == 0) {
            uint32_t buf[thread_write_window];
            for( ; ; ) {
                uint32_t row_length;
                int rc = fread32_little(&row_length, 1, f_in);
                if (rc != 1)
                    break;
                rc = fwrite32_little(&row_length, 1, f_rw);
                ASSERT_ALWAYS(rc == 1);
                for( ; row_length ; ) {
                    int s = MIN(row_length, thread_write_window);
                    int k = fread32_little(buf, s, f_in);
                    ASSERT_ALWAYS(k == s);
                    ringbuf_put(R, (char *) buf, s * sizeof(uint32_t));
                    report.producer_report(s * sizeof(uint32_t));
                    row_length -= s;
                }
            }
            ringbuf_mark_done(R);
        } else {
            T[t-1].loop();
        }
    }
#pragma omp barrier
    report.producer_report(0, true);
    for(int i = 1 ; i < consumers ; i++) {
        for(size_t j = 0 ; j < thread_private_count ; j++)
            T[0].cw[j] += T[i].cw[j];
        T[0].colmax = MAX(T[0].colmax, T[i].colmax);
    }
    uint32_t colmax = T[0].colmax;
    uint32_t c = 0;
    for( ; c < thread_private_count && c < colmax ; c++) {
        int rc = fwrite32_little(&T[0].cw[c], 1, f_cw);
        ASSERT_ALWAYS(rc == 1);
    }
    for( ; c < colmax ; ) {
        unsigned int t = get_segment_index(c);
        std::lock_guard<std::mutex> dummy(segment_mutexes[t]);
        uint32_t c1 = c-get_segment_offset(t);
        uint32_t max1 = MIN(colmax-get_segment_offset(t), get_segment_size(t));
        uint32_t n1 = max1 - c1;
        segment * x = segments[t];
        if (!x) x = new segment(t);
        int rc = fwrite32_little((uint32_t*) &x->data[c1], n1, f_cw);
        ASSERT_ALWAYS(rc == (int) n1);
        c += n1;
        delete x;
    }

    ringbuf_clear(R);

    hwloc_topology_destroy(topology);

    fclose(f_rw);
    fclose(f_cw);
    fclose(f_in);
}
