//------------------------------------------------------------------------------
// GB_is_diagonal: check if A is a diagonal matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// Returns true if A is a square diagonal matrix, with all diagonal entries
// present.  All pending tuples are ignored.  Zombies are treated as entries.

#include "GB_mxm.h"
#include "GB_atomics.h"

bool GB_is_diagonal             // true if A is diagonal
(
    const GrB_Matrix A,         // input matrix to examine
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (A != NULL) ;
    ASSERT_MATRIX_OK (A, "A check diag", GB0) ;
    ASSERT (!GB_ZOMBIES (A)) ;
    ASSERT (GB_JUMBLED_OK (A)) ;
    ASSERT (!GB_PENDING (A)) ;

    //--------------------------------------------------------------------------
    // trivial cases
    //--------------------------------------------------------------------------

    int64_t n     = GB_NROWS (A) ;
    int64_t ncols = GB_NCOLS (A) ;

    if (n != ncols)
    { 
        // A is rectangular
        return (false) ;
    }

    if (GB_IS_BITMAP (A))
    { 
        // never treat bitmaps as diagonal
        return (false) ;
    }

    if (GB_IS_FULL (A))
    { 
        // A is full, and is diagonal only if 1-by-1
        return (n == 1) ;
    }

    int64_t anz  = GB_NNZ (A) ;
    int64_t nvec = A->nvec ;

    if (n != anz || n != nvec)
    { 
        // A must have exactly n entries in n vectors.  A can be sparse or
        // hypersparse.  If hypersparse, all vectors must be present, so
        // Ap has size n+1 whether sparse or hypersparse.
        return (false) ;
    }

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    // Break the work into lots of tasks so the early-exit can be exploited.

    GB_GET_NTHREADS_MAX (nthreads_max, chunk, Context) ;
    int nthreads = GB_nthreads (n, chunk, nthreads_max) ;
    int ntasks = (nthreads == 1) ? 1 : (256 * nthreads) ;
    ntasks = GB_IMIN (ntasks, n) ;
    ntasks = GB_IMAX (ntasks, 1) ;

    //--------------------------------------------------------------------------
    // examine each vector of A
    //--------------------------------------------------------------------------

    const int64_t *GB_RESTRICT Ap = A->p ;
    const int64_t *GB_RESTRICT Ai = A->i ;

    int diagonal = true ;

    int tid ;
    #pragma omp parallel for num_threads(nthreads) schedule(dynamic,1)
    for (tid = 0 ; tid < ntasks ; tid++)
    {

        //----------------------------------------------------------------------
        // check for early exit
        //----------------------------------------------------------------------

        int diag = true ;
        { 
            GB_ATOMIC_READ
            diag = diagonal ;
        }
        if (!diag) continue ;

        //----------------------------------------------------------------------
        // check if vectors jstart:jend-1 are diagonal
        //----------------------------------------------------------------------

        int64_t jstart, jend ;
        GB_PARTITION (jstart, jend, n, tid, ntasks) ;
        for (int64_t j = jstart ; diag && j < jend ; j++)
        {
            int64_t p = Ap [j] ;                // ok: A is sparse
            int64_t ajnz = Ap [j+1] - p ;       // ok: A is sparse
            if (ajnz != 1)
            { 
                // A(:,j) must have exactly one entry
                diag = false ;
            }
            int64_t i = Ai [p] ;        // ok: A is sparse
            if (i != j)
            { 
                // the single entry must be A(i,i)
                diag = false ;
            }
        }

        //----------------------------------------------------------------------
        // early exit: tell all other tasks to halt
        //----------------------------------------------------------------------

        if (!diag)
        { 
            GB_ATOMIC_WRITE
            diagonal = false ;
        }
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    if (diagonal)
    { 
        A->nvec_nonempty = n ;
        A->jumbled = false ;        // a diagonal matrix is never jumbled
    }
    return ((bool) diagonal) ;
}

