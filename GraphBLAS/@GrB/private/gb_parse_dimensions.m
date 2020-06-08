function [m, n] = gb_parse_dimensions (arg1, arg2)
%GB_GET_DIMENSIONS parse arguments for dimensions

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

switch (nargin)

    case { 0 }

        % C = GrB.eye
        m = 1 ;
        n = 1 ;

    case { 1 }

        if (length (arg1) == 1)
            % C = ones (n)
            m = gb_get_scalar (arg1) ;
            n = m ;
        elseif (length (arg1) == 2)
            % C = ones ([m n])
            [m, n] = gb_get_pair (arg1) ;
        else
            gb_error ('invalid dimensions') ;
        end

    case { 2 }

        % C = ones (m, n)
        m = gb_get_scalar (arg1) ;
        n = gb_get_scalar (arg2) ;

end

