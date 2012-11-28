#include "BackendLabel2FrontendLabel.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <fstream>
#include <iostream>

namespace sf1r
{
std::map<UString, UString> BackendLabelToFrontendLabel::back2front_;
BackendLabelToFrontendLabel::BackendLabelToFrontendLabel()
{
}

BackendLabelToFrontendLabel::~BackendLabelToFrontendLabel()
{
}

void BackendLabelToFrontendLabel::Init(const std::string& path)
{
    static boost::once_flag once = BOOST_ONCE_INIT;
    boost::call_once(once, boost::bind(&BackendLabelToFrontendLabel::InitOnce_, this, path));
}

void BackendLabelToFrontendLabel::InitOnce_(const std::string& path)
{
    std::ifstream in;
    in.open(path.c_str(), ios::in);
    if(in.is_open())
    {
        while(!in.eof())
        {
            std::string line;
            std::getline(in, line);
            std::vector<std::string> labels;
            boost::algorithm::split( labels, line, boost::algorithm::is_any_of(",") );
            if(2 == labels.size())
            {
                std::cout<<labels[0]<<","<<labels[1]<<std::endl;
                UString backend( labels[0], UString::UTF_8);
                UString frontend( labels[0], UString::UTF_8);
                back2front_[backend] = frontend;
            }
        }
    }
}

bool BackendLabelToFrontendLabel::Map(const izenelib::util::UString&backend,  izenelib::util::UString&frontend)
{
    std::map<UString, UString>::const_iterator mit = back2front_.find(backend);
    if(mit == back2front_.end()) return false;
    else
    {
        frontend = mit->second;
        return true;
    }
}

}
