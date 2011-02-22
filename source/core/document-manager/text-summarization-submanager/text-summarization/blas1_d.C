# include "blas1_d.H"

# include <cstdlib>
# include <iostream>
# include <iomanip>
# include <cmath>
# include <ctime>

using namespace std;


//****************************************************************************80

double dasum ( int n, double x[], int incx )

//****************************************************************************80
//
//  Purpose:
//
//    DASUM takes the sum of the absolute values of a vector.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vector.
//
//    Input, double X[*], the vector to be examined.
//
//    Input, int INCX, the increment between successive entries of X.
//    INCX must not be negative.
//
//    Output, double DASUM, the sum of the absolute values of X.
//
{
    int i;
    int j;
    double value;

    value = 0.0;
    j = 0;

    for ( i = 0; i < n; i++ )
    {
        value = value + r8_abs ( x[j] );
        j = j + incx;
    }

    return value;
}
//****************************************************************************80

void daxpy ( int n, double da, double dx[], int incx, double dy[], int incy )

//****************************************************************************80
//
//  Purpose:
//
//    DAXPY computes constant times a vector plus a vector.
//
//  Discussion:
//
//    This routine uses unrolled loops for increments equal to one.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of elements in DX and DY.
//
//    Input, double DA, the multiplier of DX.
//
//    Input, double DX[*], the first vector.
//
//    Input, int INCX, the increment between successive entries of DX.
//
//    Input/output, double DY[*], the second vector.
//    On output, DY[*] has been replaced by DY[*] + DA * DX[*].
//
//    Input, int INCY, the increment between successive entries of DY.
//
{
    int i;
    int ix;
    int iy;
    int m;

    if ( n <= 0 )
    {
        return;
    }

    if ( da == 0.0 )
    {
        return;
    }
//
//  Code for unequal increments or equal increments
//  not equal to 1.
//
    if ( incx != 1 || incy != 1 )
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( - n + 1 ) * incx;
        }

        if ( 0 <= incy )
        {
            iy = 0;
        }
        else
        {
            iy = ( - n + 1 ) * incy;
        }

        for ( i = 0; i < n; i++ )
        {
            dy[iy] = dy[iy] + da * dx[ix];
            ix = ix + incx;
            iy = iy + incy;
        }
    }
//
//  Code for both increments equal to 1.
//
    else
    {
        m = n % 4;

        for ( i = 0; i < m; i++ )
        {
            dy[i] = dy[i] + da * dx[i];
        }

        for ( i = m; i < n; i = i + 4 )
        {
            dy[i  ] = dy[i  ] + da * dx[i  ];
            dy[i+1] = dy[i+1] + da * dx[i+1];
            dy[i+2] = dy[i+2] + da * dx[i+2];
            dy[i+3] = dy[i+3] + da * dx[i+3];
        }

    }

    return;
}
//****************************************************************************80

void dcopy ( int n, double dx[], int incx, double dy[], int incy )

//****************************************************************************80
//
//  Purpose:
//
//    DCOPY copies a vector X to a vector Y.
//
//  Discussion:
//
//    The routine uses unrolled loops for increments equal to one.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of elements in DX and DY.
//
//    Input, double DX[*], the first vector.
//
//    Input, int INCX, the increment between successive entries of DX.
//
//    Output, double DY[*], the second vector.
//
//    Input, int INCY, the increment between successive entries of DY.
//
{
    int i;
    int ix;
    int iy;
    int m;

    if ( n <= 0 )
    {
        return;
    }

    if ( incx == 1 && incy == 1 )
    {
        m = n % 7;

        if ( m != 0 )
        {
            for ( i = 0; i < m; i++ )
            {
                dy[i] = dx[i];
            }
        }

        for ( i = m; i < n; i = i + 7 )
        {
            dy[i] = dx[i];
            dy[i + 1] = dx[i + 1];
            dy[i + 2] = dx[i + 2];
            dy[i + 3] = dx[i + 3];
            dy[i + 4] = dx[i + 4];
            dy[i + 5] = dx[i + 5];
            dy[i + 6] = dx[i + 6];
        }
    }
    else
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( -n + 1 ) * incx;
        }

        if ( 0 <= incy )
        {
            iy = 0;
        }
        else
        {
            iy = ( -n + 1 ) * incy;
        }

        for ( i = 0; i < n; i++ )
        {
            dy[iy] = dx[ix];
            ix = ix + incx;
            iy = iy + incy;
        }

    }

    return;
}
//****************************************************************************80

double ddot ( int n, double dx[], int incx, double dy[], int incy )

//****************************************************************************80
//
//  Purpose:
//
//    DDOT forms the dot product of two vectors.
//
//  Discussion:
//
//    This routine uses unrolled loops for increments equal to one.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vectors.
//
//    Input, double DX[*], the first vector.
//
//    Input, int INCX, the increment between successive entries in DX.
//
//    Input, double DY[*], the second vector.
//
//    Input, int INCY, the increment between successive entries in DY.
//
//    Output, double DDOT, the sum of the product of the corresponding
//    entries of DX and DY.
//
{
    double dtemp;
    int i;
    int ix;
    int iy;
    int m;

    dtemp = 0.0;

    if ( n <= 0 )
    {
        return dtemp;
    }
//
//  Code for unequal increments or equal increments
//  not equal to 1.
//
    if ( incx != 1 || incy != 1 )
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( - n + 1 ) * incx;
        }

        if ( 0 <= incy )
        {
            iy = 0;
        }
        else
        {
            iy = ( - n + 1 ) * incy;
        }

        for ( i = 0; i < n; i++ )
        {
            dtemp = dtemp + dx[ix] * dy[iy];
            ix = ix + incx;
            iy = iy + incy;
        }
    }
//
//  Code for both increments equal to 1.
//
    else
    {
        m = n % 5;

        for ( i = 0; i < m; i++ )
        {
            dtemp = dtemp + dx[i] * dy[i];
        }

        for ( i = m; i < n; i = i + 5 )
        {
            dtemp = dtemp + dx[i  ] * dy[i  ]
                    + dx[i+1] * dy[i+1]
                    + dx[i+2] * dy[i+2]
                    + dx[i+3] * dy[i+3]
                    + dx[i+4] * dy[i+4];
        }

    }

    return dtemp;
}
//****************************************************************************80

double dmach ( int job )

//****************************************************************************80
//
//  Purpose:
//
//    DMACH computes machine parameters of double precision real arithmetic.
//
//  Discussion:
//
//    This routine is for testing only.  It is not required by LINPACK.
//
//    If there is trouble with the automatic computation of these quantities,
//    they can be set by direct assignment statements.
//
//    We assume the computer has
//
//      B = base of arithmetic;
//      T = number of base B digits;
//      L = smallest possible exponent;
//      U = largest possible exponent.
//
//    then
//
//      EPS = B**(1-T)
//      TINY = 100.0 * B**(-L+T)
//      HUGE = 0.01 * B**(U-T)
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int JOB:
//    1: requests EPS;
//    2: requests TINY;
//    3: requests HUGE.
//
//    Output, double DMACH, the requested value.
//
{
    double eps;
    double huge;
    double s;
    double tiny;
    double value;

    eps = 1.0;
    for ( ; ; )
    {
        value = 1.0 + ( eps / 2.0 );
        if ( value <= 1.0 )
        {
            break;
        }
        eps = eps / 2.0;
    }

    s = 1.0;

    for ( ; ; )
    {
        tiny = s;
        s = s / 16.0;

        if ( s * 1.0 == 0.0 )
        {
            break;
        }

    }

    tiny = ( tiny / eps ) * 100.0;
    huge = 1.0 / tiny;

    if ( job == 1 )
    {
        value = eps;
    }
    else if ( job == 2 )
    {
        value = tiny;
    }
    else if ( job == 3 )
    {
        value = huge;
    }
    else
    {
        cout << "\n";
        cout << "DMACH - Fatal error!\n";
        cout << "  Illegal input value of JOB = " << job << "\n";
        exit ( 1 );
    }

    return value;
}
//****************************************************************************80

double dnrm2 ( int n, double x[], int incx )

//****************************************************************************80
//
//  Purpose:
//
//    DNRM2 returns the euclidean norm of a vector.
//
//  Discussion:
//
//     DNRM2 ( X ) = sqrt ( X' * X )
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Sven Hammarling
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vector.
//
//    Input, double X[*], the vector whose norm is to be computed.
//
//    Input, int INCX, the increment between successive entries of X.
//
//    Output, double DNRM2, the Euclidean norm of X.
//
{
    double absxi;
    int i;
    int ix;
    double norm;
    double scale;
    double ssq;
    //double value;

    if ( n < 1 || incx < 1 )
    {
        norm = 0.0;
    }
    else if ( n == 1 )
    {
        norm = r8_abs ( x[0] );
    }
    else
    {
        scale = 0.0;
        ssq = 1.0;
        ix = 0;

        for ( i = 0; i < n; i++ )
        {
            if ( x[ix] != 0.0 )
            {
                absxi = r8_abs ( x[ix] );
                if ( scale < absxi )
                {
                    ssq = 1.0 + ssq * ( scale / absxi ) * ( scale / absxi );
                    scale = absxi;
                }
                else
                {
                    ssq = ssq + ( absxi / scale ) * ( absxi / scale );
                }
            }
            ix = ix + incx;
        }

        norm  = scale * sqrt ( ssq );
    }

    return norm;
}
//****************************************************************************80

void drot ( int n, double x[], int incx, double y[], int incy, double c,
            double s )

//****************************************************************************80
//
//  Purpose:
//
//    DROT applies a plane rotation.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vectors.
//
//    Input/output, double X[*], one of the vectors to be rotated.
//
//    Input, int INCX, the increment between successive entries of X.
//
//    Input/output, double Y[*], one of the vectors to be rotated.
//
//    Input, int INCY, the increment between successive elements of Y.
//
//    Input, double C, S, parameters (presumably the cosine and
//    sine of some angle) that define a plane rotation.
//
{
    int i;
    int ix;
    int iy;
    double stemp;

    if ( n <= 0 )
    {
    }
    else if ( incx == 1 && incy == 1 )
    {
        for ( i = 0; i < n; i++ )
        {
            stemp = c * x[i] + s * y[i];
            y[i]  = c * y[i] - s * x[i];
            x[i]  = stemp;
        }
    }
    else
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( - n + 1 ) * incx;
        }

        if ( 0 <= incy )
        {
            iy = 0;
        }
        else
        {
            iy = ( - n + 1 ) * incy;
        }

        for ( i = 0; i < n; i++ )
        {
            stemp = c * x[ix] + s * y[iy];
            y[iy] = c * y[iy] - s * x[ix];
            x[ix] = stemp;
            ix = ix + incx;
            iy = iy + incy;
        }

    }

    return;
}
//****************************************************************************80

void drotg ( double *sa, double *sb, double *c, double *s )

//****************************************************************************80
//
//  Purpose:
//
//    DROTG constructs a Givens plane rotation.
//
//  Discussion:
//
//    Given values A and B, this routine computes
//
//    SIGMA = sign ( A ) if abs ( A ) >  abs ( B )
//          = sign ( B ) if abs ( A ) <= abs ( B );
//
//    R     = SIGMA * ( A * A + B * B );
//
//    C = A / R if R is not 0
//      = 1     if R is 0;
//
//    S = B / R if R is not 0,
//        0     if R is 0.
//
//    The computed numbers then satisfy the equation
//
//    (  C  S ) ( A ) = ( R )
//    ( -S  C ) ( B ) = ( 0 )
//
//    The routine also computes
//
//    Z = S     if abs ( A ) > abs ( B ),
//      = 1 / C if abs ( A ) <= abs ( B ) and C is not 0,
//      = 1     if C is 0.
//
//    The single value Z encodes C and S, and hence the rotation:
//
//    If Z = 1, set C = 0 and S = 1;
//    If abs ( Z ) < 1, set C = sqrt ( 1 - Z * Z ) and S = Z;
//    if abs ( Z ) > 1, set C = 1/ Z and S = sqrt ( 1 - C * C );
//
//  Modified:
//
//    15 May 2006
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input/output, double *SA, *SB,  On input, SA and SB are the values
//    A and B.  On output, SA is overwritten with R, and SB is
//    overwritten with Z.
//
//    Output, double *C, *S, the cosine and sine of the Givens rotation.
//
{
    double r;
    double roe;
    double scale;
    double z;

    if ( r8_abs ( *sb ) < r8_abs ( *sa ) )
    {
        roe = *sa;
    }
    else
    {
        roe = *sb;
    }

    scale = r8_abs ( *sa ) + r8_abs ( *sb );

    if ( scale == 0.0 )
    {
        *c = 1.0;
        *s = 0.0;
        r = 0.0;
    }
    else
    {
        r = scale * sqrt ( ( *sa / scale ) * ( *sa / scale )
                           + ( *sb / scale ) * ( *sb / scale ) );
        r = r8_sign ( roe ) * r;
        *c = *sa / r;
        *s = *sb / r;
    }

    if ( 0.0 < r8_abs ( *c ) && r8_abs ( *c ) <= *s )
    {
        z = 1.0 / *c;
    }
    else
    {
        z = *s;
    }

    *sa = r;
    *sb = z;

    return;
}
//****************************************************************************80

void dscal ( int n, double sa, double x[], int incx )

//****************************************************************************80
//
//  Purpose:
//
//    DSCAL scales a vector by a constant.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    Jack Dongarra
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vector.
//
//    Input, double SA, the multiplier.
//
//    Input/output, double X[*], the vector to be scaled.
//
//    Input, int INCX, the increment between successive entries of X.
//
{
    int i;
    int ix;
    int m;

    if ( n <= 0 )
    {
    }
    else if ( incx == 1 )
    {
        m = n % 5;

        for ( i = 0; i < m; i++ )
        {
            x[i] = sa * x[i];
        }

        for ( i = m; i < n; i = i + 5 )
        {
            x[i]   = sa * x[i];
            x[i+1] = sa * x[i+1];
            x[i+2] = sa * x[i+2];
            x[i+3] = sa * x[i+3];
            x[i+4] = sa * x[i+4];
        }
    }
    else
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( - n + 1 ) * incx;
        }

        for ( i = 0; i < n; i++ )
        {
            x[ix] = sa * x[ix];
            ix = ix + incx;
        }

    }

    return;
}
//****************************************************************************80

void dswap ( int n, double x[], int incx, double y[], int incy )

//****************************************************************************80
//
//  Purpose:
//
//    DSWAP interchanges two vectors.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vectors.
//
//    Input/output, double X[*], one of the vectors to swap.
//
//    Input, int INCX, the increment between successive entries of X.
//
//    Input/output, double Y[*], one of the vectors to swap.
//
//    Input, int INCY, the increment between successive elements of Y.
//
{
    int i;
    int ix;
    int iy;
    int m;
    double temp;

    if ( n <= 0 )
    {
    }
    else if ( incx == 1 && incy == 1 )
    {
        m = n % 3;

        for ( i = 0; i < m; i++ )
        {
            temp = x[i];
            x[i] = y[i];
            y[i] = temp;
        }

        for ( i = m; i < n; i = i + 3 )
        {
            temp = x[i];
            x[i] = y[i];
            y[i] = temp;

            temp = x[i+1];
            x[i+1] = y[i+1];
            y[i+1] = temp;

            temp = x[i+2];
            x[i+2] = y[i+2];
            y[i+2] = temp;
        }
    }
    else
    {
        if ( 0 <= incx )
        {
            ix = 0;
        }
        else
        {
            ix = ( - n + 1 ) * incx;
        }

        if ( 0 <= incy )
        {
            iy = 0;
        }
        else
        {
            iy = ( - n + 1 ) * incy;
        }

        for ( i = 0; i < n; i++ )
        {
            temp = x[ix];
            x[ix] = y[iy];
            y[iy] = temp;
            ix = ix + incx;
            iy = iy + incy;
        }

    }

    return;
}
//****************************************************************************80

int i4_max ( int i1, int i2 )

//****************************************************************************80
//
//  Purpose:
//
//    I4_MAX returns the maximum of two I4's.
//
//  Modified:
//
//    13 October 1998
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int I1, I2, are two integers to be compared.
//
//    Output, int I4_MAX, the larger of I1 and I2.
//
//
{
    if ( i2 < i1 )
    {
        return i1;
    }
    else
    {
        return i2;
    }

}
//****************************************************************************80

int i4_min ( int i1, int i2 )

//****************************************************************************80
//
//  Purpose:
//
//    I4_MIN returns the smaller of two I4's.
//
//  Modified:
//
//    13 October 1998
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int I1, I2, two integers to be compared.
//
//    Output, int I4_MIN, the smaller of I1 and I2.
//
//
{
    if ( i1 < i2 )
    {
        return i1;
    }
    else
    {
        return i2;
    }

}
//****************************************************************************80

int idamax ( int n, double dx[], int incx )

//****************************************************************************80
//
//  Purpose:
//
//    IDAMAX finds the index of the vector element of maximum absolute value.
//
//  Discussion:
//
//    WARNING: This index is a 1-based index, not a 0-based index!
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, int N, the number of entries in the vector.
//
//    Input, double X[*], the vector to be examined.
//
//    Input, int INCX, the increment between successive entries of SX.
//
//    Output, int IDAMAX, the index of the element of maximum
//    absolute value.
//
{
    double dmax;
    int i;
    int ix;
    int value;

    value = 0;

    if ( n < 1 || incx <= 0 )
    {
        return value;
    }

    value = 1;

    if ( n == 1 )
    {
        return value;
    }

    if ( incx == 1 )
    {
        dmax = r8_abs ( dx[0] );

        for ( i = 1; i < n; i++ )
        {
            if ( dmax < r8_abs ( dx[i] ) )
            {
                value = i + 1;
                dmax = r8_abs ( dx[i] );
            }
        }
    }
    else
    {
        ix = 0;
        dmax = r8_abs ( dx[0] );
        ix = ix + incx;

        for ( i = 1; i < n; i++ )
        {
            if ( dmax < r8_abs ( dx[ix] ) )
            {
                value = i + 1;
                dmax = r8_abs ( dx[ix] );
            }
            ix = ix + incx;
        }
    }

    return value;
}
//****************************************************************************80

bool lsame ( char ca, char cb )

//****************************************************************************80
//
//  Purpose:
//
//    LSAME returns TRUE if CA is the same letter as CB regardless of case.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, char CA, CB, the characters to compare.
//
//    Output, bool LSAME, is TRUE if the characters are equal,
//    disregarding case.
//
{
    if ( ca == cb )
    {
        return true;
    }

    if ( 'A' <= ca && ca <= 'Z' )
    {
        if ( ca - 'A' == cb - 'a' )
        {
            return true;
        }
    }
    else if ( 'a' <= ca && ca <= 'z' )
    {
        if ( ca - 'a' == cb - 'A' )
        {
            return true;
        }
    }

    return false;
}
//****************************************************************************80

double r8_abs ( double x )

//****************************************************************************80
//
//  Purpose:
//
//    R8_ABS returns the absolute value of a R8.
//
//  Modified:
//
//    02 April 2005
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, double X, the quantity whose absolute value is desired.
//
//    Output, double R8_ABS, the absolute value of X.
//
{
    double value;

    if ( 0.0 <= x )
    {
        value = x;
    }
    else
    {
        value = -x;
    }
    return value;
}
//****************************************************************************80

double r8_max ( double x, double y )

//****************************************************************************80
//
//  Purpose:
//
//    R8_MAX returns the maximum of two R8's.
//
//  Modified:
//
//    18 August 2004
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, double X, Y, the quantities to compare.
//
//    Output, double R8_MAX, the maximum of X and Y.
//
{
    double value;

    if ( y < x )
    {
        value = x;
    }
    else
    {
        value = y;
    }
    return value;
}
//****************************************************************************80

double r8_sign ( double x )

//****************************************************************************80
//
//  Purpose:
//
//    R8_SIGN returns the sign of a R8.
//
//  Modified:
//
//    18 October 2004
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, double X, the number whose sign is desired.
//
//    Output, double R8_SIGN, the sign of X.
//
{
    double value;

    if ( x < 0.0 )
    {
        value = -1.0;
    }
    else
    {
        value = 1.0;
    }
    return value;
}
//****************************************************************************80

void xerbla ( char *srname, int info )

//****************************************************************************80
//
//  Purpose:
//
//    XERBLA is an error handler for the LAPACK routines.
//
//  Modified:
//
//    02 May 2005
//
//  Author:
//
//    C++ version by John Burkardt
//
//  Reference:
//
//    Jack Dongarra, Jim Bunch, Cleve Moler, Pete Stewart,
//    LINPACK User's Guide,
//    SIAM, 1979,
//    ISBN13: 978-0-898711-72-1,
//    LC: QA214.L56.
//
//    Charles Lawson, Richard Hanson, David Kincaid, Fred Krogh,
//    Basic Linear Algebra Subprograms for Fortran Usage,
//    Algorithm 539,
//    ACM Transactions on Mathematical Software,
//    Volume 5, Number 3, September 1979, pages 308-323.
//
//  Parameters:
//
//    Input, char *SRNAME, the name of the routine
//    which called XERBLA.
//
//    Input, int INFO, the position of the invalid parameter in
//    the parameter list of the calling routine.
//
{
    cout << "\n";
    cout << "XERBLA - Fatal error!\n";
    cout << "  On entry to routine " << srname << "\n";
    cout << "  input parameter number " << info << " had an illegal value.\n";
    exit ( 1 );
}
