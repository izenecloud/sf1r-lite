/**
 * @brief   Defines common variables, methods used in the test
 */

#ifndef _LAM_TEST_DEF_H_
#define _LAM_TEST_DEF_H_

#include <la-manager/LAManager.h>
#include <la-manager/LAPool.h>

#include <util/ustring/UString.h>

#include <vector>
#include <iostream>

//izenelib::util::UString::EncodingType SYSTEM_ENCODING;
//izenelib::util::UString::EncodingType IO_ENCODING;
//izenelib::util::UString::EncodingType SYSTEM_ENCODING = izenelib::util::UString::UTF_8;;
//izenelib::util::UString::EncodingType IO_ENCODING = izenelib::util::UString::UTF_8;

enum ANALYZER { 
    KOREAN_ALL, 
    KOREAN_NOUN,
    KOREAN_LABEL,
#ifdef USE_IZENECMA
    CHINESE_ALL,
    CHINESE_NOUN,
    CHINESE_LABEL,
#endif
    TOKEN, 
    NGRAM, 
    MATRIX };

enum TOKENIZER
{
    NONE,
    UNITE,
    DIVIDE,
    ALLOW
};


std::string getKmaKnowledgePath();
void initConfig( std::vector<sf1r::AnalysisInfo>& analysisInfoLIst, sf1r::LAManagerConfig & config );

void downcase( std::string & str );

void initLAs( sf1r::LAManagerConfig & config );


#endif // _LAM_TEST_DEF_H_

