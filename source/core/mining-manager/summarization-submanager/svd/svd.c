#include <memory.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

#if (!defined(_DEBUG) && !defined(NDEBUG))
#define NDEBUG
#endif // CB doesn't define NDEBUG automatically when release!
#include <assert.h> // assert is ignored by 'NDEBUG'!

#include "svd.h"

// Effective precision is that of sqrt(REAL)
static double hypot_d(double a, double b)  // sqrt(a*a + b*b)
{
    double aa = fabs(a);    // |a|
    double bb = fabs(b);    // |b|
    if (aa > bb)
    {
        double r = bb / aa;
        return aa * sqrt(1. + r * r);
    }
    else if (bb != 0.)
    {
        double r = aa / bb;
        return bb * sqrt(1. + r * r);
    }
    return 0.; // a = b = 0
}

/// @remark Singular Value Decomposition.
/// For an m-by-n matrix A with m >= n, the singular value decomposition
/// is an m-by-n orthogonal matrix U, an n-by-n diagonal matrix S, and
/// an n-by-n orthogonal matrix V so that A = U*S*V'.
///
/// The singular values, sigma[k] = S[k][k], are ordered so that
/// sigma[0] >= sigma[1] >= ... >= sigma[n - 1].
///
/// The singular value decompostion always exists.
/// The matrix condition number and the effective numerical
/// rank can be computed from this decomposition.
///
/// (Adapted from JAMA, a Java Matrix Library, developed by jointly by the
/// Mathworks and NIST; see http://math.nist.gov/javanumerics/jama).

/// @param m,n: number of rows and cols of X(A):@n
///      A(m*n): U(m*m), S(n), V(n*n)
/// @param X,U,S,V: matrice, where X = U*S*V',
///      and U, V also work as booleans for wantu, wantv
int mat_svd(double **X, int m, int n, double **U, double *S, double **V)
{
    const double eps = DBL_EPSILON;
    double **A, *A_, *s , *e , *work;
    int nu = m < n ? m : n;
    int p = n < m + 1 ? n : m + 1;
    int pp = p - 1;
    int iter = 0;
    int i, j, k;
    int wantu = U ? 1 : 0;
    int wantv = V ? 1 : 0;
    int nct = m - 1 < n ? m - 1 : n;
    int nrt = n - 2 < m ? n - 2 : m;
    if (nrt < 0) nrt = 0;

    // arguments must be distinct.
    if (X == NULL || S == NULL) return 0;
    if (U == V && U != NULL) return 0;
    if (U == X || V == X) return 0;

    // we can also use 1D array to store S[][] by s[p]
    // the effective range is U[m][m] by U[m][nu]
    s = (double *) malloc(p * sizeof(double));
    e = (double *) malloc(n * sizeof(double));
    work = (double *) malloc(m * sizeof(double));
    A_ = (double *) malloc(m * n * sizeof(double));
    A  = (double**) malloc(m * sizeof(double *));
    for (j = 0; j < m; j++)
    {
        A[j] = A_ + j * n;
        memcpy(A[j], X[j], n * sizeof(double));
        if (wantu) memset(U[j], 0, n * sizeof(double));
    }

    // Reduce A to bidiagonal form, storing the diagonal elements
    // in s and the super-diagonal elements in e.
    for (k = 0; k < (nct > nrt ? nct : nrt); k++)
    {
        if (k < nct)
        {
            // Compute the transformation for the k-th column and
            // place the k-th diagonal in s[k].
            // Compute 2-norm of k-th column without under/overflow.
            s[k] = 0.;
            for (i = k; i < m; i++)
            {
                s[k] = hypot_d(s[k], A[i][k]);
            }
            if (s[k] != 0.)
            {
                if (A[k][k] < 0.)
                {
                    s[k] = -s[k];
                }
                for (i = k; i < m; i++)
                {
                    A[i][k] /= s[k];
                }
                A[k][k] += 1.;
            }
            s[k] = -s[k];
        }
        for (j = k + 1; j < n; j++)
        {
            if (k < nct && s[k] != 0.)
            {
                // Apply the transformation.
                long double t = 0.;
                for (i = k; i < m; i++)
                {
                    t += A[i][k] * A[i][j];
                }
                t = -t / A[k][k];
                for (i = k; i < m; i++)
                {
                    A[i][j] += t * A[i][k];
                }
            }

            // Place the k-th row of A into e for the
            // subsequent calculation of the row transformation.

            e[j] = A[k][j];
        }
        if (wantu && (k < nct))
        {
            // Place the transformation in U for subsequent back
            // multiplication.

            for (i = k; i < m; i++)
            {
                U[i][k] = A[i][k];
            }
        }
        if (k < nrt)
        {
            // Compute the k-th row transformation and place the
            // k-th super-diagonal in e[k].
            // Compute 2-norm without under/overflow.
            e[k] = 0.;
            for (i = k + 1; i < n; i++)
            {
                e[k] = hypot_d(e[k], e[i]);
            }
            if (e[k] != 0.)
            {
                if (e[k + 1] < 0.)
                {
                    e[k] = -e[k];
                }
                for (i = k + 1; i < n; i++)
                {
                    e[i] /= e[k];
                }
                e[k + 1] += 1.;
            }
            e[k] = -e[k];
            if (k + 1 < m && e[k] != 0.)
            {
                // Apply the transformation.

                for (i = k + 1; i < m; i++)
                {
                    work[i] = 0.;
                }
                for (j = k + 1; j < n; j++)
                {
                    for (i = k + 1; i < m; i++)
                    {
                        work[i] += e[j] * A[i][j];
                    }
                }
                for (j = k + 1; j < n; j++)
                {
                    double t = -e[j] / e[k + 1];
                    for (i = k + 1; i < m; i++)
                    {
                        A[i][j] += t * work[i];
                    }
                }
            }
            if (wantv)
            {
                // Place the transformation in V for subsequent
                // back multiplication.

                for (i = k + 1; i < n; i++)
                {
                    V[i][k] = e[i];
                }
            }
        }
    }

    // Set up the final bidiagonal matrix or order p.
    if (nct < n)
    {
        s[nct] = A[nct][nct];
    }
    if (m < p)
    {
        s[p - 1] = 0.;
    }
    if (nrt + 1 < p)
    {
        e[nrt] = A[nrt][p - 1];
    }
    e[p - 1] = 0.;

    // If required, generate U.
    if (wantu)
    {
        for (j = nct; j < nu; j++)
        {
            for (i = 0; i < m; i++)
            {
                U[i][j] = 0.;
            }
            U[j][j] = 1.;
        }
        for (k = nct - 1; k >= 0; k--)
        {
            if (s[k] != 0.)
            {
                for (j = k + 1; j < nu; j++)
                {
                    long double t = 0.;
                    for (i = k; i < m; i++)
                    {
                        t += U[i][k] * U[i][j];
                    }
                    t = -t / U[k][k];
                    for (i = k; i < m; i++)
                    {
                        U[i][j] += t * U[i][k];
                    }
                }
                for (i = k; i < m; i++ )
                {
                    U[i][k] = -U[i][k];
                }
                U[k][k] += 1.;
                for (i = 0; i < k - 1; i++)
                {
                    U[i][k] = 0.;
                }
            }
            else
            {
                for (i = 0; i < m; i++)
                {
                    U[i][k] = 0.;
                }
                U[k][k] = 1.;
            }
        }
    }

    // If required, generate V.
    if (wantv)
    {
        for (k = n - 1; k >= 0; k--)
        {
            if ((k < nrt) && (e[k] != 0.))
            {
                for (j = k + 1; j < nu; j++)
                {
                    long double t = 0.;
                    for (i = k + 1; i < n; i++)
                    {
                        t += V[i][k] * V[i][j];
                    }
                    t = -t / V[k + 1][k];
                    for (i = k + 1; i < n; i++)
                    {
                        V[i][j] += t * V[i][k];
                    }
                }
            }
            for (i = 0; i < n; i++)
            {
                V[i][k] = 0.;
            }
            V[k][k] = 1.;
        }
    }

    // Main iteration loop for the singular values.
    while (p > 0)
    {
        int k;
        int kase=0;

        // Here is where a test for too many iterations would go.

        // This section of the program inspects for
        // negligible elements in the s and e arrays.  On
        // completion the variables kase and k are set as follows.

        // kase = 1     if s(p) and e[k - 1] are negligible and k<p
        // kase = 2     if s(k) is negligible and k<p
        // kase = 3     if e[k - 1] is negligible, k<p, and
        //              s(k), ..., s(p) are not negligible (qr step).
        // kase = 4     if e(p - 1) is negligible (convergence).

        for (k = p - 2; k >= -1; k--)
        {
            if (k == -1) break;
            if (fabs(e[k]) <= eps * (fabs(s[k]) + fabs(s[k + 1])))
            {
                e[k] = 0.;
                break;
            }
        }
        if (k == p - 2)
        {
            kase = 4;
        }
        else
        {
            double t;
            int ks;
            for (ks = p - 1; ks >= k; ks--)
            {
                if (ks == k) break;
                t = (ks != p ? fabs(e[ks]) : 0.) +
                    (ks != k + 1 ? fabs(e[ks - 1]) : 0.);
                if (fabs(s[ks]) <= eps * t)
                {
                    s[ks] = 0.;
                    break;
                }
            }
            if (ks == k)
            {
                kase = 3;
            }
            else if (ks == p - 1)
            {
                kase = 1;
            }
            else
            {
                kase = 2;
                k = ks;
            }
        }
        k++;

        // Perform the task indicated by kase.
        switch (kase)
        {
        // Deflate negligible s(p).
        case 1:
            {
                double f = e[p - 2];
                e[p - 2] = 0.;
                for (j = p - 2; j >= k; j--)
                {
                    double t = hypot_d(s[j], f);
                    double cs = s[j] / t;
                    double sn = f / t;
                    s[j] = t;
                    if (j != k)
                    {
                        f = -sn * e[j - 1];
                        e[j - 1] = cs * e[j - 1];
                    }
                    if (wantv)
                    {
                        for (i = 0; i < n; i++)
                        {
                            t = cs * V[i][j] + sn * V[i][p - 1];
                            V[i][p - 1] = -sn * V[i][j] + cs * V[i][p - 1];
                            V[i][j] = t;
                        }
                    }
                }
            }
            break;

        // Split at negligible s(k).
        case 2:
            {
                double f = e[k - 1];
                e[k - 1] = 0.;
                for (j = k; j < p; j++)
                {
                    double t = hypot_d(s[j], f);
                    double cs = s[j] / t;
                    double sn = f / t;
                    s[j] = t;
                    f = -sn * e[j];
                    e[j] = cs * e[j];
                    if (wantu)
                    {
                        for (i = 0; i < m; i++)
                        {
                            t = cs * U[i][j] + sn * U[i][k - 1];
                            U[i][k - 1]= -sn * U[i][j] + cs * U[i][k - 1];
                            U[i][j] = t;
                        }
                    }
                }
            }
            break;

        // Perform one qr step.
        case 3:
            {
                // Calculate the shift.
                double s1 = fabs(s[p - 1]), s2 = fabs(s[p - 2]);
                double s12 = s1 > s2 ? s1 : s2;
                double s3 = fabs(e[p - 2]), s4 = fabs(s[k]);
                double s34 = s3 > s4 ? s3 : s4;
                double s0 = s12 > s34 ? s12 : s34, s5 = fabs(e[k]);
                double scale = s0 > s5 ? s0 : s5; // MAX(s1, s2, s3, s4, s5)

                double sp = s[p - 1] / scale;
                double spm1 = s[p - 2] / scale;
                double epm1 = e[p - 2] / scale;
                double sk = s[k] / scale;
                double ek = e[k] / scale;
                double b = ((spm1 + sp) * (spm1 - sp) + epm1 * epm1) / 2.;
                double c = (sp * epm1) * (sp * epm1);
                double shift = 0.;
                double f, g;
                if ((b != 0.) | (c != 0.))
                {
                    shift = sqrt(b * b + c);
                    if (b < 0.)
                    {
                        shift = -shift;
                    }
                    shift = c / (b + shift);
                }
                f = (sk + sp) * (sk - sp) + shift;
                g = sk * ek;

                // Chase zeros.
                for (j = k; j < p - 1; j++)
                {
                    double t = hypot_d(f, g);
                    double cs = f / t;
                    double sn = g / t;
                    if (j != k)
                    {
                        e[j - 1] = t;
                    }
                    f = cs * s[j] + sn * e[j];
                    e[j] = cs * e[j] - sn * s[j];
                    g = sn * s[j + 1];
                    s[j + 1] = cs * s[j + 1];
                    if (wantv)
                    {
                        for (i = 0; i < n; i++)
                        {
                            t = cs * V[i][j] + sn * V[i][j + 1];
                            V[i][j + 1] = -sn * V[i][j] + cs * V[i][j + 1];
                            V[i][j] = t;
                        }
                    }
                    t = hypot_d(f, g);
                    cs = f / t;
                    sn = g / t;
                    s[j] = t;
                    f = cs * e[j] + sn * s[j + 1];
                    s[j + 1] = -sn * e[j] + cs * s[j + 1];
                    g = sn * e[j + 1];
                    e[j + 1] = cs * e[j + 1];
                    if (wantu && j < m - 1)
                    {
                        for (i = 0; i < m; i++)
                        {
                            t = cs * U[i][j] + sn * U[i][j + 1];
                            U[i][j + 1] = -sn * U[i][j] + cs * U[i][j + 1];
                            U[i][j] = t;
                        }
                    }
                }
                e[p - 2] = f;
                iter = iter + 1;
            }
            break;

        // Convergence.
        case 4:
            {
                // Make the singular values positive.
                if (s[k] <= 0.)
                {
                    s[k] = fabs(s[k]);
                    if (wantv)
                    {
                        for (i = 0; i <= pp; i++)
                        {
                            V[i][k] = -V[i][k];
                        }
                    }
                }

                // Order the singular values.
                while (k < pp)
                {
                    double t;
                    if (s[k] >= s[k + 1]) break;
                    t = s[k];
                    s[k] = s[k + 1];
                    s[k + 1] = t;
                    if (wantv && k < n - 1)
                    {
                        for (i = 0; i < n; i++)
                        {
                            t = V[i][k + 1];
                            V[i][k + 1] = V[i][k];
                            V[i][k] = t;
                        }
                    }
                    if (wantu && k < m - 1)
                    {
                        for (i = 0; i < m; i++)
                        {
                            t = U[i][k + 1];
                            U[i][k + 1]= U[i][k];
                            U[i][k] = t;
                        }
                    }
                    k++;
                }
                iter = 0;
                p--;
            }
            break;
        }
    }

    for (i = 0; i < nu; i++) S[i] = s[i]; // save back.

    free(A); free(A_);
    free(work); free(e); free(s);
    return 1;
}
