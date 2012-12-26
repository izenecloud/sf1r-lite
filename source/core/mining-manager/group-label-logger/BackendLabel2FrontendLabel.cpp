#include "BackendLabel2FrontendLabel.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <fstream>
#include <iostream>
#include <map>

namespace sf1r
{
izenelib::am::succinct::ux::Map<UString> BackendLabelToFrontendLabel::back2front_;
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
    std::map<std::string, UString> map;
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
                //UString backend( labels[0], UString::UTF_8);
                std::string backend(labels[0]);
                //std::string frontend(labels[1]);
                UString frontend( labels[1], UString::UTF_8);
                map[backend] = frontend;
            }
        }
    }
    back2front_.build(map);
}

bool BackendLabelToFrontendLabel::Map(const UString&backend,  UString&frontend)
{
    std::string backend_str;
    backend.convertString(backend_str, UString::UTF_8);

    izenelib::am::succinct::ux::id_t retID;
    std::string result;
    retID = back2front_.get(backend_str.c_str(), backend_str.size(), frontend);
    if(retID == 0)
    {
        return true;
    }
    return false;
/*
    std::map<UString, UString>::const_iterator mit = back2front_.find(backend);
    if(mit == back2front_.end()) return false;
    else
    {
        frontend = mit->second;
        return true;
    }
*/
}

bool BackendLabelToFrontendLabel::PrefixMap(const UString&backend,  UString&frontend)
{
    std::string backend_str;
    backend.convertString(backend_str, UString::UTF_8);

    izenelib::am::succinct::ux::id_t retID;
    std::string result;
    size_t retLen;
    retID = back2front_.prefixSearch(backend_str.c_str(), backend_str.size(), retLen, frontend);
    if(retID == 0)
    {
        return true;
    }
    return false;
}

}
