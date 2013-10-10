#include "TaobaoParser.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    

#include <string.h>

namespace sf1r
{
namespace Recommend
{

int TaobaoParser::LINE_SIZE = 1024;

void TaobaoParser::load(const std::string& path)
{
    if (!boost::filesystem::exists(path))
        return;
    if (in_.is_open())
        in_.close();
    in_.open(path.c_str(), std::ifstream::in);
}

bool TaobaoParser::next()
{
    memset(cLine_, 0, LINE_SIZE);
    return in_.getline(cLine_, LINE_SIZE);
}

void TaobaoParser::lineToUserQuery(const std::string& str, UserQuery& query)
{
    std::string category;
    std::string userQuery;
    std::string freqstr;
    int nTab = 0;
    std::size_t pos = 0;
    while (true)
    {
        std::size_t found = str.find("\t", pos);
        if (std::string::npos == found)
            break; // dismiss ppc.
        else
            nTab++;
        if (nTab <= 3)
        {
            std::string substr= str.substr(pos, found - pos);
            if (0 == substr.size() || "0" == substr)
            {
            }
            else
            {
                category += str.substr(pos, found - pos);
                category += ">";
            }
        }
        else if (nTab == 4)
        {
            userQuery = str.substr(pos, found - pos);
        }
        else if (nTab == 6)
        {
            freqstr = str.substr(pos, found - pos);
            break;
        }
        pos = found + 1;
    }
    query.setUserQuery(userQuery);
    query.setCategory(category);
    pos = freqstr.find("以");
    if (std::string::npos == pos)
    {
        pos = freqstr.find("-");
    }
    
    if (std::string::npos != pos)
        query.setFreq(atoi(freqstr.substr(0, pos).c_str()));
}

const UserQuery& TaobaoParser::get() const
{
    query_->reset();
    lineToUserQuery(cLine_, *query_);
    return *query_;
}

bool TaobaoParser::isValid(const std::string& path)
{ 
    const boost::filesystem::path p(path);
    //std::cout<<boost::filesystem::extension(p)<<"\n";
    return ".log" == boost::filesystem::extension(p);
}

}
}
