/*
 * QueryRecommendSubmanager.cpp
 *
 *  Created on: 2009-6-10
 *      Author: jinglei
 *  Modified on: 2009-06-15
 *       Author: Liang
 *  Refined on: 2009-11-05
 *       Author: Jia
 *  Refined on: 2009-11-24
 *       Author: Jinglei
 *
 */

#include "QueryRecommendSubmanager.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/timer.hpp>
using namespace sf1r;

QueryRecommendSubmanager::QueryRecommendSubmanager(const boost::shared_ptr<RecommendManager>& rmDb, const std::string& inject_file)
:rmDb_(rmDb), inject_file_(inject_file), has_new_inject_(false)
{

}

QueryRecommendSubmanager::~QueryRecommendSubmanager()
{

}

bool QueryRecommendSubmanager::Load()
{
    if(!boost::filesystem::exists(inject_file_)) return true;
    std::vector<izenelib::util::UString> str_list;
    std::ifstream ifs(inject_file_.c_str());
    std::string line;
    while( getline(ifs, line) )
    {
        boost::algorithm::trim(line);
        if(line.length()==0)
        {
            //do with str_list;
            if(str_list.size()>=1)
            {
                izenelib::util::UString query = str_list[0];
                std::string str_query;
                query.convertString(str_query, izenelib::util::UString::UTF_8);
                std::vector<izenelib::util::UString> result_list(str_list.begin()+1, str_list.end());
                inject_data_.insert(std::make_pair(str_query, result_list));
            }
            str_list.resize(0);
            continue;
        }
        str_list.push_back( izenelib::util::UString(line, izenelib::util::UString::UTF_8));
    }
    ifs.close();

    return true;
}
    
void QueryRecommendSubmanager::Inject(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    std::string str_query;
    query.convertString(str_query, izenelib::util::UString::UTF_8);
    boost::algorithm::trim(str_query);
    if(str_query.empty()) return;
    boost::algorithm::to_lower(str_query);
    std::cout<<"Inject query recommend : "<<str_query<<std::endl;
    std::string str_result;
    result.convertString(str_result, izenelib::util::UString::UTF_8);
    std::vector<std::string> vec_result;
    boost::algorithm::split( vec_result, str_result, boost::algorithm::is_any_of("|") );
    std::vector<izenelib::util::UString> vec_ustr;
    for(uint32_t i=0;i<vec_result.size();i++)
    {
        if(vec_result[i].length()>0)
        {
            vec_ustr.push_back( izenelib::util::UString( vec_result[i], izenelib::util::UString::UTF_8) );
        }
    }
    inject_data_.erase( str_query );
    inject_data_.insert(std::make_pair( str_query, vec_ustr) );
    has_new_inject_ = true;
}

void QueryRecommendSubmanager::FinishInject()
{
    if(!has_new_inject_) return;
    
    if(boost::filesystem::exists( inject_file_) )
    {
        boost::filesystem::remove_all( inject_file_);
    }
    std::ofstream ofs(inject_file_.c_str());
    boost::unordered_map<std::string, std::vector<izenelib::util::UString> >::iterator it = inject_data_.begin();
    while(it!= inject_data_.end())
    {
        ofs<<it->first<<std::endl;
        for(uint32_t i=0;i<it->second.size();i++)
        {
            std::string result;
            it->second[i].convertString(result, izenelib::util::UString::UTF_8);
            ofs<<result<<std::endl;
        }
        ofs<<std::endl;
        ++it;
    }
    ofs.close();
    has_new_inject_ = false;
    std::cout<<"Finish inject query recommend."<<std::endl;
}


void QueryRecommendSubmanager::getRecommendQuery(const izenelib::util::UString& query,
        const std::vector<docid_t>& topDocIdList, unsigned int maxNum,
        QueryRecommendRep& recommendRep)
{
    std::string str_query;
    query.convertString(str_query, izenelib::util::UString::UTF_8);
    boost::algorithm::to_lower(str_query);
    boost::unordered_map<std::string, std::vector<izenelib::util::UString> >::iterator it = inject_data_.find(str_query);
    if(it!=inject_data_.end())
    {
        recommendRep.recommendQueries_.assign(it->second.begin(), it->second.end());
    }
    else
    {
        rmDb_->getRelatedConcepts(query, maxNum, recommendRep.recommendQueries_);
    }
    recommendRep.scores_.resize(recommendRep.recommendQueries_.size());
    for (uint32_t i=0;i<recommendRep.scores_.size();i++)
    {
        recommendRep.scores_[i] = 1.0;
    }

}

