function gbtest35
%GBTEST35 test reshape

rng ('default')

for m = 1:6
    for n = 1:10
        A = rand (m, n) ;
        G = gb (A) ;
        mn = m*n ;
        f = factor (mn) ;
        for k = 1:length (f)
            S = nchoosek (f, k) ;
            for i = 1:size(S,1)
                m2 = prod (S (i,:)) ;
                n2 = mn / m2 ;
                C1 = reshape (A, m2, n2) ;
                C2 = reshape (G, m2, n2) ;
                assert (isequal (C1, double (C2))) ;
            end
        end
    end
end

fprintf ('gbtest35: all tests passed\n') ;
