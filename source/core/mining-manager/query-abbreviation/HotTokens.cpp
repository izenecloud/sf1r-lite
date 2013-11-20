#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "HotTokens.h"
#include "HotTokenParser.h"

namespace sf1r
{
namespace QA
{
static const std::string uuid = "HotTokens";

HotTokens::HotTokens(const std::string& workdir)
    : workdir_(workdir)
{
    
}
    
bool HotTokens::isNeedBuild(const std::string& path)
{
    if (!boost::filesystem::exists(path) || !boost::filesystem::is_regular_file(path))
        return false;
    
    std::string serialize = workdir_ + "/" + uuid;
    if ((!boost::filesystem::exists(serialize)))
        return true;

    return boost::filesystem::last_write_time(path) > boost::filesystem::last_write_time(serialize);
}

void HotTokens::build(const std::string& path)
{
    if (!boost::filesystem::exists(path) || !boost::filesystem::is_regular_file(path))
        return ;
    if (!HotTokenParser::isValid(path))
        return;

    HotTokenParser* parser = new HotTokenParser();
    parser->load(path);
    while(parser->next())
    {
        const HotToken& hot  = parser->get();
        TokenContainer::iterator it = hotTokens_.find(hot.token());
        if (it != hotTokens_.end())
            continue;
        std::string t = hot.token();
        hotTokens_[t] = hot.factor();
    }
    delete parser;

    save();
}

void HotTokens::save() const
{
    std::string serialize = workdir_ + "/" + uuid;
    std::ofstream ofs(serialize.c_str(), std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    oa << hotTokens_;
}

const uint32_t HotTokens::search(const std::string& key)
{
    TokenContainer::const_iterator it = hotTokens_.find(key);
    if (hotTokens_.end() == it)
    {
        return std::numeric_limits<uint32_t>::max();
    }
    return it->second;
}

void HotTokens::load()
{
    std::string serialize = workdir_ + "/" + uuid;
    if (boost::filesystem::exists(serialize))
    {
        std::ifstream ifs(serialize.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> hotTokens_;
    }
}

void HotTokens::clear()
{
    hotTokens_.clear();
}

}
}
