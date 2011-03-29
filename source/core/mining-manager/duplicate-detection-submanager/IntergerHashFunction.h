
/**
 * @file IntergerHashFunction.h
 * @author Yeogirl Yun
 * @date 2009.11.24
 * @brief integer hash functions defined here
 * @detail
 * An integer hash function accepts an integer hash key, 
 * and returns an integer hash result with uniform distribution.
 *
 * All functions here are from Thomas Wang's java function.
 * Please view http://www.concentric.net/~Ttwang/tech/inthash.htm
 * for details
 *
 * Since there is no >>> operator in c++ like that in java, 
 * all values are setted to unsigned to support this function.
 */

#ifndef __INTEGERHASHFUNCTION_H__
#define __INTEGERHASHFUNCTION_H__

#include <stdint.h>

/** 
 * @brief namespace of hash functions
 */
namespace int_hash {
    /** 
     * @brief mix 3 int
     * 
     * @param a first int
     * @param b second int
     * @param c third int
     * 
     * @return mixed value
     */
    inline unsigned int mix_ints(unsigned int a, unsigned int b, unsigned int c)
    {
        a=a-b;  a=a-c;  a=a^(c >> 13);
        b=b-c;  b=b-a;  b=b^(a << 8); 
        c=c-a;  c=c-b;  c=c^(b >> 13);
        a=a-b;  a=a-c;  a=a^(c >> 12);
        b=b-c;  b=b-a;  b=b^(a << 16);
        c=c-a;  c=c-b;  c=c^(b >> 5);
        a=a-b;  a=a-c;  a=a^(c >> 3);
        b=b-c;  b=b-a;  b=b^(a << 10);
        c=c-a;  c=c-b;  c=c^(b >> 15);
        return c;
    }

    /** 
     * @brief hash 32 bit int
     * 
     * @param key source
     * 
     * @return hashed value
     */
    inline uint32_t hash32shift(uint32_t key) {
        key = ~key + (key << 15); // key = (key << 15) - key - 1;
        key = key ^ (key >> 12);
        key = key + (key << 2);
        key = key ^ (key >> 4);
        key = key * 2057; // key = (key + (key << 3)) + (key << 11);
        key = key ^ (key >> 16);
        return key;
    }

    /** 
     * @brief hash unsinged 32-bit int
     * 
     * @param a source 
     * 
     * @return hashed value
     */
    inline uint32_t hash( uint32_t a) {
        a = (a+0x7ed55d16) + (a<<12);
        a = (a^0xc761c23c) ^ (a>>19);
        a = (a+0x165667b1) + (a<<5);
        a = (a+0xd3a2646c) ^ (a<<9);
        a = (a+0xfd7046c5) + (a<<3);
        a = (a^0xb55a4f09) ^ (a>>16);
        return a;
    }

    /** 
     * @brief hash 32 bit
     * 
     * @param key source
     * 
     * @return hashed value
     */
    inline uint32_t hash32shiftmult(uint32_t key) {
        uint32_t c2=0x27d4eb2d; // a prime or an odd constant
        key = (key ^ 61) ^ (key >> 16);
        key = key + (key << 3);
        key = key ^ (key >> 4);
        key = key * c2;
        key = key ^ (key >> 15);
        return key;
    }


    /** 
     * @brief hash 64 bit interger
     * 
     * @param key source
     * 
     * @return hashed value
     */
    inline unsigned long hash64shift(unsigned long key) {
        key = (~key) + (key << 21); // key = (key << 21) - key - 1;
        key = key ^ (key >> 24);
        key = (key + (key << 3)) + (key << 8); // key * 265
        key = key ^ (key >> 14);
        key = (key + (key << 2)) + (key << 4); // key * 21
        key = key ^ (key >> 28);
        key = key + (key << 31);
        return key;
    }

    /** 
     * @brief hash a 64-bit integer to 32-bit interger
     * 
     * @param key 64-bit integer to hash
     * 
     * @return hashed 32-bit result.
     */
    inline uint32_t hash6432shift(uint64_t key) {
        key = (~key) + (key << 18); // key = (key << 18) - key - 1;
        key = key ^ (key >> 31);
        key = key * 21; // key = (key + (key << 2)) + (key << 4);
        key = key ^ (key >> 11);
        key = key + (key << 6);
        key = key ^ (key >> 22);
        return (uint32_t) key;
    }
}
#endif

