//------------------------------------------------------------------------------
// gb_mxstring_to_type: return the GraphBLAS type from a MATLAB string
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

#include "gbmex.h"

GrB_Type gb_mxstring_to_type    // return the GrB_Type from a MATLAB string
(
    const mxArray *s
)
{

    #define LEN 256
    char classname [LEN+2] ;
    gb_mxstring_to_string (classname, LEN, s) ;
    return (gb_string_to_type (classname)) ;
}

