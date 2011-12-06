#ifndef  _BLAS1_D_H_
#define _BLAS1_D_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

double dasum ( int n, double x[], int incx );
void daxpy ( int n, double da, double dx[], int incx, double dy[], int incy );
void dcopy ( int n, double dx[], int incx, double dy[], int incy );
double ddot ( int n, double dx[], int incx, double dy[], int incy );
double dmach ( int job );
double dnrm2 ( int n, double x[], int incx );
void drot ( int n, double x[], int incx, double y[], int incy, double c,
            double s );
void drotg ( double *sa, double *sb, double *c, double *s );
void dscal ( int n, double sa, double x[], int incx );
void dswap ( int n, double x[], int incx, double y[], int incy );
int i4_max ( int i1, int i2 );
int i4_min ( int i1, int i2 );
int idamax ( int n, double dx[], int incx );
bool lsame ( char ca, char cb );
double r8_abs ( double x );
double r8_max ( double x, double y );
double r8_sign ( double x );
void xerbla ( char *srname, int info );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //_BLAS1_D_H_
