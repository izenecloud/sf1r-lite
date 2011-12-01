#ifndef __SVD_H_2009__
#define __SVD_H_2009__

/// @ Singular Value Decomposition (SVD)

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Singular Value Decomposition: X = U*S*V',
// U can be NULL, and V can be NULL
// NOTE: given X(m*n) and m >= n, then
// the results have dim as: U(m*n), S(n) and V(n*n)
int mat_svd(double ** X, int m, int n, double **U, double *S, double **V);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__SVD_H_2009__
