#include "PositiveEvaluator.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    

#include <string.h>

namespace sf1r
{
namespace Recommend
{

const int PositiveEvaluator::LINE_SIZE = 512;

void PositiveEvaluator::load(const std::string& path)
{
    if (!boost::filesystem::exists(path))
        return;
    if (in_.is_open())
        in_.close();
    in_.open(path.c_str(), std::ifstream::in);
}

bool PositiveEvaluator::next()
{
    memset(cLine_, 0, LINE_SIZE);
    return in_.getline(cLine_, LINE_SIZE);
}


const EvaluateItem& PositiveEvaluator::get() const
{
    std::string sLine(cLine_);
    std::size_t pos = sLine.find("\t");
    if (std::string::npos != pos)
    {
        item_->userQuery_ = sLine.substr(0, pos);
        item_->correct_ = sLine.substr(pos + 1);
    }
    return *item_;
}

bool PositiveEvaluator::isValid(const std::string& path)
{ 
    const boost::filesystem::path p(path);
    //std::cout<<boost::filesystem::extension(p)<<"\n";
    return ".pos" == boost::filesystem::extension(p);
}

}
}
