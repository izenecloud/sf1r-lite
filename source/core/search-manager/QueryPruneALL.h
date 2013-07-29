/**
 * @file sf1r/search-manager/QueryPruneALL.h
 * @author Hongliang Zhao
 * @date Created <2013-05-23>
 * @brief include all query Prune algorithms
 */

#ifndef QUERY_PRUNE_ALL_H
#define QUERY_PRUNE_ALL_H 

#include "QueryPruneBase.h"
#include "QueryHelper.h"
#include <mining-manager/MiningManager.h>
#include "mining-manager/suffix-match-manager/SuffixMatchManager.hpp"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <list>
#include <algorithm>

#include <util/ustring/UString.h>

namespace  sf1r
{
using izenelib::ir::idmanager::IDManager;

bool greater_pair(const std::pair<std::string, double>& obj1, const std::pair<std::string, double>& obj2)
{
    return obj1.second > obj2.second;
}

class AddQueryPrune : public QueryPruneBase
{
public:
    AddQueryPrune(boost::shared_ptr<MiningManager>& miningManager)
        : miningManager_(miningManager)
    {
        //setTermReader();
    }

    ~AddQueryPrune()
    {
    }

    /// care about muti thread....
    bool queryPrune(std::string& query_orig,
            std::vector<izenelib::util::UString> keywords,
            std::string& newQuery)
    {
        cout<<"====================Query Prune begin========================"<<endl;
        std::string pattern = query_orig;

        std::list<std::pair<UString, double> > major_tokens;
        std::list<std::pair<UString, double> > minor_tokens;
        izenelib::util::UString analyzedQuery;
        double rank_boundary = 0;
        miningManager_->getSuffixManager()->GetTokenResults(pattern, major_tokens, minor_tokens, analyzedQuery, rank_boundary);

        std::vector<std::pair<std::string, double> > v_major;
        std::vector<std::pair<std::string, double> > v_minor;
        for (std::list<std::pair<UString, double> >::iterator i = major_tokens.begin(); i != major_tokens.end(); ++i)
        {
            std::string key;
            (i->first).convertString(key, izenelib::util::UString::UTF_8);
            v_major.push_back(make_pair(key, i->second));
            cout << key << " " << i->second << endl;
        }
        for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin(); i != minor_tokens.end(); ++i)
        {
            std::string key;
            (i->first).convertString(key, izenelib::util::UString::UTF_8);
            v_minor.push_back(make_pair(key, i->second));
        }
        std::sort(v_major.begin(), v_major.end(), greater_pair);
        std::sort(v_minor.begin(), v_minor.end(), greater_pair);

        for (unsigned int i = 0; i < v_major.size(); ++i)
        {
            cout<<v_major[i].second<<endl;
        }

        unsigned int maxTokenCount = 4;

        cout<<"maxTokenCount"<<maxTokenCount<<endl;
        if (major_tokens.size() > maxTokenCount)
        {
            unsigned int count = 0;
            for (std::vector<std::pair<std::string, double> >::iterator i = v_major.begin(); i != v_major.end(); ++i)
            {
                if (count < maxTokenCount)
                {
                    newQuery += " ";
                    newQuery += i->first;
                    count++;
                }
                else
                    break;
            }
            return true;
        }

        if (major_tokens.size() > 0 && minor_tokens.size() > 0)
        {
            cout<<minor_tokens.size()<<endl;
            if (major_tokens.size() + minor_tokens.size() > maxTokenCount)
            {
                unsigned int tmpCount = v_major.size();
                for (unsigned int i = 0; i < v_major.size(); ++i)
                {
                    newQuery += " ";
                    newQuery += v_major[i].first;
                }
                for (unsigned int i = 0; i < (maxTokenCount - tmpCount) && i < v_minor.size(); ++i)
                {
                    newQuery += " ";
                    newQuery += v_minor[i].first;
                }
            }
            else
            {
                for (unsigned int i = 0; i < v_major.size(); ++i)
                {
                    newQuery += " ";
                    newQuery += v_major[i].first;
                }
            }
            return true;
        }
        else if (minor_tokens.size() == 0) /// only major <= 4
        {
            for (unsigned int i = 0; i < v_major.size() - 1; ++i)
            {
                newQuery += " ";
                newQuery += v_major[i].first;
            }
            return true;
        }
        else /// only minor
        {
            return true;
            // maybe not exits
        }
        return true;
    }

private:
    boost::shared_ptr<MiningManager> miningManager_;
};


//////////////////////////
/////////////////////////
class QAQueryPrune : public QueryPruneBase
{
public:
    QAQueryPrune()
    {

    }

    ~QAQueryPrune()
    {

    }

    bool queryPrune(std::string& query_orig,
        std::vector<izenelib::util::UString> keywords, std::string& newQuery)
    {
        assembleDisjunction(keywords, newQuery);
        return true;
    }
};

}
#endif //QUERY_Prune_ALL_H