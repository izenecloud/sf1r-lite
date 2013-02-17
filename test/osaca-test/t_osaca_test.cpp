#include <am/succinct/osaca/osaca.h>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <vector>

using namespace std;
#define DEBUGLEVEL 1
#include <time.h>
#if !defined( unix )
#include <io.h>
#include <fcntl.h>
#endif

typedef unsigned char char_type;
typedef uint40_t savalue_type;
using namespace izenelib::am::succinct;
/* Globals */
char* input_file;
char* output_file;

void version()
{
    std::cerr << "OSACA test version 0.0.1" << std::endl
              << "Written by Kuilong Liu" << std::endl << std::endl;
}

void usage()
{
    std::cerr << std::endl
        << "Usage: ./t_osaca_test input-filename output-filename" << std::endl << std::endl
        << "             input-filename       (input string)" << std::endl
        << "             output-filename      (suffix array)" << std::endl
        << std::endl;
    exit(0);
}

void parse_parameters (int argc, char **argv)
{
    if(argc != 3)usage();
    input_file = argv[1];
    output_file = argv[2];
}

// uncomment the below line to verify the result SA
#define _verify_sa

// output values:
//  1: s1<s2
//  0: s1=s2
// -1: s1>s2
template<typename string_type, typename index_type>
int sless(string_type s1, string_type s2, index_type n)
{
    for(index_type i=0; i<n; i++)
    {
        if (s1[i] < s2[i]) return 1;
        if (s1[i] > s2[i]) return -1;
    }
    return 0;
}

// test if SA is sorted for the input string s
template<typename sarray_type, typename string_type, typename index_type>
bool isSorted(sarray_type SA, string_type s, index_type n)
{
    for(index_type i = 0;  i < n-1;  i++)
    {
        index_type d=SA[i]<SA[i+1]?n-SA[i+1]:n-SA[i];
        int rel=sless(s+SA[i], s+SA[i+1], d);
        if(rel==-1 || (rel==0 && SA[i+1]>SA[i]))
            return false;
    }
    return true;
}


int main(int argc, char **argv)
{
/*
    cout << sizeof(uint40_t)<<endl;
    osaca::uint40_t EMPTY = ((osaca::uint40_t)1) << 5;
    cout << (osaca::uint40_t)1 <<endl;
    cout << EMPTY << endl;
    osaca::int40_t a = EMPTY;
    cout << a << endl;
    return 0;
*/
    version();
    parse_parameters(argc, argv);

    fprintf(stderr, "\nComputing suffix array by OSACA on");
    if(freopen(input_file, "rb", stdin)==NULL) return 0;
    fprintf(stderr, "%s", input_file);
    fprintf(stderr, " to ");
    if(freopen(output_file, "wb", stdout)==NULL) return 0;
    fprintf(stderr, "%s", output_file);
    fprintf(stderr, "\n");

#if !defined(unix)
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    // Allocate 5 bytes memory for input string and output suffix array
    fseek(stdin, 0, SEEK_END);
    savalue_type n=ftell(stdin);
    if(n==(savalue_type)0)
    {
        fprintf(stderr, "\nEmpty string, nothing to sort!");
        return 0;
    }
    n++; // append the virtual sentinel

    std::vector<char_type> v_s_ch;
    std::vector<savalue_type> v_SA;
    v_s_ch.resize(n);
    v_SA.resize(n);
    std::vector<char_type>::iterator it = v_s_ch.begin();
    std::vector<savalue_type>::iterator iter = v_SA.begin();
    char_type *s_ch=(char_type *)&it[0];
    savalue_type *SA=(savalue_type *)&iter[0];


	// read the string into buffer.
	fprintf(stderr, "\nReading input string...");
	fseek(stdin, 0, SEEK_SET );
	if(fread((char_type *) s_ch, 1, n-1, stdin) == 0)return 0;
	// set the virtual sentinel
	s_ch[n-1]=0;

	clock_t start, finish;
	double  duration;
	start = clock();

	fprintf(stderr, "\nConstructing the suffix array...");

    izenelib::am::succinct::osaca::osacaxx(v_s_ch.begin(), v_SA.begin(), n, (savalue_type)256);

	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;

	fprintf(stderr, "\nSize: %u bytes, Time: %5.3f seconds\n", (unsigned int)(n-1), duration);

#ifdef _verify_sa
    fprintf(stderr, "\nVerifying the suffix array...");
    fprintf(stderr, "\nSorted: %d", (int)isSorted(SA+1, s_ch, n-1));
#endif


    fprintf(stderr, "\nOutputing the suffix array...");
    for(savalue_type i=1; i<n; i++)
    {
        cout<<SA[i]<<endl;
    }
  return 0;
}
