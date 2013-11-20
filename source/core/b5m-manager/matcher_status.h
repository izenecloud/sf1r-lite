#ifndef SF1R_B5MMANAGER_MATCHERSTATUS_H_
#define SF1R_B5MMANAGER_MATCHERSTATUS_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <3rdparty/json/json.h>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>

NS_SF1R_B5M_BEGIN

class MatcherStatus
{
public:
    struct Status
    {
        std::vector<uint32_t> value;
        std::vector<Document> docs;
        std::map<std::string, uint32_t> why_map;
        void Increase(int type, uint32_t v=1)
        {
            if((std::size_t)type>=value.size())
            {
                value.resize(type+1, 0);
            }
            value[type]+=v;
        }
        void Increase(int type, const Document& doc, uint32_t v=1)
        {
            Increase(type, v);
            docs.push_back(doc);
        }
        void IncreaseWhy(const std::string& why, uint32_t v=1)
        {
            std::map<std::string, uint32_t>::iterator it = why_map.find(why);
            if(it==why_map.end()) why_map.insert(std::make_pair(why, v));
            else it->second+=v;
        }
    };
    struct CategoryGroup
    {
        boost::regex regexp;
        std::string group_name;
        double prob;
    };
    typedef std::pair<boost::regex, std::string> Category2Group;
    typedef boost::unordered_map<std::string, Status> Map;

    void AddCategoryGroup(const std::string& category_regexp, const std::string& group_name, double prob=0.0)
    {
        CategoryGroup cg;
        cg.regexp = boost::regex(category_regexp);
        cg.group_name = group_name;
        cg.prob = prob;
        category_groups_.push_back(cg);
    }

    bool Insert(const Document& doc, const Product& product)
    {
        std::string category;
        doc.getString("Category", category);
        std::string group_name;
        double prob = 0.0;
        for(std::size_t i=0;i<category_groups_.size();i++)
        {
            const CategoryGroup& cg = category_groups_[i];
            if(boost::regex_match(category, cg.regexp))
            {
                group_name = cg.group_name;
                prob = cg.prob;
                break;
            }
        }
        boost::unique_lock<boost::mutex> lock(mutex_);
        map_["__all__"].Increase((int)(product.type), 1);
        if(!product.why.empty())
        {
            map_["__all__"].IncreaseWhy(product.why, 1);
        }
        if(group_name.empty()) return false;
        if(!product.why.empty())
        {
            map_[group_name].IncreaseWhy(product.why, 1);
        }
        if(prob>=1.0)
        {
            map_[group_name].Increase((int)(product.type), doc, 1);
        }
        else if(prob>0.0)
        {
            static boost::mt19937 gen;
            boost::uniform_real<> dist(0.0, 1.0);
            double value = dist(gen);
            if(value<prob)
            {
                map_[group_name].Increase((int)(product.type), doc, 1);
            }
            else
            {
                map_[group_name].Increase((int)(product.type), 1);
            }
        }
        else
        {
            map_[group_name].Increase((int)(product.type), 1);
        }
        return true;
    }

    void Flush(const std::string& path) const
    {
        std::ofstream ofs(path.c_str());
        Json::Value json(Json::arrayValue);
        json.resize(map_.size());
        std::size_t i=0;
        for(Map::const_iterator it=map_.begin();it!=map_.end();++it)
        {
            const Status& status = it->second;
            Json::Value& json2 = json[i++];
            json2["name"] = it->first;
            Json::Value count_json(Json::arrayValue);
            count_json.resize(status.value.size());
            for(std::size_t j=0;j<status.value.size();j++)
            {
                count_json[j] = Json::Value::UInt(status.value[j]);
            }
            json2["count"] = count_json;
            Json::Value docs_json;
            JsonDocument::ToJson(status.docs, docs_json);
            json2["docs"] = docs_json;
            Json::Value why_json;
            for(std::map<std::string, uint32_t>::const_iterator it2 = status.why_map.begin(); it2!=status.why_map.end(); it2++)
            {
                why_json[it2->first] = Json::Value::UInt(it2->second);
            }
            json2["why"] = why_json;
            
            //ofs<<it->first;
            //for(std::size_t i=0;i<status.value.size();i++)
            //{
            //    ofs<<","<<status.value[i];
            //}
            //ofs<<std::endl;
        }
        Json::FastWriter writer;
        std::string str_value = writer.write(json);
        ofs<<str_value<<std::endl;
        ofs.close();
    }

private:
    std::vector<CategoryGroup> category_groups_;
    Map map_;
    boost::mutex mutex_;

};

NS_SF1R_B5M_END

#endif

