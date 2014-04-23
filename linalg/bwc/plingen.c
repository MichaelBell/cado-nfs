/* Copyright (C) 1999--2007 Emmanuel Thom'e --- see LICENSE file */
#include "cado.h"

#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>


#include <assert.h>

#include "portability.h"
#include "macros.h"
#include "utils.h"
#include "abase.h"
#include "memusage.h"
#include "lingen-matpoly.h"

#include "lingen-bigmatpoly.h"

#ifdef HAVE_MPIR
#include "lingen-bigmatpoly-ft.h"
#endif

#include "bw-common.h"		/* Handy. Allows Using global functions
                                 * for recovering parameters */
#include "bw-common-mpi.h"
#include "filenames.h"
#include "plingen.h"
#include "plingen-tuning.h"
#include "logline.h"

/* Call tree for methods within this program:
 *
 * Two separate entry points.
 *
 * bw_biglingen_collective        when need to go collective
 *     |<--> bw_biglingen_recursive , loops to bw_biglingen_collective
 *     \->bw_lingen_single               when it makes sense to do this locally again
 * bw_lingen_single                      when computation can be done locally
 *    |<->bw_lingen_recursive
 *    |->bw_lingen_basecase
 *
 */

#define MPI_MY_SIZE_T   MPI_UNSIGNED_LONG

static unsigned int display_threshold = 10;
static int with_timings = 0;

/* This is an indication of the number of bytes we read at a time for A
 * (input) and F (output) */
static unsigned int io_block_size = 1 << 20;

/* If non-zero, then reading from A is actually replaced by reading from
 * a random generator */
static unsigned int random_input_length = 0;

gmp_randstate_t rstate;

#ifdef HAVE_MPIR
static int caching = 1;
static unsigned int caching_threshold = 10;
#else
static int caching = 0;
#endif

static const char * checkpoint_directory;
static unsigned int checkpoint_threshold = 100;
static int save_gathered_checkpoints = 0;


struct bmstatus_s {
    dims d[1];
    unsigned int t;
    int * lucky;

    double t_basecase;
    double t_mp;
    double t_mul;
    double t_cp_io;

    unsigned int lingen_threshold;
    unsigned int lingen_mpi_threshold;
    int mpi_dims[2]; /* mpi_dims[0] = mpi[0] * thr[0] */
    MPI_Comm com[3]; /* [0]: MPI_COMM_WORLD, reordered.
                        [1]: row-wise
                        [2]: column-wise */
};
typedef struct bmstatus_s bmstatus[1];
typedef struct bmstatus_s *bmstatus_ptr;


/*{{{ statistics for projected running time. One global struct. */
struct tree_level_stats_s {
    unsigned int ncalled;
    unsigned int min_inputsize;
    unsigned int max_inputsize;
    unsigned int sum_inputsize;
    double spent;
    double projected_time;;
    const char * func;
    int warned;
    double last_printed_projected_time;
};
typedef struct tree_level_stats_s tree_level_stats[1];
typedef struct tree_level_stats_s *tree_level_stats_ptr;

struct tree_running_stats_s {
    const char * func;
    unsigned int inputsize;
    double time_self;
    double time_children;
};

typedef struct tree_running_stats_s tree_running_stats[1];
typedef struct tree_running_stats_s *tree_running_stats_ptr;



struct {
    struct tree_level_stats_s stats_stack[64];
    struct tree_running_stats_s stats_curstack[64];
    unsigned int depth;

    unsigned tree_total_breadth;
    double last_print_time;
} stats[1];

void tree_stats_print(unsigned int level)
{
    double sum = 0;
    int nstars=0;
    int nok=0;
    double firstok = 0;
    double complement = 0;
    for(unsigned int k = 1 ; k < 64 ; k++) {
        tree_level_stats_ptr u = stats->stats_stack + k;

        if (k > level && !u->ncalled)
            break;

        if (!u->ncalled) {
            printf("%u *\n", k-1);
            nstars++;
            continue;
        }
        u->projected_time = stats->tree_total_breadth * u->spent / u->sum_inputsize;
        if (nstars && nok == 0) {
            firstok = u->projected_time;
        } else if (nstars && nok == 1) {
            double ratio = firstok / u->projected_time;
            complement = ratio * firstok * (pow(ratio, nstars) - 1) / (ratio - 1);
        }
        nok++;

        sum += u->projected_time;
        printf("%u [%u-%u, %s] %u/%u %.1f -> %.1f (total: %.1f)\n",
                k-1, u->min_inputsize, u->max_inputsize,
                u->func,
                u->ncalled, 1u << (k-1),
                u->spent / u->ncalled, u->projected_time, sum);
        u->last_printed_projected_time = u->projected_time;
    }
    if (nstars && nok >= 2) {
        printf("expected time for levels 0-%u: %.1f (total: %.1f)\n",
                nstars-1, complement, sum + complement);
    }
}

void tree_stats_enter(const char * func, unsigned int inputsize)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank) { ++stats->depth; return; }
    tree_running_stats_ptr s = stats->stats_curstack + ++stats->depth;
    memset(s, 0, sizeof(struct tree_running_stats_s));
    s->time_self -= wct_seconds();
    s->func = func;
    s->inputsize = inputsize;
}

int tree_stats_leave(int rc)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank) { --stats->depth; return rc; }
    double now = wct_seconds();
    tree_running_stats_ptr s = stats->stats_curstack + stats->depth;
    s->time_self += now;
    double tt = s->time_self;
    s->time_self -= s->time_children;
    unsigned int level = stats->depth;

    tree_level_stats_ptr t = stats->stats_stack + level;
    if (t->ncalled == 0) {
        t->min_inputsize = s->inputsize;
        t->max_inputsize =s-> inputsize;
        t->func = s->func;
    }
    t->ncalled++;
    if (s->inputsize < t->min_inputsize)
        t->min_inputsize = s->inputsize;
    if (s->inputsize > t->max_inputsize)
        t->max_inputsize = s->inputsize;
    t->sum_inputsize += s->inputsize;
    t->spent += s->time_self;
    if (t->func != s->func && !t->warned) {
        fprintf(stderr, "Warning: at depth level %d (input size %u to %u), function called is not constant: %s or %s\n",
                level, t->min_inputsize, t->max_inputsize, t->func, s->func);
        t->warned=1;
    }

    stats->stats_curstack[stats->depth - 1].time_children += tt;
    --stats->depth;

    /* Is it any useful to print something new ? */

    if (now < stats->last_print_time + 2)
        return rc;

    int needprint = 0;
    for(unsigned int k = 0 ; !needprint && k < 64 ; k++) {
        tree_level_stats_ptr u = stats->stats_stack + k;

        if (k > level && !u->ncalled)
            break;

        if (!u->ncalled)
            continue;

        /* compute the projected time */
        u->projected_time = stats->tree_total_breadth * u->spent / u->sum_inputsize;
        if (u->projected_time < 0.98 * u->last_printed_projected_time)
            needprint = 1;
        else if (u->projected_time > 1.02 * u->last_printed_projected_time)
            needprint = 1;
    }

    if (!needprint)
        return rc;

    stats->last_print_time = now;

    tree_stats_print(level);

    return rc;
}

/*}}}*/

/*{{{ basecase */

/* {{{ col sorting */
/* We sort only with respect to the global delta[] parameter. As it turns
 * out, we also access the column index in the same aray and sort with
 * respect to it, but this is only cosmetic.
 *
 * Note that unlike what is asserted in other coiped of the code, sorting
 * w.r.t. the local delta[] value is completely useless. Code which
 * relies on this should be fixed.
 */

typedef int (*sortfunc_t) (const void*, const void*);

static int col_cmp(const int x[2], const int y[2])
{
    for(int i = 0 ; i < 2 ; i++) {
        int d = x[i] - y[i];
        if (d) return d;
    }
    return 0;
}

/* }}} */

static inline unsigned int expected_pi_length(dims * d, unsigned int len)/*{{{*/
{
    /* The idea is that we want something which may account for something
     * exceptional, bounded by probability 2^-64. This corresponds to a
     * column in e (matrix of size m*b) to be spontaneously equal to
     * zero. This happens with probability (#K)^-m.
     * The solution to
     * (#K)^(-m*x) > 2^-64
     * is m*x*log_2(#K) < 64
     *
     * We thus need to get an idea of the value of log_2(#K).
     *
     * (Note that we know that #K^abgroupsize(ab) < 2^64, but that bound
     * might be very gross).
     *
     * The same esitmate can be used to appreciate what we mean by
     * ``luck'' in the end. If a column happens to be zero more than
     * expected_pi_length(d,0) times in a row, then the cause must be
     * more than sheer luck, and we use it to detect generating rows.
     */
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    abdst_field ab MAYBE_UNUSED = d->ab;
    unsigned int res = iceildiv(len * m, b);
    mpz_t p;
    mpz_init(p);
    abfield_characteristic(ab, p);
    unsigned int l = mpz_sizeinbase(p, 2);
    l *= abfield_degree(ab);    /* roughtly log_2(#K) */
    mpz_clear(p);
    // unsigned int safety = iceildiv(abgroupsize(ab), m * sizeof(abelt));
    unsigned int safety = iceildiv(64, m * l);
    return res + safety;
}/*}}}*/

/* This destructively cancels the first len coefficients of E, and
 * computes the appropriate matrix pi which achieves this. The
 * elimination is done in accordance with the nominal degrees found in
 * delta.
 *
 * The result is expected to have degree ceil(len*m/b) coefficients, so
 * that E*pi is divisible by X^len.
 */

static int bw_lingen_basecase(bmstatus_ptr bm, matpoly pi, matpoly E, unsigned int *delta) /*{{{*/
{
    tree_stats_enter(__func__, E->size);
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    abdst_field ab = d->ab;
    ASSERT(E->m == m);
    ASSERT(E->n == b);

    ASSERT(pi->m == 0);
    ASSERT(pi->n == 0);
    ASSERT(pi->alloc == 0);

    /* Allocate something large enough for the result. This will be
     * soon freed anyway. Set it to identity. */
    unsigned int pi_room_base = expected_pi_length(d, E->size);

    matpoly_init(ab, pi, b, b, pi_room_base);
    pi->size = pi_room_base;

    /* Also keep track of the
     * number of coefficients for the columns of pi. Set pi to Id */

    unsigned int *pi_lengths = malloc(b * sizeof(unsigned int));
    for(unsigned int i = 0 ; i < b ; i++) {
        abset_ui(ab, matpoly_coeff(ab, pi, i, i, 0), 1);
        pi_lengths[i] = 1;
    }

    /* This is used below */
    int generator_found = 0;

    /* Keep a list of columns which have been used as pivots at the
     * previous iteration */
    unsigned int * pivots = malloc(m * sizeof(unsigned int));
    int * is_pivot = malloc(b * sizeof(int));
    memset(is_pivot, 0, b * sizeof(int));

    matpoly e;
    matpoly_init(ab, e, m, b, 1);
    e->size = 1;

    for (unsigned int t = 0; t < E->size ; t++, bm->t++) {

        /* {{{ Update the columns of e for degree t. Save computation
         * time by not recomputing those which can easily be derived from
         * previous iteration. Notice that the columns of e are exactly
         * at the physical positions of the corresponding columns of pi.
         */

        abelt_ur tmp_ur;
        abelt_ur_init(ab, &tmp_ur);

        abvec_ur e_ur;
        abvec_ur_init(ab, &e_ur, m*b);
        abvec_ur_set_zero(ab, e_ur, m*b);
        for(unsigned int j = 0 ; j < b ; j++) {
            if (is_pivot[j]) continue;
            /* We should never have to recompute from pi using discarded
             * columns. Discarded columns should always correspond to
             * pivots */
            ASSERT_ALWAYS(bm->lucky[j] >= 0);
            for(unsigned int s = 0 ; s < pi_lengths[j] ; s++) {
                for(unsigned int i = 0 ; i < m ; i++) {
                    for(unsigned int k = 0 ; k < b ; k++) {
                        abmul_ur(ab, tmp_ur,
                                matpoly_coeff(ab, E, i, k, t - s),
                                matpoly_coeff(ab, pi, k, j, s));
                        abelt_ur_add(ab,
                                abvec_ur_coeff_ptr(ab, e_ur, i*b+j),
                                abvec_ur_coeff_ptr_const(ab, e_ur, i*b+j),
                                tmp_ur);
                    }
                }
            }
        }
        unsigned int newluck = 0;
        for(unsigned int j = 0 ; j < b ; j++) {
            if (is_pivot[j]) continue;
            unsigned int nz = 0;
            for(unsigned int i = 0 ; i < m ; i++) {
                abreduce(ab, matpoly_coeff(ab, e, i, j, 0),
                        abvec_ur_coeff_ptr(ab, e_ur, i*b+j));
                nz += abcmp_ui(ab, matpoly_coeff(ab, e, i, j, 0), 0) == 0;
            }
            if (nz == m) {
                newluck++, bm->lucky[j]++;
            } else if (bm->lucky[j] > 0) {
                bm->lucky[j] = 0;
            }
        }
        abelt_ur_clear(ab, &tmp_ur);
        abvec_ur_clear(ab, &e_ur, m*b);
        if (newluck) {
            /* If newluck == n, then we probably have a generator. We add an
             * extra guarantee. newluck==n, for a total of k iterations in a
             * row, means m*n*k coefficients cancelling magically. We would
             * like this to be impossible by mere chance. Thus we want n*k >
             * luck_mini, which can easily be checked */

            int luck_mini = expected_pi_length(d, 0);
            unsigned int luck_sure = 0;

            printf("t=%d, canceled columns:", bm->t);
            for(unsigned int j = 0 ; j < b ; j++) {
                if (bm->lucky[j] > 0) {
                    printf(" %u", j);
                    luck_sure += bm->lucky[j] > luck_mini;
                }
            }

            if (newluck == n && luck_sure == n) {
                if (!generator_found) {
                    printf(", complete generator found, for sure");
                }
                generator_found = 1;
            }
            printf(".\n");
        }
        /* }}} */

        if (generator_found) break;

        int (*ctable)[2] = malloc(b * 2 * sizeof(int));
        /* {{{ Now see in which order I may look at the columns of pi, so
         * as to keep the nominal degrees correct. In contrast with what
         * we used to do before, we no longer apply the permutation to
         * delta. So the delta[] array keeps referring to physical
         * indices, and we'll tune this in the end. */
        for(unsigned int j = 0; j < b; j++) {
            ctable[j][0] = delta[j];
            ctable[j][1] = j;
        }
        qsort(ctable, b, 2 * sizeof(int), (sortfunc_t) & col_cmp);
        /* }}} */

        /* {{{ Now do Gaussian elimination */
        memset(is_pivot, 0, b * sizeof(int));
        unsigned int r = 0;
        /* Loop through logical indices */
        for(unsigned int jl = 0; jl < b; jl++) {
            unsigned int j = ctable[jl][1];
            unsigned int u = 0;
            /* {{{ Find the pivot */
            for( ; u < m ; u++) {
                if (abcmp_ui(ab, matpoly_coeff(ab, e, u, j, 0), 0) != 0)
                    break;
            }
            if (u == m) continue;
            assert(r < m);
            /* }}} */
            pivots[r++] = j;
            is_pivot[j] = 1;
            /* {{{ Cancel this coeff in all other columns. */
            abelt inv;
            abinit(ab, &inv);
            int rc = abinv(ab, inv, matpoly_coeff(ab, e, u, j, 0));
            if (!rc) {
                fprintf(stderr, "Error, found a factor of the modulus: ");
                abfprint(ab, stderr, inv);
                fprintf(stderr, "\n");
                exit(EXIT_FAILURE);
            }
            abneg(ab, inv, inv);
            for (unsigned int kl = jl + 1; kl < b ; kl++) {
                unsigned int k = ctable[kl][1];
                if (abcmp_ui(ab, matpoly_coeff(ab, e, u, k, 0), 0) == 0)
                    continue;
                // add lambda = e[u,k]*-e[u,j]^-1 times col j to col k.
                abelt lambda;
                abinit(ab, &lambda);
                abmul(ab, lambda, inv, matpoly_coeff(ab, e, u, k, 0));

                assert(delta[j] <= delta[k]);
                /* {{{ Apply on both e and pi */
                abelt tmp;
                abinit(ab, &tmp);
                for(unsigned int i = 0 ; i < m ; i++) {
                    /* TODO: Would be better if mpfq had an addmul */
                    abmul(ab, tmp, lambda, matpoly_coeff(ab, e, i, j, 0));
                    abadd(ab,
                            matpoly_coeff(ab, e, i, k, 0),
                            matpoly_coeff(ab, e, i, k, 0),
                            tmp);
                }
                if (bm->lucky[k] < 0) {
                    /* This column is already discarded, don't bother */
                    continue;
                }
                if (bm->lucky[j] < 0) {
                    /* This column is discarded. This is going to
                     * invalidate another column of pi. Not a problem,
                     * unless it's been marked as lucky previously ! */
                    ASSERT_ALWAYS(bm->lucky[k] <= 0);
                    printf("Column %u discarded from now on (through addition from column %u)\n", k, j);
                    bm->lucky[k] = -1;
                    continue;
                }
                for(unsigned int i = 0 ; i < b ; i++) {
                    /* Beware. One may be tempted to think that the code
                     * above is dubious. It is, in fact, not a problem.
                     * As long as the delta[] array undergoes no
                     * disturbing modification, everything is ok.
                     */
                    if (pi_lengths[k] < pi_lengths[j])
                        pi_lengths[k] = pi_lengths[j];
                    for(unsigned int s = 0 ; s < pi_lengths[j] ; s++) {
                        /* TODO: Would be better if mpfq had an addmul */
                        abmul(ab, tmp, lambda,
                                matpoly_coeff(ab, pi, i, j, s));
                        abadd(ab,
                                matpoly_coeff(ab, pi, i, k, s),
                                matpoly_coeff(ab, pi, i, k, s),
                                tmp);
                    }
                }
                abclear(ab, &tmp);
                /* }}} */
                abclear(ab, &lambda);
            }
            abclear(ab, &inv); /* }}} */
        }
        /* }}} */
        free(ctable);

        ASSERT_ALWAYS(r == m);

        /* {{{ Now for all pivots, multiply column in pi by x */
        for (unsigned int j = 0; j < b ; j++) {
            if (!is_pivot[j]) continue;
            if (pi_lengths[j] >= pi->alloc) {
                if (!generator_found) {
                    matpoly_realloc(ab, pi, pi->alloc + MAX(pi->alloc / (m+n), 1));
                    printf("t=%u, expanding allocation for pi (now %zu%%) ; lengths: ",
                            bm->t,
                            100 * pi->alloc / pi_room_base);
                    for(unsigned int j = 0; j < b; j++)
                        printf(" %u", pi_lengths[j]);
                    printf("\n");
                } else {
                    ASSERT_ALWAYS(bm->lucky[j] <= 0);
                    if (bm->lucky[j] == 0)
                        printf("t=%u, column %u discarded from now on\n",
                                bm->t, j);
                    bm->lucky[j] = -1;
                    pi_lengths[j]++;
                    delta[j]++;
                    continue;
                }
            }
            matpoly_multiply_column_by_x(ab, pi, j, pi_lengths[j]);
            pi_lengths[j]++;
            delta[j]++;
        }
        /* }}} */
    }
    pi->size = 0;
    for(unsigned int j = 0; j < b; j++) {
        if (pi_lengths[j] > pi->size)
            pi->size = pi_lengths[j];
    }
    pi->size = MIN(pi->size, pi->alloc);
    matpoly_clear(ab, e);
    free(is_pivot);
    free(pivots);
    free(pi_lengths);   /* What shall we do with this one ??? */

    return tree_stats_leave(generator_found);
}/*}}}*/

/*}}}*/

double avg_matsize(abdst_field ab, unsigned int m, unsigned int n, int ascii)/*{{{*/
{
    if (!ascii) {
        /* Easy case first. If we have binary input, then we know a priori
         * that the input data must have size a multiple of the element size.
         */
        size_t elemsize = abvec_elt_stride(ab, 1);
        size_t matsize = elemsize * m * n;
        return matsize;
    }

    /* Ascii is more complicated. We're necessarily fragile here.
     * However, assuming that each coefficient comes with only one space,
     * and each matrix with an extra space (this is how the GPU program
     * prints data -- not that this ends up having a considerable impact
     * anyway...), we can guess the number of bytes per matrix. */

    /* Formula for the average number of digits of an integer mod p,
     * written in base b:
     *
     * (k-((b^k-1)/(b-1)-1)/p)  with b = Ceiling(Log(p)/Log(b)).
     */
    double avg;
    mpz_t p, a;
    mpz_init(p);
    mpz_init(a);
    abfield_characteristic(ab, p);
    unsigned long k = ceil(log(mpz_get_d(p))/log(10));
    unsigned long b = 10;
    mpz_ui_pow_ui(a, b, k);
    mpz_sub_ui(a, a, 1);
    mpz_fdiv_q_ui(a, a, b-1);
    avg = k - mpz_get_d(a) / mpz_get_d(p);
    mpz_clear(p);
    mpz_clear(a);
    // printf("Expect roughly %.2f decimal digits for integers mod p.\n", avg);
    double matsize = (avg + 1) * m * n + 1;
    // printf("Expect roughly %.2f bytes for each sequence matrix.\n", matsize);
    return matsize;
}/*}}}*/

/* {{{ I/O helpers */
/* {{{ matpoly_write
 * writes some of the matpoly data to f, either in ascii or binary
 * format. This can be used to write only part of the data (degrees
 * [k0..k1[). Returns the number of coefficients (i.e., matrices, so at
 * most k1-k0) successfully written, or
 * -1 on error (e.g. when some matrix was only partially written).
 */
int matpoly_write(abdst_field ab, FILE * f, matpoly_srcptr M, unsigned int k0, unsigned int k1, int ascii, int transpose)
{
    unsigned int m = transpose ? M->n : M->m;
    unsigned int n = transpose ? M->m : M->n;
    ASSERT_ALWAYS(k0 == k1 || (k0 < M->size && k1 <= M->size));
    abelt tmp;
    abinit(ab, &tmp);
    for(unsigned int k = k0 ; k < k1 ; k++) {
        int err = 0;
        int matnb = 0;
        for(unsigned int i = 0 ; !err && i < m ; i++) {
            for(unsigned int j = 0 ; !err && j < n ; j++) {
                absrc_elt x;
                x = transpose ? matpoly_coeff_const(ab, M, j, i, k)
                              : matpoly_coeff_const(ab, M, i, j, k);
                if (ascii) {
                    if (j) err = fprintf(f, " ") <= 0;
                    if (!err) err = abfprint(ab, f, x) <= 0;
                } else {
                    err = fwrite(x, abvec_elt_stride(ab, 1), 1, f) < 1;
                }
                if (!err) matnb++;
            }
            if (!err && ascii) err = fprintf(f, "\n") <= 0;
        }
        if (ascii) err = err || fprintf(f, "\n") <= 0;
        if (err) return (matnb == 0) ? (int) (k - k0) : -1;
    }
    abclear(ab, &tmp);
    return k1 - k0;
}/* }}} */

/* {{{ matpoly_read
 * reads some of the matpoly data from f, either in ascii or binary
 * format. This can be used to parse only part of the data (degrees
 * [k0..k1[, k1 being an upper bound). Returns the number of coefficients
 * (i.e., matrices, so at most k1-k0) successfully read, or
 * -1 on error (e.g. when some matrix was only partially read).
 *
 * Note that the matrix must *not* be in pre-init state. It must have
 * been already allocated.
 */

int matpoly_read(abdst_field ab, FILE * f, matpoly_ptr M, unsigned int k0, unsigned int k1, int ascii, int transpose)
{
    ASSERT_ALWAYS(!matpoly_check_pre_init(M));
    unsigned int m = transpose ? M->n : M->m;
    unsigned int n = transpose ? M->m : M->n;
    ASSERT_ALWAYS(k0 == k1 || (k0 < M->size && k1 <= M->size));
    abelt tmp;
    abinit(ab, &tmp);
    for(unsigned int k = k0 ; k < k1 ; k++) {
        int err = 0;
        int matnb = 0;
        for(unsigned int i = 0 ; !err && i < m ; i++) {
            for(unsigned int j = 0 ; !err && j < n ; j++) {
                abdst_elt x;
                x = transpose ? matpoly_coeff(ab, M, j, i, k)
                              : matpoly_coeff(ab, M, i, j, k);
                if (ascii) {
                    err = abfscan(ab, f, x) != 1;
                } else {
                    err = fread(x, abvec_elt_stride(ab, 1), 1, f) < 1;
                }
                if (!err) matnb++;
            }
        }
        if (err) return (matnb == 0) ? (int) (k - k0) : -1;
    }
    abclear(ab, &tmp);
    return k1 - k0;
}/* }}} */

/* }}} */

/*{{{ Checkpoints */

/* There's much copy-paste here */

struct cp_info {
    bmstatus_ptr bm;
    int level;
    unsigned int t0;
    unsigned int t1;
    int mpi;
    int rank;
    char * auxfile;
    char * sdatafile;
    char * gdatafile;
    const char * datafile;
    FILE * aux;
    FILE * data;
};

void cp_info_init_backend(struct cp_info * cp)
{
    int rc;
    cp->level = stats->depth;
    rc = asprintf(&cp->auxfile, "%s/pi.%d.%u.%u.aux",
            checkpoint_directory, cp->level, cp->t0, cp->t1);
    ASSERT_ALWAYS(rc >= 0);
    rc = asprintf(&cp->gdatafile, "%s/pi.%d.%u.%u.single.data",
            checkpoint_directory, cp->level, cp->t0, cp->t1);
    ASSERT_ALWAYS(rc >= 0);
    rc = asprintf(&cp->sdatafile, "%s/pi.%d.%u.%u.%d.data",
            checkpoint_directory, cp->level, cp->t0, cp->t1, cp->rank);
    ASSERT_ALWAYS(rc >= 0);
}

void cp_info_init(struct cp_info * cp, bmstatus_ptr bm, unsigned int t0, unsigned int t1)
{
    memset(cp, 0, sizeof(*cp));
    cp->bm = bm;
    cp->t0 = t0; cp->t1 = t1;
    cp_info_init_backend(cp);
    cp->datafile = cp->gdatafile;
}

void cp_info_init_mpi(struct cp_info * cp, bmstatus_ptr bm, unsigned int t0, unsigned int t1)
{
    memset(cp, 0, sizeof(*cp));
    cp->mpi = 1;
    cp->bm = bm;
    cp->t0 = t0; cp->t1 = t1;
    MPI_Comm_rank(bm->com[0], &(cp->rank));
    cp_info_init_backend(cp);
    /* a priori default. However load_mpi_checkpoint_file looks up both */
    cp->datafile = cp->sdatafile;
}

void cp_info_clear(struct cp_info * cp)
{
    free(cp->sdatafile);
    free(cp->gdatafile);
    free(cp->auxfile);
}

int cp_save_aux_file(struct cp_info * cp, size_t pi_size, unsigned int *delta, int done)
{
    dims * d = cp->bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    if (cp->rank) return 1;
    FILE * aux = fopen(cp->auxfile, "w");
    int rc;
    if (aux == NULL) {
        fprintf(stderr, "Warning: cannot open %s\n", cp->auxfile);
        return 0;
    }
    rc = fprintf(aux, "%zu\n", pi_size);
    if (rc <= 0) goto cp_save_aux_file_bailout;
    for(unsigned int i = 0 ; i < m + n ; i++) {
        rc = fprintf(aux, "%s%u", i?" ":"", delta[i]);
        if (rc <= 0) goto cp_save_aux_file_bailout;
    }
    rc = fprintf(aux, "\n");
    if (rc <= 0) goto cp_save_aux_file_bailout;
    for(unsigned int i = 0 ; i < m + n ; i++) {
        rc = fprintf(aux, "%s%d", i?" ":"", cp->bm->lucky[i]);
        if (rc <= 0) goto cp_save_aux_file_bailout;
    }
    rc = fprintf(aux, "\n");
    if (rc <= 0) goto cp_save_aux_file_bailout;
    rc = fprintf(aux, "%d\n", done);
    if (rc <= 0) goto cp_save_aux_file_bailout;
    rc = fclose(aux);
    if (rc == 0) return 1;
cp_save_aux_file_bailout:
    fclose(aux);
    unlink(cp->auxfile);
    return 0;
}

int cp_load_aux_file(struct cp_info * cp, size_t * p_pi_size, unsigned int *delta, int * p_done)
{
    dims * d = cp->bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    if (cp->rank) return 1;
    FILE * aux = fopen(cp->auxfile, "r");
    int rc;
    if (aux == NULL) {
        // fprintf(stderr, "Warning: cannot open %s\n", cp->auxfile);
        return 0;
    }
    rc = fscanf(aux, "%zu", p_pi_size);
    if (rc != 1) { fclose(aux); return 0; }
    for(unsigned int i = 0 ; i < m + n ; i++) {
        rc = fscanf(aux, "%u", delta + i);
        if (rc != 1) { fclose(aux); return 0; }
    }
    for(unsigned int i = 0 ; i < m + n ; i++) {
        rc = fscanf(aux, "%d", cp->bm->lucky + i);
        if (rc != 1) { fclose(aux); return 0; }
    }
    rc = fscanf(aux, "%d", p_done);
    if (rc != 1) { fclose(aux); return 0; }
    rc = fclose(aux);
    return rc == 0;
}

int cp_load_data_file(struct cp_info * cp, matpoly pi, size_t pi_size)
{
    dims * d = cp->bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    FILE * data = fopen(cp->datafile, "rb");
    int rc;
    if (data == NULL) {
        fprintf(stderr, "Warning: cannot open %s\n", cp->datafile);
        return 0;
    }
    matpoly_init(ab, pi, m+n, m+n, pi_size);
    pi->size = pi_size;
    rc = matpoly_read(ab, data, pi, 0, pi->size, 0, 0);
    if (rc != (int) pi->size) { fclose(data); return 0; }
    rc = fclose(data);
    return rc == 0;
}

/* I think we always have pi_size == pi->size, the only questionable
 * situation is when we're saving part of a big matrix */
int cp_save_data_file(struct cp_info * cp, matpoly pi, size_t pi_size)
{
    abdst_field ab = cp->bm->d->ab;
    FILE * data = fopen(cp->datafile, "wb");
    int rc;
    if (data == NULL) {
        fprintf(stderr, "Warning: cannot open %s\n", cp->datafile);
        unlink(cp->auxfile);
        return 0;
    }
    rc = matpoly_write(ab, data, pi, 0, pi_size, 0, 0);
    if (rc != (int) pi->size) goto cp_save_data_file_bailout;
    rc = fclose(data);
    if (rc == 0)
        return 1;
cp_save_data_file_bailout:
    fclose(data);
    unlink(cp->datafile);
    unlink(cp->auxfile);
    return 0;
}

int load_checkpoint_file(bmstatus_ptr bm, matpoly pi, unsigned int t0, unsigned int t1, unsigned int *delta, int * p_done)/*{{{*/
{
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    struct cp_info cp[1];
    cp_info_init(cp, bm, t0, t1);
    ASSERT_ALWAYS(matpoly_check_pre_init(pi));
    size_t pi_size;
    /* Don't output a message just now, since after all it's not
     * noteworthy if the checkpoint file does not exist. */
    int ok = cp_load_aux_file(cp, &pi_size, delta, p_done);
    if (ok) {
        logline_begin(stdout, SIZE_MAX, "Reading checkpoint %s", cp->datafile);
        ok = cp_load_data_file(cp, pi, pi_size);
        logline_end(&bm->t_cp_io,"");
        if (!ok)
            fprintf(stderr, "Warning: I/O error while reading %s\n", cp->datafile);
    }
    cp_info_clear(cp);
    if (ok) bm->t = t1;
    return ok;
}/*}}}*/

int save_checkpoint_file(bmstatus_ptr bm, matpoly pi, unsigned int t0, unsigned int t1, unsigned int *delta, int done)/*{{{*/
{
    /* corresponding t is bm->t - E->size ! */
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    struct cp_info cp[1];
    cp_info_init(cp, bm, t0, t1);
    logline_begin(stdout, SIZE_MAX, "Saving checkpoint %s%s",
            cp->datafile,
            cp->mpi ? " (MPI, scattered)" : "");
    int ok = cp_save_aux_file(cp, pi->size, delta, done);
    if (ok) ok = cp_save_data_file(cp, pi, pi->size);
    logline_end(&bm->t_cp_io,"");
    if (!ok && !cp->rank)
        fprintf(stderr, "Warning: I/O error while saving %s\n", cp->datafile);
    cp_info_clear(cp);
    return ok;
}/*}}}*/

int load_mpi_checkpoint_file_scattered(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int * p_done)/*{{{*/
{
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    dims * d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    struct cp_info cp[1];
    cp_info_init_mpi(cp, bm, t0, t1);
    ASSERT_ALWAYS(bigmatpoly_check_pre_init(xpi));
    size_t pi_size;
    int ok = cp_load_aux_file(cp, &pi_size, delta, p_done);
    MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
    MPI_Bcast(&pi_size, 1, MPI_MY_SIZE_T, 0, bm->com[0]);
    MPI_Bcast(delta, m + n, MPI_UNSIGNED, 0, bm->com[0]);
    MPI_Bcast(bm->lucky, m + n, MPI_INT, 0, bm->com[0]);
    MPI_Bcast(p_done, 1, MPI_INT, 0, bm->com[0]);
    if (ok) {
        logline_begin(stdout, SIZE_MAX, "Reading checkpoint %s (MPI, scattered)",
                cp->datafile);
        do {
            FILE * data = fopen(cp->datafile, "rb");
            int rc;
            ok = data != NULL;
            MPI_Allreduce(MPI_IN_PLACE, &ok, 1, MPI_INT, MPI_MIN, bm->com[0]);
            if (!ok) {
                if (!rank)
                    fprintf(stderr, "Warning: cannot open %s\n", cp->datafile);
                if (data) free(data);
                break;
            }
            bigmatpoly_finish_init(ab, xpi, m+n, m+n, pi_size);
            bigmatpoly_set_size(xpi, pi_size);
            rc = matpoly_read(ab, data, bigmatpoly_my_cell(xpi), 0, xpi->size, 0, 0);
            ok = ok && rc == (int) xpi->size;
            rc = fclose(data);
            ok = ok && (rc == 0);
        } while (0);
        MPI_Allreduce(MPI_IN_PLACE, &ok, 1, MPI_INT, MPI_MIN, bm->com[0]);
        logline_end(&bm->t_cp_io,"");
        if (!ok && !rank) {
            fprintf(stderr, "Warning: I/O error while reading %s\n",
                    cp->datafile);
        }
    }
    cp_info_clear(cp);
    if (ok) bm->t = t1;
    return ok;
}/*}}}*/

int save_mpi_checkpoint_file_scattered(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int done)/*{{{*/
{
    /* corresponding t is bm->t - E->size ! */
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    struct cp_info cp[1];
    cp_info_init_mpi(cp, bm, t0, t1);
    int ok = cp_save_aux_file(cp, xpi->size, delta, done);
    MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
    if (!ok && !rank) unlink(cp->auxfile);
    if (ok) {
        logline_begin(stdout, SIZE_MAX, "Saving checkpoint %s (MPI, scattered)",
                cp->datafile);
        ok = cp_save_data_file(cp, bigmatpoly_my_cell(xpi), xpi->size);
        logline_end(&bm->t_cp_io,"");
        MPI_Allreduce(MPI_IN_PLACE, &ok, 1, MPI_INT, MPI_MIN, bm->com[0]);
        if (!ok) {
            if (cp->datafile) unlink(cp->datafile);
            if (!rank) unlink(cp->auxfile);
        }
    }
    if (!ok && !rank) {
        fprintf(stderr, "Warning: I/O error while saving %s\n", cp->datafile);
    }
    cp_info_clear(cp);
    return ok;
}/*}}}*/

int load_mpi_checkpoint_file_gathered(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int * p_done)/*{{{*/
{
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    dims * d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    struct cp_info cp[1];
    cp_info_init_mpi(cp, bm, t0, t1);
    cp->datafile = cp->gdatafile;
    size_t pi_size;
    int ok = cp_load_aux_file(cp, &pi_size, delta, p_done);
    MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
    MPI_Bcast(&pi_size, 1, MPI_MY_SIZE_T, 0, bm->com[0]);
    MPI_Bcast(delta, m + n, MPI_UNSIGNED, 0, bm->com[0]);
    MPI_Bcast(bm->lucky, m + n, MPI_INT, 0, bm->com[0]);
    MPI_Bcast(p_done, 1, MPI_INT, 0, bm->com[0]);
    if (ok) {
        logline_begin(stdout, SIZE_MAX, "Reading checkpoint %s (MPI, gathered)",
                cp->datafile);
        do {
            FILE * data = NULL;
            if (!rank) ok = (data = fopen(cp->datafile, "rb")) != NULL;
            MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
            if (!ok) {
                if (!rank)
                    fprintf(stderr, "Warning: cannot open %s\n", cp->datafile);
                if (data) free(data);
                break;
            }

            bigmatpoly_finish_init(ab, xpi, m+n, m+n, pi_size);
            bigmatpoly_set_size(xpi, pi_size);

            double avg = avg_matsize(ab, m + n, m + n, 0);
            unsigned int B = iceildiv(io_block_size, avg);

            /* This is only temp storage ! */
            matpoly pi;
            matpoly_init(ab, pi, m + n, m + n, B);
            pi->size = B;

            for(unsigned int k = 0 ; ok && k < xpi->size ; k += B) {
                unsigned int nc = MIN(B, xpi->size - k);
                if (!rank)
                    ok = matpoly_read(ab, data, pi, 0, nc, 0, 0) == (int) nc;
                MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
                bigmatpoly_scatter_mat_partial(ab, xpi, pi, k, nc);
            }
            matpoly_clear(ab, pi);

            if (!rank) {
                int rc = fclose(data);
                ok = ok && (rc == 0);
            }
        } while (0);
        MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
        logline_end(&bm->t_cp_io,"");
        if (!ok && !rank) {
            fprintf(stderr, "Warning: I/O error while reading %s\n",
                    cp->datafile);
        }
    }
    cp_info_clear(cp);
    if (ok) bm->t = t1;
    return ok;
}/*}}}*/

int save_mpi_checkpoint_file_gathered(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int done)/*{{{*/
{
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    dims * d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    struct cp_info cp[1];
    cp_info_init_mpi(cp, bm, t0, t1);
    cp->datafile = cp->gdatafile;
    logline_begin(stdout, SIZE_MAX, "Saving checkpoint %s (MPI, gathered)",
            cp->datafile);
    int ok = cp_save_aux_file(cp, xpi->size, delta, done);
    MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
    if (ok) {
        do {
            FILE * data = NULL;
            int rc;
            if (!rank) ok = (data = fopen(cp->datafile, "wb")) != NULL;
            MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
            if (!ok) {
                if (!rank)
                    fprintf(stderr, "Warning: cannot open %s\n", cp->datafile);
                if (data) free(data);
                break;
            }

            double avg = avg_matsize(ab, m + n, m + n, 0);
            unsigned int B = iceildiv(io_block_size, avg);

            /* This is only temp storage ! */
            matpoly pi;
            matpoly_init(ab, pi, m + n, m + n, B);
            pi->size = B;

            for(unsigned int k = 0 ; ok && k < xpi->size ; k += B) {
                unsigned int nc = MIN(B, xpi->size - k);
                bigmatpoly_gather_mat_partial(ab, pi, xpi, k, nc);
                if (!rank)
                    ok = matpoly_write(ab, data, pi, 0, nc, 0, 0) == (int) nc;
                MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
            }
            matpoly_clear(ab, pi);

            if (!rank) {
                rc = fclose(data);
                ok = ok && (rc == 0);
            }
        } while (0);
        MPI_Bcast(&ok, 1, MPI_INT, 0, bm->com[0]);
        if (!ok && !rank) {
            if (cp->datafile) unlink(cp->datafile);
            unlink(cp->auxfile);
        }
    }
    logline_end(&bm->t_cp_io,"");
    if (!ok && !rank) {
        fprintf(stderr, "Warning: I/O error while saving %s\n", cp->datafile);
    }
    cp_info_clear(cp);
    return ok;
}/*}}}*/

int load_mpi_checkpoint_file(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int * p_done)/*{{{*/
{
    /* read scattered checkpoint with higher priority if available,
     * because we like distributed I/O. Otherwise, read gathered
     * checkpoint if we could find one.
     */
    if (!checkpoint_directory) return 0;
    if ((t1 - t0) < checkpoint_threshold) return 0;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    struct cp_info cp[1];
    cp_info_init_mpi(cp, bm, t0, t1);
    int ok = 0;
    int aux_ok = rank || access(cp->auxfile, R_OK) == 0;
    int sdata_ok = access(cp->sdatafile, R_OK) == 0;
    int scattered_ok = aux_ok && sdata_ok;
    MPI_Allreduce(MPI_IN_PLACE, &scattered_ok, 1, MPI_INT, MPI_MIN, bm->com[0]);
    if (scattered_ok) {
        ok = load_mpi_checkpoint_file_scattered(bm, xpi, t0, t1, delta, p_done);
        if (ok) return ok;
    }
    int gdata_ok = rank || access(cp->gdatafile, R_OK) == 0;
    int gathered_ok = aux_ok && gdata_ok;
    MPI_Bcast(&gathered_ok, 1, MPI_INT, 0, bm->com[0]);
    if (gathered_ok) {
        ok = load_mpi_checkpoint_file_gathered(bm, xpi, t0, t1, delta, p_done);
    }
    cp_info_clear(cp);
    return ok;
}/*}}}*/

int save_mpi_checkpoint_file(bmstatus_ptr bm, bigmatpoly xpi, unsigned int t0, unsigned int t1, unsigned int *delta, int done)/*{{{*/
{
    if (save_gathered_checkpoints) {
        return save_mpi_checkpoint_file_gathered(bm, xpi, t0, t1, delta, done);
    } else {
        return save_mpi_checkpoint_file_scattered(bm, xpi, t0, t1, delta, done);
    }
}/*}}}*/

/*}}}*/

/**********************************************************************/

/*{{{ Main entry points and recursive algorithm (with and without MPI) */

/* Forward declaration, it's used by the recursive version */
static int bw_lingen_single(bmstatus_ptr bm, matpoly pi, matpoly E, unsigned int *delta);
static int bw_biglingen_collective(bmstatus_ptr bm, bigmatpoly pi, bigmatpoly E, unsigned int *delta);

static int bw_lingen_recursive(bmstatus_ptr bm, matpoly pi, matpoly E, unsigned int *delta) /*{{{*/
{
    size_t z = E->size;
    tree_stats_enter(__func__, E->size);
    dims * d = bm->d;
    abdst_field ab = d->ab;
    int done;

    /* XXX I think we have to start with something large enough to get
     * all coefficients of E_right correct */
    size_t half = E->size - (E->size / 2);

    // fprintf(stderr, "Enter %s\n", __func__);
    /* declare an lazy-alloc all matrices */
    matpoly E_left;
    matpoly pi_left;
    matpoly pi_right;
    matpoly E_right;
    matpoly_init(ab, E_left, 0, 0, 0);
    matpoly_init(ab, pi_left, 0, 0, 0);
    matpoly_init(ab, pi_right, 0, 0, 0);
    matpoly_init(ab, E_right, 0, 0, 0);

    matpoly_truncate(ab, E_left, E, half);
    done = bw_lingen_single(bm, pi_left, E_left, delta);
    ASSERT_ALWAYS(pi_left->size);
    matpoly_clear(ab, E_left);

    if (done) {
        matpoly_swap(pi_left, pi);
        matpoly_clear(ab, pi_left);
        // fprintf(stderr, "Leave %s\n", __func__);
        return tree_stats_leave(1);
    }

    matpoly_rshift(ab, E, E, half - pi_left->size + 1);
    logline_begin(stdout, z, "t=%u MP(%zu, %zu) -> %zu",
            bm->t, E->size, pi_left->size, E->size - pi_left->size + 1);
    matpoly_mp(ab, E_right, E, pi_left);
    logline_end(&(bm->t_mp), "");

    done = bw_lingen_single(bm, pi_right, E_right, delta);
    matpoly_clear(ab, E_right);

    logline_begin(stdout, z, "t=%u MUL(%zu, %zu) -> %zu",
            bm->t, pi_left->size, pi_right->size, pi_left->size + pi_right->size - 1);
    matpoly_mul(ab, pi, pi_left, pi_right);
    logline_end(&bm->t_mul, "");

    matpoly_clear(ab, pi_left);
    matpoly_clear(ab, pi_right);

    // fprintf(stderr, "Leave %s\n", __func__);
    return tree_stats_leave(done);
}/*}}}*/

static int bw_biglingen_recursive(bmstatus_ptr bm, bigmatpoly pi, bigmatpoly E, unsigned int *delta) /*{{{*/
{
    size_t z = E->size;
    tree_stats_enter(__func__, E->size);

    dims * d = bm->d;
    abdst_field ab = d->ab;
    int done;

    int rank;
    MPI_Comm_rank(bm->com[0], &rank);

    // fprintf(stderr, "Enter %s\n", __func__);
    /* XXX I think we have to start with something large enough to get
     * all coefficients of E_right correct */
    size_t half = E->size - (E->size / 2);

    /* declare an lazy-alloc all matrices */
    bigmatpoly E_left;
    bigmatpoly E_right;
    bigmatpoly pi_left;
    bigmatpoly pi_right;
    bigmatpoly_init(ab, E_left, E, 0, 0, 0);
    bigmatpoly_init(ab, pi_left, pi, 0, 0, 0);
    bigmatpoly_init(ab, pi_right, pi, 0, 0, 0);
    bigmatpoly_init(ab, E_right, E, 0, 0, 0);

    bigmatpoly_truncate_loc(ab, E_left, E, half);
    done = bw_biglingen_collective(bm, pi_left, E_left, delta);
    bigmatpoly_clear(ab, E_left);

    if (done) {
        bigmatpoly_swap(pi_left, pi);
        bigmatpoly_clear(ab, pi_left);
        bigmatpoly_clear(ab, pi_right);
        bigmatpoly_clear(ab, E_right);
        // fprintf(stderr, "Leave %s\n", __func__);
        return tree_stats_leave(1);
    }

    bigmatpoly_rshift(ab, E, E, half - pi_left->size + 1);

    logline_begin(stdout, z, "t=%u MPI-MP%s(%zu, %zu) -> %zu",
            bm->t, caching ? "-caching" : "",
            E->size, pi_left->size, E->size - pi_left->size + 1);
#ifdef  HAVE_MPIR
    if (caching)
        bigmatpoly_mp_caching(ab, E_right, E, pi_left);
    else
#endif
        bigmatpoly_mp(ab, E_right, E, pi_left);
    logline_end(&bm->t_mp, "");

    done = bw_biglingen_collective(bm, pi_right, E_right, delta);
    bigmatpoly_clear(ab, E_right);

    logline_begin(stdout, z, "t=%u MPI-MUL%s(%zu, %zu) -> %zu",
            bm->t, caching ? "-caching" : "",
            pi_left->size, pi_right->size, pi_left->size + pi_right->size - 1);
#ifdef  HAVE_MPIR
    if (caching)
        bigmatpoly_mul_caching(ab, pi, pi_left, pi_right);
    else
#endif
        bigmatpoly_mul(ab, pi, pi_left, pi_right);
    logline_end(&bm->t_mul, "");
    bigmatpoly_clear(ab, pi_left);
    bigmatpoly_clear(ab, pi_right);

    // fprintf(stderr, "Leave %s\n", __func__);
    return tree_stats_leave(done);
}/*}}}*/

static int bw_lingen_single(bmstatus_ptr bm, matpoly pi, matpoly E, unsigned int *delta) /*{{{*/
{
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);
    ASSERT_ALWAYS(!rank);
    unsigned int t0 = bm->t;
    unsigned int t1 = bm->t + E->size;

    int done;

    if (load_checkpoint_file(bm, pi, t0, t1, delta, &done))
        return done;

    // ASSERT_ALWAYS(E->size < bm->lingen_mpi_threshold);

    // fprintf(stderr, "Enter %s\n", __func__);
    if (E->size < bm->lingen_threshold) {
        bm->t_basecase -= seconds();
        done = bw_lingen_basecase(bm, pi, E, delta);
        bm->t_basecase += seconds();
    } else {
        done = bw_lingen_recursive(bm, pi, E, delta);
    }
    // fprintf(stderr, "Leave %s\n", __func__);

    save_checkpoint_file(bm, pi, t0, t1, delta, done);

    return done;
}/*}}}*/

static int bw_biglingen_collective(bmstatus_ptr bm, bigmatpoly pi, bigmatpoly E, unsigned int *delta)/*{{{*/
{
    dims *d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    int done;
    int rank;
    int size;
    MPI_Comm_rank(bm->com[0], &rank);
    MPI_Comm_size(bm->com[0], &size);
    unsigned int t0 = bm->t;
    unsigned int t1 = bm->t + E->size;

    if (load_mpi_checkpoint_file(bm, pi, t0, t1, delta, &done))
        return done;

    // fprintf(stderr, "Enter %s\n", __func__);
    if (E->size >= bm->lingen_mpi_threshold)  {
        done = bw_biglingen_recursive(bm, pi, E, delta);
    } else {
        /* Fall back to local code */
        /* This entails gathering E locally, computing pi locally, and
         * dispathing it back. */

        matpoly sE, spi;
        matpoly_init(ab, sE, m, b, E->size);
        matpoly_init(ab, spi, 0, 0, 0);

        bigmatpoly_gather_mat_alt2(ab, sE, E);
        /* Only the master node does the local computation */
        if (!rank)
            done = bw_lingen_single(bm, spi, sE, delta);
        bigmatpoly_scatter_mat_alt2(ab, pi, spi);

        MPI_Bcast(&done, 1, MPI_INT, 0, bm->com[0]);
        MPI_Bcast(delta, b, MPI_UNSIGNED, 0, bm->com[0]);
        MPI_Bcast(bm->lucky, b, MPI_UNSIGNED, 0, bm->com[0]);
        MPI_Bcast(&(bm->t), 1, MPI_UNSIGNED, 0, bm->com[0]);
        /* Don't forget to broadcast delta from root node to others ! */
        matpoly_clear(ab, spi);
        matpoly_clear(ab, sE);
    }
    // fprintf(stderr, "Leave %s\n", __func__);

    save_mpi_checkpoint_file(bm, pi, t0, t1, delta, done);

    return done;
}/*}}}*/

/*}}}*/

/**********************************************************************/

void get_minmax_delta_on_solutions(bmstatus_ptr bm, unsigned int * res, unsigned int * delta)/*{{{*/
{
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int maxdelta = 0;
    unsigned int mindelta = UINT_MAX;
    for(unsigned int j = 0 ; j < m + n ; j++) {
        if (bm->lucky[j] <= 0)
            continue;
        if (delta[j] > maxdelta)
            maxdelta = delta[j];
        if (delta[j] < mindelta)
            mindelta = delta[j];
    }
    res[0] = mindelta;
    res[1] = maxdelta;
}/*}}}*/

unsigned int get_max_delta_on_solutions(bmstatus_ptr bm, unsigned int * delta)/*{{{*/
{
    unsigned int res[2];
    get_minmax_delta_on_solutions(bm, res, delta);
    return res[1];
}/*}}}*/

/**********************************************************************/

struct bm_io_s {/*{{{*/
    bmstatus_ptr bm;
    unsigned int t0;
    FILE * f;
    char * iobuf;
    const char * input_file;
    const char * output_file;
    int ascii;
    /* This is only a rolling window ! */
    matpoly A, F;
    unsigned int (*fdesc)[2];
    /* This k is the coefficient in A(X) div X of the next coefficient to
     * be read. This is thus the total number of coefficients of A(X) div
     * X which have been read so far.
     * In writing mode, k is the number of coefficients of F which have
     * been written so far.
     */
    unsigned int k;

    unsigned int guessed_length;
};

typedef struct bm_io_s bm_io[1];
typedef struct bm_io_s * bm_io_ptr;/*}}}*/

/* The reading mode of bm_io is streaming, but with a look-back
 * functionality: we want to be able to access coefficients a few places
 * earlier, so we keep them in memory.
 *
 * The writing mode has a write-ahead feature. Coefficients of the result
 * are written at various times, some earlier than others. The time span
 * is the same as for the reading mode.
 */


/* We write the coefficients of the reversed polynomial \hat{F*\pi}, in
 * increasing degree order. Thus the first coefficients written
 * corresponds to high degree coefficients in \pi.
 * This is mostly for historical reasons, since in fact, we would prefer
 * having the coefficients in the reverse order.
 */

/* let mindelta and maxdelta be (as their name suggests) the minimum and
 * maximum over all deltas corresponding to solution columns.
 *
 * For 0<=i<n, coeff i,j,k of pi becomes coeff i,j,k' of the final f,
 * with k'=k-(maxdelta-delta[j]).
 *
 * For 0<=i<m, coeff n+i,j,k of pi becomes coeff c[i],j,k' of the final f,
 * with k'=k-(maxdelta-delta[j])-(t0-e[j]).
 *
 * Therefore the maximum write-behind distance is (maxdelta-mindelta)+t0.
 * We need one coeff more (because offset goes from 0 to maxoffset).
 */
unsigned int bm_io_set_write_behind_size(bm_io_ptr aa, unsigned int * delta)
{
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    abdst_field ab = d->ab;
    unsigned int res[2];
    get_minmax_delta_on_solutions(bm, res, delta);
    unsigned int mindelta = res[0];
    unsigned int maxdelta = res[1];
    unsigned int window = maxdelta - mindelta + aa->t0 + 1;
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (rank) return window;
    matpoly_realloc(ab, aa->F, window);
    matpoly_zero(ab, aa->F);
    aa->F->size = window;
    return window;
}

/* Note that coefficients of F are written transposed */
void bm_io_write_one_F_coeff(bm_io_ptr aa, unsigned int deg)
{
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    abdst_field ab = d->ab;
    deg = deg % aa->F->alloc;
    // if (random_input_length) return;
    if (!aa->ascii || !random_input_length)
        matpoly_write(ab, aa->f, aa->F, deg, deg + 1, aa->ascii, 1);
}

void bm_io_clear_one_F_coeff(bm_io_ptr aa, unsigned int deg)
{
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    abdst_field ab = d->ab;
    unsigned int n = d->n;
    deg = deg % aa->F->alloc;
    for(unsigned int j = 0 ; j < n ; j++) {
        for(unsigned int i = 0 ; i < n ; i++) {
            abset_zero(ab, matpoly_coeff(ab, aa->F, i, j, deg));
        }
    }
}

void bm_io_compute_final_F(bm_io_ptr aa, bigmatpoly_ptr xpi, unsigned int * delta)/*{{{*/
{
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    abdst_field ab = d->ab;
    int rank;
    MPI_Comm_rank(bm->com[0], &rank);


    /* We are not interested by xpi->size, but really by the number of
     * coefficients for the columns which give solutions. */
    unsigned int maxdelta = get_max_delta_on_solutions(bm, delta);
    unsigned int pilen = maxdelta + 1 - aa->t0;

    if (!rank) printf("Final f(X)=f0(X)pi(X) has degree %u\n", maxdelta);

    /* Decide on the temp storage size */
    double avg = avg_matsize(ab, n, n, aa->ascii);
    unsigned int B = iceildiv(io_block_size, avg);
    if (!rank && !random_input_length) {
        printf("Writing F to %s\n", aa->output_file);
        printf("Writing F by blocks of %u coefficients (%.1f bytes each)\n",
                B, avg);
    }
    /* This is only temp storage ! */
    matpoly pi;
    matpoly_init(ab, pi, b, b, B);

    unsigned int window = aa->F->alloc;

    /* Which columns of F*pi will make the final generator ? */
    unsigned int * sols = malloc(n * sizeof(unsigned int));
    for(unsigned int j = 0, jj=0 ; j < m + n ; j++) {
        if (bm->lucky[j] <= 0)
            continue;
        sols[jj++]=j;
    }

    double tt0 = wct_seconds();
    double next_report_t = tt0 + 10;
    unsigned next_report_k = pilen / 100;

#if 0 /* {{{ save the code which reads pi forwards. */
    /* To be clear: it's untested, but probably a better base to work on
     * than the version reading backwards... Someday, we might fix the
     * way we write F. This will ease the job for mksol if we consider
     * forcing it to evaluate with Horner. (presently, after much turmoil
     * for writing F*pi reversed, doing so would require mksol to read
     * the coefficients reversed too...)
     */
    unsigned int kpi;
    for(unsigned int kpi0 = 0 ; kpi0 < pilen ; kpi0 += B) {
        matpoly_zero(ab, pi);
        pi->size = B;

        bigmatpoly_gather_mat_partial(ab, pi, xpi, kpi0, B);

        if (rank) continue;

        kpi = kpi0;

        for(unsigned int s = 0 ; s < B ; s++, kpi++) {
            /* coefficient of degree kpi-window is now complete */
            if (kpi >= window) {
                bm_io_write_one_F_coeff(aa, kpi - window);
                bm_io_clear_one_F_coeff(aa, kpi - window);
            }
            /* Now use our coefficient. This might tinker with
             * coefficients up to degree kpi-(window-1) in the file F */

            if (kpi > maxdelta + aa->t0 ) {
                /* this implies that we always have kF > delta[jpi],
                 * whence we expect a zero contribution */
                for(unsigned int i = 0 ; i < m + n ; i++) {
                    for(unsigned int jF = 0 ; jF < n ; jF++) {
                        unsigned int jpi = sols[jF];
                        absrc_elt src = matpoly_coeff_const(ab, pi, i, jpi, s);
                        ASSERT_ALWAYS(abis_zero(ab, src));
                    }
                }
                break;
            }

            for(unsigned int i = 0 ; i < n ; i++) {
                for(unsigned int jF = 0 ; jF < n ; jF++) {
                    unsigned int jpi = sols[jF];
                    unsigned int subtract = maxdelta - delta[jpi];
                    ASSERT(subtract < window);
                    if (kpi < subtract) continue;
                    unsigned int kF = kpi - subtract;
                    absrc_elt src = matpoly_coeff_const(ab, pi, i, jpi, s);
                    abdst_elt dst = matpoly_coeff(ab, aa->F, i, jF, kF % window);
                    ASSERT_ALWAYS(kF <= delta[jpi] || abis_zero(ab, src));
                    abadd(ab, dst, dst, src);
                }
            }
            for(unsigned int ipi = n ; ipi < m + n ; ipi++) {
                for(unsigned int jF = 0 ; jF < n ; jF++) {
                    unsigned int jpi = sols[jF];
                    unsigned int iF = aa->fdesc[ipi-n][1];
                    unsigned int offset = aa->t0 - aa->fdesc[ipi-n][0];
                    unsigned int subtract = maxdelta - delta[jpi] + offset;
                    ASSERT(subtract < window);
                    if (kpi < subtract) continue;
                    unsigned int kF = kpi - subtract;
                    abdst_elt dst = matpoly_coeff(ab, aa->F, iF, jF, kF % window);
                    absrc_elt src = matpoly_coeff_const(ab, pi, ipi, jpi, s);
                    ASSERT_ALWAYS(kF <= delta[jpi] || abis_zero(ab, src));
                    abadd(ab, dst, dst, src);
                }
            }
        }

        if (kpi != kpi0 + B)
            break;

        if (!rank && kpi > next_report_k) {
            double tt = wct_seconds();
            if (tt > next_report_t) {
                fprintf(stderr,
                        "Written %u coefficients (%.1f%%) in %.1f s\n",
                        kpi, 100.0 * kpi / pilen,
                        tt-tt0);
                next_report_t = tt + 10;
                next_report_k = kpi + pilen/ 100;
            }
        }
    }
    /* flush the pipe */
    for(unsigned int s = 0 ; s < window ; s++, kpi++)
        bm_io_write_one_F_coeff(aa, kpi - window);
#endif /* }}} */

    /* we need to read pi backwards. The number of coefficients in pi is
     * pilen = maxdelta + 1 - t0. Hence the first interesting index is
     * maxdelta - t0. However, for notational ease, we'll access
     * coefficients from index maxdelta downwards.
     */

    unsigned int kpi = maxdelta;
    /* index kpi1 is end fence for window */
    for(unsigned int kpi1 = iceildiv(maxdelta + 1, B) * B ; kpi1 > 0 ; kpi1 -= B) {
        unsigned int kpi0 = kpi1 - B;
        /* rare corner case */
        if (kpi0 >= pilen) continue;

        matpoly_zero(ab, pi);
        pi->size = B;

        bigmatpoly_gather_mat_partial(ab, pi, xpi, kpi0, MIN(B, pilen - kpi0));

        if (rank) continue;

        kpi = kpi1 - 1;

        for(unsigned int s = B ; s-- > 0 ; kpi--) {
            if (kpi + window == maxdelta) {
                /* Avoid writing zero coefficients. This can occur !
                 * Example:
                 * tt=(2*4*1200) mod 1009, a = (tt cat tt cat * tt[0..10])
                 */
                for(unsigned int j = 0 ; j < n ; j++) {
                    int z = 1;
                    for(unsigned int i = 0 ; z && i < n ; i++) {
                        absrc_elt src = matpoly_coeff_const(ab, aa->F, i, j, 0);
                        z = abis_zero(ab, src);
                    }

                    if (z) {
                        /* This is a bit ugly. Given that we're going to
                         * shift one column of F, we'll have a
                         * potentially deepedr write-back buffer. Columns
                         * which seemed to be ready still are, but they
                         * will now be said so only at the next step.
                         */
                        printf("Reduced solution column #%u from"
                                " delta=%u to delta=%u\n",
                                sols[j], delta[sols[j]], delta[sols[j]]-1);
                        window++;
                        matpoly_realloc(ab, aa->F, window);
                        aa->F->size = window;
                        delta[sols[j]]--;
                        /* shift this column */
                        for(unsigned int k = 1 ; k < window ; k++) {
                            matpoly_extract_column(ab,
                                    aa->F, j, k-1, aa->F, j, k);
                        }
                        matpoly_zero_column(ab, aa->F, j, window - 1);
                        break;
                    }
                }
            }
            /* coefficient of degree maxdelta-kpi-window is now complete */
            if (kpi + window <= maxdelta) {
                bm_io_write_one_F_coeff(aa, (maxdelta-kpi) - window);
                bm_io_clear_one_F_coeff(aa, (maxdelta-kpi) - window);
            }
            /* Now use our coefficient. This might tinker with
             * coefficients up to degree kpi-(window-1) in the file F */

            if (kpi > maxdelta + aa->t0 ) {
                /* this implies that we always have kF > delta[jpi],
                 * whence we expect a zero contribution */
                for(unsigned int i = 0 ; i < m + n ; i++) {
                    for(unsigned int jF = 0 ; jF < n ; jF++) {
                        unsigned int jpi = sols[jF];
                        absrc_elt src = matpoly_coeff_const(ab, pi, i, jpi, s);
                        ASSERT_ALWAYS(abis_zero(ab, src));
                    }
                }
                continue;
            }

            for(unsigned int i = 0 ; i < n ; i++) {
                for(unsigned int jF = 0 ; jF < n ; jF++) {
                    unsigned int jpi = sols[jF];
                    unsigned int subtract = maxdelta - delta[jpi];
                    ASSERT(subtract < window);
                    if ((maxdelta-kpi) < subtract) continue;
                    unsigned int kF = (maxdelta - kpi) - subtract;
                    absrc_elt src = matpoly_coeff_const(ab, pi, i, jpi, s);
                    abdst_elt dst = matpoly_coeff(ab, aa->F, i, jF, kF % window);
                    ASSERT_ALWAYS(kF <= delta[jpi] || abis_zero(ab, src));
                    abadd(ab, dst, dst, src);
                }
            }
            for(unsigned int ipi = n ; ipi < m + n ; ipi++) {
                for(unsigned int jF = 0 ; jF < n ; jF++) {
                    unsigned int jpi = sols[jF];
                    unsigned int iF = aa->fdesc[ipi-n][1];
                    unsigned int offset = aa->t0 - aa->fdesc[ipi-n][0];
                    unsigned int subtract = maxdelta - delta[jpi] + offset;
                    ASSERT(subtract < window);
                    if ((maxdelta-kpi) < subtract) continue;
                    unsigned int kF = (maxdelta-kpi) - subtract;
                    abdst_elt dst = matpoly_coeff(ab, aa->F, iF, jF, kF % window);
                    absrc_elt src = matpoly_coeff_const(ab, pi, ipi, jpi, s);
                    ASSERT_ALWAYS(kF <= delta[jpi] || abis_zero(ab, src));
                    abadd(ab, dst, dst, src);
                }
            }
        }

        if (!rank && (maxdelta-kpi) > next_report_k) {
            double tt = wct_seconds();
            if (tt > next_report_t) {
                printf(
                        "Written %u coefficients (%.1f%%) in %.1f s\n",
                        (maxdelta-kpi), 100.0 * (maxdelta-kpi) / pilen,
                        tt-tt0);
                next_report_t = tt + 10;
                next_report_k = (maxdelta-kpi) + pilen/ 100;
            }
        }
    }
    /* flush the pipe */
    if (!rank && kpi + window <= maxdelta) {
        for(unsigned int s = window ; s-- > 0 ; kpi--)
            bm_io_write_one_F_coeff(aa, maxdelta - kpi - window);
    }

    matpoly_clear(ab, pi);

    free(sols);
}/*}}}*/

/* read 1 coefficient into the sliding window of input coefficients of
 * the input series A. The io_window parameter controls the size of the
 * sliding window. There are in fact two behaviours:
 *  - io_window == 0: there is no sliding window, really, and the new
 *    coefficient is appended as the last coefficient of aa->A.
 *  - io_window > 0: there, we really have a sliding window. Coeffs
 *    occupy places in a circular fashion within the buffer.
 */
int bm_io_read1(bm_io_ptr aa, unsigned int io_window)/*{{{*/
{
    bmstatus_ptr bm = aa->bm;
    dims *d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    matpoly_ptr A = aa->A;
    static unsigned int generated_random_coefficients = 0;

    unsigned int pos = aa->k;
    if (io_window) {
        pos = pos % io_window;
        ASSERT_ALWAYS(A->size == io_window);
    } else {
        ASSERT_ALWAYS(A->size == aa->k);
        if (aa->k >= A->alloc) {
            matpoly_realloc(aa->bm->d->ab, A, A->alloc + 1);
        }
        A->size++;
    }
    if (random_input_length) {
        if (generated_random_coefficients >= random_input_length)
            return 0;

        for (unsigned int i = 0; i < m ; i++) {
            for (unsigned int j = 0; j < n ; j++) {
                abdst_elt x = matpoly_coeff(ab, A, i, j, pos);
                abrandom(ab, x, rstate);
            }
        }
        generated_random_coefficients++;
        aa->k++;
        return 1;
    }

    for (unsigned int i = 0; i < m ; i++) {
        for (unsigned int j = 0; j < n ; j++) {
            abdst_elt x = matpoly_coeff(ab, A, i, j, pos);
            int rc;
            if (aa->ascii) {
                rc = abfscan(ab, aa->f, x);
                rc = rc == 1;
            } else {
                size_t elemsize = abvec_elt_stride(ab, 1);
                rc = fread(x, elemsize, 1, aa->f);
                rc = rc == 1;
                abnormalize(ab, x);
            }
            if (!rc) {
                if (i == 0 && j == 0) {
                    return 0;
                }
                fprintf(stderr,
                        "Parse error while reading coefficient (%d,%d,%d)%s\n",
                        i, j, 1 + aa->k,
                        aa->ascii ? "" : " (forgot --ascii?)");
                exit(EXIT_FAILURE);
            }
        }
    }
    aa->k++;
    return 1;
}/*}}}*/

void bm_io_init(bm_io_ptr aa, bmstatus_ptr bm, const char * input_file, const char * output_file, int ascii)/*{{{*/
{
    memset(aa, 0, sizeof(*aa));
    aa->bm = bm;
    aa->ascii = ascii;
    aa->input_file = input_file;
    aa->output_file = output_file;
}/*}}}*/

void bm_io_begin_read(bm_io_ptr aa)/*{{{*/
{
    bmstatus_ptr bm = aa->bm;
    dims *d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (rank) return;

    matpoly_init(ab, aa->A, m, n, 1);

    if (random_input_length)
        return;

    aa->f = fopen(aa->input_file, aa->ascii ? "r" : "rb");

    DIE_ERRNO_DIAG(aa->f == NULL, "fopen", aa->input_file);
    aa->iobuf = malloc(2 * io_block_size);
    setbuffer(aa->f, aa->iobuf, 2 * io_block_size);

    /* read and discard the first coefficient */
    if (!bm_io_read1(aa, 0)) {
        fprintf(stderr, "Read error from %s\n", aa->input_file);
        exit(EXIT_FAILURE);
    }
    /* discard ! */
    aa->A->size = aa->k = 0;
}/*}}}*/

void bm_io_end_read(bm_io_ptr aa)/*{{{*/
{
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (rank) return;
    matpoly_clear(aa->bm->d->ab, aa->A);
    if (random_input_length) return;
    fclose(aa->f);
    aa->f = NULL;
    free(aa->iobuf);
    aa->iobuf = 0;
}/*}}}*/

void bm_io_begin_write(bm_io_ptr aa)/*{{{*/
{
    bmstatus_ptr bm = aa->bm;
    dims *d = bm->d;
    abdst_field ab = d->ab;
    unsigned int n = d->n;
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (rank) return;

    matpoly_init(ab, aa->F, n, n, aa->t0 + 1);

    if (random_input_length) {
        /* It's horrible. I really want "checksum" to be printed at the
         * exact right moment, hence the following hack...  */
        aa->f = popen("(echo \"checksum:\" ; sha1sum) | xargs echo", "w");
    } else {
        aa->f = fopen(aa->output_file, aa->ascii ? "w" : "wb");
    }
    DIE_ERRNO_DIAG(aa->f == NULL, "fopen", aa->output_file);
    if (!random_input_length) {
        aa->iobuf = malloc(2 * io_block_size);
        setbuffer(aa->f, aa->iobuf, 2 * io_block_size);
    }
}/*}}}*/

void bm_io_end_write(bm_io_ptr aa)/*{{{*/
{
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (rank) return;
    matpoly_clear(aa->bm->d->ab, aa->F);
    if (random_input_length) {
        pclose(aa->f);
    } else {
        fclose(aa->f);
    }
    aa->f = NULL;
    free(aa->iobuf);
    aa->iobuf = 0;
}/*}}}*/

void bm_io_clear(bm_io_ptr aa)/*{{{*/
{
    if (aa->fdesc) free(aa->fdesc);
    memset(aa, 0, sizeof(*aa));
}/*}}}*/

void bm_io_guess_length(bm_io_ptr aa)/*{{{*/
{
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    abdst_field ab = d->ab;
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);

    if (random_input_length) {
        aa->guessed_length = random_input_length;
        return;
    }

    if (!rank) {
        struct stat sbuf[1];
        int rc = fstat(fileno(aa->f), sbuf);
        if (rc < 0) {
            perror(aa->input_file);
            exit(EXIT_FAILURE);
        }

        size_t filesize = sbuf->st_size;

        if (!aa->ascii) {
            size_t avg = avg_matsize(ab, m, n, aa->ascii);
            if (filesize % avg) {
                fprintf(stderr, "File %s has %zu bytes, while its size should be amultiple of %zu bytes (assuming binary input; perhaps --ascii is missing ?).\n", aa->input_file, filesize, avg);
                exit(EXIT_FAILURE);
            }
            aa->guessed_length = filesize / avg;
        } else {
            double avg = avg_matsize(ab, m, n, aa->ascii);
            double expected_length = filesize / avg;
            if (!rank)
                printf("Expect roughly %.2f items in the sequence.\n", expected_length);

            /* First coefficient is always lighter, so we add a +1. */
            aa->guessed_length = 1 + expected_length;
        }
#if 0
        /* we don't have the struct bw at hand here... */
        if (bw->end || bw->start) {
            printf("(Note: from bw parameters, we expect %u).\n",
                    bw->end - bw->start);
        }
        printf(".\n");
#endif
    }
    MPI_Bcast(&(aa->guessed_length), 1, MPI_UNSIGNED, 0, aa->bm->com[0]);
}/*}}}*/

void bm_io_compute_initial_F(bm_io_ptr aa) /*{{{ */
{
    bmstatus_ptr bm = aa->bm;
    dims *d = bm->d;
    abdst_field ab = d->ab;
    unsigned int m = d->m;
    unsigned int n = d->n;
    matpoly_ptr A = aa->A;

    unsigned int (*fdesc)[2] = malloc(2 * m * sizeof(unsigned int));

    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);
    if (!rank) {
        /* read the first few coefficients. Expand A accordingly as we are
         * doing the read */

        ASSERT(A->m == m);
        ASSERT(A->n == n);

        abelt tmp;
        abinit(ab, &tmp);

        /* First try to create the initial F matrix */
        printf("Computing t0\n");

        /* We want to create a full rank m*m matrix M, by extracting columns
         * from the first coefficients of A */

        matpoly M;
        matpoly_init(ab, M, m, m, 1);
        M->size = 1;

        /* For each integer i between 0 and m-1, we have a column, picked
         * from column cnum[i] of coeff exponent[i] of A which, once reduced modulo
         * the other ones, has coefficient at row pivots[i] unequal to zero.
         */
        unsigned int *pivots = malloc(m * sizeof(unsigned int));
        unsigned int *exponents = malloc(m * sizeof(unsigned int));
        unsigned int *cnum = malloc(m * sizeof(unsigned int));
        unsigned int r = 0;

        for (unsigned int k = 0; r < m ; k++) {
            /* read a new coefficient */
            bm_io_read1(aa, 0);

            for (unsigned int j = 0; r < m && j < n; j++) {
                /* Extract a full column into M */
                matpoly_extract_column(ab, M, r, 0, A, j, k);

                /* Now reduce it modulo all other columns */
                for (unsigned int v = 0; v < r; v++) {
                    unsigned int u = pivots[v];
                    /* the v-th column in the M is known to
                     * kill coefficient u (more exactly, to have a -1 as u-th
                     * coefficient, and zeroes for the other coefficients
                     * referenced in the pivots[0] to pivots[v-1] indices).
                     */
                    /* add M[u,r]*column v of M to column r of M */
                    for(unsigned int i = 0 ; i < m ; i++) {
                        if (i == u) continue;
                        abmul(ab, tmp,
                                  matpoly_coeff(ab, M, i, v, 0),
                                  matpoly_coeff(ab, M, u, r, 0));
                        abadd(ab, matpoly_coeff(ab, M, i, r, 0),
                                  matpoly_coeff(ab, M, i, r, 0),
                                  tmp);
                    }
                    abset_zero(ab,
                            matpoly_coeff(ab, M, u, r, 0));
                }
                unsigned int u = 0;
                for( ; u < m ; u++) {
                    if (abcmp_ui(ab, matpoly_coeff(ab, M, u, r, 0), 0) != 0)
                        break;
                }
                if (u == m) {
                    printf("[X^%d] A, col %d does not increase rank (still %d)\n",
                           k, j, r);

                    /* we need at least m columns to get as starting matrix
                     * with full rank. Given that we have n columns per
                     * coefficient, this means at least m/n matrices.
                     */

                    if (k * n > m + 40) {
                        printf("The choice of starting vectors was bad. "
                               "Cannot find %u independent cols within A\n", m);
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }

                /* Bingo, it's a new independent col. */
                pivots[r] = u;
                cnum[r] = j;
                exponents[r] = k;

                /* Multiply the column so that the pivot becomes -1 */
                int rc = abinv(ab, tmp, matpoly_coeff(ab, M, u, r, 0));
                if (!rc) {
                    fprintf(stderr, "Error, found a factor of the modulus: ");
                    abfprint(ab, stderr, tmp);
                    fprintf(stderr, "\n");
                    exit(EXIT_FAILURE);
                }
                abneg(ab, tmp, tmp);
                for(unsigned int i = 0 ; i < m ; i++) {
                    abmul(ab, matpoly_coeff(ab, M, i, r, 0),
                              matpoly_coeff(ab, M, i, r, 0),
                              tmp);
                }

                r++;

                // if (r == m)
                    printf
                        ("[X^%d] A, col %d increases rank to %d (head row %d)\n",
                         aa->k, j, r, u);
            }
        }

        if (r != m) {
            printf("This amount of data is insufficient. "
                   "Cannot find %u independent cols within A\n", m);
            exit(EXIT_FAILURE);
        }

        aa->t0 = exponents[r - 1] + 1;
        ASSERT_ALWAYS(aa->t0 == aa->k);

        printf("Found satisfying init data for t0=%d\n", aa->t0);

        /* We've also got some adjustments to make: room for one extra
         * coefficient is needed in A. Reading of further coefficients will
         * pickup where they stopped, and will always leave the last t0+1
         * coefficients readable. */
        matpoly_realloc(ab, A, aa->t0 + 1);
        A->size++;


        for(unsigned int j = 0 ; j < m ; j++) {
            fdesc[j][0] = exponents[j];
            fdesc[j][1] = cnum[j];
            ASSERT_ALWAYS(exponents[j] < aa->t0);
        }
        free(pivots);
        free(exponents);
        free(cnum);
        matpoly_clear(ab, M);
        abclear(ab, &tmp);
    }
    aa->fdesc = fdesc;
    MPI_Bcast(aa->fdesc, 2*m, MPI_UNSIGNED, 0, aa->bm->com[0]);
    MPI_Bcast(&(aa->t0), 1, MPI_UNSIGNED, 0, aa->bm->com[0]);
    bm->t = aa->t0;
}				/*}}} */

void bm_io_compute_E(bm_io_ptr aa, bigmatpoly_ptr xE)/*{{{*/
{
    // F0 is exactly the n x n identity matrix, plus the
    // X^(s-exponent)e_{cnum} vectors. fdesc has the (exponent, cnum)
    // pairs
    bmstatus_ptr bm = aa->bm;
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    abdst_field ab = d->ab;
    int rank;
    MPI_Comm_rank(aa->bm->com[0], &rank);

    unsigned int guess = aa->guessed_length;

    size_t safe_guess = guess;

    if (aa->ascii) {
        /* The 5% are here really only to take into account the deviations,
         * but we don't expect much */
        safe_guess = ceil(1.05 * guess);
    }

    ASSERT(bigmatpoly_check_pre_init(xE));
    bigmatpoly_finish_init(ab, xE, m, b, safe_guess);

    /* Decide on the temp storage size */
    double avg = avg_matsize(ab, m, n, aa->ascii);
    unsigned int B = iceildiv(io_block_size, avg);
    if (!rank) {
        if (aa->input_file)
            printf("Reading A from %s\n", aa->input_file);
        printf("Reading A by blocks of %u coefficients (%.1f bytes each)\n",
                B, avg);
    }
    /* This is only temp storage ! */
    matpoly E;
    matpoly_init(ab, E, m, b, B);

    unsigned int window = aa->t0 + 1;

    double tt0 = wct_seconds();
    double next_report_t = tt0 + 10;
    unsigned next_report_k = guess / 100;

    unsigned int kE = 0;

    for(unsigned int kE0 = 0 ; kE0 + aa->t0 < safe_guess ; kE0 += B) {
        /* Setting E->size is rather artificial, since we use E essentially
         * as an area where we may write coefficients freely. The only aim is
         * to escape some safety checks involving ->size in matpoly_part */
        matpoly_zero(ab, E);
        E->size = B;

        kE = kE0;

        if (!rank) {
            for(unsigned int s = 0 ; s < B ; s++, kE++) {
                if (!bm_io_read1(aa, window)) {
                    /* the +1 is because we're playing a nasty trick by
                     * forcibly discarding the first coefficient in the
                     * sequence. Just don't convey bad information... */
                    printf("EOF met after reading %u coefficients\n",
                            aa->k + 1);
                    break;
                }

                if (kE + aa->t0 > safe_guess) {
                    fprintf(stderr, "Going way past guessed length%s ???\n", aa->ascii ? " (more than 5%%)" : "");
                }

                for(unsigned int j = 0 ; j < n ; j++) {
                    unsigned int kA = kE + aa->t0;
                    matpoly_extract_column(ab, E, j, s, aa->A, j, kA % window);
                }
                for(unsigned int jE = n ; jE < m + n ; jE++) {
                    unsigned int jA = aa->fdesc[jE-n][1];
                    unsigned int offset = aa->fdesc[jE-n][0];
                    unsigned int kA = kE + offset;
                    matpoly_extract_column(ab, E, jE, s, aa->A, jA, kA % window);
                }
            }
        }
        MPI_Bcast(&(aa->k), 1, MPI_UNSIGNED, 0, aa->bm->com[0]);
        MPI_Bcast(&(kE), 1, MPI_UNSIGNED, 0, aa->bm->com[0]);
        E->size = kE - kE0;

        bigmatpoly_scatter_mat_partial(ab, xE, E, kE0, kE - kE0);

        if (!rank) {
            ASSERT_ALWAYS(kE + aa->t0 == aa->k);
            if (aa->k > next_report_k) {
                double tt = wct_seconds();
                if (tt > next_report_t) {
                    printf(
                            "Read %u coefficients (%.1f%%)"
                            " in %.1f s (%.1f MB/s)\n",
                            aa->k, 100.0 * aa->k / guess,
                            tt-tt0, aa->k * avg / (tt-tt0)/1.0e6);
                    next_report_t = tt + 10;
                    next_report_k = aa->k + guess/ 100;
                }
            }
        }
        if (kE < kE0 + B)
            break;
    }
    bigmatpoly_set_size(xE, kE);
    matpoly_clear(ab, E);
}/*}}}*/

void usage()
{
    fprintf(stderr, "Usage: ./plingen [options, to be documented]\n");
    fprintf(stderr,
	    "General information about bwc options is in the README file\n");
    exit(EXIT_FAILURE);
}

void bmstatus_init(bmstatus_ptr bm, unsigned int m, unsigned int n)/*{{{*/
{
    /* Fields will not all be significant right now. The initialization
     * is a long process */
    memset(bm, 0, sizeof(bmstatus));
    bm->d->m = m;
    bm->d->n = n;
    bm->lucky = malloc((m + n) * sizeof(int));
    memset(bm->lucky, 0, (m + n) * sizeof(int));
}/*}}}*/

void bmstatus_clear(bmstatus_ptr bm)/*{{{*/
{
    free(bm->lucky);
    memset(bm, 0, sizeof(bmstatus));
}/*}}}*/


unsigned int count_lucky_columns(bmstatus_ptr bm)/*{{{*/
{
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int b = m + n;
    int luck_mini = expected_pi_length(d, 0);
    MPI_Bcast(bm->lucky, b, MPI_UNSIGNED, 0, bm->com[0]);
    unsigned int nlucky = 0;
    for(unsigned int j = 0 ; j < b ; nlucky += bm->lucky[j++] >= luck_mini) ;
    return nlucky;
}/*}}}*/

int check_luck_condition(bmstatus_ptr bm)/*{{{*/
{
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;
    unsigned int nlucky = count_lucky_columns(bm);

    int rank;
    MPI_Comm_rank(bm->com[0], &rank);

    if (!rank) {
        printf("Number of lucky columns: %u (%u wanted)\n", nlucky, n);
    }

    if (nlucky == n)
        return 1;

    if (!rank) {
        fprintf(stderr, "Could not find the required set of solutions (nlucky=%u)\n", nlucky);
    }
    if (random_input_length) {
        static int once=0;
        if (once++) {
            if (!rank) {
                fprintf(stderr, "Solution-faking loop crashed\n");
            }
            MPI_Abort(bm->com[0], EXIT_FAILURE);
        }
        if (!rank) {
            printf("Random input: faking successful computation\n");
        }
        for(unsigned int j = 0 ; j < n ; j++) {
            bm->lucky[(j * 1009) % (m+n)] = expected_pi_length(d, 0);
        }
        return check_luck_condition(bm);
    }

    return 0;
}/*}}}*/

void display_deltas(bmstatus_ptr bm, unsigned int * delta)/*{{{*/
{
    dims * d = bm->d;
    unsigned int m = d->m;
    unsigned int n = d->n;

    int rank;
    MPI_Comm_rank(bm->com[0], &rank);

    if (!rank) {
        printf("Final, t=%u: delta =", bm->t);
        for(unsigned int j = 0; j < m + n; j++) {
            printf(" %u", delta[j]);
            if (bm->lucky[j] < 0) {
                printf("(*)");
            }
        }
        printf("\n");
    }
}/*}}}*/

void print_node_assignment(MPI_Comm comm)
{
    int rank;
    int size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    struct utsname me[1];
    int rc = uname(me);
    if (rc < 0) { perror("uname"); MPI_Abort(comm, 1); }
    size_t sz = 1 + sizeof(me->nodename);
    char * global = malloc(size * sz);
    memset(global, 0, size * sz);
    memcpy(global + rank * sz, me->nodename, sizeof(me->nodename));

    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
            global, sz, MPI_BYTE, comm);
    if (rank == 0) {
        char name[80];
        int len=80;
        MPI_Comm_get_name(comm, name, &len);
        name[79]=0;
        for(int i = 0 ; i < size ; i++) {
            printf("%s rank %d: %s\n", name, i, global + i * sz);
        }
    }
    free(global);
}


int main(int argc, char *argv[])
{
    bmstatus bm;
    dims * d = bm->d;
    int tune = 0;
    int ascii = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    gmp_randinit_default(rstate);

    /* {{{ Parameter list. Feed the common bw[] struct */
    param_list pl;
    param_list_init(pl);

    param_list_configure_switch(pl, "--tune", &tune);
    param_list_configure_switch(pl, "--ascii", &ascii);
    param_list_configure_switch(pl, "--timings", &with_timings);
    bw_common_init_mpi(bw, pl, &argc, &argv);

    int rank;
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    logline_init_timer();

    param_list_parse_uint(pl, "random-input-with-length", &random_input_length);
    param_list_parse_int(pl, "caching", &caching);

    const char * afile = param_list_lookup_string(pl, "afile");

    if (bw->m == -1) {
	fprintf(stderr, "no m value set\n");
	exit(EXIT_FAILURE);
    }
    if (bw->n == -1) {
	fprintf(stderr, "no n value set\n");
	exit(EXIT_FAILURE);
    }
    if (!tune && !(afile || random_input_length)) {
        fprintf(stderr, "No afile provided\n");
        exit(EXIT_FAILURE);
    }
    if (!param_list_lookup_string(pl, "prime")) {
	fprintf(stderr, "no prime set\n");
	exit(EXIT_FAILURE);
    }

    /* we allow ffile and ffile to be both NULL */
    const char * tmp = param_list_lookup_string(pl, "ffile");
    char * ffile = NULL;
    if (tmp) {
        ffile = strdup(tmp);
    } else if (afile) {
        int rc = asprintf(&ffile, "%s.gen", afile);
        ASSERT_ALWAYS(rc >= 0);
    }
    ASSERT_ALWAYS((afile==NULL) == (ffile == NULL));

#ifndef HAVE_MPIR
    if (caching) {
        fprintf(stderr, "--caching=1 only supported with MPIR\n");
        exit(EXIT_FAILURE);
    }
#endif

    bmstatus_init(bm, bw->m, bw->n);

    unsigned int m = d->m;
    unsigned int n = d->n;
    abdst_field ab = d->ab;

    abfield_init(ab);
    {
	mpz_t p;
	mpz_init_set_ui(p, 2);
	param_list_parse_mpz(pl, "prime", p);
	abfield_specify(ab, MPFQ_PRIME_MPZ, p);
	mpz_clear(p);
    }
    abmpi_ops_init(ab);

    bm->lingen_threshold = 10;
    bm->lingen_mpi_threshold = 1000;
    param_list_parse_uint(pl, "lingen-threshold", &(bm->lingen_threshold));
    param_list_parse_uint(pl, "display-threshold", &(display_threshold));
#ifdef HAVE_MPIR
    param_list_parse_uint(pl, "caching-threshold", &(caching_threshold));
#endif
    param_list_parse_uint(pl, "lingen-mpi-threshold", &(bm->lingen_mpi_threshold));
    param_list_parse_uint(pl, "io-block-size", &(io_block_size));
    {
        unsigned long random_seed = 0;
        param_list_parse_ulong(pl, "random_seed", &random_seed);
        gmp_randseed_ui(rstate, random_seed);
    }
    if (bm->lingen_mpi_threshold < bm->lingen_threshold) {
        bm->lingen_mpi_threshold = bm->lingen_threshold;
        fprintf(stderr, "Argument fixing: setting lingen-mpi-threshold=%u (because lingen-threshold=%u)\n",
                bm->lingen_mpi_threshold, bm->lingen_threshold);
    }
    checkpoint_directory = param_list_lookup_string(pl, "checkpoint-directory");
    param_list_parse_uint(pl, "checkpoint-threshold", &checkpoint_threshold);
    param_list_parse_int(pl, "save_gathered_checkpoints", &save_gathered_checkpoints);



#if defined(FAKEMPI_H_)
    bm->lingen_mpi_threshold = UINT_MAX;
#endif

    /* }}} */

    /* {{{ Parse MPI args. Make bm->com[0] a better mpi communicator */
    bm->mpi_dims[0] = 1;
    bm->mpi_dims[1] = 1;
    param_list_parse_intxint(pl, "mpi", bm->mpi_dims);
    {
        /* Display node index wrt MPI_COMM_WORLD */
        print_node_assignment(MPI_COMM_WORLD);

        /* Reorder all mpi nodes so that each node gets the given number
         * of jobs, but close together.
         */
        int mpi[2] = { bm->mpi_dims[0], bm->mpi_dims[1], };
#ifdef  FAKEMPI_H_
        if (mpi[0]*mpi[1] > 1) {
            fprintf(stderr, "non-trivial option mpi= can't be used with fakempi. Please do an MPI-enabled build (MPI=1)\n");
            exit(EXIT_FAILURE);
        }
#endif
        int thr[2] = {1,1};
        param_list_parse_intxint(pl, "thr", thr);
        /* Asssume numbering in MPI_COMM_WORLD is all mpi jobs from the
         * same node together. So we pick them by bunches of size
         * thr[0]*thr[1].
         */
        if (!rank)
            printf("size=%d mpi=%dx%d thr=%dx%d\n", size, mpi[0], mpi[1], thr[0], thr[1]);
        ASSERT_ALWAYS(size == mpi[0] * mpi[1] * thr[0] * thr[1]);
        /* The mpi and thr command line argument lead to the same number
         * of working processes than with krylov. Except that here, we're
         * not really doing threads, but running real mpi jobs with their
         * own address space (hence mpirun will need a bigger -n
         * argument).  In a sense, here we strive to follow the semantics
         * used in bwc otherwise.  In fact, we should probably use the
         * pi_wiring structures as well, so that we can say that we
         * consistently use the same infrastructure.  For simplicity of
         * the code, we don't.
         */
        bm->mpi_dims[0] *= thr[0];
        bm->mpi_dims[1] *= thr[1];
        if (bm->mpi_dims[0] != bm->mpi_dims[1]) {
            if (!rank)
                fprintf(stderr, "The current plingen code is limited to square splits ; here, we received a %d x %d split, which will not work\n",
                    bm->mpi_dims[0], bm->mpi_dims[1]);
            abort();
        } else if ((m % bm->mpi_dims[0] != 0) || (n % bm->mpi_dims[0] != 0)) {
            if (!rank)
                fprintf(stderr, "The process grid dimensions must divide gcd(m,n)\n");
            abort();
        }
        int tl_grank = rank % (thr[0] * thr[1]); // thread-level global rank
        int tl_irank = tl_grank / thr[1];
        int tl_jrank = tl_grank % thr[1];
        int ml_grank = rank / (thr[0] * thr[1]); // thread-level global rank
        int ml_irank = ml_grank / mpi[1];
        int ml_jrank = ml_grank % mpi[1];
        int irank = ml_irank * thr[0] + tl_irank;
        int jrank = ml_jrank * thr[1] + tl_jrank;
        int newrank = irank * mpi[1] * thr[1] + jrank;

        MPI_Comm_split(MPI_COMM_WORLD, 0, newrank, &(bm->com[0]));
        MPI_Comm_set_name(bm->com[0], "world");

        char commname[12];
        snprintf(commname, 12, "row%d\n", irank);
        MPI_Comm_split(MPI_COMM_WORLD, irank, jrank, &(bm->com[1]));
        MPI_Comm_set_name(bm->com[1], commname);

        snprintf(commname, 12, "col%d\n", jrank);
        MPI_Comm_split(MPI_COMM_WORLD, jrank, irank, &(bm->com[2]));
        MPI_Comm_set_name(bm->com[2], commname);

        print_node_assignment(bm->com[0]);
    }
    /* }}} */


    /* plingen tuning accepts some arguments. We look them up so as to
     * avoid failures down the line */
    param_list_lookup_string(pl, "B");
    param_list_lookup_string(pl, "catchsig");
    
    logline_parse_params(pl);

    if (param_list_warn_unused(pl))
	usage();

    if (tune) {
        plingen_tuning(bm->d->ab, bm->d->m, bm->d->n, bm->com[0], pl);
        MPI_Finalize();
        return 0;
    }


    /* We now have a protected structure for a_reading task which does
     * the right thing concerning parallelism among MPI nodes (meaning
     * that non-root nodes essentially do nothing while the master job
     * does the I/O stuff) */
    bm_io aa;
    bm_io_init(aa, bm, afile, ffile, ascii);
    bm_io_begin_read(aa);
    bm_io_guess_length(aa);

    bm_io_compute_initial_F(aa);

    /* this is somewhat ugly, too */
    unsigned int t0 = bm->t;
    unsigned int * delta = malloc((m + n) * sizeof(unsigned int));
    for(unsigned int j = 0 ; j < m + n ; delta[j++]=t0);

    stats->tree_total_breadth = aa->guessed_length;

    int go_mpi = aa->guessed_length >= bm->lingen_mpi_threshold;

    if (go_mpi && !rank) {
        if (size) {
            printf("Expected length %u exceeds MPI threshold %u, going MPI now.\n", aa->guessed_length, bm->lingen_mpi_threshold);
        } else {
            printf("Expected length %u exceeds MPI threshold %u, but the process is not running  in an MPI context.\n", aa->guessed_length, bm->lingen_mpi_threshold);
        }
    }

    bigmatpoly model;
    bigmatpoly xpi, xE;
    bigmatpoly_init_model(model, bm->com, bm->mpi_dims[0], bm->mpi_dims[1]);
    bigmatpoly_init(ab, xE, model, 0, 0, 0);
    bigmatpoly_init(ab, xpi, model, 0, 0, 0);   /* pre-init for now */

    /* This is a dispatching read */
    bm_io_compute_E(aa, xE);
    bm_io_end_read(aa);

    if (size > 1) {
        bw_biglingen_collective(bm, xpi, xE, delta);
    } else {
        /* We have to gather and scatter, but these are fairly trivial.
         * Avoid copies, and wrap around bw_lingen_single. */
        matpoly E, pi;
        matpoly_init(ab, E, 0, 0, 0);
        matpoly_init(ab, pi, 0, 0, 0);
        matpoly_swap(E, bigmatpoly_my_cell(xE));
        bw_lingen_single(bm, pi, E, delta);
        matpoly_swap(E, bigmatpoly_my_cell(xE));
        bigmatpoly_finish_init(ab, xpi, m + n, m + n, 1);
        xpi->size = pi->size;
        matpoly_swap(pi, bigmatpoly_my_cell(xpi));
        matpoly_clear(ab, E);
        matpoly_clear(ab, pi);
    }

    display_deltas(bm, delta);
    if (!rank) printf("(pi->alloc = %zu)\n", bigmatpoly_my_cell(xpi)->alloc);

    if (check_luck_condition(bm)) {
        /* this is a gathering write */
        bm_io_begin_write(aa);

        /* this reallocates F */
        bm_io_set_write_behind_size(aa, delta);
        bm_io_compute_final_F(aa, xpi, delta);
        bm_io_end_write(aa);
    }

    if (!rank) {
        printf("t_basecase = %.2f\n", bm->t_basecase);
        printf("t_mp = %.2f\n", bm->t_mp);
        printf("t_mul = %.2f\n", bm->t_mul);
        printf("t_cp_io = %.2f\n", bm->t_cp_io);
    }

    /* clear everything */
    bigmatpoly_clear(ab, xE);
    bigmatpoly_clear(ab, xpi);
    bigmatpoly_clear_model(model);
    bm_io_clear(aa);
    free(delta);
    abmpi_ops_clear(ab);
    abfield_clear(ab);
    bmstatus_clear(bm);
    bw_common_clear(bw);
    param_list_clear(pl);
    if (ffile) free(ffile);

    gmp_randclear(rstate);

    MPI_Finalize();
    return 0;
}

/* vim:set sw=4 sta et: */
