/*
 * Util.h
 *
 *  Created on: Nov 3, 2010
 *      Author: wps
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <util/ustring/UString.h>

namespace sf1r
{

//Check if one word is pure english word
bool isEnglishWord(const izenelib::util::UString& queryToken);

//Check if one word is pure korean
bool isKoreanWord(const izenelib::util::UString& queryToken) ;

//Check if one word is pure Chinese
bool hasChineseChar(const izenelib::util::UString& queryToken);

}

#endif /* UTIL_H_ */
