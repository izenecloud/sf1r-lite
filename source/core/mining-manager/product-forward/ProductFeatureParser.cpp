#include "ProductFeatureParser.h"
#include <la-manager/TitlePCAWrapper.h>
#include <util/hashFunction.h>
#include <sstream>
#include <algorithm>

using namespace sf1r;

namespace
{
typedef std::pair<std::string, float> TokenScore;

bool compareTokenScore(const TokenScore& x, const TokenScore& y)
{
    return x.second > y.second;
}
}

void ProductFeatureParser::getFeatureIds(
    const std::string& source,
    uint32_t& brand,
    uint32_t& model,
    std::vector<uint32_t>& featureIds)
{
    std::vector<std::pair<std::string, float> > sub_tokens;
    std::vector<std::pair<std::string, float> > r;
    std::string b;
    std::string m;

    TitlePCAWrapper* title_pca = TitlePCAWrapper::get();
    title_pca->pca(source, r, b, m, sub_tokens, 0);

    if (!b.empty())
        brand = izenelib::util::HashFunction<std::string>::generateHash32(b);
    else
        brand = 0;
    if (!m.empty())
        model = izenelib::util::HashFunction<std::string>::generateHash32(m);
    else
        model = 0;

    //remove duplicate
    if (r.empty())
        return;

    std::sort(r.begin(), r.end(), compareTokenScore);

    std::vector<std::pair<std::string, float> > dup_r;
    std::set<std::string> tmpset;
    double sum = 0;
    for (size_t i = 0; i < r.size(); ++i)if(r[i].first != b && r[i].first != m && tmpset.find(r[i].first)==tmpset.end())
    {
        sum += r[i].second;
        dup_r.push_back(r[i]);
        tmpset.insert(r[i].first);
    }
    for (size_t i = 0; i < dup_r.size(); ++i)dup_r[i].second /= sum;
    featureIds.reserve(5);
    for (size_t i = 0; i < dup_r.size(); ++i)if(dup_r[i].second >= 0.06)
    {
        uint32_t tmp = izenelib::util::HashFunction<std::string>::generateHash32(dup_r[i].first);
        featureIds.push_back(tmp);
        //std::cout<<dup_r[i].first<<":"<<dup_r[i].second<<std::endl;
    }
}

void ProductFeatureParser::getFeatureStr(
    const std::string& source,
    std::string& featureStr)
{
    uint32_t brand, model;
    std::vector<uint32_t> ids;

    getFeatureIds(source, brand, model, ids);

    std::ostringstream oss;
    oss << brand << ' ' << model << ' ';

    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        oss << ids[i];
        if (i < ids.size() - 1)
            oss << ' ';
    }

    featureStr = oss.str();
}

void ProductFeatureParser::convertStrToIds(
    const std::string& featureStr,
    uint32_t& brand,
    uint32_t& model,
    std::vector<uint32_t>& featureIds)
{
    std::istringstream iss(featureStr);

    iss >> brand >> model;

    uint32_t id = 0;
    while (iss >> id)
    {
        featureIds.push_back(id);
    }
}
