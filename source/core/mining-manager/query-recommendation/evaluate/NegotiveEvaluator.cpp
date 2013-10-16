#include "NegotiveEvaluator.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    

#include <string.h>

namespace sf1r
{
namespace Recommend
{

const int NegotiveEvaluator::LINE_SIZE = 512;

void NegotiveEvaluator::load(const std::string& path)
{
    if (!boost::filesystem::exists(path))
        return;
    if (in_.is_open())
        in_.close();
    in_.open(path.c_str(), std::ifstream::in);
}

bool NegotiveEvaluator::next()
{
    memset(cLine_, 0, LINE_SIZE);
    return in_.getline(cLine_, LINE_SIZE);
}


const EvaluateItem& NegotiveEvaluator::get() const
{
    std::string sLine(cLine_);
    boost::trim(sLine);
    item_->userQuery_ = sLine;
    return *item_;
}

bool NegotiveEvaluator::isValid(const std::string& path)
{ 
    const boost::filesystem::path p(path);
    //std::cout<<boost::filesystem::extension(p)<<"\n";
    return ".neg" == boost::filesystem::extension(p);
}

}
}
