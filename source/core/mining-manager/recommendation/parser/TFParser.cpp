#include "TFParser.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    

#include <string.h>

namespace sf1r
{
namespace Recommend
{

int TFParser::LINE_SIZE = 1024;

void TFParser::load(const std::string& path)
{
    if (!boost::filesystem::exists(path))
        return;
    if (in_.is_open())
        in_.close();
    in_.open(path.c_str(), std::ifstream::in);
}

bool TFParser::next()
{
    memset(cLine_, 0, LINE_SIZE);
    return in_.getline(cLine_, LINE_SIZE);
}

void TFParser::lineToUserQuery(const std::string& str, UserQuery& query)
{
    std::string userQuery;
    std::size_t found = str.find("\t");
    if (std::string::npos == found)
        return;

    userQuery = str.substr(0, found);
    query.setUserQuery(userQuery);
    query.setFreq(atof(str.substr(found + 1).c_str()));
}

const UserQuery& TFParser::get() const
{
    query_->reset();
    lineToUserQuery(cLine_, *query_);
    return *query_;
}

bool TFParser::isValid(const std::string& path)
{ 
    const boost::filesystem::path p(path);
    //std::cout<<boost::filesystem::extension(p)<<"\n";
    return ".tf" == boost::filesystem::extension(p);
}

}
}
