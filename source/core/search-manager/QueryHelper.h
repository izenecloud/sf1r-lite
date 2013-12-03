#ifndef QUERY_MANAGER_QUERY_HELPER_H
#define QUERY_MANAGER_QUERY_HELPER_H

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include <util/ustring/UString.h>
#include <string>
#include <vector>

namespace sf1r
{
    void assembleConjunction(std::vector<izenelib::util::UString> keywords, std::string& result)
    {
        result.clear();
        int size = keywords.size();
        for(int i = 0; i < size; ++i)
        {
            std::string str;
            keywords[i].convertString(str, izenelib::util::UString::UTF_8);
            result += str;
            result += " ";
        }
    }

    void assembleDisjunction(std::vector<izenelib::util::UString> keywords, std::string& result)
    {
        result.clear();
        int size = keywords.size();
        for(int i = 0; i < size; ++i)
        {
            std::string str;
            keywords[i].convertString(str, izenelib::util::UString::UTF_8);
            result += str;
            result += "|";
        }
        boost::trim_right_if(result, boost::is_any_of("|"));
    }
    
}
#endif // QUERY_MANAGER_QUERY_HELPER_H