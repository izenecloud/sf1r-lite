#include "brand_manager.h"
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/set.hpp>
using namespace sf1r;
using namespace sf1r::b5m;

BrandManager::BrandManager()
{
}

BrandManager::~BrandManager()
{
}

bool BrandManager::Load(const std::string& path)
{
    if(!boost::filesystem::exists(path)) return false;
    LOG(INFO)<<"loading brand manager at "<<path<<std::endl;
    izenelib::am::ssf::Util<>::Load(path, map_);
    return true;
}

bool BrandManager::Save(const std::string& path)
{
    izenelib::am::ssf::Util<>::Save(path, map_);
    std::cerr<<"OUTPUT Brands"<<std::endl;
    for(Map::const_iterator it = map_.begin();it!=map_.end();++it)
    {
        std::cerr<<it->first<<std::endl;
    }
    return true;
}

void BrandManager::LoadBrandErrorFile(const std::string& file)
{
    std::ifstream ifs(file.c_str());
    std::string line;
    while(getline(ifs, line))
    {
        boost::algorithm::trim(line);
        if(line.empty()) continue;
        if(boost::algorithm::starts_with(line, "^"))
        {
            error_regexps_.push_back(boost::regex(line));
        }
        else
        {
            error_set_.insert(line);
        }
    }
    ifs.close();
}

void BrandManager::Insert(const std::string& category, const std::string& title, const std::string& brand)
{
    std::string lbrand = boost::algorithm::to_lower_copy(brand);
    if(IsErrorBrand(lbrand)) 
    {
        //std::cerr<<"find error brand "<<lbrand<<std::endl;
        return;
    }
    boost::unique_lock<boost::mutex> lock(mutex_);
    map_[lbrand].insert(category);
}
bool BrandManager::IsBrand(const std::string& category, const std::string& brand) const
{
    std::string lbrand = boost::algorithm::to_lower_copy(brand);
    Map::const_iterator it = map_.find(lbrand);
    if(it==map_.end()) return false;
    if(category.empty()) return true;
    const CategorySet& cset = it->second;
    CategorySet::const_iterator cit = cset.find(category);
    if(cit==cset.end()) return false;
    return true;
}

bool BrandManager::IsErrorBrand(const std::string& brand) const
{
    boost::unordered_set<std::string>::const_iterator it = error_set_.find(brand);
    if(it!=error_set_.end()) return true;
    for(uint32_t i=0;i<error_regexps_.size();i++)
    {
        if(boost::regex_match(brand, error_regexps_[i])) return true;
    }
    return false;
}

