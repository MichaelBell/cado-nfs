#ifndef __CONFIG_H__
#define __CONFIG_H__

#define HAVE_MSRH 1
#define WANT_ASSERT 1
#define WANT_ASSERT_EXPENSIVE 0

/* Number of blocking levels for small factor base primes, should
   correspond to cache levels. Sieving will be done in SIEVE_BLOCKING + 1
   passes: SIEVE_BLOCKING passes updating directly, and one pass with
   bucket sorting. */
#define SIEVE_BLOCKING 1
static const unsigned long CACHESIZES[SIEVE_BLOCKING] = {16384};

#endif
