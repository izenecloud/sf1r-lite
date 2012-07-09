/*
 * Util.cpp
 *
 *  Created on: Nov 3, 2010
 *      Author: wps
 */

#ifndef UTIL_CPP_
#define UTIL_CPP_

#include  "Util.h"


namespace sf1r
{

//Check if one word is pure english word
bool isEnglishWord(const izenelib::util::UString& queryToken)
{
    bool flag = true;
    for (unsigned int i = 0; i < queryToken.length(); i++)
    {
        if (!izenelib::util::UString::isThisAlphaChar(queryToken.at(i)))
        {
            flag = false;
            return flag;
        }
    }
    return flag;
}

//Check if one word is pure korean
bool isKoreanWord(const izenelib::util::UString& queryToken)
{
    bool flag = true;
    for (unsigned int i = 0; i < queryToken.length(); i++)
    {
        if (!izenelib::util::UString::isThisKoreanChar(queryToken.at(i)))
        {
            flag = false;
            return flag;
        }
    }
    return flag;
}

bool hasChineseChar(const izenelib::util::UString& queryToken)
{
    bool flag = false;
    for (size_t i = 0; i < queryToken.length(); i++)
    {
        if (queryToken.isChineseChar(i))
        {
            flag = true;
            break;
        }
    }
    return flag;
}

}

#endif /* UTIL_CPP_ */
