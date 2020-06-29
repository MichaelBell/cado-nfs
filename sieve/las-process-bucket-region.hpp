#ifndef LAS_PROCESS_BUCKET_REGION_HPP_
#define LAS_PROCESS_BUCKET_REGION_HPP_

#include <cstdint>
#include <memory>
#include <array>
#include "threadpool.hpp"
#include "las-threads-work-data.hpp"
#include "las-auxiliary-data.hpp"
#include "las-report-stats.hpp"

/* process_many_bucket_regions is found in las.cpp, currently */

extern void process_many_bucket_regions(nfs_work & ws, std::shared_ptr<nfs_work_cofac> wc_p, std::shared_ptr<nfs_aux> aux_p, thread_pool & pool, int first_region0_index, where_am_I & w);

/* {{{ process_one_bucket_region */

struct process_bucket_region_spawn {
    nfs_work & ws;
    std::shared_ptr<nfs_work_cofac> wc_p;
    std::shared_ptr<nfs_aux> aux_p;
    where_am_I w_saved;
    
    /* These two indices are set from within process_many_bucket_regions,
     * prior to spawning all threads.
     *
     * first_region0_index is the bucket index of the first region for
     * which we filled the buckets.
     *
     * already done is, relative to first_region0_index, the index of the
     * first region for which the small sieve start position are
     * available in ssdpos_many.
     *
     * The i-th process_bucket_region task thus handles the bucket region
     * of index first_region0_index + already_done + i
     */
    int first_region0_index;
    int already_done;

    process_bucket_region_spawn(
            nfs_work & ws,
            std::shared_ptr<nfs_work_cofac> wc_p,
            std::shared_ptr<nfs_aux> aux_p,
            where_am_I const & w)
    : ws(ws), wc_p(wc_p), aux_p(aux_p), w_saved(w) {}

    void operator()(worker_thread * worker, int id);
};

/*}}}*/

#endif	/* LAS_PROCESS_BUCKET_REGION_HPP_ */
