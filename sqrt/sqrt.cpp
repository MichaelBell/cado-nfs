/* square root, it can be used in two ways:

 * either do everything in one call:

   sqrt -poly cxxx.poly -prefix cxxx.dep.gz -purged cxxx.purged.gz -index cxxx.index.gz -ker cxxx.kernel

 * or in two steps:

   sqrt -poly cxxx.poly -prefix cxxx.dep.gz -purged cxxx.purged.gz -index cxxx.index.gz -ker cxxx.kernel -ab
   sqrt -poly cxxx.poly -prefix cxxx.dep.gz -sqrt0 -sqrt1 -gcd
 */

#include "cado.h" // IWYU pragma: keep
/* the following avoids the warnings "Unknown pragma" if OpenMP is not
   available, and should come after cado.h, which sets -Werror=all */
#ifdef  __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <cstdint>     /* AIX wants it first (it's a bug) */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <cmath> /* for log */
#include <cerrno>
#include <sys/stat.h>
#include <pthread.h>
#include <gmp.h>
#include <mutex>
#include <string>
#include <vector>
#include <utility> // pair
#include "cado_poly.h"  // cado_poly
#include "cxx_mpz.hpp"   // for cxx_mpz
#include "filter_io.h"  // filter_rels
#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/printf.h" // fmt::fprintf // IWYU pragma: keep
#include "gmp_aux.h"
#include "getprime.h"  // for getprime_mt, prime_info_clear, prime_info_init
#include "gzip.h"       // fopen_maybe_compressed
#include "memusage.h"   // PeakMemusage
#include "modul_poly.h" // modul_poly
#include "mpz_poly.h"   // mpz_poly
#include "omp_proxy.h"
#include "purgedfile.h" // purgedfile_read_firstline
#include "version_info.h" // cado_revision_string
#include "portability.h" // strndup // IWYU pragma: keep
#include "macros.h"
#include "params.h"
#include "timing.h"

/* frequency of messages "read xxx (a,b) pairs" */
#define REPORT 10000000

/* maximal number of threads when reading dependency files */
#define MAX_IO_THREADS 16

static int verbose = 0;
static double wct0;

std::mutex stdio_guard;

struct cxx_mpz_polymod_scaled {
  cxx_mpz_poly p;
  unsigned long v;
  cxx_mpz_polymod_scaled(int deg) : p(deg), v(0) {}
  cxx_mpz_polymod_scaled() = default;
  cxx_mpz_polymod_scaled(cxx_mpz_polymod_scaled const &) = delete;
  cxx_mpz_polymod_scaled(cxx_mpz_polymod_scaled &&) = default;
  cxx_mpz_polymod_scaled& operator=(cxx_mpz_polymod_scaled const &) = delete;
  cxx_mpz_polymod_scaled& operator=(cxx_mpz_polymod_scaled &&) = default;
};

/* Pseudo-reduce a plain polynomial p modulo a non-monic polynomial F.
   The result is of type cxx_mpz_polymod_scaled P, and satisfies:
   P->p = lc(F)^(P->v) * p mod F.
   WARNING: this function destroys its input p !!! */
static void
cxx_mpz_polymod_scaled_reduce(cxx_mpz_polymod_scaled & P, cxx_mpz_poly & p, cxx_mpz_poly const & F) {
  unsigned long v = 0;

  if (p->deg < F->deg) {
    mpz_poly_set(P.p, p);
    P.v = 0;
    return;
  }

  const int d = F->deg;

  while (p->deg >= d) {
    const int k = p->deg;

    /* We compute F[d]*p - p[k]*F. In case F[d] divides p[k], we can simply
       compute p - p[k]/F[d]*F. However this will happen rarely with
       Kleinjung's polynomial selection, since lc(F) is large. */

    /* FIXME: in msieve, Jason Papadopoulos reduces by F[d]^d*F(x/F[d])
       instead of F(x). This might avoid one of the for-loops below. */

    // temporary hack: account for the possibility that we're indeed
    // using f_hat instead of f.
    if (mpz_cmp_ui(F->coeff[d], 1) != 0) {
      v++; /* we consider p/F[d]^v */
#pragma omp parallel for
      for (int i = 0; i < k; ++i)
        mpz_mul (p->coeff[i], p->coeff[i], F->coeff[d]);
    }

#pragma omp parallel for
    for (int i = 0; i < d; ++i)
      mpz_submul (p->coeff[k-d+i], p->coeff[k], F->coeff[i]);

    mpz_poly_cleandeg (p, k-1);
  }

  mpz_poly_set(P.p, p);
  P.v = v;
}

/* Set Q=P1*P2 (mod F). Warning: Q might equal P1 (or P2). */
void
cxx_mpz_polymod_scaled_mul (cxx_mpz_polymod_scaled & Q, cxx_mpz_polymod_scaled const & P1, cxx_mpz_polymod_scaled const & P2,
                   cxx_mpz_poly const & F) {
  unsigned long v;

  /* beware: if P1 and P2 are zero, P1.p->deg + P2.p->deg = -2 */
  cxx_mpz_poly prd ((P1.p->deg == -1) ? -1 : P1.p->deg + P2.p->deg);

  ASSERT_ALWAYS(mpz_poly_normalized_p (P1.p));
  ASSERT_ALWAYS(mpz_poly_normalized_p (P2.p));

  mpz_poly_mul (prd, P1.p, P2.p);
  v = P1.v + P2.v;
  ASSERT_ALWAYS(v >= P1.v); /* no overflow */

  cxx_mpz_polymod_scaled_reduce (Q, prd, F);
  Q.v += v;
  ASSERT_ALWAYS(Q.v >= v); /* no overflow */
}

static char*
get_depname (const char *prefix, const char *algrat, int numdep)
{
  char *depname;
  const char* suffixes[] = {".gz", ".bz2", ".lzma", ""};
  const char *suffix;
  char *prefix_base;
  int ret;

  for (int i = 0; strlen (suffix = suffixes[i]) != 0; i++)
    if (strcmp (prefix + strlen (prefix) - strlen (suffix), suffix) == 0)
      break;
  prefix_base = strndup(prefix, strlen (prefix) - strlen (suffix));
  ASSERT_ALWAYS(prefix_base != NULL);
  ret = asprintf (&depname, "%s.%s%03d%s", prefix_base, algrat, numdep, suffix);
  ASSERT_ALWAYS(ret > 0);
  free (prefix_base);
  return depname;
}

static char*
get_depsidename (const char *prefix, int numdep, int side)
{
  char S[10];
  snprintf(S, 10, "side%d.", side);
  return get_depname (prefix, S, numdep);
}

static FILE*
fopen_maybe_compressed_lock (const char * name, const char * mode)
{
  std::lock_guard<std::mutex> dummy(stdio_guard);
  return fopen_maybe_compressed (name, mode);
}

static int
fclose_maybe_compressed_lock (FILE * f, const char * name)
{
  std::lock_guard<std::mutex> dummy(stdio_guard);
  return fclose_maybe_compressed (f, name);
}

/* this function is run sequentially, thus no need to be thread-safe */
static int
check_dep (const char *prefix, int numdep)
{
  char *depname;
  FILE *depfile;

  depname = get_depname (prefix, "", numdep);
  depfile = fopen (depname, "r");
  free (depname);
  if (depfile == NULL)
    return 0;
  fclose (depfile);
  return 1;
}

/*************************************************************************/
/* Function to handle the initial accumulation (rat & alg) */


/* replace the vector of elements of type T by
 * the product of elements in the range. It's fairly trivial to do this
 * in a DFS manner, but it's slightly less so for a BFS algorithm. The
 * latter is more amenable to parallelization.
 *
 * The most balanced split is of course when the size of the vector v is a
 * power of two. When the size of the vector v is N=2^k+r, the optimal way
 * to fall back to the balanced case is as follows. Form a vector w of
 * length 2^k by moving all elements of the vector v to the vector w, one
 * by one, except that the i-th element of w is created from *two*
 * consecutive elements of v whenever the bit-reversal of i is less than
 * r.
 *
 * Of course we prefer to create w from v in place, which is done easily
 * by having two pointers go through v.
 */
static inline uint64_t bitrev(uint64_t a)
{
    a = (a >> 32) ^ (a << 32);
    uint64_t m;
    m = UINT64_C(0x0000ffff0000ffff);
    a = ((a >> 16) & m) ^ ((a << 16) & ~m);
    m = UINT64_C(0x00ff00ff00ff00ff);
    a = ((a >> 8) & m) ^ ((a << 8) & ~m);
    m = UINT64_C(0x0f0f0f0f0f0f0f0f);
    a = ((a >> 4) & m) ^ ((a << 4) & ~m);
    m = UINT64_C(0x3333333333333333);
    a = ((a >> 2) & m) ^ ((a << 2) & ~m);
    m = UINT64_C(0x5555555555555555);
    a = ((a >> 1) & m) ^ ((a << 1) & ~m);
    return a;
}

/* Modify the range [vb, ve[ so that in the end, *vb contains the product
 * of the range, and [vb+1, ve[ contains only ones.
 *
 * The array A is used internally by this function, but we provide a
 * prototype that exposes it because two consecutive calls to this
 * function with similar-size ranges can profitably re-use the storage in
 * A.
 */
template<typename M>
void accumulate(std::vector<typename M::T> & A, typename std::vector<typename M::T>::iterator vb, typename std::vector<typename M::T>::iterator ve, M const & m)
{
    /* This is a single-threaded routine. We put the result in *vb, and
     * the caller is reponsible of freeing the range [vb+1,ve[
     */
    if (ve - vb < 16) {
        /* Don't bother with very small vectors. Here we have a vector of
         * size less than 16*2*thr/2, so don't bother. */
        for(typename std::vector<typename M::T>::iterator vi = vb+1; vi < ve ; ++vi) {
            m(*vb, *vb, *vi);
        }
        return;
    }
    constexpr const unsigned int ratio = 2; /* must be an integer > 1 */
    /* create an array of products of exponentially growing size.
     *
     * after each input value considered, we maintain the property that
     * the i-th cell in the vector A[] contains the product of at most
     * ratio^i items in the original range [vb, ve[
     *
     * Hence with n cells int the table A, we can have up to
     * (ratio^n-1)/(ratio-1) values stored.
     */
    size_t as = 1 + (ratio - 1) * (ve - vb);
    unsigned int ncells = 0;
    for(size_t s = 1 ; s < as ; s *= ratio, ncells++) ;
    /* make sure A is large enough */
    for(; A.size() < ncells; A.emplace_back());
    for(auto& x: A) m.set1(x);

    for(typename std::vector<typename M::T>::iterator vi = vb ; vi < ve ; ++vi) {
        m(A[0], A[0], *vi);
        /* We won't need *vi again. This frees indirect storage */
        *vi = typename M::T();
        /* now maintain the condition */
        unsigned int nprd = vi + 1 - vb;
        for(unsigned int i = 0 ; nprd % ratio == 0 ; ++i, nprd /= ratio) {
            ASSERT_ALWAYS(i + 1 < A.size());
            m(A[i+1], A[i+1], A[i]);
            /* Do not replace by a fresh object. Prefer something that
             * does not trigger a free() yet, since reallocation would
             * occur eventually */
            m.set1(A[i]);
        }
    }
    /* final: accumulate everything in A.back(). Note that by doing it
     * this way, we should normally not trigger reallocation. */
    for(unsigned int i = 0 ; i + 1 < A.size() ; i++) {
        if (m.is1(A[i+1])) {
            // it is tempting to *swap* here, but in fact it would be a
            // terrible idea, since we're being careful to keep the
            // *storage* of A[] grow exponentially. With swap's, we would
            // in effect make all cells stabilize at the maximal storage,
            // which would incur a significant increase of the total
            // storage amount!
            // std::swap(A[i], A[i+1]);
            m.set(A[i+1], A[i]);
            m.set1(A[i]);
        } else {
            m(A[i+1], A[i+1], A[i]);
            // do not free A[i] yet, as we might want to reuse it for another
            // block. Set it to 1 instead.
            // A[i]=T();
            m.set1(A[i]);
        }
    }
    *vb = std::move(A.back());
}
/* not used, just to mention that the simplest prototype for the function
 * above would be as follows, in a way */
template<typename M>
void accumulate(typename std::vector<typename M::T>::iterator vb, typename std::vector<typename M::T>::iterator ve, M const & m)
{
    std::vector<typename M::T> A;
    accumulate(A, vb, ve, m);
}
/* This does the "floor" level. From a vector of T's or arbitrary size,
 * return a modified vector whose size is a power of two, and whose
 * elements are products of the original elements in sub-ranges of the
 * initial vector */
template<typename M>
void accumulate_level00(std::vector<typename M::T> & v, M const & m, std::string const & message)
{
    unsigned int nthr;
    /* We want to know the number of threads that we get when starting a
     * parallel region -- but this is probably unneeded. I think that
     * omp_get_max_threads would do an equally good job. */
    #pragma omp parallel
    #pragma omp critical
    nthr = omp_get_num_threads (); /* total number of threads */

    size_t N = v.size();
    /* We're going to split the input in that 1<<n pieces exacly
     * -- we want a power of two, that is a strict condition. We'll
     *  strive to make pieces of balanced size.
     */
    /* min_pieces per_thread is there so that each thread at level 0 has
     * at least that many pieces to work on. Increasing this parameter
     * allows more progress reporting, but more importantly this acts on
     * the balance of the computation. (having "2 or 3" pieces to handle
     * per threads leads to longer idle wait than if we have "16 or 17"
     * pieces to handle, of course)
     */
    constexpr const unsigned int min_pieces_per_thread = 8;
    unsigned int n = 1;
    for( ; (1UL << n) < min_pieces_per_thread * nthr ; n++) ;

    if ((N >> n) < 16) {
        /* Don't bother with very small vectors. Here we have a vector of
         * size less than 16*min_pieces_per_thread*thr/2, so don't bother. */
        for(size_t j = 1 ; j < v.size() ; j++) {
            /* use the instance with no multithreading. This is on
             * purpose, for small ranges and small objects.
             */
            m(v[0], v[0], v[j]);
        }
        v.erase(v.begin() + 1, v.end());
        return;
    }

    uint64_t r = N % (1UL << n);
    {
        fmt::fprintf (stderr, "%s: starting level 00 at wct=%1.2fs,"
                " %zu -> 2^%zu*%zu+%zu\n",
                message, wct_seconds () - wct0,
                v.size(), n, N>>n, r);
        fflush (stderr);
    }

    /* Interesting job: we want to compute the start and end points of
     * the i-th part among 1<<n of a vector of size N.
     *
     * we know that this i-th part has size (N>>n)+1 if and only if the
     * n-bit reversal of i is less than N mod (1<<n) (denoted by r)
     */
    std::vector<size_t> endpoints;
    endpoints.push_back(0);
    for(uint64_t i = 0 ; i < (UINT64_C(1) << n) ; i++) {
        uint64_t ir = bitrev(i) >> (64 - n);
        size_t fragment_size = (N>>n) + (ir < (uint64_t) r);
        endpoints.push_back(endpoints.back() + fragment_size);
    }
    ASSERT_ALWAYS(endpoints.back() == v.size());

    /* Now do the real job */
#pragma omp parallel
    {
        /* This array is thread-private, and we're going to re-use it for
         * all sub-ranges that we multiply. The goal at this point is to
         * avoid hammering the malloc layer.
         */
        std::vector<typename M::T> A;
#pragma omp for
        for(unsigned int i = 0 ; i < (1u << n) ; i++) {
            typename std::vector<typename M::T>::iterator vb = v.begin() + endpoints[i];
            typename std::vector<typename M::T>::iterator ve = v.begin() + endpoints[i+1];
            accumulate(A, vb, ve, m);
	      {
                std::lock_guard<std::mutex> dummy(stdio_guard);
                fmt::fprintf (stderr, "%s: fragment %u/%u"
			      " of level 00 done by thread %u at wct=%1.2fs\n",
			      message,
			      i, 1u<<n, omp_get_thread_num(),
			      wct_seconds () - wct0);
                fflush (stderr);
	      }
        }
    }
    /* This puts pressure at the malloc level, but takes negligible time */
    {
        typename std::vector<typename M::T>::iterator dst = v.begin();
        endpoints.pop_back();
        for(size_t z : endpoints)
            std::swap(v[z], *dst++);
        v.erase(dst, v.end());
    }

    N = v.size();

    ASSERT_ALWAYS(!(N & (N-1)));
}


template<typename M>
typename M::T accumulate(std::vector<typename M::T> & v, M const & m, std::string const & message)
{
    accumulate_level00(v, m, message);

    unsigned int nthr;
    /* We want to know the number of threads that we get when starting a
     * parallel region -- but this is probably unneeded. I think that
     * omp_get_max_threads would do an equally good job. */
    #pragma omp parallel
    #pragma omp critical
    nthr = omp_get_num_threads (); /* total number of threads */

    size_t vs = v.size();
    ASSERT_ALWAYS(!(vs & (vs - 1)));

    /* At this point v has size a power of two */
  for(int level = 0 ; v.size() > 1 ; level++) {
      {
        std::lock_guard<std::mutex> dummy(stdio_guard);
	fmt::fprintf (stderr, "%s: starting level %d at cpu=%1.2fs (wct=%1.2fs), %zu values to multiply\n",
		      message, level, seconds (), wct_seconds () - wct0, v.size());
	fflush (stderr);
      }

      double st = seconds (), wct = wct_seconds ();
      size_t N = v.size() - 1;
      int local_nthreads;
      /* the loop below performs floor((N+1)/2) products */
      size_t nloops = (N + 1) / 2;
      if (nthr < 2 * nloops)
	{
	  omp_set_nested (0);
	  local_nthreads = 1;
	}
      else
	{
	  /* we have to set omp_set_nested here and not at the beginning of
	     main(), since it seems that the pthreads reset OpenMP's "nested"
	     value to 0 */
	  omp_set_nested (1);
	  local_nthreads = nthr / nloops;
	}
#pragma omp parallel for
      for(size_t j = 0 ; j < N ; j += 2) {
	  omp_set_num_threads (local_nthreads);
	  m(v[j], v[j], v[j+1]);
          v[j+1] = typename M::T();
      }

      /* reset "nested" to 0 */
      omp_set_nested (0);

      /* shrink (not parallel), takes negligible time */
      for(size_t j = 2 ; j < v.size() ; j += 2) {
          std::swap(v[j], v[j/2]);
      }
      v.erase(v.begin() + (v.size() + 1) / 2, v.end());
      {
        std::lock_guard<std::mutex> dummy(stdio_guard);
	fmt::fprintf (stderr, "%s: level %d took cpu=%1.2fs (wct=%1.2fs)\n",
		      message, level, seconds () - st, wct_seconds () - wct);
	fflush (stderr);
      }
  }
  return std::move(v.front());
}

/*************************************************************************/
/* Parallel I/O for reading the dep file */

template<typename M>
std::vector<typename M::T>
read_ab_pairs_from_depfile(const char * depname, M const & m, std::string const & message, unsigned long & nab, unsigned long & nfree)
{
    nab = nfree = 0;
    FILE * depfile = fopen_maybe_compressed_lock (depname, "rb");
    ASSERT_ALWAYS(depfile != NULL);
    std::vector<typename M::T> prd;

    if (fseek(depfile, 0, SEEK_END) < 0) {
        fmt::fprintf(stderr, "%s: cannot seek in dependency file, using single-thread I/O\n", message);
        cxx_mpz a, b;
        /* Cannot seek: we have to use serial i/o */
        while (gmp_fscanf(depfile, "%Zd %Zd", (mpz_ptr) a, (mpz_ptr) b) != EOF)
        {
            if(!(nab % REPORT)) {
                fmt::fprintf(stderr, "%s: read %lu (a,b) pairs in %.2fs (wct %.2fs, peak %luM)\n",
                        message, nab, seconds (), wct_seconds () - wct0,
                        PeakMemusage () >> 10);
                fflush (stderr);
            }
            if (mpz_cmp_ui (a, 0) == 0 && mpz_cmp_ui (b, 0) == 0)
                break;
            prd.emplace_back(m.from_ab(a, b));
            nab++;
            if (mpz_cmp_ui (b, 0) == 0)
                nfree++;
        }
    } else {
        /* We _can_ seek. Good news ! */
        off_t endpos = ftell(depfile);
        /* Find accurate starting positions for everyone */
        unsigned int nthreads = omp_get_max_threads();
        /* cap the number of I/O threads */
        if (nthreads > MAX_IO_THREADS)
            nthreads = MAX_IO_THREADS;
        fmt::fprintf(stderr, "%s: Doing I/O with %u threads\n", message, nthreads);
        std::vector<off_t> spos_tab;
        for(unsigned int i = 0 ; i < nthreads ; i++)
            spos_tab.push_back((endpos * i) / nthreads);
        spos_tab.push_back(endpos);
        /* All threads get their private reading head. */
#pragma omp parallel num_threads(nthreads)
        {
            int i = omp_get_thread_num();
            FILE * fi = fopen_maybe_compressed_lock (depname, "rb");
            int rc = fseek(fi, spos_tab[i], SEEK_SET);
            ASSERT_ALWAYS(rc == 0);
            if (i > 0) {
                /* Except when we're at the end of the stream, read until
                 * we get a newline */
                for( ; fgetc(fi) != '\n' ; ) ;
            }
            spos_tab[i] = ftell(fi);
#pragma omp barrier
            std::vector<typename M::T> loc_prd;
            unsigned long loc_nab = 0;
            unsigned long loc_nfree = 0;
            cxx_mpz a, b;
            for(off_t pos = ftell(fi) ; pos < spos_tab[i+1] ; ) {
                int rc = gmp_fscanf(fi, "%Zd %Zd", (mpz_ptr) a, (mpz_ptr) b);
                pos = ftell(fi);
                /* We may be tricked by initial white space, since our
                 * line parsing does not gobble whitespace at end of
                 * line.
                 * Therefore we must check the position _after_ the
                 * parse.
                 */
                if ((rc != 2 && feof(fi)) || pos >= spos_tab[i+1])
                    break;
                ASSERT_ALWAYS(rc == 2);
                if (mpz_cmp_ui (a, 0) == 0 && mpz_cmp_ui (b, 0) == 0)
                    ASSERT_ALWAYS(0);
                loc_prd.emplace_back(m.from_ab(a, b));
                loc_nab++;
                loc_nfree += (mpz_cmp_ui (b, 0) == 0);

                if(!(loc_nab % 100000))
#pragma omp critical
                {
                    nab += loc_nab;
                    nfree += loc_nfree;
                    loc_nab = 0;
                    loc_nfree = 0;
                    /* in truth, a splice() would be best. */
                    for(auto & x: loc_prd)
                        prd.emplace_back(std::move(x));
                    loc_prd.clear();
                    if(!(nab % REPORT)) {
                        fmt::fprintf(stderr, "%s: read %lu (a,b) pairs in %.2fs (wct %.2fs, peak %luM)\n",
                                message, nab, seconds (), wct_seconds () - wct0,
                                PeakMemusage () >> 10);
                        fflush (stderr);
                    }
                }
            }
#pragma omp critical
            {
                nab += loc_nab;
                nfree += loc_nfree;
                for(auto & x: loc_prd)
                    prd.emplace_back(std::move(x));
                loc_prd.clear();
            }
            fclose_maybe_compressed_lock (fi, depname);
        }
        {
            fmt::fprintf (stderr, "%s read %lu (a,b) pairs, including %lu free, in %1.2fs (wct %1.2fs)\n",
                    message, nab, nfree, seconds (), wct_seconds () - wct0);
            fflush (stderr);
        }
    }
    fclose_maybe_compressed_lock (depfile, depname);
    return prd;
}

/*************************************************************************/


/********** RATSQRT **********/

/* This is where we store side-relative functions for the rational square
 * root. We deal with integers, here.
 */
struct cxx_mpz_functions {
    cxx_mpz_poly const & P;
    typedef cxx_mpz T;
    void set1(T & x) const { mpz_set_ui(x, 1); }
    void set(T & y, T const & x) const { mpz_set(y, x); }
    bool is1(T & x) const { return mpz_cmp_ui(x, 1) == 0; }
    void operator()(T & res, T const & a, T const & b) const {
        mpz_mul(res, a, b);
    }
    T from_ab(cxx_mpz const& a, cxx_mpz const& b) const
    {
        cxx_mpz v;
        /* accumulate g1*a+g0*b */
        mpz_mul (v, P->coeff[1], a);
        mpz_addmul (v, P->coeff[0], b);
        return v;
    }
    cxx_mpz_functions(cxx_mpz_poly const & P) : P(P) {}
};


int
calculateSqrtRat (const char *prefix, int numdep, cado_poly pol,
        int side, mpz_t Np)
{
  char * sidename = get_depsidename (prefix, numdep, side);
  FILE *resfile;
  unsigned long nab = 0, nfree = 0;

  ASSERT_ALWAYS (pol->pols[side]->deg == 1);

#pragma omp critical
  {
#ifdef __MPIR_VERSION
    fprintf (stderr, "Using MPIR %s\n", mpir_version);
#else
    fprintf (stderr, "Using GMP %s\n", gmp_version);
#endif
    fflush (stderr);
  }

  cxx_mpz_poly F(pol->pols[side]);
  cxx_mpz prod;

  {
      cxx_mpz_functions M(F);

      std::string message = fmt::format(FMT_STRING("Rat({})"), numdep);
      char * depname = get_depname (prefix, "", numdep);
      std::vector<cxx_mpz> prd = read_ab_pairs_from_depfile(depname, M, message, nab, nfree);
      free(depname);

      prod = accumulate(prd, M, message);
  }

  /* we must divide by g1^nab: if the number of (a,b) pairs is odd, we
     multiply by g1, and divide by g1^(nab+1) */
  if (nab & 1)
    mpz_mul (prod, prod, F->coeff[1]);

#pragma omp critical
  {
    fprintf (stderr, "Rat(%d): size of product = %zu bits (peak %luM)\n",
	     numdep, mpz_sizeinbase (prod, 2),
	     PeakMemusage () >> 10);
    fflush (stderr);
  }

  if (mpz_sgn (prod) < 0)
    {
      fprintf (stderr, "Error, product is negative: try another dependency\n");
      exit (1);
    }

#pragma omp critical
  {
    fprintf (stderr, "Rat(%d): starting rational square root at %.2fs (wct %.2fs)\n",
	     numdep, seconds (), wct_seconds () - wct0);
    fflush (stderr);
  }

  cxx_mpz v;
  /* since we know we have a square, take the square root */
  mpz_sqrtrem (prod, v, prod);

#pragma omp critical
  {
    fprintf (stderr, "Rat(%d): computed square root at %.2fs (wct %.2fs)\n",
	     numdep, seconds (), wct_seconds () - wct0);
    fflush (stderr);
  }

  if (mpz_cmp_ui (v, 0) != 0)
    {
      unsigned long p = 2, e, errors = 0;
      mpz_t pp;

      mpz_init (pp);
      fprintf (stderr, "Error, rational square root remainder is not zero\n");
      /* reconstruct the initial value of prod to debug */
      mpz_mul (prod, prod, prod);
      mpz_add (prod, prod, v);
      prime_info pi;
      prime_info_init (pi);
      while (mpz_cmp_ui (prod, 1) > 0)
        {
          e = 0;
          if (verbose)
            printf ("Removing p=%lu:", p);
          mpz_set_ui (pp, p);
          e = mpz_remove (prod, prod, pp);
          if (verbose)
            printf (" exponent=%lu, remaining %zu bits\n", e,
                    mpz_sizeinbase (prod, 2));
          if ((e % 2) != 0)
            {
              errors ++;
              fprintf (stderr, "Prime %lu appears to odd power %lu\n", p, e);
              if (verbose || errors >= 10)
                break;
            }
          p = getprime_mt (pi);
        }
      mpz_clear (pp);
      prime_info_clear (pi);
      exit (EXIT_FAILURE);
    }

  mpz_mod (prod, prod, Np);

#pragma omp critical
  {
    fprintf (stderr, "Rat(%d): reduced mod n at %.2fs (wct %.2fs)\n",
	     numdep, seconds (), wct_seconds () - wct0);
    fflush (stderr);
  }

  /* now divide by g1^(nab/2) if nab is even, and g1^((nab+1)/2)
     if nab is odd */

  mpz_powm_ui (v, F->coeff[1], (nab + 1) / 2, Np);
#pragma omp critical
  {
    fprintf (stderr, "Rat(%d): computed g1^(nab/2) mod n at %.2fs (wct %.2fs)\n",
	     numdep, seconds (), wct_seconds () - wct0);
    fflush (stderr);
  }
  resfile = fopen_maybe_compressed_lock (sidename, "wb");

  mpz_invert (v, v, Np);
  mpz_mul (prod, prod, v);
  mpz_mod (prod, prod, Np);

  gmp_fprintf (resfile, "%Zd\n", (mpz_srcptr) prod);
  fclose_maybe_compressed_lock (resfile, sidename);
  free (sidename);

#pragma omp critical
  {
    gmp_fprintf (stderr, "Rat(%d): square root is %Zd\n", numdep, (mpz_srcptr) prod);
    fprintf (stderr, "Rat(%d): square root time: %.2fs (wct %.2fs)\n",
	     numdep, seconds (), wct_seconds () - wct0);
    fflush (stderr);
  }

  return 0;
}

typedef struct
{
  const char *prefix;
  int task;            /* 0:ratsqrt 1:algsqrt 2:gcd */
  int numdep;
  cado_poly_ptr pol;
  int side;
  mpz_ptr Np;
} __tab_struct;
typedef __tab_struct tab_t[1];

/********** ALGSQRT **********/
static cxx_mpz_polymod_scaled
cxx_mpz_polymod_scaled_from_ab (cxx_mpz const & a, cxx_mpz const & b)
{
    if (mpz_cmp_ui (b, 0) == 0) {
        cxx_mpz_polymod_scaled tmp(0);
        mpz_set (tmp.p->coeff[0], a);
        mpz_poly_cleandeg(tmp.p, 0);
        return tmp;
    } else {
        cxx_mpz_polymod_scaled tmp(1);
        mpz_neg (tmp.p->coeff[1], b);
        mpz_set (tmp.p->coeff[0], a);
        mpz_poly_cleandeg(tmp.p, 1);
        return tmp;
    }
}

/* On purpose, this does not free the resources. Doing so would be
 * premature optimization. In this file we knowingly call this function
 * when we do _not_ want reallocation to happen.
 */
void cxx_mpz_polymod_scaled_set_ui(cxx_mpz_polymod_scaled & P, unsigned long x)
{
    P.v = 0;
    mpz_poly_realloc(P.p, 1);
    mpz_set_ui (P.p->coeff[0], x);
    mpz_poly_cleandeg(P.p, 0);
}

void cxx_mpz_polymod_scaled_set(cxx_mpz_polymod_scaled & y, cxx_mpz_polymod_scaled const & x)
{
    y.v = x.v;
    mpz_poly_set(y.p, x.p);
}

/* This is the analogue of cxx_mpz_functions for the algebraic square
 * root. Here, the object must carry a reference to the field polynomial
 */
struct cxx_mpz_polymod_scaled_functions {
    typedef cxx_mpz_polymod_scaled T;
    cxx_mpz_poly const & F;
    cxx_mpz_polymod_scaled_functions(cxx_mpz_poly & F) : F(F) {}
    T from_ab(cxx_mpz const & a, cxx_mpz const & b) const {
        return cxx_mpz_polymod_scaled_from_ab(a, b);
    }
    bool is1(T & res) const {
        return res.p.degree() == 0 && res.v == 0 && mpz_cmp_ui(res.p->coeff[0], 1) == 0;
    }
    void set(T & y, T const & x) const {
        cxx_mpz_polymod_scaled_set(y, x);
    }
    void set1(T & res) const {
        cxx_mpz_polymod_scaled_set_ui(res, 1);
    }
    void operator()(T &res, T const & a, T const & b) const {
        cxx_mpz_polymod_scaled_mul(res, a, b, F);
    }
};


/* Reduce the coefficients of R in [-m/2, m/2) */
static void
mpz_poly_mod_center (mpz_poly R, const mpz_t m)
{
#pragma omp parallel for
  for (int i=0; i <= R->deg; i++)
    mpz_ndiv_r (R->coeff[i], R->coeff[i], m);
}

#if 0
/* Check whether the coefficients of R (that are given modulo m) are in
   fact genuine integers. We assume that x mod m is a genuine integer if
   x or |x-m| is less than m/10^6, i.e., the bit size of x or |x-m| is
   less than that of m minus 20.
   Assumes the coefficients x satisfy 0 <= x < m.
*/
static int
mpz_poly_integer_reconstruction (mpz_poly R, const mpz_t m)
{
  int i;
  size_t sizem = mpz_sizeinbase (m, 2), sizer;

  for (i=0; i <= R->deg; ++i)
    {
      sizer = mpz_sizeinbase (R->coeff[i], 2);
      if (sizer + 20 > sizem)
        {
          mpz_sub (R->coeff[i], R->coeff[i], m);
          sizer = mpz_sizeinbase (R->coeff[i], 2);
          if (sizer + 20 > sizem)
            return 0;
        }
    }
  return 1;
}
#endif

// compute res := sqrt(a) in Fp[x]/f(x)
static void
TonelliShanks (mpz_poly res, const mpz_poly a, const mpz_poly F, unsigned long p)
{
  int d = F->deg;
  mpz_t q;
  mpz_poly delta;  // a non quadratic residue
  mpz_poly auxpol;
  mpz_t aux;
  mpz_t t;
  int s;
  mpz_t myp;

  mpz_init_set_ui(myp, p);

  mpz_init(aux);
  mpz_init(q);
  mpz_poly_init(auxpol, d);
  mpz_ui_pow_ui(q, p, (unsigned long)d);

  // compute aux = (q-1)/2
  // and (s,t) s.t.  q-1 = 2^s*t
  mpz_sub_ui(aux, q, 1);
  mpz_divexact_ui(aux, aux, 2);
  mpz_init_set(t, aux);
  s = 1;
  while (mpz_divisible_2exp_p(t, 1)) {
    s++;
    mpz_divexact_ui(t, t, 2);
  }
  // find a non quadratic residue delta
  {
    mpz_poly_init(delta, d);
    gmp_randstate_t state;
    gmp_randinit_default(state);
    do {
      int i;
      // pick a random delta
      for (i = 0; i < d; ++i)
	mpz_urandomm(delta->coeff[i], state, myp);
      mpz_poly_cleandeg(delta, d-1);
      // raise it to power (q-1)/2
      mpz_poly_pow_mod_f_mod_ui(auxpol, delta, F, aux, p);
      /* Warning: the coefficients of auxpol might either be reduced in
	 [0, p) or in [-p/2, p/2). This code should work in both cases. */
    } while (auxpol->deg != 0 || (mpz_cmp_ui (auxpol->coeff[0], p-1) != 0 &&
				  mpz_cmp_si (auxpol->coeff[0], -1) != 0));
    gmp_randclear (state);
  }

  // follow the description of Crandall-Pomerance, page 94
  {
    mpz_poly A, D;
    mpz_t m;
    int i;
    mpz_poly_init(A, d);
    mpz_poly_init(D, d);
    mpz_init_set_ui(m, 0);
    mpz_poly_pow_mod_f_mod_ui(A, a, F, t, p);
    mpz_poly_pow_mod_f_mod_ui(D, delta, F, t, p);
    for (i = 0; i <= s-1; ++i) {
      mpz_poly_pow_mod_f_mod_ui(auxpol, D, F, m, p);
      mpz_poly_mul_mod_f_mod_mpz(auxpol, auxpol, A, F, myp, NULL, NULL);
      mpz_ui_pow_ui(aux, 2, (s-1-i));
      mpz_poly_pow_mod_f_mod_ui(auxpol, auxpol, F, aux, p);
      if ((auxpol->deg == 0) && (mpz_cmp_ui(auxpol->coeff[0], p-1)== 0))
    mpz_add_ui(m, m, 1UL<<i);
    }
    mpz_add_ui(t, t, 1);
    mpz_divexact_ui(t, t, 2);
    mpz_poly_pow_mod_f_mod_ui(res, a, F, t, p);
    mpz_divexact_ui(m, m, 2);
    mpz_poly_pow_mod_f_mod_ui(auxpol, D, F, m, p);

    mpz_poly_mul_mod_f_mod_mpz(res, res, auxpol, F, myp, NULL, NULL);
    mpz_poly_clear(D);
    mpz_poly_clear(A);
    mpz_clear(m);
  }

  mpz_poly_clear(auxpol);
  mpz_poly_clear(delta);
  mpz_clear(q);
  mpz_clear(aux);
  mpz_clear(myp);
  mpz_clear (t);
}

// res <- Sqrt(AA) mod F, using p-adic lifting, at prime p.
void
cxx_mpz_polymod_scaled_sqrt (cxx_mpz_polymod_scaled & res, cxx_mpz_polymod_scaled & AA, cxx_mpz_poly const & F, unsigned long p,
	       int numdep)
{
  mpz_poly A, *P;
  unsigned long v;
  int d = F->deg;
  unsigned long k, target_k;
  unsigned long K[65];
  int lk, logk, logk0;
  size_t target_size; /* target bit size for Hensel lifting */

  /* The size of the coefficients of the square root of A should be about half
     the size of the coefficients of A. Here is an heuristic argument: let
     K = Q[x]/(f(x)) where f(x) is the algebraic polynomial. The square root
     r(x) might be considered as a random element of K: it is smooth, not far
     from an integer, but except that has no relationship with the coefficients
     of f(x). When we square r(x), we obtain a polynomial with coefficients
     twice as large, before reduction by f(x). The reduction modulo f(x)
     produces A(x), however that reduction should not decrease the size of
     the coefficients. */
  target_size = mpz_poly_sizeinbase (AA.p, 2);
  target_size = target_size / 2;
  target_size += target_size / 10;
#pragma omp critical
  {
    fprintf (stderr, "Alg(%d): target_size=%lu\n", numdep,
	     (unsigned long int) target_size);
    fflush (stderr);
  }

  mpz_poly_init(A, d-1);
  // Clean up the mess with denominator: if it is an odd power of fd,
  // then multiply num and denom by fd to make it even.
  mpz_poly_swap(A, AA.p);
  if (((AA.v)&1) == 0) {
    v = AA.v / 2;
  } else {
    v = (1+AA.v) / 2;
    mpz_poly_mul_mpz(A, A, F->coeff[d]);
  }

  // Now, we just have to take the square root of A (without denom) and
  // divide by fd^v.

  // Variables for the lifted values
  mpz_poly invsqrtA;
  // variables for A and F modulo pk
  mpz_poly a;
  mpz_poly_init(invsqrtA, d-1);
  mpz_poly_init(a, d-1);
  // variable for the current pk
  mpz_t pk;
  mpz_init (pk);

  /* Jason Papadopoulos's trick: since we will lift the square root of A to at
     most target_size bits, we can reduce A accordingly */
  double st = seconds (), wct = wct_seconds ();
  target_k = (unsigned long) ((double) target_size * log ((double) 2) / log((double) p));
  mpz_ui_pow_ui (pk, p, target_k);
  while (mpz_sizeinbase (pk, 2) <= target_size)
    {
      mpz_mul_ui (pk, pk, p);
      target_k ++;
    }
  mpz_poly_mod_mpz (A, A, pk, NULL);
  for (k = target_k, logk = 0; k > 1; k = (k + 1) / 2, logk ++)
    K[logk] = k;
  K[logk] = 1;
#pragma omp critical
  {
    fprintf (stderr, "Alg(%d): reducing A mod p^%lu took %.2fs (wct %.2fs)\n",
	     numdep, target_k, seconds () - st, wct_seconds () - wct);
    fflush (stderr);
  }

  // Initialize things modulo p:
  mpz_set_ui (pk, p);
  k = 1; /* invariant: pk = p^k */
  lk = 0; /* k = 2^lk */
  st = seconds ();
  wct = wct_seconds ();
  P = mpz_poly_base_modp_init (A, p, K, logk0 = logk);
#pragma omp critical
  {
    fprintf (stderr, "Alg(%d): mpz_poly_base_modp_init took %.2fs (wct %.2fs)\n",
	     numdep, seconds () - st, wct_seconds () - wct);
    fflush (stderr);
  }

  /* A is not needed anymore, thus we can clear it now */
  mpz_poly_clear (A);

  mpz_poly_set (a, P[0]);

  // First compute the inverse square root modulo p
  {
    mpz_t q, aux;
    mpz_init(q);
    mpz_init(aux);
    mpz_ui_pow_ui(q, p, (unsigned long)d);

#if 0
    // compute (q-2)(q+1)/4   (assume q == 3 mod 4, here !!!!!)
    // where q = p^d, the size of the finite field extension.
    // since we work mod q-1, this gives (3*q-5)/4
    mpz_mul_ui(aux, q, 3);
    mpz_sub_ui(aux, aux, 5);
    mpz_divexact_ui(aux, aux, 4);               // aux := (3q-5)/4
    mpz_poly_pow_mod_f_mod_ui(invsqrtA, a, F, aux, p);
#else
    TonelliShanks(invsqrtA, a, F, p);
    mpz_sub_ui(aux, q, 2);
    mpz_poly_pow_mod_f_mod_ui(invsqrtA, invsqrtA, F, aux, p);
#endif

    mpz_clear(aux);
    mpz_clear(q);
  }

  // Now, the lift begins
  // When entering the loop, invsqrtA contains the inverse square root
  // of A computed modulo p.

  mpz_poly tmp;
  mpz_t invpk;
  mpz_init (invpk);
  mpz_poly_init (tmp, 2*d-1);
  do {
    double st;

    if (mpz_sizeinbase (pk, 2) > target_size)
      {
        fprintf (stderr, "Failed to reconstruct an integer polynomial\n");
        printf ("Failed\n");
        exit (1);
      }

    /* invariant: invsqrtA = 1/sqrt(A) bmod p^k */

    lk += 1;
    st = seconds ();
    wct = wct_seconds ();
    /* a <- a + pk*P[lk] */
    mpz_poly_base_modp_lift (a, P, lk, pk);
    /* free P[lk] which is no longer needed */
    mpz_poly_clear (P[lk]);
    if (verbose)
#pragma omp critical
      {
	fprintf (stderr, "Alg(%d):    mpz_poly_base_modp_lift took %.2fs (wct %.2fs, peak %luM)\n",
		 numdep, seconds () - st, wct_seconds () - wct,
		 PeakMemusage () >> 10);
	fflush (stderr);
      }

    mpz_mul (pk, pk, pk);   // double the current precision
    k = k + k;
    logk --;
    if (K[logk] & 1)
      {
        mpz_div_ui (pk, pk, p);
        k --;
      }
    barrett_precompute_inverse (invpk, pk);

    /* check the invariant k = K[logk] */
    ASSERT_ALWAYS(k == K[logk]);

#pragma omp critical
    {
      fprintf (stderr, "Alg(%d): start lifting mod p^%lu (%lu bits) at %.2fs (wct %.2fs)\n",
	       numdep, k, (unsigned long int) mpz_sizeinbase (pk, 2),
	       seconds (), wct_seconds () - wct0);
      fflush (stderr);
    }

    // now, do the Newton operation x <- 1/2(3*x-a*x^3)
    st = seconds ();
    wct = wct_seconds ();
    mpz_poly_sqr_mod_f_mod_mpz (tmp, invsqrtA, F, pk, NULL, invpk); /* tmp = invsqrtA^2 */
    if (verbose)
#pragma omp critical
      {
        fprintf (stderr, "Alg(%d):    mpz_poly_sqr_mod_f_mod_mpz took %.2fs (wct %.2fs, peak %luM)\n",
		 numdep, seconds () - st, wct_seconds () - wct,
		 PeakMemusage () >> 10);
        fflush (stderr);
      }

    /* Faster version which computes x <- x + x/2*(1-a*x^2).
       However I don't see how to use the fact that the coefficients
       if 1-a*x^2 are divisible by p^(k/2). */
    st = seconds ();
    wct = wct_seconds ();
    mpz_poly_mul_mod_f_mod_mpz (tmp, tmp, a, F, pk, NULL, invpk); /* tmp=a*invsqrtA^2 */
    if (verbose)
#pragma omp critical
      {
        fprintf (stderr, "Alg(%d):    mpz_poly_mul_mod_f_mod_mpz took %.2fs (wct %.2fs, peak %luM)\n",
		 numdep, seconds () - st, wct_seconds () - wct,
		 PeakMemusage () >> 10);
        fflush (stderr);
      }
    mpz_poly_sub_ui (tmp, tmp, 1); /* a*invsqrtA^2-1 */
    mpz_poly_div_2_mod_mpz (tmp, tmp, pk); /* (a*invsqrtA^2-1)/2 */
    st = seconds ();
    wct = wct_seconds ();
    mpz_poly_mul_mod_f_mod_mpz (tmp, tmp, invsqrtA, F, pk, NULL, invpk);
    if (verbose)
#pragma omp critical
      {
        fprintf (stderr, "Alg(%d):    mpz_poly_mul_mod_f_mod_mpz took %.2fs (wct %.2fs, peak %luM)\n",
		 numdep, seconds () - st, wct_seconds () - wct,
		 PeakMemusage () >> 10);
        fflush (stderr);
      }
    /* tmp = invsqrtA/2 * (a*invsqrtA^2-1) */
    mpz_poly_sub_mod_mpz (invsqrtA, invsqrtA, tmp, pk);


  } while (k < target_k);

  /* multiply by a to get an approximation of the square root */
  st = seconds ();
  wct = wct_seconds ();
  mpz_poly_mul_mod_f_mod_mpz (tmp, invsqrtA, a, F, pk, NULL, invpk);
  mpz_clear (invpk);
  if (verbose)
#pragma omp critical
    {
      fprintf (stderr, "Alg(%d):    final mpz_poly_mul_mod_f_mod_mpz took %.2fs (wct %.2fs, peak %luM)\n",
	       numdep, seconds () - st, wct_seconds () - wct,
	       PeakMemusage () >> 10);
      fflush (stderr);
    }
  mpz_poly_mod_center (tmp, pk);

  mpz_poly_base_modp_clear (P, logk0);

  mpz_poly_set(res.p, tmp);
  res.v = v;

  mpz_clear (pk);
  mpz_poly_clear(tmp);
  mpz_poly_clear (invsqrtA);
  mpz_poly_clear (a);

  size_t sqrt_size = mpz_poly_sizeinbase (res.p, 2);
#pragma omp critical
  {
    fprintf (stderr, "Alg(%d): maximal sqrt bit-size = %zu (%.0f%% of target size)\n",
	     numdep, sqrt_size, 100.0 * (double) sqrt_size / target_size);
    fflush (stderr);
  }
}

static unsigned long
FindSuitableModP (mpz_poly F, mpz_t N)
{
  unsigned long p = 2;
  int dF = F->deg;

  modul_poly_t fp;

  modul_poly_init (fp, dF);
  prime_info pi;
  prime_info_init (pi);
  while (1)
    {
    int d;

    p = getprime_mt (pi);

    /* check p does not divide N */
    if (mpz_gcd_ui (NULL, N, p) != 1)
      continue;

    /* check the leading coefficient of F does not vanish mod p */
    d = modul_poly_set_mod (fp, F, &p);
    if (d != dF)
      continue;

    /* check that F is irreducible mod p */
    if (modul_poly_is_irreducible (fp, &p))
      break;

#define MAXP 1000000
    if (p > MAXP)
      {
	fprintf (stderr, "Error, found no suitable prime up to %d\n", MAXP);
	fprintf (stderr, "See paragraph \"Factoring with SNFS\" in README\n");
	exit (1);
      }
    }
  modul_poly_clear (fp);
  prime_info_clear (pi);

  return p;
}

/*
   Process dependencies numdep to numdep + nthreads - 1.
*/
int
calculateSqrtAlg (const char *prefix, int numdep,
                  cado_poly_ptr pol, int side, mpz_t Np)
{
  FILE *resfile;
  unsigned long p;
  unsigned long nab = 0, nfree = 0;

  ASSERT_ALWAYS(side == 0 || side == 1);

  char * sidename = get_depsidename (prefix, numdep, side);

  // Init F to be the corresponding polynomial
  cxx_mpz_poly F(pol->pols[side]);
  cxx_mpz_polymod_scaled prod;

  // Accumulate product with a subproduct tree
  {
      cxx_mpz_polymod_scaled_functions M(F);

      std::string message = fmt::format(FMT_STRING("Alg({})"), numdep);
      char * depname = get_depname (prefix, "", numdep);
      std::vector<cxx_mpz_polymod_scaled> prd = read_ab_pairs_from_depfile(depname, M, message, nab, nfree);
      free(depname);

      ASSERT_ALWAYS ((nab & 1) == 0);
      ASSERT_ALWAYS ((nfree & 1) == 0);
      /* nfree being even is forced by a specific character column added
       * by character.c. The correspond assert should not fail.
       *
       * nab being even is kind of a mystery. There is a character column
       * that gives the sign of the rational side. It could well be that
       * with the parameters we usually use, it is negative for all
       * relations; that would force the overall number of relations to
       * be even. Another possibility is when f_d contains a big prime
       * number that does not occur anywhere else, so that the power of
       * this prime is controlled by the usual characters, and since f_d
       * is always present...
       *
       * But: I wouldn't be surprised if the assert(even(nab)) fails.
       * Then, a patch would be:
       *    - remove the assert (!)
       *    - in the numerator of the big product, eliminate powers of
       *       f_d that divides all coefficients.
       *    - this should finally give an even power of f_d in the
       *      denominator, and the algorithm can continue.
       */

      prod = accumulate(prd, M, message);
  }

    p = FindSuitableModP(F, Np);
#pragma omp critical
    {
      fprintf (stderr, "Alg(%d): finished accumulating product at %.2fs (wct %.2fs)\n",
	       numdep, seconds(), wct_seconds () - wct0);
      fprintf (stderr, "Alg(%d): nab = %lu, nfree = %lu, v = %lu\n", numdep,
	       nab, nfree, prod.v);
      fprintf (stderr, "Alg(%d): maximal polynomial bit-size = %lu\n", numdep,
	       (unsigned long) mpz_poly_sizeinbase (prod.p, 2));
      fprintf (stderr, "Alg(%d): using p=%lu for lifting\n", numdep, p);
      fflush (stderr);
    }

    cxx_mpz_polymod_scaled_sqrt (prod, prod, F, p, numdep);
#pragma omp critical
    {
      fprintf (stderr, "Alg(%d): square root lifted at %.2fs (wct %.2fs)\n",
	       numdep, seconds(), wct_seconds () - wct0);
      fflush (stderr);
    }

    mpz_t algsqrt, aux;
    mpz_init(algsqrt);
    mpz_init(aux);
    mpz_t m;
    int ret;
    mpz_init(m);
    do {
        ret = cado_poly_getm(m, pol, Np);
        if (!ret) {
            gmp_fprintf(stderr, "When trying to compute m, got the factor %Zd\n", m);
            mpz_divexact(Np, Np, m);
        }
    } while (!ret);
    mpz_poly_eval_mod_mpz(algsqrt, prod.p, m, Np);
    mpz_clear(m);
    mpz_invert(aux, F->coeff[F->deg], Np);  // 1/fd mod n
    mpz_powm_ui(aux, aux, prod.v, Np);      // 1/fd^v mod n
    mpz_mul(algsqrt, algsqrt, aux);
    mpz_mod(algsqrt, algsqrt, Np);

    resfile = fopen_maybe_compressed_lock (sidename, "wb");
    gmp_fprintf (resfile, "%Zd\n", algsqrt);
    fclose_maybe_compressed_lock (resfile, sidename);

#pragma omp critical
    {
      gmp_fprintf (stderr, "Alg(%d): square root is: %Zd\n",
		   numdep, algsqrt);
      fprintf (stderr, "Alg(%d): square root completed at %.2fs (wct %.2fs)\n",
	       numdep, seconds(), wct_seconds () - wct0);
      fflush (stderr);
    }
    free (sidename);
    mpz_clear(aux);
    mpz_clear(algsqrt);
    return 0;
}

/*
 * Try to factor input using trial division up to bound B.
 * Found factors are printed (one per line).
 * Returns 1 if input is completely factored, otherwise, returns
 * remaining factor.
 */
unsigned long
trialdivide_print(unsigned long N, unsigned long B)
{
    ASSERT(N != 0);
    if (N == 1) return 1;
    unsigned long p;
    prime_info pi;
    prime_info_init (pi);
    for (p = 2; p <= B; p = getprime_mt (pi)) {
        while ((N%p) == 0) {
            N /= p;
            printf("%ld\n", p);
            if (N == 1) {
                prime_info_clear (pi);
                return N;
            }
        }
    }
    prime_info_clear (pi);
    return N;
}

void print_nonsmall(mpz_t zx)
{
    if (mpz_probab_prime_p(zx, 10))
        gmp_printf("%Zd\n", zx);
    else {
        int pp = mpz_perfect_power_p(zx);
        if (pp) {
            pp = mpz_sizeinbase(zx, 2);
            mpz_t roo;
            mpz_init(roo);
            while (!mpz_root(roo, zx, pp))
                pp--;
            int i;
            for (i = 0; i < pp; ++i)
                gmp_printf("%Zd\n", roo);
            mpz_clear(roo);
        } else
            gmp_printf("%Zd\n", zx);
    }
    fflush (stdout);
}

void print_factor(mpz_t N)
{
    unsigned long xx = mpz_get_ui(N);
    if (mpz_cmp_ui(N, xx) == 0) {
        xx = trialdivide_print(xx, 1000000);
        if (xx != 1) {
            mpz_t zx;
            mpz_init(zx);
            mpz_set_ui(zx, xx);
            print_nonsmall(zx);
            mpz_clear(zx);
        }
    } else
        print_nonsmall(N);
}


/********** GCD **********/
int
calculateGcd (const char *prefix, int numdep, mpz_t Np)
{
    char *sidename[2];
    FILE *sidefile[2] = {NULL, NULL};
    mpz_t sidesqrt[2];
    int found = 0;

    for (int side = 0; side < 2; ++side) {
        sidename[side] = get_depsidename (prefix, numdep, side);
        sidefile[side] = fopen_maybe_compressed_lock (sidename[side], "rb");
        mpz_init(sidesqrt[side]);
        if (sidefile[side] == NULL) {
            std::lock_guard<std::mutex> dummy(stdio_guard);
            fprintf(stderr, "Error, cannot open file %s for reading\n",
                    sidename[side]);
            exit(EXIT_FAILURE);
        }
        gmp_fscanf (sidefile[side], "%Zd", sidesqrt[side]);
        fclose_maybe_compressed_lock (sidefile[side], sidename[side]);
        free(sidename[side]);
    }

    mpz_t g1, g2;
    mpz_init(g1);
    mpz_init(g2);

    // reduce mod Np
    mpz_mod(sidesqrt[0], sidesqrt[0], Np);
    mpz_mod(sidesqrt[1], sidesqrt[1], Np);

    // First check that the squares agree
    mpz_mul(g1, sidesqrt[0], sidesqrt[0]);
    mpz_mod(g1, g1, Np);

    mpz_mul(g2, sidesqrt[1], sidesqrt[1]);
    mpz_mod(g2, g2, Np);

    if (mpz_cmp(g1, g2)!=0) {
      std::lock_guard<std::mutex> dummy(stdio_guard);
      fprintf(stderr, "Bug: the squares do not agree modulo n!\n");
      ASSERT_ALWAYS(0);
      //      gmp_printf("g1:=%Zd;\ng2:=%Zd;\n", g1, g2);
    }

    mpz_sub(g1, sidesqrt[0], sidesqrt[1]);
    mpz_gcd(g1, g1, Np);
    if (mpz_cmp(g1,Np)) {
      if (mpz_cmp_ui(g1,1)) {
        found = 1;
        std::lock_guard<std::mutex> dummy(stdio_guard);
        print_factor(g1);
      }
    }

    mpz_add(g2, sidesqrt[0], sidesqrt[1]);
    mpz_gcd(g2, g2, Np);
    if (mpz_cmp(g2,Np)) {
      if (mpz_cmp_ui(g2,1)) {
        found = 1;
        std::lock_guard<std::mutex> dummy(stdio_guard);
        print_factor(g2);
      }
    }
    mpz_clear(g1);
    mpz_clear(g2);

    if (!found) {
      std::lock_guard<std::mutex> dummy(stdio_guard);
      printf ("Failed\n");
    }

    mpz_clear(sidesqrt[0]);
    mpz_clear(sidesqrt[1]);

    return 0;
}

typedef struct
{
  uint64_t *abs;
  uint64_t *dep_masks;
  unsigned int *dep_counts;
  unsigned int nonzero_deps;
  FILE **dep_files;
} sqrt_data_t;

void *
thread_sqrt (void * context_data, earlyparsed_relation_ptr rel)
{
  sqrt_data_t *data = (sqrt_data_t *) context_data;
  for(unsigned int j = 0 ; j < data->nonzero_deps ; j++)
  {
    if (data->abs[rel->num] & data->dep_masks[j])
    {
      fprintf(data->dep_files[j], "%" PRId64 " %" PRIu64 "\n", rel->a, rel->b);
      data->dep_counts[j]++;
    }
  }
  return NULL;
}

void create_dependencies(const char * prefix, const char * indexname, const char * purgedname, const char * kername)
{
    FILE * ix = fopen_maybe_compressed(indexname, "r");
    uint64_t small_nrows;
    int ret;

    ret = fscanf(ix, "%" SCNu64 "\n", &small_nrows);
    ASSERT(ret == 1);

    FILE * ker;
    size_t ker_stride;
    /* Check that kername has consistent size */
    {
        ker = fopen(kername, "rb");
        if (ker == NULL) { perror(kername); exit(errno); }
        struct stat sbuf[1];
        ret = fstat(fileno(ker), sbuf);
        if (ret < 0) { perror(kername); exit(errno); }
        ASSERT_ALWAYS(sbuf->st_size % small_nrows == 0);
        unsigned int ndepbytes = sbuf->st_size / small_nrows;
        fprintf(stderr, "%s contains %u dependencies (including padding)\n",
                kername, 8 * ndepbytes);
        ker_stride = ndepbytes - sizeof(uint64_t);
        if (ker_stride)
            fprintf(stderr, "Considering only the first 64 dependencies\n");
    }

    /* Read the number of (a,b) pairs */
    uint64_t nrows, ncols;
    purgedfile_read_firstline (purgedname, &nrows, &ncols);

    uint64_t * abs = (uint64_t *) malloc(nrows * sizeof(uint64_t));
    ASSERT_ALWAYS(abs != NULL);
    memset(abs, 0, nrows * sizeof(uint64_t));

    for(uint64_t i = 0 ; i < small_nrows ; i++) {
        uint64_t v;
        ret = fread(&v, sizeof(uint64_t), 1, ker);
        if (ker_stride) fseek(ker, ker_stride, SEEK_CUR);

        /* read the index row */
        int nc;
        ret = fscanf(ix, "%d", &nc); ASSERT_ALWAYS(ret == 1);
        for(int k = 0 ; k < nc ; k++) {
            uint64_t col;
            ret = fscanf(ix, "%" SCNx64 "", &col); ASSERT_ALWAYS(ret == 1);
            ASSERT_ALWAYS(col < nrows);
            abs[col] ^= v;
        }
    }
    fclose_maybe_compressed(ix, indexname);
    fclose(ker);

    unsigned int nonzero_deps = 0;
    uint64_t sanity = 0;
    for(uint64_t i = 0 ; i < nrows ; i++) {
        sanity |= abs[i];
    }
    uint64_t dep_masks[64]={0,};
    char * dep_names[64];
    FILE * dep_files[64];
    unsigned int dep_counts[64]={0,};

    for(int i = 0 ; i < 64 ; i++) {
        uint64_t m = UINT64_C(1) << i;
        if (sanity & m)
            dep_masks[nonzero_deps++] = m;
    }
    fprintf(stderr, "Total: %u non-zero dependencies\n", nonzero_deps);
    for(unsigned int i = 0 ; i < nonzero_deps ; i++) {
        dep_names[i] = get_depname (prefix, "", i);
        dep_files[i] = fopen_maybe_compressed (dep_names[i], "wb");
        ASSERT_ALWAYS(dep_files[i] != NULL);
    }

    sqrt_data_t data = {.abs = abs, .dep_masks = dep_masks,
                        .dep_counts = dep_counts, .nonzero_deps = nonzero_deps,
                        .dep_files = dep_files};
    char *fic[2] = {(char *) purgedname, NULL};
    filter_rels (fic, (filter_rels_callback_t) thread_sqrt, &data,
          EARLYPARSE_NEED_AB_HEXA, NULL, NULL);


    fprintf(stderr, "Written %u dependencies files\n", nonzero_deps);
    for(unsigned int i = 0 ; i < nonzero_deps ; i++) {
        fprintf(stderr, "%s : %u (a,b) pairs\n", dep_names[i], dep_counts[i]);
        fclose_maybe_compressed (dep_files[i], dep_names[i]);
        free (dep_names[i]);
    }
    free (abs);
}

#define TASK_SQRT 0
#define TASK_GCD  2
/* perform one task (rat or alg or gcd) on one dependency */
void*
one_thread (void* args)
{
  tab_t *tab = (tab_t*) args;
  if (tab[0]->task == TASK_SQRT) {
      if (tab[0]->pol->pols[tab[0]->side]->deg == 1) {
          calculateSqrtRat (tab[0]->prefix, tab[0]->numdep, tab[0]->pol,
                  tab[0]->side, tab[0]->Np);
      } else {
          calculateSqrtAlg (tab[0]->prefix, tab[0]->numdep, tab[0]->pol,
                  tab[0]->side, tab[0]->Np);
      }
  } else /* gcd */
    calculateGcd (tab[0]->prefix, tab[0]->numdep, tab[0]->Np);
  return NULL;
}

/* process task (0=sqrt, 2=gcd) in parallel for
   dependencies numdep to numdep + nthreads - 1 */
void
calculateTaskN (int task, const char *prefix, int numdep, int nthreads,
                cado_poly pol, int side, mpz_t Np)
{
  pthread_t *tid;
  tab_t *T;
  int j;

  omp_set_num_threads(iceildiv(omp_get_max_threads(), nthreads));

  tid = (pthread_t*) malloc (nthreads * sizeof (pthread_t));
  ASSERT_ALWAYS(tid != NULL);
  T = (tab_t*) malloc (nthreads * sizeof (tab_t));
  ASSERT_ALWAYS(T != NULL);
  for (j = 0; j < nthreads; j++)
    {
      T[j]->prefix = prefix;
      T[j]->task = task;
      T[j]->numdep = numdep + j;
      T[j]->pol = pol;
      T[j]->side = side;
      T[j]->Np = Np;
    }
#ifdef __OpenBSD__
  /* On openbsd, we have obscure failures that seem to be triggered
   * by multithreading. So let's play it simple.
   */
  for (j = 0; j < nthreads; j++)
      (*one_thread)((void*)(T+j));
#else
  for (j = 0; j < nthreads; j++)
      pthread_create (&tid[j], NULL, one_thread, (void *) (T+j));
  while (j > 0)
      pthread_join (tid[--j], NULL);
#endif
  free (tid);
  free (T);
}

void declare_usage(param_list pl)
{
    param_list_decl_usage(pl, "poly", "Polynomial file");
    param_list_decl_usage(pl, "purged", "Purged relations file, as produced by 'purge'");
    param_list_decl_usage(pl, "index", "Index file, as produced by 'merge'");
    param_list_decl_usage(pl, "ker", "Kernel file, as produced by 'characters'");
    param_list_decl_usage(pl, "prefix", "File name prefix used for output files");
    param_list_decl_usage(pl, "ab", "For each dependency, create file with the a,b-values of the relations used in that dependency");
    param_list_decl_usage(pl, "side0", "Compute square root for side 0 and store in file");
    param_list_decl_usage(pl, "side1", "Compute square root for side 1 and store in file");
    param_list_decl_usage(pl, "gcd", "Compute gcd of the two square roots. Requires square roots on both sides");
    param_list_decl_usage(pl, "dep", "The initial dependency for which to compute square roots");
    param_list_decl_usage(pl, "t",   "The number of dependencies to process (default 1)");
    param_list_decl_usage(pl, "v", "More verbose output");
    param_list_decl_usage(pl, "force-posix-threads", "force the use of posix threads, do not rely on platform memory semantics");
}

void usage(param_list pl, const char * argv0, FILE *f)
{
    param_list_print_usage(pl, argv0, f);
    fprintf(f, "Usage: %s [-ab || -side0 || -side1 || -gcd] -poly polyname -prefix prefix -dep numdep -t ndep", argv0);
    fprintf(f, " -purged purgedname -index indexname -ker kername\n");
    fprintf(f, "or %s (-side0 || -side1 || -gcd) -poly polyname -prefix prefix -dep numdep -t ndep\n\n", argv0);
    fprintf(f, "(a,b) pairs of dependency relation 'numdep' will be r/w in file 'prefix.numdep',");
    fprintf(f, " side0 sqrt in 'prefix.side0.numdep' ...\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    cado_poly pol;
    int numdep = -1, nthreads = 1, ret MAYBE_UNUSED, i;

    char * me = *argv;
    /* print the command line */
    fprintf (stderr, "%s.r%s", argv[0], cado_revision_string);
    for (i = 1; i < argc; i++)
      fprintf (stderr, " %s", argv[i]);
    fprintf (stderr, "\n");

    param_list pl;
    param_list_init(pl);
    declare_usage(pl);

    int opt_ab = 0;    /* create dependency files */
    int opt_side0 = 0; /* compute square root on side 0 */
    int opt_side1 = 0; /* compute square root on side 1 */
    int opt_gcd = 0;   /* compute gcd */
    param_list_configure_switch(pl, "ab", &opt_ab);
    param_list_configure_switch(pl, "side0", &opt_side0);
    param_list_configure_switch(pl, "side1", &opt_side1);
    param_list_configure_switch(pl, "gcd", &opt_gcd);
    param_list_configure_switch(pl, "-v", &verbose);
    param_list_configure_switch(pl, "force-posix-threads", &filter_rels_force_posix_threads);
    argc--,argv++;
    for( ; argc ; ) {
        if (param_list_update_cmdline(pl, &argc, &argv)) continue;
        if (strcmp(*argv, "--help") == 0) {
            usage(pl, me, stderr);
            exit(0);
        } else {
            fprintf(stderr, "unexpected argument: %s\n", *argv);
            usage(pl, me, stderr);
            exit(1);
        }
    }
    const char * tmp;
    if((tmp = param_list_lookup_string(pl, "poly")) == NULL) {
        fprintf(stderr, "Parameter -poly is missing\n");
        usage(pl, me, stderr);
        exit(1);
    }
    cado_poly_init(pol);
    ret = cado_poly_read(pol, tmp);
    if (ret == 0) {
        fprintf(stderr, "Could not read polynomial file\n");
        exit(1);
    }

    param_list_parse_int (pl, "dep", &numdep);
    param_list_parse_int (pl, "t", &nthreads);
    const char * purgedname = param_list_lookup_string(pl, "purged");
    const char * indexname = param_list_lookup_string(pl, "index");
    const char * kername = param_list_lookup_string(pl, "ker");
    const char * prefix = param_list_lookup_string(pl, "prefix");
    if (prefix == NULL) {
        fprintf(stderr, "Parameter -prefix is missing\n");
        exit(1);
    }
    if (param_list_warn_unused(pl))
        exit(1);

    /* if no options then -ab -side0 -side1 -gcd */
    if (!(opt_ab || opt_side0 || opt_side1 || opt_gcd))
        opt_ab = opt_side0 = opt_side1 = opt_gcd = 1;

    double cpu0 = seconds ();
    wct0 = wct_seconds();

    /*
     * In the case where the number N to factor has a prime factor that
     * divides the leading coefficient of f or g, the reduction modulo N
     * will fail. Let's compute N', the factor of N that is coprime to
     * those leading coefficients.
     */
    mpz_t Np;
    mpz_init(Np);
    {
        mpz_t gg;
        mpz_init(gg);
        mpz_set(Np, pol->n);
        for (int side = 0; side < 2; ++side) {
            do {
                mpz_gcd(gg, Np, pol->pols[side]->coeff[pol->pols[side]->deg]);
                if (mpz_cmp_ui(gg, 1) != 0) {
                    gmp_fprintf(stderr, "Warning: found the following factor of N as a factor of g: %Zd\n", gg);
                    print_factor(gg);
                    mpz_divexact(Np, Np, gg);
                }
            } while (mpz_cmp_ui(gg, 1) != 0);
        }
        mpz_clear(gg);
        /* Trial divide Np, to avoid bug if a stupid input is given */
        {
            unsigned long p;
            prime_info pi;
            prime_info_init (pi);
            for (p = 2; p <= 1000000; p = getprime_mt (pi)) {
                while (mpz_tdiv_ui(Np, p) == 0) {
                    printf("%lu\n", p);
                    mpz_divexact_ui(Np, Np, p);
                }
            }
            prime_info_clear (pi);
        }
        if (mpz_cmp(pol->n, Np) != 0)
            gmp_fprintf(stderr, "Now factoring N' = %Zd\n", Np);
        if (mpz_cmp_ui(Np, 1) == 0) {
            gmp_fprintf(stderr, "Hey N' is 1! Stopping\n");
            cado_poly_clear (pol);
            param_list_clear(pl);
            mpz_clear(Np);
            return 0;
        }
        if (mpz_probab_prime_p(Np, 10) || mpz_perfect_power_p(Np)) {
            gmp_fprintf(stderr, "Hey N' is (power of) prime! Stopping\n");
            print_factor(Np);
            cado_poly_clear (pol);
            param_list_clear(pl);
            mpz_clear(Np);
            return 0;
        }
    }

    if (opt_ab) {
        /* Computing (a,b) pairs is now done in batch for 64 dependencies
         * together -- should be enough for our purposes, even if we do
         * have more dependencies !
         */
        if (indexname == NULL) {
            fprintf(stderr, "Parameter -index is missing\n");
            exit(1);
        }
        if (purgedname == NULL) {
            fprintf(stderr, "Parameter -purged is missing\n");
            exit(1);
        }
        if (kername == NULL) {
            fprintf(stderr, "Parameter -ker is missing\n");
            exit(1);
        }
        create_dependencies(prefix, indexname, purgedname, kername);
    }

#ifdef __OpenBSD__
    if (nthreads > 1) {
        fprintf(stderr, "Warning: reducing number of threads to 1 for openbsd ; unexplained failure https://ci.inria.fr/cado/job/compile-openbsd-59-amd64-random-integer/2775/console\n");
        /* We'll still process everything we've been told to. But in a
         * single-threaded fashion */
    }
#endif

    if (opt_side0 || opt_side1 || opt_gcd)
      {
        int i;

        for (i = 0; i < nthreads; i++)
          if (check_dep (prefix, numdep + i) == 0)
            {
              fprintf (stderr, "Warning: dependency %d does not exist, reducing the number of threads to %d\n",
                       numdep + i, i);
              nthreads = i;
              break;
            }
      }

    if (nthreads == 0)
      {
        fprintf (stderr, "Error, no more dependency\n");
        cado_poly_clear (pol);
        param_list_clear (pl);
        mpz_clear (Np);
        return 1;
      }

    if (opt_side0) {
        ASSERT_ALWAYS(numdep != -1);
        calculateTaskN(TASK_SQRT, prefix, numdep, nthreads, pol, 0, Np);
    }

    if (opt_side1) {
        ASSERT_ALWAYS(numdep != -1);
        calculateTaskN(TASK_SQRT, prefix, numdep, nthreads, pol, 1, Np);
    }

    if (opt_gcd) {
        ASSERT_ALWAYS(numdep != -1);
        calculateTaskN(TASK_GCD, prefix, numdep, nthreads, pol, 0, Np);
    }

    cado_poly_clear (pol);
    param_list_clear (pl);
    mpz_clear (Np);
    print_timing_and_memory (stderr, cpu0, wct0);
    return 0;
}
