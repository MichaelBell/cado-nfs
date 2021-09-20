#ifndef LAS_REDUCE_PLATTICE_SIMPLISTIC_HPP_
#define LAS_REDUCE_PLATTICE_SIMPLISTIC_HPP_

/* This is example code that is meant to go __into__ a class which has
 * several data members and function members defined. For maintenance,
 * all these pieces of code are put in separate files, but these must be
 * #included from within the class itself.
 */
#ifndef LAS_PLATTICE_H
#error "This prologue is only here to please clangd !!!"
#include <cstdint>
#include "macros.h"
struct mock_plattice_info {
        uint32_t mi0;  /* This encodes i0 = -mi0 */
        uint32_t j0;
        uint32_t i1;
        uint32_t j1;
        void lattice_with_vertical_vector(uint32_t I);
#endif

void reduce_plattice_simplistic(uint32_t I) {
    /* This is the main reduce_plattice loop */
    for( ;; ) {
        ASSERT(j0 <= j1);
        if (i1 < I) {
            if (i1 == 0) {
                // Lo=matrix([ (mi0, j1-j0), (i1, j1)])
                j0 = j1 - j0;
                lattice_with_vertical_vector(I);
                return;
            }
            ASSERT(mi0 + i1 >= I);
            int a = (mi0 + i1 - I) / i1;
            mi0 -= a * i1;
            j0  += a * j1;
            return;
        }
        {
            int k = mi0 / i1; mi0 -= k * i1; j0 += k * j1;
            ASSERT(k);
        }

        ASSERT(j1 <= j0);
        if (mi0 < I) {
            if (mi0 == 0) {
                mi0 = i1;
                i1 = j0 ; j0 = j1 ; j1 = i1;
                i1 = 0;
                lattice_with_vertical_vector(I);
                return;
            }
            ASSERT(mi0 + i1 >= I);
            int a = (mi0 + i1 - I) / mi0;
            i1 -= a * mi0;
            j1 += a * j0;
            return;
        }
        {
            int k = i1 / mi0; i1 -= k * mi0; j1 += k * j0;
            ASSERT(k);
        }
    }
}

#ifndef LAS_PLATTICE_H
#error "This epilogue is only here to please clangd !!!"
};
#endif


#endif	/* LAS_REDUCE_PLATTICE_SIMPLISTIC_HPP_ */
