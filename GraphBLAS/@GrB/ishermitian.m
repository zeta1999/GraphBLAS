function s = ishermitian (G, option)
%ISHERMITIAN Determine if a GraphBLAS matrix is Hermitian or real symmetric
% ishermitian (G) is true if G equals G' and false otherwise.
% ishermitian (G, 'skew') is true if G equals -G' and false otherwise.
% ishermitian (G, 'nonskew') is the same as ishermitian (G).
%
% See also GrB/issymmetric.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

if (nargin < 2)
    option = 'nonskew' ;
end

s = gb_issymmetric (G.opaque, option, true) ;

