#include "QueryNormalizer.h"
#include <util/ustring/UString.h>
#include <cctype> // isspace, tolower
#include <algorithm> // sort

using namespace sf1r;
using izenelib::util::UString;

namespace
{
enum CharType 
{
    DefultChar = 0,
    AlphaChar,
    ChineseChar,
    SpaceChar
};

struct AppleType
{
    UString majorQuery;
    std::vector<std::string> types;

    AppleType(const UString& majorQuery, const std::vector<std::string>& types)
    {
        this->majorQuery = majorQuery;
        this->types = types;
    } 
};

typedef UString::const_iterator UStrIter;

const unsigned int PRODUCT_MODEL_SIZE = 3;
const UString::EncodingType kEncodeType = UString::UTF_8;

/*
 * To make such strings "5.5", "NO.1" as one character,
 * the full stop character is seemed as alphabet type.
 */
const UString::CharT kFullStop = UString(".", kEncodeType)[0];

/*
 * In UString::isThisSpaceChar(), it doesn't recognize the carriage return
 * character "\r" as space type, so we have to check it by ourselves.
 */
const UString::CharT kCarriageReturn = UString("\r", kEncodeType)[0];

const char kMergeTokenDelimiter = ' ';

void mergeTokens(const std::vector<std::string>& tokens, std::string& output)
{
    output.clear();
    const int lastPos = static_cast<int>(tokens.size()) - 1;

    for (int i = 0; i < lastPos; ++i)
    {
        output += tokens[i];
        output += kMergeTokenDelimiter;
    }

    if (lastPos >= 0)
    {
        output += tokens[lastPos];
    }
}

} // namespace

void QueryNormalizer::tokenizeQuery(const std::string& query, std::vector<std::string>& tokens)
{
    std::string current;
    const std::size_t length = query.size();

    for (std::size_t i = 0; i < length; ++i)
    {
        const char ch = query[i];

        if (!std::isspace(ch))
        {
            current += std::tolower(ch);
        }
        else if (!current.empty())
        {
            tokens.push_back(current);
            current.clear();
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }
}

void QueryNormalizer::spaceQuery(const std::string& query, std::string& queryOut)// here is major , and get minor...
{
    UString ustr(query, kEncodeType);
    const UString ubadPunctuation("/+(){}［］[]:：,!;", kEncodeType);

    UString spaceUstr(" ", kEncodeType);
    const UStrIter begIt = spaceUstr.begin();
    UString newustring;
    const UStrIter endIt = ustr.end();

    CharType type = DefultChar;

    for (UStrIter it = ustr.begin(); it != endIt; ++it)
    {
        if (UString::isThisAlphaChar(*it) || 
            UString::isThisNumericChar(*it) || 
            (UString::isThisPunctuationChar(*it) && ubadPunctuation.find(*it, 0) == UString::NOT_FOUND)
            )
        {
            if (type != AlphaChar)
                newustring += *begIt;
            newustring += *it;
            type = AlphaChar;
        }
        else if (UString::isThisChineseChar(*it))
        {
            if (type != ChineseChar)
                newustring += *begIt;
            newustring += *it;
            type = ChineseChar;
        }
        else 
        {
            type = SpaceChar;
        }
    }
    newustring.convertString(queryOut, izenelib::util::UString::UTF_8);
}

void QueryNormalizer::normalize(const std::string& fromStr, std::string& toStr)
{
    std::vector<std::string> tokens;

    tokenizeQuery(fromStr, tokens);

    std::sort(tokens.begin(), tokens.end());

    mergeTokens(tokens, toStr);
}

std::size_t QueryNormalizer::countCharNum(const std::string& query)
{
    std::size_t count = 0;
    bool isAlphaNumChar = false;

    UString ustr(query, kEncodeType);
    const UStrIter endIt = ustr.end();

    for (UStrIter it = ustr.begin(); it != endIt; ++it)
    {
        if (UString::isThisAlphaChar(*it) ||
            UString::isThisNumericChar(*it) ||
            *it == kFullStop)
        {
            if (isAlphaNumChar == false)
            {
                ++count;
                isAlphaNumChar = true;
            }
            continue;
        }

        if (isAlphaNumChar)
        {
            isAlphaNumChar = false;
        }

        if (UString::isThisPunctuationChar(*it) ||
            UString::isThisSpaceChar(*it) ||
            *it == kCarriageReturn)
        {
            continue;
        }

        ++count;
    }

    return count;
}


bool QueryNormalizer::isProductType(const std::string &str)
{
    UString ustr(str, kEncodeType);

    const UStrIter endIt = ustr.end();
    bool existNumber = false;
    bool existAlpha = false;
    bool existPunctuation = false;
    bool isTypeStr = false;
    for (UStrIter it = ustr.begin(); it != endIt; ++it)
    {
        if (UString::isThisAlphaChar(*it))
        {
            existAlpha = true;
        }
        else if (UString::isThisNumericChar(*it))
        {
            existNumber = true;
        }
        else if (UString::isThisPunctuationChar(*it) )
        {
            existPunctuation = true;
        }
    }

    if ( (existAlpha && existNumber) ||
        (existNumber && existPunctuation) )
        isTypeStr = true;

    return isTypeStr;
}

void QueryNormalizer::getProductTypes(const std::string & query, std::vector<std::string>& productTypes,
                                std::string& minorQuery)
{
    std::cout << "DO GET PRODUCT MODEL..." << std::endl;
    std::vector<std::string> tokens;

    std::string majorQuery;
    std::string minorQuery_p;
    std::string minorQuery_k;
    std::string clearQuery;
    std::string queryout;
    std::string finalMajorQuery;

    clearUselessWord(query, clearQuery);
    ///cout << clearQuery << endl;
    getMajorQueryByPunctuation(clearQuery, majorQuery, minorQuery_p);
    ///std::cout << "majorQuery" << majorQuery << std::endl;
    spaceQuery(majorQuery, queryout);
    ///std::cout << "queryout" <<  queryout<< std::endl;
    majorQuery.clear();
    getMajorQueryByKeyWord(queryout, majorQuery, minorQuery_k);
    ///std::cout << "majorQuery:" << majorQuery << std::endl;
    getProductTypesForApple(majorQuery, finalMajorQuery, productTypes);
    ///std::cout << "finalMajorQuery" <<  finalMajorQuery<< std::endl;

    tokenizeQuery(majorQuery, tokens);

    for (std::vector<std::string>::iterator i = tokens.begin(); i != tokens.end(); ++i)
    {
        if (isProductType(*i) && i->size() > PRODUCT_MODEL_SIZE)
            productTypes.push_back(*i);
        /*else
        {
            newquery += *i;
            newquery += " ";
        }*/
    }
    std::sort(productTypes.begin(), productTypes.end());

    if (!minorQuery_k.empty() || !minorQuery_p.empty())
    {
        minorQuery += minorQuery_k;
        minorQuery += " ";
        minorQuery += minorQuery_p;
    }
}

void QueryNormalizer::getMajorQueryByPunctuation(const std::string& query, std::string& majorQuery, std::string& minorQuery)
{
    const UString ubadPunctuation("(){}［］[]（）【】", kEncodeType);

    UString ustr(query, kEncodeType);
    UString umajorString("", kEncodeType);
    UString uminorString("", kEncodeType);
    UString spaceUstr(" ", kEncodeType);
    const UStrIter endIt = ustr.end();
    bool ismajor = true;
    
    for (UStrIter it = ustr.begin(); it != endIt; ++it)
    {
        if (ubadPunctuation.find(*it, 0) == UString::NOT_FOUND)
        {
            if (ismajor)
                umajorString += *it;
            else
                uminorString += *it;
        }
        else 
        {
            if (ubadPunctuation.find(*it, 0)%2 == 0)
            {
                ismajor = false;
                umajorString += spaceUstr;
            }
            else
            {
                ismajor = true;
                uminorString += spaceUstr;
            }
        }

    }

    umajorString.convertString(majorQuery, kEncodeType);
    uminorString.convertString(minorQuery, kEncodeType);
}

void QueryNormalizer::getMajorQueryByKeyWord(const std::string& query, std::string& majorQuery, std::string& minorQuery)
{
    unsigned int maxMinorSize = 5;
    UString uquery(query, kEncodeType);
    UString ukeyword("送", kEncodeType);
    UString spaceQuery(" ", kEncodeType);
    UString umajorString("", kEncodeType);
    UString uminorString("", kEncodeType);

    // to the end OR to the space OR to the maxMinorSize;
    for (UStrIter i = uquery.begin(); i != uquery.end(); ++i)
    {
        if (*ukeyword.begin() == *i)
        {
            for (unsigned int j = 0; j < maxMinorSize; ++j)
            {
                ++i;
                if (*i != *spaceQuery.begin() && i != uquery.end())
                {
                    uminorString += *i;
                }
                else
                    break;
            }
        }
        else
            umajorString += *i;

        if (i == uquery.end())
            break;
    }
    umajorString.convertString(majorQuery, kEncodeType);
    uminorString.convertString(minorQuery, kEncodeType);
}

void QueryNormalizer::clearUselessWord(const std::string& query, std::string& newquery)
{
    std::set<UString> uminorKeyWords;
    uminorKeyWords.insert(UString("正品", kEncodeType));
    uminorKeyWords.insert(UString("包邮", kEncodeType));
    uminorKeyWords.insert(UString("特价", kEncodeType));
    uminorKeyWords.insert(UString("行货", kEncodeType));
    uminorKeyWords.insert(UString("国行", kEncodeType));
    uminorKeyWords.insert(UString("美版", kEncodeType));
    uminorKeyWords.insert(UString("港版", kEncodeType));
    uminorKeyWords.insert(UString("安卓", kEncodeType));
    uminorKeyWords.insert(UString("皇冠", kEncodeType));
    uminorKeyWords.insert(UString("原装", kEncodeType));
    uminorKeyWords.insert(UString("未开封", kEncodeType));
    uminorKeyWords.insert(UString("未激活", kEncodeType));
    uminorKeyWords.insert(UString("四代", kEncodeType));
    uminorKeyWords.insert(UString("五代", kEncodeType));
    uminorKeyWords.insert(UString("正品", kEncodeType));
    uminorKeyWords.insert(UString("店庆", kEncodeType));
    uminorKeyWords.insert(UString("大促", kEncodeType));
    uminorKeyWords.insert(UString("促销", kEncodeType));
    uminorKeyWords.insert(UString("现货", kEncodeType));

    UString uquery(query, kEncodeType);
    UString utempQuery("", kEncodeType);
    UString uNewQuery("", kEncodeType);

    UStrIter tmp;
    bool isFind = false;

    for (UStrIter i = uquery.begin(); i != uquery.end(); ++i)
    {
        isFind = false;
        tmp = i;
        utempQuery.clear();
        utempQuery += *i;
        for (unsigned int j = 0; j < 2; j++)
        {
            ++i;
            if ( i == uquery.end())
                break;
            utempQuery += *i;
            if (uminorKeyWords.find(utempQuery) != uminorKeyWords.end())
            {
                isFind = true;
                break;
            }
        }

        if (!isFind)
        {
            uNewQuery += *tmp;
            i = tmp;
        }
    }
    uNewQuery.convertString(newquery, kEncodeType);
}

void QueryNormalizer::getProductTypesForApple(const std::string& query, std::string& newquery, std::vector<std::string>& productTypes)
{
    UString uquery(query, kEncodeType);
    UString uNewQuery("", kEncodeType);

    if (uquery.length() > 256)
    {
        newquery = query;
        return;
    }

    std::vector<AppleType> umajorAppleType;
    std::vector<std::string> types;

    //types.push_back("iphone");
    types.push_back("4");
    umajorAppleType.push_back(AppleType(UString("iphone 4 ", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("iphone4 ", kEncodeType),types));
    
    types.clear();
    //types.push_back("iphone");
    types.push_back("4s");
    umajorAppleType.push_back(AppleType(UString("iphone 4s", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("iphone4s", kEncodeType),types));
    
    types.clear();
    //types.push_back("iphone");
    types.push_back("5");
    umajorAppleType.push_back(AppleType(UString("iphone 5", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("iphone5", kEncodeType),types));

    //ipad
    types.clear();
    //types.push_back("ipad");
    types.push_back("2");
    umajorAppleType.push_back(AppleType(UString("ipad2", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("ipad 2", kEncodeType),types));

    types.clear();
    //types.push_back("ipad");
    types.push_back("3");
    umajorAppleType.push_back(AppleType(UString("ipad3", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("ipad 3", kEncodeType),types));

    types.clear();
    //types.push_back("ipad");
    types.push_back("4");
    umajorAppleType.push_back(AppleType(UString("ipad4", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("ipad 4", kEncodeType),types));

    types.clear();
    //types.push_back("ipad");
    types.push_back("mini");
    umajorAppleType.push_back(AppleType(UString("ipadmini", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("ipad mini", kEncodeType),types));

    //itouch
    types.clear();
    //types.push_back("itouch");
    types.push_back("4");
    umajorAppleType.push_back(AppleType(UString("itouch4", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("itouch 4", kEncodeType),types));


    types.clear();
    //types.push_back("itouch");
    types.push_back("5");
    umajorAppleType.push_back(AppleType(UString("itouch5", kEncodeType),types));
    umajorAppleType.push_back(AppleType(UString("itouch 5", kEncodeType),types));

    //nokia
    types.clear();
    //types.push_back("itouch");
    types.push_back("720");
    umajorAppleType.push_back(AppleType(UString("lumia 720", kEncodeType),types));
    
    types.clear();
    types.push_back("520");
    umajorAppleType.push_back(AppleType(UString("lumia 520", kEncodeType),types));
    
    types.clear();
    types.push_back("920");
    umajorAppleType.push_back(AppleType(UString("lumia 920", kEncodeType),types));
    
    types.clear();
    types.push_back("800");
    umajorAppleType.push_back(AppleType(UString("lumia 800", kEncodeType),types));
    
    types.clear();
    types.push_back("900");
    umajorAppleType.push_back(AppleType(UString("lumia 900", kEncodeType),types));
    
    types.clear();
    types.push_back("925");
    umajorAppleType.push_back(AppleType(UString("lumia 925", kEncodeType),types));


    unsigned char bitmap[32];
    for (int i = 0; i < 32; ++i)
        bitmap[i] = 0;

    for (std::vector<AppleType>::iterator i = umajorAppleType.begin(); i != umajorAppleType.end(); ++i)
    {
        unsigned int hitpos = 0;
        if ((hitpos = uquery.find(i->majorQuery, 0)) != UString::NOT_FOUND)
        {
            for (std::vector<std::string>::iterator j = i->types.begin(); j != i->types.end(); ++j)
                productTypes.push_back(*j);

            for (unsigned int k = hitpos; k < hitpos + (i->majorQuery).length(); ++k)
                bitmap[k>>3] |= 0x80 >> (k&7);
        }
    }

    unsigned int length = 0;
    for (UStrIter i = uquery.begin(); i != uquery.end(); i++)
    {
        if ((bitmap[length>>3] & (0x80 >>(length&7))) == 0)
            uNewQuery += *i;
        length++;
    }

    uNewQuery.convertString(newquery, kEncodeType);
}