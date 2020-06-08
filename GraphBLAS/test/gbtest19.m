function gbtest19
%GBTEST19 test mpower

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

rng ('default') ;
A = rand (4) ;
G = GrB (A) ;
maxerr = 0 ;

for k = 0:10
    C1 = A^k ;
    C2 = G^k ;
    err = norm (C1 - C2, 1) ;
    assert (norm (err,1) < 1e-10) ;
    maxerr = max (maxerr, err) ;
end

fprintf ('max error: %g\n', maxerr) ;
fprintf ('gbtest19: all tests passed\n') ;

