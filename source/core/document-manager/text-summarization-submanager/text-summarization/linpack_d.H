#ifndef _LINPACK_D_H_
#define _LINPACK_D_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int dchdc ( double a[], int lda, int p, double work[], int ipvt[], int job );
int dchdd ( double r[], int ldr, int p, double x[], double z[], int ldz,
            int nz, double y[], double rho[], double c[], double s[] );
void dchex ( double r[], int ldr, int p, int k, int l, double z[], int ldz,
             int nz, double c[], double s[], int job );
void dchud ( double r[], int ldr, int p, double x[], double z[], int ldz,
             int nz, double y[], double rho[], double c[], double s[] );
double dgbco ( double abd[], int lda, int n, int ml, int mu, int ipvt[],
               double z[] );
void dgbdi ( double abd[], int lda, int n, int ml, int mu, int ipvt[],
             double det[2] );
int dgbfa ( double abd[], int lda, int n, int ml, int mu, int ipvt[] );
void dgbsl ( double abd[], int lda, int n, int ml, int mu, int ipvt[],
             double b[], int job );
double dgeco ( double a[], int lda, int n, int ipvt[], double z[] );
void dgedi ( double a[], int lda, int n, int ipvt[], double det[],
             double work[], int job );
int dgefa ( double a[], int lda, int n, int ipvt[] );
void dgesl ( double a[], int lda, int n, int ipvt[], double b[], int job );
int dgtsl ( int n, double c[], double d[], double e[], double b[] );
double dpbco ( double abd[], int lda, int n, int m, double z[] );
void dpbdi ( double abd[], int lda, int n, int m, double det[] );
int dpbfa ( double abd[], int lda, int n, int m );
void dpbsl ( double abd[], int lda, int n, int m, double b[] );
double dpoco ( double a[], int lda, int n, double z[] );
void dpodi ( double a[], int lda, int n, double det[], int job );
int dpofa ( double a[], int lda, int n );
void dposl ( double a[], int lda, int n, double b[] );
double dppco ( double ap[], int n, double z[] );
void dppdi ( double ap[], int n, double det[2], int job );
int dppfa ( double ap[], int n );
void dppsl ( double ap[], int n, double b[] );
void dptsl ( int n, double d[], double e[], double b[] );
void dqrdc ( double a[], int lda, int n, int p, double qraux[], int jpvt[],
             double work[], int job );
int dqrsl ( double a[], int lda, int n, int k, double qraux[], double y[],
            double qy[], double qty[], double b[], double rsd[], double ab[], int job );
double dsico ( double a[], int lda, int n, int kpvt[], double z[] );
void dsidi ( double a[], int lda, int n, int kpvt[], double det[2],
             int inert[3], double work[], int job );
int dsifa ( double a[], int lda, int n, int kpvt[] );
void dsisl ( double a[], int lda, int n, int kpvt[], double b[] );
double dspco ( double ap[], int n, int kpvt[], double z[] );
void dspdi ( double ap[], int n, int kpvt[], double det[2], int inert[3],
             double work[], int job );
int dspfa ( double ap[], int n, int kpvt[] );
void dspsl ( double ap[], int n, int kpvt[], double b[] );
int dsvdc ( double a[], int lda, int m, int n, double s[], double e[],
            double u[], int ldu, double v[], int ldv, double work[], int job );
double dtrco ( double t[], int ldt, int n, double z[], int job );
int dtrdi ( double t[], int ldt, int n, double det[], int job );
int dtrsl ( double t[], int ldt, int n, double b[], int job );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //_LINPACK_D_H_
