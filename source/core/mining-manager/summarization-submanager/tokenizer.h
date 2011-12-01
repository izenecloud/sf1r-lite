/*
 * tokenizer.h
 *
 *  Created on: 2009-6-10
 *      Author: Huajie Zhang
 */

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_TOKENIZER_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_TOKENIZER_H_

#include <iostream>
#include <string>
#include <cstring>


const int TWO_BYPE_MAX = 1 << 16;

class Tokenizer
{
public:
    typedef unsigned char CharType;

    static const CharType ALLOW = 0x00;
    static const CharType DELIMITER = 0x01;
    static const CharType SPACE = 0x02;
    static const CharType UNITE = 0x03;

private:
    CharType m_aCharTypeTable[TWO_BYPE_MAX];
    const char* m_strData;
    int m_dwLength;
    int m_dwOffset;
    int m_dwTokenStart;
    int m_dwTokenEnd;
    bool isBigram;

public:
    Tokenizer()
        :m_dwOffset(0), isBigram(false)
    {
        memset(m_aCharTypeTable, 0, TWO_BYPE_MAX);
        // set the lower 4 bit to record default char type
        for (int i = 0; i < TWO_BYPE_MAX; ++ i)
        {
            if (std::isalpha(i) || std::isalnum(i))
                continue;
            else if (std::isspace(i))
                m_aCharTypeTable[i] = SPACE;
            else
                m_aCharTypeTable[i] = DELIMITER;
        }
    }



    void set_char_type(const std::string& str, unsigned char type)
    {
        if (str.length() > 0)
        {
            for (unsigned int j = 0; j < str.length(); j++)
                m_aCharTypeTable[str[j]] = type;
        }
    }

    void load(const char* input, int length)
    {
        m_strData = input;
        m_dwLength = length;
        m_dwOffset = 0;
        isBigram = false;
        m_dwTokenStart = 0;
    }

    bool next_token(std::string& token)
    {
        token.clear();
        if (m_dwOffset > 0)
            isBigram = true;
        for (; m_dwOffset < m_dwLength; ++ m_dwOffset)
        {
            char c = m_strData[m_dwOffset];
            CharType type = m_aCharTypeTable[c];
            if (type == ALLOW)
            {
                m_dwTokenStart = m_dwOffset;
                token += c;
                break;
            }
            else if(type == DELIMITER)
                isBigram = false;
        }
        if (m_dwOffset == m_dwLength)
            return false;

        for (++m_dwOffset; m_dwOffset < m_dwLength; ++m_dwOffset)
        {
            char c = m_strData[m_dwOffset];
            CharType type = m_aCharTypeTable[c];
            if (type == ALLOW)
            {
                token += c;
            }
            else if (type == UNITE)
            {
            }
            else
            {
                m_dwTokenEnd = m_dwOffset;
                return true;
            }
        }
        m_dwTokenEnd = m_dwOffset;
        return false;
    }
};

#endif /* _TOKENIZER_H_ */
