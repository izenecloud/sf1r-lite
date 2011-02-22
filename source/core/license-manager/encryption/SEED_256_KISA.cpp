/*******************************************************************************
* Created 29, June, 1999
* Modified 4, June, 2008
* FILE:         SEED_256_KISA.c
*
* DESCRIPTION: Core routines for the enhanced SEED
* 
*******************************************************************************/

#include "SEED_256_KISA.h"
#include "SEED_256_KISA.tab"

//---------------------------------------------------------------------------

/******************* Encryption/Decryption *******************/

#define GetB0(A)  ( (BYTE)((A)    ) )
#define GetB1(A)  ( (BYTE)((A)>> 8) )
#define GetB2(A)  ( (BYTE)((A)>>16) )
#define GetB3(A)  ( (BYTE)((A)>>24) )

#define SeedRound(L0, L1, R0, R1, K) {             \
    T0 = R0 ^ (K)[0];                              \
    T1 = R1 ^ (K)[1];                              \
    T1 ^= T0;                                      \
    T1 = SS0[GetB0(T1)] ^ SS1[GetB1(T1)] ^         \
         SS2[GetB2(T1)] ^ SS3[GetB3(T1)];          \
    T0 += T1;                                      \
    T0 = SS0[GetB0(T0)] ^ SS1[GetB1(T0)] ^         \
         SS2[GetB2(T0)] ^ SS3[GetB3(T0)];          \
    T1 += T0;                                      \
    T1 = SS0[GetB0(T1)] ^ SS1[GetB1(T1)] ^         \
         SS2[GetB2(T1)] ^ SS3[GetB3(T1)];          \
    T0 += T1;                                      \
    L0 ^= T0; L1 ^= T1;                            \
}


#define EndianChange(dwS)                       \
    ( (ROTL((dwS),  8) & (DWORD)0x00ff00ff) |   \
      (ROTL((dwS), 24) & (DWORD)0xff00ff00) )


/************************ Block Encryption *************************/
void SeedEncrypt(BYTE *pbData, DWORD *pdwRoundKey)
{
	DWORD L0, L1, R0, R1, T0, T1, *K = pdwRoundKey;

    L0 = ((DWORD *)pbData)[0];
    L1 = ((DWORD *)pbData)[1];
	R0 = ((DWORD *)pbData)[2];
    R1 = ((DWORD *)pbData)[3];
#ifndef SEED_BIG_ENDIAN
    L0 = EndianChange(L0);
    L1 = EndianChange(L1);
    R0 = EndianChange(R0);
    R1 = EndianChange(R1);
#endif

    SeedRound(L0, L1, R0, R1, K   ); /*   1 */
    SeedRound(R0, R1, L0, L1, K+ 2); /*   2 */
    SeedRound(L0, L1, R0, R1, K+ 4); /*   3 */
    SeedRound(R0, R1, L0, L1, K+ 6); /*   4 */
    SeedRound(L0, L1, R0, R1, K+ 8); /*   5 */
    SeedRound(R0, R1, L0, L1, K+10); /*   6 */
    SeedRound(L0, L1, R0, R1, K+12); /*   7 */
    SeedRound(R0, R1, L0, L1, K+14); /*   8 */
    SeedRound(L0, L1, R0, R1, K+16); /*   9 */
    SeedRound(R0, R1, L0, L1, K+18); /*  10 */
    SeedRound(L0, L1, R0, R1, K+20); /*  11 */
    SeedRound(R0, R1, L0, L1, K+22); /*  12 */
    SeedRound(L0, L1, R0, R1, K+24); /*  13 */
    SeedRound(R0, R1, L0, L1, K+26); /*  14 */
    SeedRound(L0, L1, R0, R1, K+28); /*  15 */
    SeedRound(R0, R1, L0, L1, K+30); /*  16 */
	SeedRound(L0, L1, R0, R1, K+32); /*  17 */
    SeedRound(R0, R1, L0, L1, K+34); /*  18 */
    SeedRound(L0, L1, R0, R1, K+36); /*  19 */
    SeedRound(R0, R1, L0, L1, K+38); /*  20 */
	SeedRound(L0, L1, R0, R1, K+40); /*  21 */
    SeedRound(R0, R1, L0, L1, K+42); /*  22 */
    SeedRound(L0, L1, R0, R1, K+44); /*  23 */
    SeedRound(R0, R1, L0, L1, K+46); /*  24 */

#ifndef SEED_BIG_ENDIAN
    L0 = EndianChange(L0);
    L1 = EndianChange(L1);
    R0 = EndianChange(R0);
    R1 = EndianChange(R1);
#endif
    ((DWORD *)pbData)[0] = R0;
    ((DWORD *)pbData)[1] = R1;
    ((DWORD *)pbData)[2] = L0;
    ((DWORD *)pbData)[3] = L1;
}


/* same as encrypt, except that round keys are applied in reverse order. */
void SeedDecrypt(BYTE *pbData, DWORD *pdwRoundKey)
{
    DWORD L0, L1, R0, R1, T0, T1, *K=pdwRoundKey;

    L0 = ((DWORD *)pbData)[0];
    L1 = ((DWORD *)pbData)[1];
    R0 = ((DWORD *)pbData)[2];
    R1 = ((DWORD *)pbData)[3];
#ifndef SEED_BIG_ENDIAN
    L0 = EndianChange(L0);
    L1 = EndianChange(L1);
    R0 = EndianChange(R0);
    R1 = EndianChange(R1);
#endif

	SeedRound(L0, L1, R0, R1, K+46); /*   1 */
    SeedRound(R0, R1, L0, L1, K+44); /*   2 */
    SeedRound(L0, L1, R0, R1, K+42); /*   3 */
    SeedRound(R0, R1, L0, L1, K+40); /*   4 */    
	SeedRound(L0, L1, R0, R1, K+38); /*   5 */
    SeedRound(R0, R1, L0, L1, K+36); /*   6 */
    SeedRound(L0, L1, R0, R1, K+34); /*   7 */
    SeedRound(R0, R1, L0, L1, K+32); /*   8 */
    SeedRound(L0, L1, R0, R1, K+30); /*   9 */
    SeedRound(R0, R1, L0, L1, K+28); /*  10 */
    SeedRound(L0, L1, R0, R1, K+26); /*  11 */
    SeedRound(R0, R1, L0, L1, K+24); /*  12 */
    SeedRound(L0, L1, R0, R1, K+22); /*  13 */
    SeedRound(R0, R1, L0, L1, K+20); /*  14 */
    SeedRound(L0, L1, R0, R1, K+18); /*  15 */
    SeedRound(R0, R1, L0, L1, K+16); /*  16 */
    SeedRound(L0, L1, R0, R1, K+14); /*  17 */
    SeedRound(R0, R1, L0, L1, K+12); /*  18 */
    SeedRound(L0, L1, R0, R1, K+10); /*  19 */
    SeedRound(R0, R1, L0, L1, K+ 8); /*  20 */
    SeedRound(L0, L1, R0, R1, K+ 6); /*  21 */
    SeedRound(R0, R1, L0, L1, K+ 4); /*  22 */
    SeedRound(L0, L1, R0, R1, K+ 2); /*  23 */
    SeedRound(R0, R1, L0, L1, K+ 0); /*  24 */

#ifndef SEED_BIG_ENDIAN
    L0 = EndianChange(L0);
    L1 = EndianChange(L1);
    R0 = EndianChange(R0);
    R1 = EndianChange(R1);
#endif
    ((DWORD *)pbData)[0] = R0;
    ((DWORD *)pbData)[1] = R1;
    ((DWORD *)pbData)[2] = L0;
    ((DWORD *)pbData)[3] = L1;
}



/******************** Key Scheduling **********************/

/* Constants for key schedule:
KC0 = golden ratio; KCi = ROTL(KCi-1, 1) */
#define KC0 0x9e3779b9UL
#define KC1 0x3c6ef373UL
#define KC2 0x78dde6e6UL
#define KC3 0xf1bbcdccUL
#define KC4 0xe3779b99UL
#define KC5 0xc6ef3733UL
#define KC6 0x8dde6e67UL
#define KC7 0x1bbcdccfUL
#define KC8 0x3779b99eUL
#define KC9 0x6ef3733cUL
#define KC10 0xdde6e678UL
#define KC11 0xbbcdccf1UL
#define KC12 0x779b99e3UL
#define KC13 0xef3733c6UL
#define KC14 0xde6e678dUL
#define KC15 0xbcdccf1bUL
#define KC16 0x79b99e37UL
#define KC17 0xf3733c6eUL
#define KC18 0xe6e678ddUL
#define KC19 0xcdccf1bbUL
#define KC20 0x9b99e377UL
#define KC21 0x3733c6efUL
#define KC22 0x6e678ddeUL
#define KC23 0xdccf1bbcUL



#define EncRoundKeyUpdate0(K, A, B, C, D, E, F, G, H, KC, rot) {  \
    T0 = A;                                      \
    A = (A>>rot) ^ (B<<(32-rot));                \
    B = (B>>rot) ^ (C<<(32-rot));                \
	C = (C>>rot) ^ (D<<(32-rot));                \
	D = (D>>rot) ^ (T0<<(32-rot));               \
    T0 = (((A + C) ^ E ) - F) ^ KC;              \
    T1 = (((B - D) ^ G ) + H) ^ KC;              \
    (K)[0] = SS0[GetB0(T0)] ^ SS1[GetB1(T0)] ^   \
             SS2[GetB2(T0)] ^ SS3[GetB3(T0)];    \
    (K)[1] = SS0[GetB0(T1)] ^ SS1[GetB1(T1)] ^   \
             SS2[GetB2(T1)] ^ SS3[GetB3(T1)];    \
}


#define EncRoundKeyUpdate1(K, A, B, C, D, E, F, G, H, KC, rot) {  \
    T0 = E;                                      \
    E = (E<<rot) ^ (F>>(32-rot));                \
    F = (F<<rot) ^ (G>>(32-rot));                \
	G = (G<<rot) ^ (H>>(32-rot));                \
	H = (H<<rot) ^ (T0>>(32-rot));               \
    T0 = (((A + C) ^ E ) - F) ^ KC;              \
    T1 = (((B - D) ^ G ) + H) ^ KC;              \
    (K)[0] = SS0[GetB0(T0)] ^ SS1[GetB1(T0)] ^   \
             SS2[GetB2(T0)] ^ SS3[GetB3(T0)];    \
    (K)[1] = SS0[GetB0(T1)] ^ SS1[GetB1(T1)] ^   \
             SS2[GetB2(T1)] ^ SS3[GetB3(T1)];    \
}


/* Encryption key schedule */
void SeedRoundKey(DWORD *pdwRoundKey, BYTE *pbUserKey)
{
  DWORD A, B, C, D, E, F, G, H, T0, T1, *K=pdwRoundKey;

  A = ((DWORD *)pbUserKey)[0];
  B = ((DWORD *)pbUserKey)[1];
  C = ((DWORD *)pbUserKey)[2];
  D = ((DWORD *)pbUserKey)[3];
  E = ((DWORD *)pbUserKey)[4];
  F = ((DWORD *)pbUserKey)[5];
  G = ((DWORD *)pbUserKey)[6];
  H = ((DWORD *)pbUserKey)[7];

#ifndef SEED_BIG_ENDIAN
  A = EndianChange(A);
  B = EndianChange(B);
  C = EndianChange(C);
  D = EndianChange(D);
  E = EndianChange(E);
  F = EndianChange(F);
  G = EndianChange(G);
  H = EndianChange(H);
#endif
    T0 = (((A + C) ^ E ) - F) ^ KC0;              
    T1 = (((B - D) ^ G ) + H) ^ KC0;              
    K[0] = SS0[GetB0(T0)] ^ SS1[GetB1(T0)] ^
           SS2[GetB2(T0)] ^ SS3[GetB3(T0)];
    K[1] = SS0[GetB0(T1)] ^ SS1[GetB1(T1)] ^
           SS2[GetB2(T1)] ^ SS3[GetB3(T1)];

    EncRoundKeyUpdate0(K+ 2, A, B, C, D, E, F, G, H, KC1 ,  9);
    EncRoundKeyUpdate1(K+ 4, A, B, C, D, E, F, G, H, KC2 ,  9);
    EncRoundKeyUpdate0(K+ 6, A, B, C, D, E, F, G, H, KC3 , 11);
    EncRoundKeyUpdate1(K+ 8, A, B, C, D, E, F, G, H, KC4 , 11);
    EncRoundKeyUpdate0(K+10, A, B, C, D, E, F, G, H, KC5 , 12);
    EncRoundKeyUpdate1(K+12, A, B, C, D, E, F, G, H, KC6 , 12);
    EncRoundKeyUpdate0(K+14, A, B, C, D, E, F, G, H, KC7 ,  9);
    EncRoundKeyUpdate1(K+16, A, B, C, D, E, F, G, H, KC8 ,  9);
    EncRoundKeyUpdate0(K+18, A, B, C, D, E, F, G, H, KC9 , 11);
    EncRoundKeyUpdate1(K+20, A, B, C, D, E, F, G, H, KC10, 11);
    EncRoundKeyUpdate0(K+22, A, B, C, D, E, F, G, H, KC11, 12);
    EncRoundKeyUpdate1(K+24, A, B, C, D, E, F, G, H, KC12, 12);
    EncRoundKeyUpdate0(K+26, A, B, C, D, E, F, G, H, KC13,  9);
    EncRoundKeyUpdate1(K+28, A, B, C, D, E, F, G, H, KC14,  9);
    EncRoundKeyUpdate0(K+30, A, B, C, D, E, F, G, H, KC15, 11);
	EncRoundKeyUpdate1(K+32, A, B, C, D, E, F, G, H, KC16, 11);
    EncRoundKeyUpdate0(K+34, A, B, C, D, E, F, G, H, KC17, 12);
    EncRoundKeyUpdate1(K+36, A, B, C, D, E, F, G, H, KC18, 12);
    EncRoundKeyUpdate0(K+38, A, B, C, D, E, F, G, H, KC19,  9);
	EncRoundKeyUpdate1(K+40, A, B, C, D, E, F, G, H, KC20,  9);
    EncRoundKeyUpdate0(K+42, A, B, C, D, E, F, G, H, KC21, 11);
    EncRoundKeyUpdate1(K+44, A, B, C, D, E, F, G, H, KC22, 11);
    EncRoundKeyUpdate0(K+46, A, B, C, D, E, F, G, H, KC23, 12);

}

/************************ END ****************************/
