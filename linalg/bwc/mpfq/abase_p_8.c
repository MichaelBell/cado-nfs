/* MPFQ generated file -- do not edit */

#define _POSIX_C_SOURCE 200112L
#include "abase_p_8.h"

#include <inttypes.h>
static int abase_p_8_impl_mpi_attr;     /* for MPI functions */
static MPI_Datatype abase_p_8_impl_mpi_datatype;
static MPI_Datatype abase_p_8_impl_mpi_datatype_ur;
static MPI_Op abase_p_8_impl_mpi_addition_op;
static MPI_Op abase_p_8_impl_mpi_addition_op_ur;
static int abase_p_8_impl_mpi_use_count;   /* several stacked init()/clear() pairs are supported */
/* Active handler: simd_gfp */
/* Automatically generated code  */
/* Active handler: Mpfq::defaults */
/* Active handler: Mpfq::defaults::vec */
/* Active handler: Mpfq::defaults::poly */
/* Active handler: Mpfq::gfp::field */
/* Active handler: Mpfq::gfp::elt */
/* Active handler: Mpfq::defaults::mpi_flat */
/* Options used:{
   w=64,
   fieldtype=prime,
   n=8,
   nn=17,
   vtag=p_8,
   vbase_stuff={
    vc:includes=[ <stdarg.h>, ],
    member_templates_restrict={
     p_1=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_1, }, ],
     p_4=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_4, }, ],
     u64k2=[ u64k1, u64k2, ],
     p_3=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_3, }, ],
     p_8=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_8, }, ],
     u64k1=[ u64k1, u64k2, ],
     },
    families=[
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_4, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_1, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_3, }, ],
     [ u64k1, u64k2, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_8, }, ],
     ],
    choose_byfeatures=<code>,
    },
   tag=p_8,
   type=plain,
   opthw=,
   virtual_base={
    filebase=abase_vbase,
    substitutions=[
     [ (?^:abase_p_8_elt \*), void *, ],
     [ (?^:abase_p_8_src_elt\b), const void *, ],
     [ (?^:abase_p_8_elt\b), void *, ],
     [ (?^:abase_p_8_dst_elt\b), void *, ],
     [ (?^:abase_p_8_elt_ur \*), void *, ],
     [ (?^:abase_p_8_src_elt_ur\b), const void *, ],
     [ (?^:abase_p_8_elt_ur\b), void *, ],
     [ (?^:abase_p_8_dst_elt_ur\b), void *, ],
     [ (?^:abase_p_8_vec \*), void *, ],
     [ (?^:abase_p_8_src_vec\b), const void *, ],
     [ (?^:abase_p_8_vec\b), void *, ],
     [ (?^:abase_p_8_dst_vec\b), void *, ],
     [ (?^:abase_p_8_vec_ur \*), void *, ],
     [ (?^:abase_p_8_src_vec_ur\b), const void *, ],
     [ (?^:abase_p_8_vec_ur\b), void *, ],
     [ (?^:abase_p_8_dst_vec_ur\b), void *, ],
     [ (?^:abase_p_8_poly \*), void *, ],
     [ (?^:abase_p_8_src_poly\b), const void *, ],
     [ (?^:abase_p_8_poly\b), void *, ],
     [ (?^:abase_p_8_dst_poly\b), void *, ],
     ],
    name=abase_vbase,
    global_prefix=abase_,
    },
   family=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELDS, tag=p_8, }, ],
   } */


/* Functions operating on the field structure */
/* *Mpfq::gfp::field::code_for_field_characteristic, Mpfq::gfp */
void abase_p_8_field_characteristic(abase_p_8_dst_field k, mpz_t z)
{
    int i;
    int n = k->kl;
    mpz_set_ui(z, k->p[n-1]);
    for (i = n-2; i >= 0; --i) {
        mpz_mul_2exp(z, z, 64);
        mpz_add_ui(z, z, k->p[i]);
    }
}

/* *Mpfq::gfp::field::code_for_field_clear, Mpfq::gfp */
void abase_p_8_field_clear(abase_p_8_dst_field k)
{
    if (k->p != NULL) {
        free(k->p);
        k->p = NULL;
    }
    if (k->bigmul_p != NULL) {
        free(k->bigmul_p);
        k->bigmul_p = NULL;
    }
    if (k->ts_info.e > 0) {
        free(k->ts_info.hh);
        free(k->ts_info.z);
    }
    mpz_clear(k->factor);
}

/* *Mpfq::gfp::field::code_for_field_specify, Mpfq::gfp */
void abase_p_8_field_specify(abase_p_8_dst_field k, unsigned long dummy MAYBE_UNUSED, void * vp)
{
        if (k->p == NULL) k->p = (mp_limb_t *)malloc(8*sizeof(mp_limb_t));
        if (k->bigmul_p == NULL) k->bigmul_p = (mp_limb_t *)malloc(17*sizeof(mp_limb_t));
        if ((!k->p) || (!k->bigmul_p))
            MALLOC_FAILED();
        k->kl = 8;
        k->url = 17;
        k->url_margin = LONG_MAX;
        int i;
        if (dummy == MPFQ_PRIME_MPN) {
            mp_limb_t *p = (mp_limb_t*) vp;
            memcpy(k->p, p, 8*sizeof(mp_limb_t));
        } else if (dummy == MPFQ_PRIME_MPZ) {
            mpz_srcptr p = (mpz_srcptr) vp;
            assert(mpz_size(p) == 8);
            for(i = 0 ; i < 8 ; i++) {
                k->p[i] = mpz_getlimbn(p, i);
            }
        } else if (dummy == MPFQ_GROUPSIZE && *(int*)vp == 1) {
            /* Do nothing, this is an admitted condition */
        } else {
            abort();
        }
    // precompute bigmul_p = largest multiple of p that fits in an elt_ur,
    //   p*Floor( (2^(17*64)-1)/p )
    {
        abase_p_8_elt_ur big;
        mp_limb_t q[17-8+1], r[8], tmp[17+1];
        
        for (i = 0; i < 17; ++i)
            big[i] = ~0UL;
        mpn_tdiv_qr(q, r, 0, big, 17, k->p, 8);
        mpn_mul(tmp, q, 17-8+1, k->p, 8);
        for (i = 0; i < 17; ++i)
            (k->bigmul_p)[i] = tmp[i];
        assert (tmp[17] == 0UL);
    }
}


/* Element allocation functions */

/* Elementary assignment functions */

/* Assignment of random values */

/* Arithmetic operations on elements */
static void abase_p_8_init_ts(abase_p_8_dst_field);
static /* *Mpfq::gfp::elt::code_for_sqrt, Mpfq::gfp */
void abase_p_8_init_ts(abase_p_8_dst_field k)
{
    mp_limb_t pp[8];
    mp_limb_t *ptr = pp;
    mp_limb_t s[8];
    gmp_randstate_t rstate;
    gmp_randinit_default(rstate);
    sub_ui_nc_8(pp, k->p, 1);
    int e = 0;
    while (*ptr == 0) {
        ptr++;
        e += 64;
    }
    int ee;
    ee = ctzl(*ptr);
    e += ee;
    if (e < 64) {
        rshift_8(pp, e);
    } else {
        long_rshift_8(pp, e/64, e%64);
    }
    s[0] = 1UL;
    int i;
    for (i = 1; i <8; ++i)
        s[i] = 0UL;
    if (e-1 < 64) {
        lshift_8(s, e-1);
    } else {
        long_rshift_8(s, (e-1)/64, (e-1)%64);
    }
    k->ts_info.e = e;
    
    k->ts_info.z = malloc(8*sizeof(mp_limb_t));
    k->ts_info.hh = malloc(8*sizeof(mp_limb_t));
    if (!k->ts_info.z || !k->ts_info.hh) 
        MALLOC_FAILED();
    
    abase_p_8_elt z, r;
    abase_p_8_init(k, &z);
    abase_p_8_init(k, &r);
    abase_p_8_set_ui(k, r, 0);
    do {
        abase_p_8_random(k, z, rstate);
        abase_p_8_pow(k, z, z, pp, 8);
        abase_p_8_pow(k, r, z, s, 8);
        abase_p_8_add_ui(k, r, r, 1);
    } while (abase_p_8_cmp_ui(k, r, 0)!=0);
    abase_p_8_set(k, (abase_p_8_dst_elt)k->ts_info.z, z);
    abase_p_8_clear(k, &z);
    abase_p_8_clear(k, &r);
    
    sub_ui_nc_8(pp, pp, 1);
    rshift_8(pp, 1);
    for (i = 0; i < 8; ++i)
        k->ts_info.hh[i] = pp[i];
    gmp_randclear(rstate);
}

/* *Mpfq::gfp::elt::code_for_sqrt, Mpfq::gfp */
int abase_p_8_sqrt(abase_p_8_dst_field k, abase_p_8_dst_elt z, abase_p_8_src_elt a)
{
    if (abase_p_8_cmp_ui(k, a, 0) == 0) {
        abase_p_8_set_ui(k, z, 0);
        return 1;
    }
    if (k->ts_info.e == 0)
        abase_p_8_init_ts(k);
    abase_p_8_elt b, x, y;
    abase_p_8_init(k, &x);
    abase_p_8_init(k, &y);
    abase_p_8_init(k, &b);
    mp_limb_t r = k->ts_info.e;
    mp_limb_t s; //= (1UL<<(r-1)); not needed...
    abase_p_8_set(k, x, a);
    abase_p_8_set(k, y, (abase_p_8_src_elt)k->ts_info.z);
    
    abase_p_8_pow(k, x, a, k->ts_info.hh, 8);
    abase_p_8_sqr(k, b, x);
    abase_p_8_mul(k, x, x, a);
    abase_p_8_mul(k, b, b, a);
    
    abase_p_8_elt t;
    abase_p_8_init(k, &t);
    mp_limb_t m;
    for(;;) {
        abase_p_8_set(k, t, b);
        for(m=0; abase_p_8_cmp_ui(k, t, 1)!=0; m++)
            abase_p_8_sqr(k, t, t);
        assert(m<=r);
        
        if (m==0 || m==r)
            break;
        
        s = 1UL<<(r-m-1);
        r = m;
        
        abase_p_8_pow(k, t, y, &s, 1);
        abase_p_8_sqr(k, y, t);
        abase_p_8_mul(k, x, x, t);
        abase_p_8_mul(k, b, b, y);
    }
    abase_p_8_set(k, z, x);
    abase_p_8_clear(k, &t);
    abase_p_8_clear(k, &x);
    abase_p_8_clear(k, &y);
    abase_p_8_clear(k, &b);
    return (m==0);
}


/* Operations involving unreduced elements */

/* Comparison functions */

/* Input/output functions */
/* *Mpfq::gfp::io::code_for_asprint, Mpfq::gfp */
void abase_p_8_asprint(abase_p_8_dst_field k, char * * pstr, abase_p_8_src_elt x)
{
    int i, n;
    // allocate enough room for base 2 conversion.
    *pstr = (char *)malloc((8*64+1)*sizeof(char));
    if (*pstr == NULL)
        MALLOC_FAILED();
    n = mpn_get_str((unsigned char*)(*pstr), k->io_base, (mp_limb_t *) x, 8);
    for (i = 0; i < n; ++i)
        (*pstr)[i] += '0';
    (*pstr)[n] = '\0';
    // Remove leading 0s
    int shift = 0;
    while (((*pstr)[shift] == '0') && ((*pstr)[shift+1] != '\0')) 
        shift++;
    if (shift>0) {
        i = 0;
        while ((*pstr)[i+shift] != '\0') {
            (*pstr)[i] = (*pstr)[i+shift];
            i++;
        }
        (*pstr)[i] = '\0';
    }
    // Return '0' instead of empty string for zero element
    if ((*pstr)[0] == '\0') {
        (*pstr)[0] = '0';
        (*pstr)[1] = '\0';
    }
}

/* *Mpfq::defaults::code_for_fprint, Mpfq::gfp */
void abase_p_8_fprint(abase_p_8_dst_field k, FILE * file, abase_p_8_src_elt x)
{
    char *str;
    abase_p_8_asprint(k,&str,x);
    fprintf(file,"%s",str);
    free(str);
}

/* *Mpfq::gfp::io::code_for_sscan, Mpfq::gfp */
int abase_p_8_sscan(abase_p_8_dst_field k, abase_p_8_dst_elt z, const char * str)
{
    mpz_t zz;
    mpz_init(zz);
    if (gmp_sscanf(str, "%Zd", zz) != 1) {
        mpz_clear(zz);
        return 0;
    }
    abase_p_8_set_mpz(k, z, zz);
    mpz_clear(zz);
    return 1;
}

/* *Mpfq::gfp::io::code_for_fscan, Mpfq::gfp */
int abase_p_8_fscan(abase_p_8_dst_field k, FILE * file, abase_p_8_dst_elt z)
{
    char *tmp;
    int allocated, len=0;
    int c, start=0;
    allocated=100;
    tmp = (char *)malloc(allocated*sizeof(char));
    if (!tmp)
        MALLOC_FAILED();
    for(;;) {
        c = fgetc(file);
        if (c==EOF)
            break;
        if (isspace((int)(unsigned char)c)) {
            if (start==0)
                continue;
            else
                break;
        } else {
            if (len==allocated) {
                allocated+=100;
                tmp = (char*)realloc(tmp, allocated*sizeof(char));
            }
            tmp[len]=c;
            len++;
            start=1;
        }
    }
    if (len==allocated) {
        allocated+=1;
        tmp = (char*)realloc(tmp, allocated*sizeof(char));
    }
    tmp[len]='\0';
    int ret=abase_p_8_sscan(k,z,tmp);
    free(tmp);
    return ret;
}


/* Vector functions */
/* *Mpfq::defaults::vec::alloc::code_for_vec_init, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_init(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec * v, unsigned int n)
{
    unsigned int i;
    *v = (abase_p_8_vec) malloc (n*sizeof(abase_p_8_elt));
    for(i = 0; i < n; i+=1)
        abase_p_8_init(K, (*v) + i);
}

/* *Mpfq::defaults::vec::alloc::code_for_vec_reinit, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_reinit(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec * v, unsigned int n, unsigned int m)
{
    if (n < m) { // increase size
        unsigned int i;
        *v = (abase_p_8_vec) realloc (*v, m * sizeof(abase_p_8_elt));
        for(i = n; i < m; i+=1)
            abase_p_8_init(K, (*v) + i);
    } else if (m < n) { // decrease size
        unsigned int i;
        for(i = m; i < n; i+=1)
            abase_p_8_clear(K, (*v) + i);
        *v = (abase_p_8_vec) realloc (*v, m * sizeof(abase_p_8_elt));
    }
}

/* *Mpfq::defaults::vec::alloc::code_for_vec_clear, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_clear(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec * v, unsigned int n)
{
        unsigned int i;
    for(i = 0; i < n; i+=1)
        abase_p_8_clear(K, (*v) + i);
    free(*v);
}

/* *Mpfq::defaults::vec::io::code_for_vec_asprint, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_asprint(abase_p_8_dst_field K MAYBE_UNUSED, char * * pstr, abase_p_8_src_vec w, unsigned int n)
{
    if (n == 0) {
        *pstr = (char *)malloc(4*sizeof(char));
        sprintf(*pstr, "[ ]");
        return;
    }
    int alloc = 100;
    int len = 0;
    *pstr = (char *)malloc(alloc*sizeof(char));
    char *str = *pstr;
    *str++ = '[';
    *str++ = ' ';
    len = 2;
    unsigned int i;
    for(i = 0; i < n; i+=1) {
        if (i) {
            (*pstr)[len++] = ',';
            (*pstr)[len++] = ' ';
        }
        char *tmp;
        abase_p_8_asprint(K, &tmp, w[i]);
        int ltmp = strlen(tmp);
        if (len+ltmp+4 > alloc) {
            alloc = len+ltmp+100;
            *pstr = (char *)realloc(*pstr, alloc*sizeof(char));
        }
        strncpy(*pstr+len, tmp, ltmp+4);
        len += ltmp;
        free(tmp);
    }
    (*pstr)[len++] = ' ';
    (*pstr)[len++] = ']';
    (*pstr)[len] = '\0';
}

/* *Mpfq::defaults::vec::io::code_for_vec_fprint, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_fprint(abase_p_8_dst_field K MAYBE_UNUSED, FILE * file, abase_p_8_src_vec w, unsigned int n)
{
    char *str;
    abase_p_8_vec_asprint(K,&str,w,n);
    fprintf(file,"%s",str);
    free(str);
}

/* *Mpfq::defaults::vec::io::code_for_vec_print, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_print(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_src_vec w, unsigned int n)
{
    abase_p_8_vec_fprint(K,stdout,w,n);
}

/* *Mpfq::defaults::vec::io::code_for_vec_sscan, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
int abase_p_8_vec_sscan(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec * w, unsigned int * n, const char * str)
{
    // start with a clean vector
    abase_p_8_vec_reinit(K, w, *n, 0);
    *n = 0;
    while (isspace((int)(unsigned char)str[0]))
        str++;
    if (str[0] != '[')
        return 0;
    str++;
    if (str[0] != ' ')
        return 0;
    str++;
    if (str[0] == ']') {
        return 1;
    }
    unsigned int i = 0;
    for (;;) {
        if (*n < i+1) {
            abase_p_8_vec_reinit(K, w, *n, i+1);
            *n = i+1;
        }
        int ret;
        ret = abase_p_8_sscan(K, (*w)[i], str);
        if (!ret) {
            return 0;
        }
        i++;
        while (isdigit((int)(unsigned char)str[0]))
            str++;
        while (isspace((int)(unsigned char)str[0]))
            str++;
        if (str[0] == ']')
            break;
        if (str[0] != ',')
            return 0;
        str++;
        while (isspace((int)(unsigned char)str[0]))
            str++;
    }
    return 1;
}

/* *Mpfq::defaults::vec::io::code_for_vec_fscan, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
int abase_p_8_vec_fscan(abase_p_8_dst_field K MAYBE_UNUSED, FILE * file, abase_p_8_vec * w, unsigned int * n)
{
    char *tmp;
    int c;
    int allocated, len=0;
    allocated=100;
    tmp = (char *)malloc(allocated*sizeof(char));
    if (!tmp)
        MALLOC_FAILED();
    for(;;) {
        c = fgetc(file);
        if (c==EOF)
            return 0;
        if (len==allocated) {
            allocated+=100;
            tmp = (char*)realloc(tmp, allocated*sizeof(char));
        }
        tmp[len]=c;
        len++;
        if (c==']')
            break;
    }
    if (len==allocated) {
        allocated+=1;
        tmp = (char*)realloc(tmp, allocated*sizeof(char));
    }
    tmp[len]='\0';
    int ret=abase_p_8_vec_sscan(K,w,n,tmp);
    free(tmp);
    return ret;
}

/* *Mpfq::defaults::vec::alloc::code_for_vec_ur_init, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_ur_init(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec_ur * v, unsigned int n)
{
    unsigned int i;
    *v = (abase_p_8_vec_ur) malloc (n*sizeof(abase_p_8_elt_ur));
    for(i = 0; i < n; i+=1)
        abase_p_8_elt_ur_init(K, &( (*v)[i]));
}

/* *Mpfq::defaults::vec::alloc::code_for_vec_ur_reinit, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_ur_reinit(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec_ur * v, unsigned int n, unsigned int m)
{
    if (n < m) { // increase size
        *v = (abase_p_8_vec_ur) realloc (*v, m * sizeof(abase_p_8_elt_ur));
        unsigned int i;
        for(i = n; i < m; i+=1)
            abase_p_8_elt_ur_init(K, (*v) + i);
    } else if (m < n) { // decrease size
        unsigned int i;
        for(i = m; i < n; i+=1)
            abase_p_8_elt_ur_clear(K, (*v) + i);
        *v = (abase_p_8_vec_ur) realloc (*v, m * sizeof(abase_p_8_elt_ur));
    }
}

/* *Mpfq::defaults::vec::alloc::code_for_vec_ur_clear, Mpfq::defaults::vec, Mpfq::defaults, Mpfq::gfp */
void abase_p_8_vec_ur_clear(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_vec_ur * v, unsigned int n)
{
    unsigned int i;
    for(i = 0; i < n; i+=1)
        abase_p_8_elt_ur_clear(K, &( (*v)[i]));
    free(*v);
}

/* *Mpfq::defaults::vec::conv::code_for_vec_conv_ur, Mpfq::gfp */
void abase_p_8_vec_conv_ur_ks(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_vec_ur w, abase_p_8_src_vec u, unsigned int n, abase_p_8_src_vec v, unsigned int m)
{
    // compute base as a power 2^GMP_NUMB_BITS
    // This is the least number of words that can accomodate
    //     log_2( (p-1)^2 * min(n,m) )
    mpz_t p;
    mpz_init(p);
    abase_p_8_field_characteristic(K, p);
    mpz_sub_ui(p, p, 1);
    mpz_mul(p, p, p);
    mpz_mul_ui(p, p, MIN(m, n));
    
    long nbits = mpz_sizeinbase(p, 2);
    long nwords = 1 + ((nbits-1) / GMP_NUMB_BITS);
    nbits = GMP_NUMB_BITS*nwords;
    mpz_clear(p);
    
    assert(sizeof(abase_p_8_elt_ur) >= nwords*sizeof(unsigned long));
    
    // Create big integers
    mpz_t U, V;
    mpz_init2(U, n*nbits);
    mpz_init2(V, m*nbits);
    memset(U->_mp_d, 0, n*nwords*sizeof(unsigned long));
    memset(V->_mp_d, 0, m*nwords*sizeof(unsigned long));
    unsigned int i;
    assert (U->_mp_alloc == n*nwords);
    for (i = 0; i < n; ++i)
        abase_p_8_get_mpn(K, U->_mp_d + i*nwords, u[i]);
    U->_mp_size = U->_mp_alloc;
    // TODO: in principle one could reduce _mp_size until its true value, 
    // but then one should take care of W->_mp_size as well...
    //while (U->_mp_size > 0 && U->_mp_d[U->_mp_size-1] == 0)
    //    U->_mp_size--;
    assert (V->_mp_alloc == m*nwords);
    for (i = 0; i < m; ++i)
        abase_p_8_get_mpn(K, V->_mp_d + i*nwords, v[i]);
    V->_mp_size = V->_mp_alloc;
    //while (V->_mp_size > 0 && V->_mp_d[V->_mp_size-1] == 0)
    //    V->_mp_size--;
    
    // Multiply
    mpz_t W;
    mpz_init(W);
    mpz_mul(W, U, V);
    mpz_clear(U);
    mpz_clear(V);
    
    // Put coefficients in w
    assert (W->_mp_size >= (m+n-1)*nwords);
    if (sizeof(abase_p_8_elt_ur) == nwords*sizeof(unsigned long)) {
        for (i = 0; i < m+n-1; ++i) 
            abase_p_8_elt_ur_set(K, w[i], (abase_p_8_src_elt_ur)(W->_mp_d + i*nwords));
    } else {
        for (i = 0; i < m+n-1; ++i) {
            abase_p_8_elt_ur_set_ui(K, w[i], 0);
            memcpy(w[i], W->_mp_d + i*nwords, nwords*sizeof(unsigned long));
        }
    }
    
    mpz_clear(W);
}


/* Polynomial functions */
/* *Mpfq::defaults::poly::code_for_poly_setmonic, Mpfq::gfp */
void abase_p_8_poly_setmonic(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_poly q, abase_p_8_src_poly p)
{
    long degp = abase_p_8_poly_deg(K, p);
    if (degp == -1) {
        q->size = 0;
        return;
    }
    if (degp == 0) {
        abase_p_8_elt aux;
        abase_p_8_init(K, &aux);
        abase_p_8_set_ui(K, aux, 1);
        abase_p_8_poly_setcoef(K, q, aux, 0);
        abase_p_8_clear(K, &aux);
        q->size = 1;
        return;
    }
    abase_p_8_elt lc;
    abase_p_8_init(K, &lc);
    abase_p_8_poly_getcoef(K, lc, p, degp);
    abase_p_8_inv(K, lc, lc);
    abase_p_8_poly_setcoef_ui(K, q, 1, degp);
    abase_p_8_vec_scal_mul(K, q->c, p->c, lc, degp);
    q->size = degp+1;
    abase_p_8_clear(K, &lc);
}

/* *Mpfq::defaults::poly::code_for_poly_divmod, Mpfq::gfp */
void abase_p_8_poly_divmod(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_poly q, abase_p_8_dst_poly r, abase_p_8_src_poly a, abase_p_8_src_poly b)
{
    if (b->size == 0) {
        fprintf(stderr, "Error: division by 0\n");
        exit(1);
    }
    if (a->size == 0) {
        q->size = 0; r->size = 0;
        return;
    }
    int dega = abase_p_8_poly_deg(K, a);
    if (dega<0) {
        q->size = 0; r->size = 0;
        return;
    }
    // Compute deg b and inverse of leading coef
    int degb = abase_p_8_poly_deg(K, b);
    if (degb<0) {
        fprintf(stderr, "Error: division by 0\n");
        exit(1);
    }
    if (degb > dega) {
        q->size=0;
        abase_p_8_poly_set(K, r, a);
        return;
    }
    int bmonic;
    abase_p_8_elt ilb;
    abase_p_8_init(K, &ilb);
    if (abase_p_8_cmp_ui(K, (b->c)[degb], 1) == 0) {
        abase_p_8_set_ui(K, ilb, 1);
        bmonic = 1;
    } else {
        abase_p_8_inv(K, ilb, (b->c)[degb]);
        bmonic = 0;
    }
    
    abase_p_8_poly qq, rr;
    abase_p_8_poly_init(K, qq, dega-degb+1);
    abase_p_8_poly_init(K, rr, dega);
    
    abase_p_8_poly_set(K, rr, a);
    abase_p_8_elt aux, aux2;
    
    abase_p_8_init(K, &aux);
    abase_p_8_init(K, &aux2);
    
    int i;
    int j;
    for (i = dega; i >= (int)degb; --i) {
        abase_p_8_poly_getcoef(K, aux, rr, i);
        if (!bmonic) 
            abase_p_8_mul(K, aux, aux, ilb);
        abase_p_8_poly_setcoef(K, qq, aux, i-degb);
        for (j = i-1; j >= (int)(i - degb); --j) {
            abase_p_8_mul(K, aux2, aux, (b->c)[j-i+degb]);
            abase_p_8_sub(K, (rr->c)[j], (rr->c)[j], aux2);
        }
    }    
    
    rr->size = degb;
    int degr = abase_p_8_poly_deg(K, rr);
    rr->size = degr+1;
    
    if (q != NULL) 
        abase_p_8_poly_set(K, q, qq);
    if (r != NULL)
        abase_p_8_poly_set(K, r, rr);
    abase_p_8_clear(K, &aux);
    abase_p_8_clear(K, &aux2);
    abase_p_8_poly_clear(K, rr);
    abase_p_8_poly_clear(K, qq);
}

static void abase_p_8_poly_preinv(abase_p_8_dst_field, abase_p_8_dst_poly, abase_p_8_src_poly, unsigned int);
static /* *Mpfq::defaults::poly::code_for_poly_precomp_mod, Mpfq::gfp */
void abase_p_8_poly_preinv(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_poly q, abase_p_8_src_poly p, unsigned int n)
{
    // Compute the inverse of p(x) modulo x^n
    // Newton iteration: x_{n+1} = x_n + x_n(1 - a*x_n)
    // Requires p(0) = 1
    // Assume p != q (no alias)
    assert (abase_p_8_cmp_ui(K, p->c[0], 1) == 0);
    assert (p != q);
    int m;
    if (n <= 2) {
        abase_p_8_poly_setcoef_ui(K, q, 1, 0);
        q->size = 1;
        m = 1;
        if (n == 1)
            return;
    } else {
        // n >= 3: recursive call at prec m = ceil(n/2)
        m = 1 + ((n-1)/2);
        abase_p_8_poly_preinv(K, q, p, m);
    }
    // enlarge q if necessary
    if (q->alloc < n) {
        abase_p_8_vec_reinit(K, &(q->c), q->alloc, n);
        q->alloc = n;
    }
    // refine value
    abase_p_8_vec tmp;
    abase_p_8_vec_init(K, &tmp, m+n-1);
    
    abase_p_8_vec_conv(K, tmp, p->c, MIN(n, p->size), q->c, m);
    int nn = MIN(n, MIN(n, p->size) + m -1);
    abase_p_8_vec_neg(K, tmp, tmp, nn);
    abase_p_8_add_ui(K, tmp[0], tmp[0], 1);
    abase_p_8_vec_conv(K, tmp, q->c, m, tmp, nn);
    abase_p_8_vec_set(K, q->c + m, tmp + m, n-m);
    q->size = n;
    
    abase_p_8_vec_clear(K, &tmp, m+n-1);
}

/* *Mpfq::defaults::poly::code_for_poly_precomp_mod, Mpfq::gfp */
void abase_p_8_poly_precomp_mod(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_poly q, abase_p_8_src_poly p)
{
    assert(p != q);
    int N = abase_p_8_poly_deg(K, p);
    abase_p_8_poly rp;
    abase_p_8_poly_init(K, rp, N+1);
    abase_p_8_vec_rev(K, rp->c, p->c, N+1);
    rp->size = N+1;
    abase_p_8_poly_preinv(K, q, rp, N);
    abase_p_8_poly_clear(K, rp);
}

/* *Mpfq::defaults::poly::code_for_poly_mod_pre, Mpfq::gfp */
void abase_p_8_poly_mod_pre(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_poly r, abase_p_8_src_poly q, abase_p_8_src_poly p, abase_p_8_src_poly irp)
{
    int N = abase_p_8_poly_deg(K, p);
    int degq = abase_p_8_poly_deg(K, q);
    if (degq < N) {
        abase_p_8_poly_set(K, r, q);
        return;
    }
    int m = degq - N;
    assert (degq <= 2*N-2);
    abase_p_8_poly revq;
    abase_p_8_poly_init(K, revq, MAX(degq+1, m+1));
    abase_p_8_vec_rev(K, revq->c, q->c, degq+1);
    revq->size = q->size;
    abase_p_8_poly_mul(K, revq, revq, irp);
    abase_p_8_vec_rev(K, revq->c, revq->c, m+1);
    revq->size = m+1;
    
    abase_p_8_poly_mul(K, revq, revq, p);
    abase_p_8_poly_sub(K, r, q, revq);
    r->size = abase_p_8_poly_deg(K, r)+1;
    abase_p_8_poly_clear(K, revq);
}


/* Functions related to SIMD operation */
/* *simd_gfp::code_for_dotprod */
void abase_p_8_dotprod(abase_p_8_dst_field K MAYBE_UNUSED, abase_p_8_dst_vec xw, abase_p_8_src_vec xu1, abase_p_8_src_vec xu0, unsigned int n)
{
        abase_p_8_elt_ur s,t;
        abase_p_8_elt_ur_init(K, &s);
        abase_p_8_elt_ur_init(K, &t);
        abase_p_8_elt_ur_set_zero(K, s);
        for(unsigned int i = 0 ; i < n ; i++) {
            abase_p_8_mul_ur(K, t, xu0[i], xu1[i]);
            abase_p_8_elt_ur_add(K, s, s, t);
        }
        abase_p_8_reduce(K, xw[0], s);
        abase_p_8_elt_ur_clear(K, &s);
        abase_p_8_elt_ur_clear(K, &t);
}


/* Member templates related to SIMD operation */

/* MPI interface */
static void abase_p_8_mpi_op_inner(void *, void *, int *, MPI_Datatype *);
static /* *Mpfq::defaults::mpi_flat::code_for_mpi_ops_init */
void abase_p_8_mpi_op_inner(void * invec, void * inoutvec, int * len, MPI_Datatype * datatype)
{
    int got_it;
    abase_p_8_dst_field K;
    MPI_Type_get_attr(*datatype, abase_p_8_impl_mpi_attr, (void*) &K, &got_it);
    assert(got_it);
    abase_p_8_vec_add(K, inoutvec, inoutvec, invec, *len);
}

static void abase_p_8_mpi_op_inner_ur(void *, void *, int *, MPI_Datatype *);
static /* *Mpfq::defaults::mpi_flat::code_for_mpi_ops_init */
void abase_p_8_mpi_op_inner_ur(void * invec, void * inoutvec, int * len, MPI_Datatype * datatype)
{
    int got_it;
    abase_p_8_dst_field K;
    MPI_Type_get_attr(*datatype, abase_p_8_impl_mpi_attr, (void*) &K, &got_it);
    assert(got_it);
    abase_p_8_vec_ur_add(K, inoutvec, inoutvec, invec, *len);
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_ops_init */
void abase_p_8_mpi_ops_init(abase_p_8_dst_field K MAYBE_UNUSED)
{
        if (abase_p_8_impl_mpi_use_count++) return;
    MPI_Type_create_keyval(MPI_TYPE_DUP_FN, MPI_TYPE_NULL_DELETE_FN, &abase_p_8_impl_mpi_attr, NULL);
    MPI_Type_contiguous(sizeof(abase_p_8_elt), MPI_BYTE, &abase_p_8_impl_mpi_datatype);
    MPI_Type_commit(&abase_p_8_impl_mpi_datatype);
    MPI_Type_contiguous(sizeof(abase_p_8_elt_ur), MPI_BYTE, &abase_p_8_impl_mpi_datatype_ur);
    MPI_Type_commit(&abase_p_8_impl_mpi_datatype_ur);
    MPI_Type_set_attr(abase_p_8_impl_mpi_datatype, abase_p_8_impl_mpi_attr, K);
    MPI_Type_set_attr(abase_p_8_impl_mpi_datatype_ur, abase_p_8_impl_mpi_attr, K);
    /* 1 here indicates that our operation is always taken to be
     * commutative */
    MPI_Op_create(&abase_p_8_mpi_op_inner, 1, &abase_p_8_impl_mpi_addition_op);
    MPI_Op_create(&abase_p_8_mpi_op_inner_ur, 1, &abase_p_8_impl_mpi_addition_op_ur);
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_datatype */
MPI_Datatype abase_p_8_mpi_datatype(abase_p_8_dst_field K MAYBE_UNUSED)
{
    return abase_p_8_impl_mpi_datatype;
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_datatype_ur */
MPI_Datatype abase_p_8_mpi_datatype_ur(abase_p_8_dst_field K MAYBE_UNUSED)
{
    return abase_p_8_impl_mpi_datatype_ur;
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_addition_op */
MPI_Op abase_p_8_mpi_addition_op(abase_p_8_dst_field K MAYBE_UNUSED)
{
    return abase_p_8_impl_mpi_addition_op;
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_addition_op_ur */
MPI_Op abase_p_8_mpi_addition_op_ur(abase_p_8_dst_field K MAYBE_UNUSED)
{
    return abase_p_8_impl_mpi_addition_op_ur;
}

/* *Mpfq::defaults::mpi_flat::code_for_mpi_ops_clear */
void abase_p_8_mpi_ops_clear(abase_p_8_dst_field K MAYBE_UNUSED)
{
        if (--abase_p_8_impl_mpi_use_count) return;
    MPI_Op_free(&abase_p_8_impl_mpi_addition_op);
    MPI_Op_free(&abase_p_8_impl_mpi_addition_op_ur);
    MPI_Type_delete_attr(abase_p_8_impl_mpi_datatype, abase_p_8_impl_mpi_attr);
    MPI_Type_delete_attr(abase_p_8_impl_mpi_datatype_ur, abase_p_8_impl_mpi_attr);
    MPI_Type_free(&abase_p_8_impl_mpi_datatype);
    MPI_Type_free(&abase_p_8_impl_mpi_datatype_ur);
    MPI_Type_free_keyval(&abase_p_8_impl_mpi_attr);
}


/* Object-oriented interface */
static void abase_p_8_wrapper_field_characteristic(abase_vbase_ptr, mpz_t);
static void abase_p_8_wrapper_field_characteristic(abase_vbase_ptr vbase MAYBE_UNUSED, mpz_t z MAYBE_UNUSED)
{
    abase_p_8_field_characteristic(vbase->obj, z);
}

static int abase_p_8_wrapper_field_degree(abase_vbase_ptr);
static int abase_p_8_wrapper_field_degree(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_field_degree(vbase->obj);
}

static void abase_p_8_wrapper_field_init(abase_vbase_ptr);
static void abase_p_8_wrapper_field_init(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_field_init(vbase->obj);
}

static void abase_p_8_wrapper_field_clear(abase_vbase_ptr);
static void abase_p_8_wrapper_field_clear(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_field_clear(vbase->obj);
}

static void abase_p_8_wrapper_field_specify(abase_vbase_ptr, unsigned long, void *);
static void abase_p_8_wrapper_field_specify(abase_vbase_ptr vbase MAYBE_UNUSED, unsigned long dummy MAYBE_UNUSED, void * vp MAYBE_UNUSED)
{
    abase_p_8_field_specify(vbase->obj, dummy, vp);
}

static void abase_p_8_wrapper_field_setopt(abase_vbase_ptr, unsigned long, void *);
static void abase_p_8_wrapper_field_setopt(abase_vbase_ptr vbase MAYBE_UNUSED, unsigned long x MAYBE_UNUSED, void * y MAYBE_UNUSED)
{
    abase_p_8_field_setopt(vbase->obj, x, y);
}

static void abase_p_8_wrapper_init(abase_vbase_ptr, abase_p_8_elt *);
static void abase_p_8_wrapper_init(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_elt * x MAYBE_UNUSED)
{
    abase_p_8_init(vbase->obj, x);
}

static void abase_p_8_wrapper_clear(abase_vbase_ptr, abase_p_8_elt *);
static void abase_p_8_wrapper_clear(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_elt * x MAYBE_UNUSED)
{
    abase_p_8_clear(vbase->obj, x);
}

static void abase_p_8_wrapper_set(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_set(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt r MAYBE_UNUSED, abase_p_8_src_elt s MAYBE_UNUSED)
{
    abase_p_8_set(vbase->obj, r, s);
}

static void abase_p_8_wrapper_set_ui(abase_vbase_ptr, abase_p_8_dst_elt, unsigned long);
static void abase_p_8_wrapper_set_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt r MAYBE_UNUSED, unsigned long x MAYBE_UNUSED)
{
    abase_p_8_set_ui(vbase->obj, r, x);
}

static void abase_p_8_wrapper_set_zero(abase_vbase_ptr, abase_p_8_dst_elt);
static void abase_p_8_wrapper_set_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt r MAYBE_UNUSED)
{
    abase_p_8_set_zero(vbase->obj, r);
}

static unsigned long abase_p_8_wrapper_get_ui(abase_vbase_ptr, abase_p_8_src_elt);
static unsigned long abase_p_8_wrapper_get_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    return abase_p_8_get_ui(vbase->obj, x);
}

static void abase_p_8_wrapper_set_mpn(abase_vbase_ptr, abase_p_8_dst_elt, mp_limb_t *, size_t);
static void abase_p_8_wrapper_set_mpn(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt r MAYBE_UNUSED, mp_limb_t * x MAYBE_UNUSED, size_t n MAYBE_UNUSED)
{
    abase_p_8_set_mpn(vbase->obj, r, x, n);
}

static void abase_p_8_wrapper_set_mpz(abase_vbase_ptr, abase_p_8_dst_elt, mpz_t);
static void abase_p_8_wrapper_set_mpz(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt r MAYBE_UNUSED, mpz_t z MAYBE_UNUSED)
{
    abase_p_8_set_mpz(vbase->obj, r, z);
}

static void abase_p_8_wrapper_get_mpn(abase_vbase_ptr, mp_limb_t *, abase_p_8_src_elt);
static void abase_p_8_wrapper_get_mpn(abase_vbase_ptr vbase MAYBE_UNUSED, mp_limb_t * r MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_get_mpn(vbase->obj, r, x);
}

static void abase_p_8_wrapper_get_mpz(abase_vbase_ptr, mpz_t, abase_p_8_src_elt);
static void abase_p_8_wrapper_get_mpz(abase_vbase_ptr vbase MAYBE_UNUSED, mpz_t z MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_get_mpz(vbase->obj, z, y);
}

static void abase_p_8_wrapper_random(abase_vbase_ptr, abase_p_8_dst_elt, gmp_randstate_t);
static void abase_p_8_wrapper_random(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_random(vbase->obj, x, state);
}

static void abase_p_8_wrapper_random2(abase_vbase_ptr, abase_p_8_dst_elt, gmp_randstate_t);
static void abase_p_8_wrapper_random2(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_random2(vbase->obj, x, state);
}

static void abase_p_8_wrapper_add(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_add(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_add(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_sub(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_sub(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_sub(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_neg(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_neg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_neg(vbase->obj, z, x);
}

static void abase_p_8_wrapper_mul(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_mul(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_mul(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_sqr(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_sqr(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_sqr(vbase->obj, z, x);
}

static int abase_p_8_wrapper_is_sqr(abase_vbase_ptr, abase_p_8_src_elt);
static int abase_p_8_wrapper_is_sqr(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    return abase_p_8_is_sqr(vbase->obj, x);
}

static int abase_p_8_wrapper_sqrt(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static int abase_p_8_wrapper_sqrt(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt a MAYBE_UNUSED)
{
    return abase_p_8_sqrt(vbase->obj, z, a);
}

static void abase_p_8_wrapper_pow(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, unsigned long *, size_t);
static void abase_p_8_wrapper_pow(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt res MAYBE_UNUSED, abase_p_8_src_elt r MAYBE_UNUSED, unsigned long * x MAYBE_UNUSED, size_t n MAYBE_UNUSED)
{
    abase_p_8_pow(vbase->obj, res, r, x, n);
}

static void abase_p_8_wrapper_frobenius(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_frobenius(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_frobenius(vbase->obj, x, y);
}

static void abase_p_8_wrapper_add_ui(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, unsigned long);
static void abase_p_8_wrapper_add_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned long y MAYBE_UNUSED)
{
    abase_p_8_add_ui(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_sub_ui(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, unsigned long);
static void abase_p_8_wrapper_sub_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned long y MAYBE_UNUSED)
{
    abase_p_8_sub_ui(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_mul_ui(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt, unsigned long);
static void abase_p_8_wrapper_mul_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned long y MAYBE_UNUSED)
{
    abase_p_8_mul_ui(vbase->obj, z, x, y);
}

static int abase_p_8_wrapper_inv(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_elt);
static int abase_p_8_wrapper_inv(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    return abase_p_8_inv(vbase->obj, z, x);
}

static void abase_p_8_wrapper_hadamard(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_dst_elt, abase_p_8_dst_elt, abase_p_8_dst_elt);
static void abase_p_8_wrapper_hadamard(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, abase_p_8_dst_elt y MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_dst_elt t MAYBE_UNUSED)
{
    abase_p_8_hadamard(vbase->obj, x, y, z, t);
}

static void abase_p_8_wrapper_elt_ur_init(abase_vbase_ptr, abase_p_8_elt_ur *);
static void abase_p_8_wrapper_elt_ur_init(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_elt_ur * x MAYBE_UNUSED)
{
    abase_p_8_elt_ur_init(vbase->obj, x);
}

static void abase_p_8_wrapper_elt_ur_clear(abase_vbase_ptr, abase_p_8_elt_ur *);
static void abase_p_8_wrapper_elt_ur_clear(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_elt_ur * x MAYBE_UNUSED)
{
    abase_p_8_elt_ur_clear(vbase->obj, x);
}

static void abase_p_8_wrapper_elt_ur_set(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt_ur);
static void abase_p_8_wrapper_elt_ur_set(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt_ur x MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set(vbase->obj, z, x);
}

static void abase_p_8_wrapper_elt_ur_set_elt(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt);
static void abase_p_8_wrapper_elt_ur_set_elt(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur r MAYBE_UNUSED, abase_p_8_src_elt s MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set_elt(vbase->obj, r, s);
}

static void abase_p_8_wrapper_elt_ur_set_zero(abase_vbase_ptr, abase_p_8_dst_elt_ur);
static void abase_p_8_wrapper_elt_ur_set_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur r MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set_zero(vbase->obj, r);
}

static void abase_p_8_wrapper_elt_ur_set_ui(abase_vbase_ptr, abase_p_8_dst_elt_ur, unsigned long);
static void abase_p_8_wrapper_elt_ur_set_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur r MAYBE_UNUSED, unsigned long x MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set_ui(vbase->obj, r, x);
}

static void abase_p_8_wrapper_elt_ur_add(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt_ur, abase_p_8_src_elt_ur);
static void abase_p_8_wrapper_elt_ur_add(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt_ur x MAYBE_UNUSED, abase_p_8_src_elt_ur y MAYBE_UNUSED)
{
    abase_p_8_elt_ur_add(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_elt_ur_neg(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt_ur);
static void abase_p_8_wrapper_elt_ur_neg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt_ur x MAYBE_UNUSED)
{
    abase_p_8_elt_ur_neg(vbase->obj, z, x);
}

static void abase_p_8_wrapper_elt_ur_sub(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt_ur, abase_p_8_src_elt_ur);
static void abase_p_8_wrapper_elt_ur_sub(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt_ur x MAYBE_UNUSED, abase_p_8_src_elt_ur y MAYBE_UNUSED)
{
    abase_p_8_elt_ur_sub(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_mul_ur(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt, abase_p_8_src_elt);
static void abase_p_8_wrapper_mul_ur(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    abase_p_8_mul_ur(vbase->obj, z, x, y);
}

static void abase_p_8_wrapper_sqr_ur(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt);
static void abase_p_8_wrapper_sqr_ur(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur z MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_sqr_ur(vbase->obj, z, x);
}

static void abase_p_8_wrapper_reduce(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_dst_elt_ur);
static void abase_p_8_wrapper_reduce(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, abase_p_8_dst_elt_ur x MAYBE_UNUSED)
{
    abase_p_8_reduce(vbase->obj, z, x);
}

static void abase_p_8_wrapper_normalize(abase_vbase_ptr, abase_p_8_dst_elt);
static void abase_p_8_wrapper_normalize(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED)
{
    abase_p_8_normalize(vbase->obj, x);
}

static void abase_p_8_wrapper_addmul_si_ur(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_elt, long);
static void abase_p_8_wrapper_addmul_si_ur(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur w MAYBE_UNUSED, abase_p_8_src_elt u MAYBE_UNUSED, long v MAYBE_UNUSED)
{
    abase_p_8_addmul_si_ur(vbase->obj, w, u, v);
}

static int abase_p_8_wrapper_cmp(abase_vbase_ptr, abase_p_8_src_elt, abase_p_8_src_elt);
static int abase_p_8_wrapper_cmp(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, abase_p_8_src_elt y MAYBE_UNUSED)
{
    return abase_p_8_cmp(vbase->obj, x, y);
}

static int abase_p_8_wrapper_cmp_ui(abase_vbase_ptr, abase_p_8_src_elt, unsigned long);
static int abase_p_8_wrapper_cmp_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned long y MAYBE_UNUSED)
{
    return abase_p_8_cmp_ui(vbase->obj, x, y);
}

static int abase_p_8_wrapper_is_zero(abase_vbase_ptr, abase_p_8_src_elt);
static int abase_p_8_wrapper_is_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt r MAYBE_UNUSED)
{
    return abase_p_8_is_zero(vbase->obj, r);
}

static void abase_p_8_wrapper_asprint(abase_vbase_ptr, char * *, abase_p_8_src_elt);
static void abase_p_8_wrapper_asprint(abase_vbase_ptr vbase MAYBE_UNUSED, char * * pstr MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_asprint(vbase->obj, pstr, x);
}

static void abase_p_8_wrapper_fprint(abase_vbase_ptr, FILE *, abase_p_8_src_elt);
static void abase_p_8_wrapper_fprint(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_fprint(vbase->obj, file, x);
}

static void abase_p_8_wrapper_print(abase_vbase_ptr, abase_p_8_src_elt);
static void abase_p_8_wrapper_print(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_print(vbase->obj, x);
}

static int abase_p_8_wrapper_sscan(abase_vbase_ptr, abase_p_8_dst_elt, const char *);
static int abase_p_8_wrapper_sscan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED, const char * str MAYBE_UNUSED)
{
    return abase_p_8_sscan(vbase->obj, z, str);
}

static int abase_p_8_wrapper_fscan(abase_vbase_ptr, FILE *, abase_p_8_dst_elt);
static int abase_p_8_wrapper_fscan(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_dst_elt z MAYBE_UNUSED)
{
    return abase_p_8_fscan(vbase->obj, file, z);
}

static int abase_p_8_wrapper_scan(abase_vbase_ptr, abase_p_8_dst_elt);
static int abase_p_8_wrapper_scan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED)
{
    return abase_p_8_scan(vbase->obj, x);
}

static void abase_p_8_wrapper_vec_init(abase_vbase_ptr, abase_p_8_vec *, unsigned int);
static void abase_p_8_wrapper_vec_init(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_init(vbase->obj, v, n);
}

static void abase_p_8_wrapper_vec_reinit(abase_vbase_ptr, abase_p_8_vec *, unsigned int, unsigned int);
static void abase_p_8_wrapper_vec_reinit(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, unsigned int m MAYBE_UNUSED)
{
    abase_p_8_vec_reinit(vbase->obj, v, n, m);
}

static void abase_p_8_wrapper_vec_clear(abase_vbase_ptr, abase_p_8_vec *, unsigned int);
static void abase_p_8_wrapper_vec_clear(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_clear(vbase->obj, v, n);
}

static void abase_p_8_wrapper_vec_set(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_set(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec r MAYBE_UNUSED, abase_p_8_src_vec s MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_set(vbase->obj, r, s, n);
}

static void abase_p_8_wrapper_vec_set_zero(abase_vbase_ptr, abase_p_8_dst_vec, unsigned int);
static void abase_p_8_wrapper_vec_set_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec r MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_set_zero(vbase->obj, r, n);
}

static void abase_p_8_wrapper_vec_setcoef(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_elt, unsigned int);
static void abase_p_8_wrapper_vec_setcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_vec_setcoef(vbase->obj, w, x, i);
}

static void abase_p_8_wrapper_vec_setcoef_ui(abase_vbase_ptr, abase_p_8_dst_vec, unsigned long, unsigned int);
static void abase_p_8_wrapper_vec_setcoef_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, unsigned long x MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_vec_setcoef_ui(vbase->obj, w, x, i);
}

static void abase_p_8_wrapper_vec_getcoef(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_getcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, abase_p_8_src_vec w MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_vec_getcoef(vbase->obj, x, w, i);
}

static void abase_p_8_wrapper_vec_add(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_add(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, abase_p_8_src_vec v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_add(vbase->obj, w, u, v, n);
}

static void abase_p_8_wrapper_vec_neg(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_neg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_neg(vbase->obj, w, u, n);
}

static void abase_p_8_wrapper_vec_rev(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_rev(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_rev(vbase->obj, w, u, n);
}

static void abase_p_8_wrapper_vec_sub(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_sub(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, abase_p_8_src_vec v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_sub(vbase->obj, w, u, v, n);
}

static void abase_p_8_wrapper_vec_scal_mul(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, abase_p_8_src_elt, unsigned int);
static void abase_p_8_wrapper_vec_scal_mul(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_scal_mul(vbase->obj, w, u, x, n);
}

static void abase_p_8_wrapper_vec_conv(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, unsigned int, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_conv(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, abase_p_8_src_vec v MAYBE_UNUSED, unsigned int m MAYBE_UNUSED)
{
    abase_p_8_vec_conv(vbase->obj, w, u, n, v, m);
}

static void abase_p_8_wrapper_vec_random(abase_vbase_ptr, abase_p_8_dst_vec, unsigned int, gmp_randstate_t);
static void abase_p_8_wrapper_vec_random(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_vec_random(vbase->obj, w, n, state);
}

static void abase_p_8_wrapper_vec_random2(abase_vbase_ptr, abase_p_8_dst_vec, unsigned int, gmp_randstate_t);
static void abase_p_8_wrapper_vec_random2(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_vec_random2(vbase->obj, w, n, state);
}

static int abase_p_8_wrapper_vec_cmp(abase_vbase_ptr, abase_p_8_src_vec, abase_p_8_src_vec, unsigned int);
static int abase_p_8_wrapper_vec_cmp(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, abase_p_8_src_vec v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    return abase_p_8_vec_cmp(vbase->obj, u, v, n);
}

static int abase_p_8_wrapper_vec_is_zero(abase_vbase_ptr, abase_p_8_src_vec, unsigned int);
static int abase_p_8_wrapper_vec_is_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_vec r MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    return abase_p_8_vec_is_zero(vbase->obj, r, n);
}

static void abase_p_8_wrapper_vec_asprint(abase_vbase_ptr, char * *, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_asprint(abase_vbase_ptr vbase MAYBE_UNUSED, char * * pstr MAYBE_UNUSED, abase_p_8_src_vec w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_asprint(vbase->obj, pstr, w, n);
}

static void abase_p_8_wrapper_vec_fprint(abase_vbase_ptr, FILE *, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_fprint(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_src_vec w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_fprint(vbase->obj, file, w, n);
}

static void abase_p_8_wrapper_vec_print(abase_vbase_ptr, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_print(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_vec w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_print(vbase->obj, w, n);
}

static int abase_p_8_wrapper_vec_sscan(abase_vbase_ptr, abase_p_8_vec *, unsigned int *, const char *);
static int abase_p_8_wrapper_vec_sscan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec * w MAYBE_UNUSED, unsigned int * n MAYBE_UNUSED, const char * str MAYBE_UNUSED)
{
    return abase_p_8_vec_sscan(vbase->obj, w, n, str);
}

static int abase_p_8_wrapper_vec_fscan(abase_vbase_ptr, FILE *, abase_p_8_vec *, unsigned int *);
static int abase_p_8_wrapper_vec_fscan(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_vec * w MAYBE_UNUSED, unsigned int * n MAYBE_UNUSED)
{
    return abase_p_8_vec_fscan(vbase->obj, file, w, n);
}

static int abase_p_8_wrapper_vec_scan(abase_vbase_ptr, abase_p_8_vec *, unsigned int *);
static int abase_p_8_wrapper_vec_scan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec * w MAYBE_UNUSED, unsigned int * n MAYBE_UNUSED)
{
    return abase_p_8_vec_scan(vbase->obj, w, n);
}

static void abase_p_8_wrapper_vec_ur_init(abase_vbase_ptr, abase_p_8_vec_ur *, unsigned int);
static void abase_p_8_wrapper_vec_ur_init(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec_ur * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_init(vbase->obj, v, n);
}

static void abase_p_8_wrapper_vec_ur_set_zero(abase_vbase_ptr, abase_p_8_dst_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_set_zero(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur r MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_set_zero(vbase->obj, r, n);
}

static void abase_p_8_wrapper_vec_ur_set_vec(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_ur_set_vec(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_set_vec(vbase->obj, w, u, n);
}

static void abase_p_8_wrapper_vec_ur_reinit(abase_vbase_ptr, abase_p_8_vec_ur *, unsigned int, unsigned int);
static void abase_p_8_wrapper_vec_ur_reinit(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec_ur * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, unsigned int m MAYBE_UNUSED)
{
    abase_p_8_vec_ur_reinit(vbase->obj, v, n, m);
}

static void abase_p_8_wrapper_vec_ur_clear(abase_vbase_ptr, abase_p_8_vec_ur *, unsigned int);
static void abase_p_8_wrapper_vec_ur_clear(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_vec_ur * v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_clear(vbase->obj, v, n);
}

static void abase_p_8_wrapper_vec_ur_set(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_set(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur r MAYBE_UNUSED, abase_p_8_src_vec_ur s MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_set(vbase->obj, r, s, n);
}

static void abase_p_8_wrapper_vec_ur_setcoef(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_elt_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_setcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_elt_ur x MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_vec_ur_setcoef(vbase->obj, w, x, i);
}

static void abase_p_8_wrapper_vec_ur_getcoef(abase_vbase_ptr, abase_p_8_dst_elt_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_getcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt_ur x MAYBE_UNUSED, abase_p_8_src_vec_ur w MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_vec_ur_getcoef(vbase->obj, x, w, i);
}

static void abase_p_8_wrapper_vec_ur_add(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_add(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec_ur u MAYBE_UNUSED, abase_p_8_src_vec_ur v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_add(vbase->obj, w, u, v, n);
}

static void abase_p_8_wrapper_vec_ur_sub(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_sub(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec_ur u MAYBE_UNUSED, abase_p_8_src_vec_ur v MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_sub(vbase->obj, w, u, v, n);
}

static void abase_p_8_wrapper_vec_ur_neg(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_neg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec_ur u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_neg(vbase->obj, w, u, n);
}

static void abase_p_8_wrapper_vec_ur_rev(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_ur_rev(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec_ur u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_ur_rev(vbase->obj, w, u, n);
}

static void abase_p_8_wrapper_vec_scal_mul_ur(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec, abase_p_8_src_elt, unsigned int);
static void abase_p_8_wrapper_vec_scal_mul_ur(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_scal_mul_ur(vbase->obj, w, u, x, n);
}

static void abase_p_8_wrapper_vec_conv_ur(abase_vbase_ptr, abase_p_8_dst_vec_ur, abase_p_8_src_vec, unsigned int, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_vec_conv_ur(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec_ur w MAYBE_UNUSED, abase_p_8_src_vec u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, abase_p_8_src_vec v MAYBE_UNUSED, unsigned int m MAYBE_UNUSED)
{
    abase_p_8_vec_conv_ur(vbase->obj, w, u, n, v, m);
}

static void abase_p_8_wrapper_vec_reduce(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_dst_vec_ur, unsigned int);
static void abase_p_8_wrapper_vec_reduce(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec w MAYBE_UNUSED, abase_p_8_dst_vec_ur u MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_vec_reduce(vbase->obj, w, u, n);
}

static ptrdiff_t abase_p_8_wrapper_vec_elt_stride(abase_vbase_ptr, int);
static ptrdiff_t abase_p_8_wrapper_vec_elt_stride(abase_vbase_ptr vbase MAYBE_UNUSED, int n MAYBE_UNUSED)
{
    return abase_p_8_vec_elt_stride(vbase->obj, n);
}

static void abase_p_8_wrapper_poly_init(abase_vbase_ptr, abase_p_8_poly, unsigned int);
static void abase_p_8_wrapper_poly_init(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_poly p MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_poly_init(vbase->obj, p, n);
}

static void abase_p_8_wrapper_poly_clear(abase_vbase_ptr, abase_p_8_poly);
static void abase_p_8_wrapper_poly_clear(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_poly p MAYBE_UNUSED)
{
    abase_p_8_poly_clear(vbase->obj, p);
}

static void abase_p_8_wrapper_poly_set(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_set(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED)
{
    abase_p_8_poly_set(vbase->obj, w, u);
}

static void abase_p_8_wrapper_poly_setmonic(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_setmonic(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly q MAYBE_UNUSED, abase_p_8_src_poly p MAYBE_UNUSED)
{
    abase_p_8_poly_setmonic(vbase->obj, q, p);
}

static void abase_p_8_wrapper_poly_setcoef(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_elt, unsigned int);
static void abase_p_8_wrapper_poly_setcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_poly_setcoef(vbase->obj, w, x, i);
}

static void abase_p_8_wrapper_poly_setcoef_ui(abase_vbase_ptr, abase_p_8_dst_poly, unsigned long, unsigned int);
static void abase_p_8_wrapper_poly_setcoef_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, unsigned long x MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_poly_setcoef_ui(vbase->obj, w, x, i);
}

static void abase_p_8_wrapper_poly_getcoef(abase_vbase_ptr, abase_p_8_dst_elt, abase_p_8_src_poly, unsigned int);
static void abase_p_8_wrapper_poly_getcoef(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt x MAYBE_UNUSED, abase_p_8_src_poly w MAYBE_UNUSED, unsigned int i MAYBE_UNUSED)
{
    abase_p_8_poly_getcoef(vbase->obj, x, w, i);
}

static int abase_p_8_wrapper_poly_deg(abase_vbase_ptr, abase_p_8_src_poly);
static int abase_p_8_wrapper_poly_deg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_poly w MAYBE_UNUSED)
{
    return abase_p_8_poly_deg(vbase->obj, w);
}

static void abase_p_8_wrapper_poly_add(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_add(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, abase_p_8_src_poly v MAYBE_UNUSED)
{
    abase_p_8_poly_add(vbase->obj, w, u, v);
}

static void abase_p_8_wrapper_poly_sub(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_sub(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, abase_p_8_src_poly v MAYBE_UNUSED)
{
    abase_p_8_poly_sub(vbase->obj, w, u, v);
}

static void abase_p_8_wrapper_poly_add_ui(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, unsigned long);
static void abase_p_8_wrapper_poly_add_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, unsigned long x MAYBE_UNUSED)
{
    abase_p_8_poly_add_ui(vbase->obj, w, u, x);
}

static void abase_p_8_wrapper_poly_sub_ui(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, unsigned long);
static void abase_p_8_wrapper_poly_sub_ui(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, unsigned long x MAYBE_UNUSED)
{
    abase_p_8_poly_sub_ui(vbase->obj, w, u, x);
}

static void abase_p_8_wrapper_poly_neg(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_neg(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED)
{
    abase_p_8_poly_neg(vbase->obj, w, u);
}

static void abase_p_8_wrapper_poly_scal_mul(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_elt);
static void abase_p_8_wrapper_poly_scal_mul(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, abase_p_8_src_elt x MAYBE_UNUSED)
{
    abase_p_8_poly_scal_mul(vbase->obj, w, u, x);
}

static void abase_p_8_wrapper_poly_mul(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_mul(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, abase_p_8_src_poly v MAYBE_UNUSED)
{
    abase_p_8_poly_mul(vbase->obj, w, u, v);
}

static void abase_p_8_wrapper_poly_divmod(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_divmod(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly q MAYBE_UNUSED, abase_p_8_dst_poly r MAYBE_UNUSED, abase_p_8_src_poly a MAYBE_UNUSED, abase_p_8_src_poly b MAYBE_UNUSED)
{
    abase_p_8_poly_divmod(vbase->obj, q, r, a, b);
}

static void abase_p_8_wrapper_poly_precomp_mod(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_precomp_mod(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly q MAYBE_UNUSED, abase_p_8_src_poly p MAYBE_UNUSED)
{
    abase_p_8_poly_precomp_mod(vbase->obj, q, p);
}

static void abase_p_8_wrapper_poly_mod_pre(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_mod_pre(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly r MAYBE_UNUSED, abase_p_8_src_poly q MAYBE_UNUSED, abase_p_8_src_poly p MAYBE_UNUSED, abase_p_8_src_poly irp MAYBE_UNUSED)
{
    abase_p_8_poly_mod_pre(vbase->obj, r, q, p, irp);
}

static void abase_p_8_wrapper_poly_gcd(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_gcd(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly g MAYBE_UNUSED, abase_p_8_src_poly a0 MAYBE_UNUSED, abase_p_8_src_poly b0 MAYBE_UNUSED)
{
    abase_p_8_poly_gcd(vbase->obj, g, a0, b0);
}

static void abase_p_8_wrapper_poly_xgcd(abase_vbase_ptr, abase_p_8_dst_poly, abase_p_8_dst_poly, abase_p_8_dst_poly, abase_p_8_src_poly, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_xgcd(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly g MAYBE_UNUSED, abase_p_8_dst_poly u0 MAYBE_UNUSED, abase_p_8_dst_poly v0 MAYBE_UNUSED, abase_p_8_src_poly a0 MAYBE_UNUSED, abase_p_8_src_poly b0 MAYBE_UNUSED)
{
    abase_p_8_poly_xgcd(vbase->obj, g, u0, v0, a0, b0);
}

static void abase_p_8_wrapper_poly_random(abase_vbase_ptr, abase_p_8_dst_poly, unsigned int, gmp_randstate_t);
static void abase_p_8_wrapper_poly_random(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_poly_random(vbase->obj, w, n, state);
}

static void abase_p_8_wrapper_poly_random2(abase_vbase_ptr, abase_p_8_dst_poly, unsigned int, gmp_randstate_t);
static void abase_p_8_wrapper_poly_random2(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, unsigned int n MAYBE_UNUSED, gmp_randstate_t state MAYBE_UNUSED)
{
    abase_p_8_poly_random2(vbase->obj, w, n, state);
}

static int abase_p_8_wrapper_poly_cmp(abase_vbase_ptr, abase_p_8_src_poly, abase_p_8_src_poly);
static int abase_p_8_wrapper_poly_cmp(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_poly u MAYBE_UNUSED, abase_p_8_src_poly v MAYBE_UNUSED)
{
    return abase_p_8_poly_cmp(vbase->obj, u, v);
}

static void abase_p_8_wrapper_poly_asprint(abase_vbase_ptr, char * *, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_asprint(abase_vbase_ptr vbase MAYBE_UNUSED, char * * pstr MAYBE_UNUSED, abase_p_8_src_poly w MAYBE_UNUSED)
{
    abase_p_8_poly_asprint(vbase->obj, pstr, w);
}

static void abase_p_8_wrapper_poly_fprint(abase_vbase_ptr, FILE *, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_fprint(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_src_poly w MAYBE_UNUSED)
{
    abase_p_8_poly_fprint(vbase->obj, file, w);
}

static void abase_p_8_wrapper_poly_print(abase_vbase_ptr, abase_p_8_src_poly);
static void abase_p_8_wrapper_poly_print(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_src_poly w MAYBE_UNUSED)
{
    abase_p_8_poly_print(vbase->obj, w);
}

static int abase_p_8_wrapper_poly_sscan(abase_vbase_ptr, abase_p_8_dst_poly, const char *);
static int abase_p_8_wrapper_poly_sscan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED, const char * str MAYBE_UNUSED)
{
    return abase_p_8_poly_sscan(vbase->obj, w, str);
}

static int abase_p_8_wrapper_poly_fscan(abase_vbase_ptr, FILE *, abase_p_8_dst_poly);
static int abase_p_8_wrapper_poly_fscan(abase_vbase_ptr vbase MAYBE_UNUSED, FILE * file MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED)
{
    return abase_p_8_poly_fscan(vbase->obj, file, w);
}

static int abase_p_8_wrapper_poly_scan(abase_vbase_ptr, abase_p_8_dst_poly);
static int abase_p_8_wrapper_poly_scan(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_poly w MAYBE_UNUSED)
{
    return abase_p_8_poly_scan(vbase->obj, w);
}

static int abase_p_8_wrapper_groupsize(abase_vbase_ptr);
static int abase_p_8_wrapper_groupsize(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_groupsize(vbase->obj);
}

static int abase_p_8_wrapper_offset(abase_vbase_ptr, int);
static int abase_p_8_wrapper_offset(abase_vbase_ptr vbase MAYBE_UNUSED, int n MAYBE_UNUSED)
{
    return abase_p_8_offset(vbase->obj, n);
}

static int abase_p_8_wrapper_stride(abase_vbase_ptr);
static int abase_p_8_wrapper_stride(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_stride(vbase->obj);
}

static void abase_p_8_wrapper_set_ui_at(abase_vbase_ptr, abase_p_8_dst_elt, int, unsigned long);
static void abase_p_8_wrapper_set_ui_at(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt p MAYBE_UNUSED, int k MAYBE_UNUSED, unsigned long v MAYBE_UNUSED)
{
    abase_p_8_set_ui_at(vbase->obj, p, k, v);
}

static void abase_p_8_wrapper_set_ui_all(abase_vbase_ptr, abase_p_8_dst_elt, unsigned long);
static void abase_p_8_wrapper_set_ui_all(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt p MAYBE_UNUSED, unsigned long v MAYBE_UNUSED)
{
    abase_p_8_set_ui_all(vbase->obj, p, v);
}

static void abase_p_8_wrapper_elt_ur_set_ui_at(abase_vbase_ptr, abase_p_8_dst_elt, int, unsigned long);
static void abase_p_8_wrapper_elt_ur_set_ui_at(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt p MAYBE_UNUSED, int k MAYBE_UNUSED, unsigned long v MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set_ui_at(vbase->obj, p, k, v);
}

static void abase_p_8_wrapper_elt_ur_set_ui_all(abase_vbase_ptr, abase_p_8_dst_elt, unsigned long);
static void abase_p_8_wrapper_elt_ur_set_ui_all(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_elt p MAYBE_UNUSED, unsigned long v MAYBE_UNUSED)
{
    abase_p_8_elt_ur_set_ui_all(vbase->obj, p, v);
}

static void abase_p_8_wrapper_dotprod(abase_vbase_ptr, abase_p_8_dst_vec, abase_p_8_src_vec, abase_p_8_src_vec, unsigned int);
static void abase_p_8_wrapper_dotprod(abase_vbase_ptr vbase MAYBE_UNUSED, abase_p_8_dst_vec xw MAYBE_UNUSED, abase_p_8_src_vec xu1 MAYBE_UNUSED, abase_p_8_src_vec xu0 MAYBE_UNUSED, unsigned int n MAYBE_UNUSED)
{
    abase_p_8_dotprod(vbase->obj, xw, xu1, xu0, n);
}

static void abase_p_8_wrapper_mpi_ops_init(abase_vbase_ptr);
static void abase_p_8_wrapper_mpi_ops_init(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_mpi_ops_init(vbase->obj);
}

static MPI_Datatype abase_p_8_wrapper_mpi_datatype(abase_vbase_ptr);
static MPI_Datatype abase_p_8_wrapper_mpi_datatype(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_mpi_datatype(vbase->obj);
}

static MPI_Datatype abase_p_8_wrapper_mpi_datatype_ur(abase_vbase_ptr);
static MPI_Datatype abase_p_8_wrapper_mpi_datatype_ur(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_mpi_datatype_ur(vbase->obj);
}

static MPI_Op abase_p_8_wrapper_mpi_addition_op(abase_vbase_ptr);
static MPI_Op abase_p_8_wrapper_mpi_addition_op(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_mpi_addition_op(vbase->obj);
}

static MPI_Op abase_p_8_wrapper_mpi_addition_op_ur(abase_vbase_ptr);
static MPI_Op abase_p_8_wrapper_mpi_addition_op_ur(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_mpi_addition_op_ur(vbase->obj);
}

static void abase_p_8_wrapper_mpi_ops_clear(abase_vbase_ptr);
static void abase_p_8_wrapper_mpi_ops_clear(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_mpi_ops_clear(vbase->obj);
}

static const char * abase_p_8_wrapper_oo_impl_name(abase_vbase_ptr);
static const char * abase_p_8_wrapper_oo_impl_name(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    return abase_p_8_oo_impl_name(vbase);
}

static void abase_p_8_wrapper_oo_field_init(abase_vbase_ptr);
static void abase_p_8_wrapper_oo_field_init(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_oo_field_init(vbase);
}

static void abase_p_8_wrapper_oo_field_clear(abase_vbase_ptr);
static void abase_p_8_wrapper_oo_field_clear(abase_vbase_ptr vbase MAYBE_UNUSED)
{
    abase_p_8_oo_field_clear(vbase);
}

void abase_p_8_oo_field_init(abase_vbase_ptr vbase)
{
    memset(vbase, 0, sizeof(struct abase_vbase_s));
    vbase->obj = malloc(sizeof(abase_p_8_field));
    abase_p_8_field_init((abase_p_8_dst_field) vbase->obj);
    vbase->field_characteristic = (void (*) (abase_vbase_ptr, mpz_t)) abase_p_8_wrapper_field_characteristic;
    vbase->field_degree = (int (*) (abase_vbase_ptr)) abase_p_8_wrapper_field_degree;
    vbase->field_init = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_field_init;
    vbase->field_clear = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_field_clear;
    vbase->field_specify = (void (*) (abase_vbase_ptr, unsigned long, void *)) abase_p_8_wrapper_field_specify;
    vbase->field_setopt = (void (*) (abase_vbase_ptr, unsigned long, void *)) abase_p_8_wrapper_field_setopt;
    vbase->init = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_init;
    vbase->clear = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_clear;
    vbase->set = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_set;
    vbase->set_ui = (void (*) (abase_vbase_ptr, void *, unsigned long)) abase_p_8_wrapper_set_ui;
    vbase->set_zero = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_set_zero;
    vbase->get_ui = (unsigned long (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_get_ui;
    vbase->set_mpn = (void (*) (abase_vbase_ptr, void *, mp_limb_t *, size_t)) abase_p_8_wrapper_set_mpn;
    vbase->set_mpz = (void (*) (abase_vbase_ptr, void *, mpz_t)) abase_p_8_wrapper_set_mpz;
    vbase->get_mpn = (void (*) (abase_vbase_ptr, mp_limb_t *, const void *)) abase_p_8_wrapper_get_mpn;
    vbase->get_mpz = (void (*) (abase_vbase_ptr, mpz_t, const void *)) abase_p_8_wrapper_get_mpz;
    vbase->random = (void (*) (abase_vbase_ptr, void *, gmp_randstate_t)) abase_p_8_wrapper_random;
    vbase->random2 = (void (*) (abase_vbase_ptr, void *, gmp_randstate_t)) abase_p_8_wrapper_random2;
    vbase->add = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_add;
    vbase->sub = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_sub;
    vbase->neg = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_neg;
    vbase->mul = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_mul;
    vbase->sqr = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_sqr;
    vbase->is_sqr = (int (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_is_sqr;
    vbase->sqrt = (int (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_sqrt;
    vbase->pow = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long *, size_t)) abase_p_8_wrapper_pow;
    vbase->frobenius = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_frobenius;
    vbase->add_ui = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long)) abase_p_8_wrapper_add_ui;
    vbase->sub_ui = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long)) abase_p_8_wrapper_sub_ui;
    vbase->mul_ui = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long)) abase_p_8_wrapper_mul_ui;
    vbase->inv = (int (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_inv;
    vbase->hadamard = (void (*) (abase_vbase_ptr, void *, void *, void *, void *)) abase_p_8_wrapper_hadamard;
    vbase->elt_ur_init = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_elt_ur_init;
    vbase->elt_ur_clear = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_elt_ur_clear;
    vbase->elt_ur_set = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_elt_ur_set;
    vbase->elt_ur_set_elt = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_elt_ur_set_elt;
    vbase->elt_ur_set_zero = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_elt_ur_set_zero;
    vbase->elt_ur_set_ui = (void (*) (abase_vbase_ptr, void *, unsigned long)) abase_p_8_wrapper_elt_ur_set_ui;
    vbase->elt_ur_add = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_elt_ur_add;
    vbase->elt_ur_neg = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_elt_ur_neg;
    vbase->elt_ur_sub = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_elt_ur_sub;
    vbase->mul_ur = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_mul_ur;
    vbase->sqr_ur = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_sqr_ur;
    vbase->reduce = (void (*) (abase_vbase_ptr, void *, void *)) abase_p_8_wrapper_reduce;
    vbase->normalize = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_normalize;
    vbase->addmul_si_ur = (void (*) (abase_vbase_ptr, void *, const void *, long)) abase_p_8_wrapper_addmul_si_ur;
    vbase->cmp = (int (*) (abase_vbase_ptr, const void *, const void *)) abase_p_8_wrapper_cmp;
    vbase->cmp_ui = (int (*) (abase_vbase_ptr, const void *, unsigned long)) abase_p_8_wrapper_cmp_ui;
    vbase->is_zero = (int (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_is_zero;
    vbase->asprint = (void (*) (abase_vbase_ptr, char * *, const void *)) abase_p_8_wrapper_asprint;
    vbase->fprint = (void (*) (abase_vbase_ptr, FILE *, const void *)) abase_p_8_wrapper_fprint;
    vbase->print = (void (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_print;
    vbase->sscan = (int (*) (abase_vbase_ptr, void *, const char *)) abase_p_8_wrapper_sscan;
    vbase->fscan = (int (*) (abase_vbase_ptr, FILE *, void *)) abase_p_8_wrapper_fscan;
    vbase->scan = (int (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_scan;
    vbase->vec_init = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_init;
    vbase->vec_reinit = (void (*) (abase_vbase_ptr, void *, unsigned int, unsigned int)) abase_p_8_wrapper_vec_reinit;
    vbase->vec_clear = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_clear;
    vbase->vec_set = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_set;
    vbase->vec_set_zero = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_set_zero;
    vbase->vec_setcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_setcoef;
    vbase->vec_setcoef_ui = (void (*) (abase_vbase_ptr, void *, unsigned long, unsigned int)) abase_p_8_wrapper_vec_setcoef_ui;
    vbase->vec_getcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_getcoef;
    vbase->vec_add = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_add;
    vbase->vec_neg = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_neg;
    vbase->vec_rev = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_rev;
    vbase->vec_sub = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_sub;
    vbase->vec_scal_mul = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_scal_mul;
    vbase->vec_conv = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int, const void *, unsigned int)) abase_p_8_wrapper_vec_conv;
    vbase->vec_random = (void (*) (abase_vbase_ptr, void *, unsigned int, gmp_randstate_t)) abase_p_8_wrapper_vec_random;
    vbase->vec_random2 = (void (*) (abase_vbase_ptr, void *, unsigned int, gmp_randstate_t)) abase_p_8_wrapper_vec_random2;
    vbase->vec_cmp = (int (*) (abase_vbase_ptr, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_cmp;
    vbase->vec_is_zero = (int (*) (abase_vbase_ptr, const void *, unsigned int)) abase_p_8_wrapper_vec_is_zero;
    vbase->vec_asprint = (void (*) (abase_vbase_ptr, char * *, const void *, unsigned int)) abase_p_8_wrapper_vec_asprint;
    vbase->vec_fprint = (void (*) (abase_vbase_ptr, FILE *, const void *, unsigned int)) abase_p_8_wrapper_vec_fprint;
    vbase->vec_print = (void (*) (abase_vbase_ptr, const void *, unsigned int)) abase_p_8_wrapper_vec_print;
    vbase->vec_sscan = (int (*) (abase_vbase_ptr, void *, unsigned int *, const char *)) abase_p_8_wrapper_vec_sscan;
    vbase->vec_fscan = (int (*) (abase_vbase_ptr, FILE *, void *, unsigned int *)) abase_p_8_wrapper_vec_fscan;
    vbase->vec_scan = (int (*) (abase_vbase_ptr, void *, unsigned int *)) abase_p_8_wrapper_vec_scan;
    vbase->vec_ur_init = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_ur_init;
    vbase->vec_ur_set_zero = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_ur_set_zero;
    vbase->vec_ur_set_vec = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_set_vec;
    vbase->vec_ur_reinit = (void (*) (abase_vbase_ptr, void *, unsigned int, unsigned int)) abase_p_8_wrapper_vec_ur_reinit;
    vbase->vec_ur_clear = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_vec_ur_clear;
    vbase->vec_ur_set = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_set;
    vbase->vec_ur_setcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_setcoef;
    vbase->vec_ur_getcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_getcoef;
    vbase->vec_ur_add = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_add;
    vbase->vec_ur_sub = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_sub;
    vbase->vec_ur_neg = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_neg;
    vbase->vec_ur_rev = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_vec_ur_rev;
    vbase->vec_scal_mul_ur = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_vec_scal_mul_ur;
    vbase->vec_conv_ur = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int, const void *, unsigned int)) abase_p_8_wrapper_vec_conv_ur;
    vbase->vec_reduce = (void (*) (abase_vbase_ptr, void *, void *, unsigned int)) abase_p_8_wrapper_vec_reduce;
    vbase->vec_elt_stride = (ptrdiff_t (*) (abase_vbase_ptr, int)) abase_p_8_wrapper_vec_elt_stride;
    vbase->poly_init = (void (*) (abase_vbase_ptr, void *, unsigned int)) abase_p_8_wrapper_poly_init;
    vbase->poly_clear = (void (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_poly_clear;
    vbase->poly_set = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_poly_set;
    vbase->poly_setmonic = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_poly_setmonic;
    vbase->poly_setcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_poly_setcoef;
    vbase->poly_setcoef_ui = (void (*) (abase_vbase_ptr, void *, unsigned long, unsigned int)) abase_p_8_wrapper_poly_setcoef_ui;
    vbase->poly_getcoef = (void (*) (abase_vbase_ptr, void *, const void *, unsigned int)) abase_p_8_wrapper_poly_getcoef;
    vbase->poly_deg = (int (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_poly_deg;
    vbase->poly_add = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_poly_add;
    vbase->poly_sub = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_poly_sub;
    vbase->poly_add_ui = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long)) abase_p_8_wrapper_poly_add_ui;
    vbase->poly_sub_ui = (void (*) (abase_vbase_ptr, void *, const void *, unsigned long)) abase_p_8_wrapper_poly_sub_ui;
    vbase->poly_neg = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_poly_neg;
    vbase->poly_scal_mul = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_poly_scal_mul;
    vbase->poly_mul = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_poly_mul;
    vbase->poly_divmod = (void (*) (abase_vbase_ptr, void *, void *, const void *, const void *)) abase_p_8_wrapper_poly_divmod;
    vbase->poly_precomp_mod = (void (*) (abase_vbase_ptr, void *, const void *)) abase_p_8_wrapper_poly_precomp_mod;
    vbase->poly_mod_pre = (void (*) (abase_vbase_ptr, void *, const void *, const void *, const void *)) abase_p_8_wrapper_poly_mod_pre;
    vbase->poly_gcd = (void (*) (abase_vbase_ptr, void *, const void *, const void *)) abase_p_8_wrapper_poly_gcd;
    vbase->poly_xgcd = (void (*) (abase_vbase_ptr, void *, void *, void *, const void *, const void *)) abase_p_8_wrapper_poly_xgcd;
    vbase->poly_random = (void (*) (abase_vbase_ptr, void *, unsigned int, gmp_randstate_t)) abase_p_8_wrapper_poly_random;
    vbase->poly_random2 = (void (*) (abase_vbase_ptr, void *, unsigned int, gmp_randstate_t)) abase_p_8_wrapper_poly_random2;
    vbase->poly_cmp = (int (*) (abase_vbase_ptr, const void *, const void *)) abase_p_8_wrapper_poly_cmp;
    vbase->poly_asprint = (void (*) (abase_vbase_ptr, char * *, const void *)) abase_p_8_wrapper_poly_asprint;
    vbase->poly_fprint = (void (*) (abase_vbase_ptr, FILE *, const void *)) abase_p_8_wrapper_poly_fprint;
    vbase->poly_print = (void (*) (abase_vbase_ptr, const void *)) abase_p_8_wrapper_poly_print;
    vbase->poly_sscan = (int (*) (abase_vbase_ptr, void *, const char *)) abase_p_8_wrapper_poly_sscan;
    vbase->poly_fscan = (int (*) (abase_vbase_ptr, FILE *, void *)) abase_p_8_wrapper_poly_fscan;
    vbase->poly_scan = (int (*) (abase_vbase_ptr, void *)) abase_p_8_wrapper_poly_scan;
    vbase->groupsize = (int (*) (abase_vbase_ptr)) abase_p_8_wrapper_groupsize;
    vbase->offset = (int (*) (abase_vbase_ptr, int)) abase_p_8_wrapper_offset;
    vbase->stride = (int (*) (abase_vbase_ptr)) abase_p_8_wrapper_stride;
    vbase->set_ui_at = (void (*) (abase_vbase_ptr, void *, int, unsigned long)) abase_p_8_wrapper_set_ui_at;
    vbase->set_ui_all = (void (*) (abase_vbase_ptr, void *, unsigned long)) abase_p_8_wrapper_set_ui_all;
    vbase->elt_ur_set_ui_at = (void (*) (abase_vbase_ptr, void *, int, unsigned long)) abase_p_8_wrapper_elt_ur_set_ui_at;
    vbase->elt_ur_set_ui_all = (void (*) (abase_vbase_ptr, void *, unsigned long)) abase_p_8_wrapper_elt_ur_set_ui_all;
    vbase->dotprod = (void (*) (abase_vbase_ptr, void *, const void *, const void *, unsigned int)) abase_p_8_wrapper_dotprod;
    vbase->mpi_ops_init = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_ops_init;
    vbase->mpi_datatype = (MPI_Datatype (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_datatype;
    vbase->mpi_datatype_ur = (MPI_Datatype (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_datatype_ur;
    vbase->mpi_addition_op = (MPI_Op (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_addition_op;
    vbase->mpi_addition_op_ur = (MPI_Op (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_addition_op_ur;
    vbase->mpi_ops_clear = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_mpi_ops_clear;
    vbase->oo_impl_name = (const char * (*) (abase_vbase_ptr)) abase_p_8_wrapper_oo_impl_name;
    vbase->oo_field_init = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_oo_field_init;
    vbase->oo_field_clear = (void (*) (abase_vbase_ptr)) abase_p_8_wrapper_oo_field_clear;
}


/* vim:set ft=cpp: */
