#ifndef __DMAT_2D_H_2010__
#define __DMAT_2D_H_2010__

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#if (!defined(_DEBUG) && !defined(NDEBUG))
#define NDEBUG
#endif // CB doesn't define NDEBUG automatically when release!
#include <assert.h> // assert is ignored by 'NDEBUG'!

#ifndef __INLINE__
#if defined(__GNUC__) || defined(__cplusplus) || defined(c_plusplus)
#define __INLINE__   inline
#else
#define __INLINE__ __inline
#endif
#endif//__INLINE__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static __INLINE__ double **mat_alloc(int row, int col)
{
    double **mat, *p;
    assert(row>0 && col>0);

    p = (double *) malloc(
            row*col*sizeof(double) +
            row*sizeof(double *));

    if (!p) return NULL;
    mat = (double **) (p+row*col);

    while(--row>=0) mat[row] = (p+row*col);

    return mat;
}

static __INLINE__ double **mat_alloc_init(int row, int col, double val)
{
    double **mat = mat_alloc(row, col);
    int i=0;
    for (; i<row; i++)
    {
        int j=0;
        for (; j<col; j++)
        {
            mat[i][j] = val;
        }
    }

    return mat;
}

static __INLINE__ void mat_free(double **mat)
{
    assert(mat);
    free(mat[0]);
}

static __INLINE__ void mat_copy(double **A, double **B, int m, int n)
{
    assert(m>0 && n>0);
    assert(A && B);
    while(--m>=0) memcpy(B[m], A[m], n*sizeof(double));
}

static __INLINE__ void mat_zero(double **A, int m, int n)
{
    assert(m>0 && n>0 && A);
    while(--m>=0) memset(A[m], 0, n*sizeof(double));
}

static __INLINE__ double **mat_clone(double **A, int m, int n)
{
    double **B;
    assert(m>0 && n>0 && A);
    B = mat_alloc(m, n);
    mat_copy(A, B, m, n);
    return B;
}

static __INLINE__ double **mat_clone_vec(double *A, int m, int n)
{
    double **B;
    int r;
    assert(m>0 && n>0 && A);
    B = mat_alloc(m, n);

    for (r=0; r<m; r++) {
        memcpy(B[r], A+r*n, n*sizeof(double));
    }
    return B;
}

static __INLINE__ double **mat_clone_diag(double *A, int m)
{
    double **B;
    int r;
    assert(m>0 && A);

    B = mat_alloc(m,m);
    mat_zero(B,m,m);
    for (r=0; r<m; r++) B[r][r] = A[r];

    return B;
}

static __INLINE__ double **mat_transpose(double **A, int m, int n)
{
    double **B;
    assert(m>0 && n>0 && A);
    B = mat_alloc(n, m);
    while(--m>=0) {
        int k = n;
        while(--k>=0) B[k][m] = A[m][k];
    }
    return B;
}


// return dot product of i and j column vector of matrix A[m*n]
                                                           // 0 <= i and j < n
static __INLINE__ double mat_dot_product(double **A, int m, int n, int i, int j)
{
    assert(A);
    assert(m > 0 && n > 0 && i > 0 && j > 0);
    assert(n > i && n > j);

    double v = 0;
    int t;
    for (t = 0; t < m; t++)
        v += A[t][i] * A[t][j];

    return v;
}

// A[m*n], B[n*p]: C=A*B
static __INLINE__ void mat_mul(double **A, int m, int n,
        double **B, int _n, int p, double **C)
{
    int i, j, k;
    assert(A && B && C);
    assert(_n==n);
    assert(m>0 && n>0 && p>0);

    if (C==A) {
        double *vec = (double *) malloc(n*sizeof(double));
        assert(C!=B);
        assert(vec && p<=n);
        for (i=0; i<m; i++)
        {
            memcpy(vec, A[i], n*sizeof(double));
            for (j=0; j<p; j++)
            {
                double s = 0;
                for (k=0; k<n; k++)
                    s += vec[k]*B[k][j];
                A[i][j] = s;
            }
        }
        free(vec);
    }
    else if (C==B) {
        double *vec = (double *) malloc(n*sizeof(double));
        assert(C!=A);
        assert(vec && m<=n);
        for (j=0; j<p; j++)
        {
            for (k=0; k<n; k++) vec[k] = B[k][j];
            for (i=0; i<m; i++) {
                double s = 0;
                for (k=0; k<n; k++)
                    s += A[i][k]*vec[k];
                B[i][j] = s;
            }
        }
        free(vec);
    }
    else {
        for (i=0; i<m; i++)
            for (j=0; j<p; j++)
            {
                double s = 0;
                for (k=0; k<n; k++)
                    s += A[i][k]*B[k][j];
                C[i][j] = s;
            }
    }
}


// A[m*n], B[m*p]: C=A'*B
static __INLINE__ void mat_mul_a(double **A, int m, int n,
        double **B, int _m, int p, double **C)
{
    int i, j, k;
    assert(C!=A && C!=B);
    assert(_m==m);
    for (i=0; i<n; i++)
        for (j=0; j<p; j++)
        {
            double s = 0;
            for (k=0; k<m; k++)
                s += A[k][i]*B[k][j];
            C[i][j] = s;
        }
}

// A[m*n], B[p*n]: C=A*B'
static __INLINE__ void mat_mul_b(double **A, int m, int n,
        double **B, int p, int _n, double **C)
{
    int i, j, k;
    assert(C!=A && C!=B);
    assert(_n==n);
    for (i=0; i<m; i++)
        for (j=0; j<p; j++)
        {
            double s = 0;
            for (k=0; k<n; k++)
                s += A[i][k]*B[j][k];
            C[i][j] = s;
        }
}

void mat_print(double **mat, int m, int n, const char *fmt);
void mat_file_print(double **mat, int m, int n, const char *fmt, FILE* f);

int mat_save(double **mat, int m, int n, const char *fn);

double **mat_load(int *m, int *n, const char *fn);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__DMAT_2D_H_2010__
