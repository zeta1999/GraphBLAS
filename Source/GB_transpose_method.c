//------------------------------------------------------------------------------
// GB_transpose_method: select method for GB_transpose
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_transpose.h"

// GB_transpose can use choose between a merge-sort-based method that takes
// O(anz*log(anz)) time, or a bucket-sort method that takes O(anz+m+n) time.
// The bucket sort has 3 methods: sequential, atomic, and non-atomic.

bool GB_transpose_method        // if true: use GB_builder, false: use bucket
(
    const GrB_Matrix A,         // matrix to transpose
    int *nworkspaces_bucket,    // # of slices of A for the bucket method
    int *nthreads_bucket,       // # of threads to use for the bucket method
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    int64_t anvec = A->nvec ;
    int64_t anz = GB_NNZ (A) ;
    int64_t avlen = A->vlen ;
    int64_t avdim = A->vdim ;
    int anzlog = (int) ceil (log2 ((double) anz + 1)) ;
    int mlog   = (int) ceil (log2 ((double) avlen + 1)) ;
    double alpha ;

    // determine # of threads for bucket method
    GB_GET_NTHREADS_MAX (nthreads_max, chunk, Context) ;
    int nthreads = GB_nthreads (anz + avlen, chunk, nthreads_max) ;

    //--------------------------------------------------------------------------
    // TODO::HACK
    //--------------------------------------------------------------------------

    int64_t hack = GB_Global_hack_get ( ) ;
    if (hack < 0) 
    {
        // bucket method
        if (nthreads == 1)
        {
            // sequential method
            (*nworkspaces_bucket) = 1 ;
        }
        else if (hack == -2)
        {
            // non-atomic method
            (*nworkspaces_bucket) = nthreads ;
        }
        else
        {
            // atomic method
            (*nworkspaces_bucket) = 1 ;
        }
        (*nthreads_bucket) = nthreads ;
        return (false) ;      // use bucket
    }
    if (hack > 0)
    {
        (*nworkspaces_bucket) = 0 ;
        (*nthreads_bucket) = 0 ;
        return (true) ;       // use builder
    }

    //--------------------------------------------------------------------------
    // select between the atomic and non-atomic bucket method
    //--------------------------------------------------------------------------

    bool atomics ;
    if (nthreads == 1)
    { 
        // sequential bucket method, no atomics needed
        atomics = false ;
    }
    else if ((double) nthreads * (double) avlen > (double) anz)
    { 
        // non-atomic workspace is too high; use atomic method
        atomics = true ;
    }
    else
    {
        // select between atomic and non-atomic methods.  This rule is based on
        // performance on a 4-core system with 4 threads with gcc 7.5.  The icc
        // compiler has much slower atomics than gcc and so beta should likely
        // be smaller when using icc.
        int beta ;
        if (anzlog < 14)
        { 
            beta = -5 ;     // fewer than 16K entries in A
        }
        else
        { 
            switch (anzlog)
            {
                case 14: beta = -4 ; break ;        // 16K entried in A
                case 15: beta = -3 ; break ;        // 32K
                case 16: beta = -2 ; break ;        // 64K
                case 17: beta = -1 ; break ;        // 128K
                case 18: beta =  0 ; break ;        // 256K
                case 19: beta =  1 ; break ;        // 512K
                case 20: beta =  2 ; break ;        // 1M
                case 21: beta =  3 ; break ;        // 2M
                case 22: beta =  4 ; break ;        // 4M
                case 23: beta =  5 ; break ;        // 8M
                case 24: beta =  6 ; break ;        // 16M
                case 25: beta =  8 ; break ;        // 32M
                case 26: beta =  9 ; break ;        // 64M
                case 27: beta =  9 ; break ;        // 128M
                case 28: beta =  9 ; break ;        // 256M TODO::
                default: beta =  9 ; break ;        // > 256M TODO::
            }
        }
        if (anzlog - mlog <= beta)
        { 
            // use atomic method
            // anzlog - mlog is the log2 of the average row degree, rounded.
            // If the average row degree is <= 2^beta, use the atomic method.
            // That is, the atomic method works better for sparser matrices,
            // and the non-atomic works better or denser matrices.  However,
            // the threshold changes as the problem gets larger, in terms of #
            // of entries in A, when the atomic method becomes more attractive
            // relative to the non-atomic method.
            atomics = true ;
        }
        else
        { 
            // use non-atomic method
            atomics = false ;
        }
    }

    (*nworkspaces_bucket) = (atomics) ? 1 : nthreads ;
    (*nthreads_bucket) = nthreads ;

    //--------------------------------------------------------------------------
    // select between GB_builder method and bucket method
    //--------------------------------------------------------------------------

    // As the problem gets larger, the GB_builder method gets faster relative
    // to the bucket method, in terms of the "constants" in the O(a log a) work
    // for GB_builder, or O (a+m+n) for the bucket method.  Clearly, O (a log
    // a) and O (a+m+n) do not fully model the performance of these two
    // methods.  Perhaps this is because of cache effects.  The bucket method
    // has more irregular memory accesses.  The GB_builder method uses
    // mergesort, which has good memory locality.

    if (anzlog < 14)
    { 
        alpha = 0.5 ;       // fewer than 2^14 = 16K entries
    }
    else
    { 
        switch (anzlog)
        {
            case 14: alpha = 0.6 ; break ;      // 16K entries in A
            case 15: alpha = 0.7 ; break ;      // 32K
            case 16: alpha = 1.0 ; break ;      // 64K
            case 17: alpha = 1.7 ; break ;      // 128K
            case 18: alpha = 3.0 ; break ;      // 256K
            case 19: alpha = 4.0 ; break ;      // 512K
            case 20: alpha = 6.0 ; break ;      // 1M
            case 21: alpha = 7.0 ; break ;      // 2M
            case 22: alpha = 8.0 ; break ;      // 4M
            case 23: alpha = 5.0 ; break ;      // 8M
            case 24: alpha = 5.0 ; break ;      // 16M
            case 25: alpha = 5.0 ; break ;      // 32M
            case 26: alpha = 5.0 ; break ;      // 64M
            case 27: alpha = 5.0 ; break ;      // 128M     TODO::
            case 28: alpha = 5.0 ; break ;      // 256M     TODO::
            default: alpha = 5.0 ; break ;      // > 256M       TODO::
        }
    }

    double bucket_work  = (double) (anz + avlen + anvec) * alpha ;
    double builder_work = (log2 ((double) anz + 1) * (anz)) ;

#if 0

    // workspace required for builder method:
    //      asize = A->type->size
    //      csize = C->type->size

    //--------------------------------------------------------------------------
    // memory space for GB_builder method
    //--------------------------------------------------------------------------

    //      iwork       anz * sizeof (int64_t), will become T->i
    //      jwork       anz * sizeof (int64_t), or zero if Ai recycled
    //      Swork       anz * asize, if op1 or op2 are present
    //      K_work      anz * sizeof (int64_t)
    //      W0          anz * sizeof (int64_t)
    //      W1          anz * sizeof (int64_t)
    //      W2          anz * sizeof (int64_t)

    //      Then W0, W1, and W2 are freed.
    //      Then T is allocated:
    //      T->h        nvec * sizeof (int64_t)
    //      T->p        (nvec+1) * sizeof (int64_t)
    //      then jwork is freed
    //      iwork is reallocated (no change) and becomes T->i
    //      T->x is allocated
    //      T->x        anz * csize
    //      Swork, K_work are freed

    int64_t mem = anz * (5 * sizeof (int64_t)) // iwork, K_work, W0, W1, W2
        + (recycle_Ai) ? 0 : (anz * sizeof (int64_t))   // jwork
        + (op_is_present) ? (anz * asize) : 0 ;         // Swork
    int64_t builder_mem = mem ;
    mem -= anz * (3 * sizeof (int64_t)) ;       // free W0, W1, W2
    // upper bound for nvec:
    int64_t nvec = GB_IMIN (avlen, anz) ;
    mem += nvec * (2 * sizeof (int64_t)) ;      // allocate T->h and T->p
    builder_mem = GB_IMAX (builder_mem, mem) ;
    mem -= anz * sizeof (int64_t) ;             // free jwork
    mem += anz * csize ;                        // allocate T->x
    builder_mem = GB_IMAX (builder_mem, mem) ;

    //--------------------------------------------------------------------------
    // memory space for GB_transpose_bucket method
    //--------------------------------------------------------------------------

    //      rowcounts       anz * sizeof (int64_t), single-threaded and atomic,
    //                      multiply by nthreads for non-atomic
    //      T->p            always size avlen * sizeof (int64_t)
    //      T->i            anz * sizeof (int64_t)
    //      T->x            anz * csize

    // This can be larger than GB_builder if avlen >> anz, but most of the
    // time the bucket method uses less workspace.  This doesn't seem to be
    // a useful metric for comparing the two methods.

#endif

    //--------------------------------------------------------------------------
    // select the method with the least amount of work
    //--------------------------------------------------------------------------

    return (builder_work < bucket_work) ;
}

