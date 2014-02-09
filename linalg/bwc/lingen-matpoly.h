#ifndef LINGEN_MATPOLY_H_
#define LINGEN_MATPOLY_H_

#include "abase.h"

/* This is used only for plingen. */

/* We use abvec because this offers the possibility of having flat data
 *
 * Note that this ends up being exactly the same data type as polymat.
 * The difference here is that the striding is not the same.
 */
struct matpoly_s {
    unsigned int m;
    unsigned int n;
    size_t size;
    size_t alloc;
    abvec x;
};
typedef struct matpoly_s matpoly[1];
typedef struct matpoly_s * matpoly_ptr;
typedef const struct matpoly_s * matpoly_srcptr;

#include "lingen-polymat.h"

#ifdef __cplusplus
extern "C" {
#endif
void matpoly_init(abdst_field ab, matpoly_ptr p, unsigned int m, unsigned int n, int len);
int matpoly_check_pre_init(matpoly_srcptr p);
void matpoly_realloc(abdst_field ab, matpoly_ptr p, size_t newalloc);
void matpoly_zero(abdst_field ab, matpoly_ptr p);
void matpoly_clear(abdst_field ab, matpoly_ptr p);
void matpoly_fill_random(abdst_field ab MAYBE_UNUSED, matpoly_ptr a, unsigned int size, gmp_randstate_t rstate);
void matpoly_swap(matpoly_ptr a, matpoly_ptr b);
int matpoly_cmp(abdst_field ab MAYBE_UNUSED, matpoly_srcptr a, matpoly_srcptr b);
static inline abelt * matpoly_part(matpoly_ptr p, unsigned int i, unsigned int j, unsigned int k);
static inline abdst_elt matpoly_coeff(matpoly_ptr p, unsigned int i, unsigned int j, unsigned int k);
void matpoly_set_polymat(abdst_field ab MAYBE_UNUSED, matpoly_ptr dst, polymat_srcptr src);



void matpoly_addmat(abdst_field ab,
        matpoly c, unsigned int kc,
        matpoly a, unsigned int ka,
        matpoly b, unsigned int kb);

void matpoly_submat(abdst_field ab,
        matpoly c, unsigned int kc,
        matpoly a, unsigned int ka,
        matpoly b, unsigned int kb);

void matpoly_truncate(abdst_field ab, matpoly_ptr dst, matpoly_ptr src, unsigned int size);
void matpoly_multiply_column_by_x(abdst_field ab, matpoly_ptr pi, unsigned int j, unsigned int size);
void matpoly_extract_column(abdst_field ab,
        matpoly_ptr dst, unsigned int jdst, unsigned int kdst,
        matpoly_ptr src, unsigned int jsrc, unsigned int ksrc);
void matpoly_extract_row_fragment(abdst_field ab,
        matpoly_ptr dst, unsigned int i1, unsigned int j1,
        matpoly_ptr src, unsigned int i0, unsigned int j0,
        unsigned int n);
void matpoly_rshift(abdst_field ab, matpoly_ptr dst, matpoly_ptr src, unsigned int k);


void matpoly_addmul(abdst_field ab, matpoly c, matpoly a, matpoly b);
void matpoly_addmp(abdst_field ab, matpoly c, matpoly a, matpoly b);
void matpoly_mul(abdst_field ab, matpoly c, matpoly a, matpoly b);
void matpoly_mp(abdst_field ab, matpoly c, matpoly a, matpoly b);
#ifdef __cplusplus
}
#endif


/* {{{ access interface for matpoly */
static inline abelt * matpoly_part(matpoly_ptr p, unsigned int i, unsigned int j, unsigned int k) {
    ASSERT_ALWAYS(p->size);
    return p->x+(i*p->n+j)*p->alloc+k;
}
static inline abdst_elt matpoly_coeff(matpoly_ptr p, unsigned int i, unsigned int j, unsigned int k) {
    return *matpoly_part(p,i,j,k);
}
static inline const abelt * matpoly_part_const(matpoly_srcptr p, unsigned int i, unsigned int j, unsigned int k) {
    ASSERT_ALWAYS(p->size);
    return (const abelt*)p->x+(i*p->n+j)*p->alloc+k;
}
static inline absrc_elt matpoly_coeff_const(matpoly_srcptr p, unsigned int i, unsigned int j, unsigned int k) {
    return *matpoly_part_const(p,i,j,k);
}
/* }}} */



#endif	/* LINGEN_MATPOLY_H_ */
