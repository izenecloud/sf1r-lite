
#ifndef _SVD_WPS_H_
#define _SVD_WPS_H_

namespace sf1r
{
namespace text_summarization
{


unsigned long get_seed ( void );
void get_svd_linpack ( int m, int n, double a[], double u[], double s[],
                       double v[] );
int i4_uniform ( int b, int c, int *seed );
double *pseudo_inverse ( int m, int n, double u[], double s[],
                         double v[] );
void pseudo_linear_solve_test ( int m, int n, double a[],
                                double a_pseudo[], int *seed );
void pseudo_product_test ( int m, int n, double a[], double a_pseudo[] );
float r4_abs ( float x );
int r4_nint ( float x );
float r4_uniform ( float b, float c, int *seed );
float r4_uniform_01 ( int *seed );
int r8_nint ( double x );
double r8mat_dif_fro ( int m, int n, double a[], double b[] );
double r8mat_norm_fro ( int m, int n, double a[] );
void r8mat_print ( int m, int n, double a[], char *title );
void r8mat_print_some ( int m, int n, double a[], int ilo, int jlo, int ihi,
                        int jhi, char *title );
double r8vec_norm_l2 ( int n, double a[] );
double *r8mat_uniform_01 ( int m, int n, int *seed );
double *r8vec_uniform_01 ( int n, int *seed );
void rank_one_print_test ( int m, int n, double a[], double u[],
                           double s[], double v[] );
void rank_one_test ( int m, int n, double a[], double u[], double s[],
                     double v[] );
int s_len_trim ( char *s );
void svd_product_test ( int m, int n, double a[], double u[],
                        double s[], double v[] );
void timestamp ( void );


}
}

#endif //_SVD_WPS_H_
