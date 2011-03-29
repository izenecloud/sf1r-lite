/*
 * Util.h
 *
 *  Created on: Nov 3, 2010
 *      Author: wps
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <util/ustring/UString.h>
#include <util/ustring/ustr_tool.h>


namespace sf1r {

//Check if one word is pure english word
bool isEnglishWord(const UString& queryToken);

//Check if one word is pure korean
bool isKoreanWord(const UString& queryToken) ;

//Check if one word is pure Chinese
bool hasChineseChar(const UString& queryToken);

}

#endif /* UTIL_H_ */
