#include "d-mat2d.h"

#include <stdio.h>

void mat_print(double **mat, int m, int n, const char *fmt)
{
    int r, c;

    for (r = 0; r < m; r++)
    {
        fprintf(stdout, "|");
        for (c = 0; c < n; c++)
        {
            fprintf(stdout, fmt, mat[r][c]);
        }
        fprintf(stdout, "|\n");
    }
}

void mat_file_print(double **mat, int m, int n, const char *fmt, FILE *f)
{
    int r, c;
    for (r = 0; r < m; r++)
    {
        fprintf(f, "|");
        for (c = 0; c < n; c++)
        {
            fprintf(f, fmt, mat[r][c]);
        }
        fprintf(f, "|\n");
    }
    fclose(f);
}

int mat_save(double **mat, int m, int n, const char *fn)
{
    int r;
    FILE *fp = fopen(fn, "wb");
    if (!fp) return 0;

    if (1 != fwrite(&m, sizeof(int), 1, fp)) goto quit;
    if (1 != fwrite(&n, sizeof(int), 1, fp)) goto quit;

    for (r = 0; r < m; r++)
    {
        if (n != fwrite(mat[r], sizeof(double), n, fp)) goto quit;
    }

    fclose(fp); return 1;

quit:
    fclose(fp); return 0;
}

double **mat_load(int *m, int *n, const char *fn)
{
    double **mat;
    int r;
    FILE *fp = fopen(fn, "rb");
    if (!fp) return 0;

    if (1 != fread(m, sizeof(int), 1, fp)) goto quit;
    if (1 != fread(n, sizeof(int), 1, fp)) goto quit;

    mat = mat_alloc(*m, *n);
    if (!mat) goto quit;

    for (r = 0; r < *m; r++)
    {
        if (*n != fread(mat[r], sizeof(double), *n, fp)) goto error;
    }

    fclose(fp); return mat;

error:
    mat_free(mat);
quit:
    fclose(fp); return NULL;
}
