#ifndef LOG_SERVER_SKETCH_MANAGER_
#define LOG_SERVER_SKETCH_MANAGER_

#include <string>
#include <vector>
#include <map>
#include <list>
#include <util/sketch/madoka/sketch.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::posix_time;
using namespace boost::gregorian;

namespace sf1r{
namespace logsketch{
class QueriesFrequencyEstimation
{
public:

    QueriesFrequencyEstimation(uint32_t ms = 100)
    {
        MaxSize_ = ms;
    }

    bool Insert(const std::string& query, uint32_t count)
    {
        if(!Insert_(query, count))
            return false;
        while(GetEstimationSize() > MaxSize_)
        {
            Estimation_.pop_back();
        }
        return true;
    }

    std::list<std::pair<std::string, uint32_t> > GetEstimation()
    {
        return Estimation_;
    }
private:
    uint32_t MaxSize_;

    uint32_t GetMinFreq()
    {
        return (Estimation_.back()).second;
    }

    uint32_t GetEstimationSize()
    {
        return Estimation_.size();
    }

    bool Insert_(const std::string& query, uint32_t count)
    {
        std::list<std::pair<std::string, uint32_t> >::iterator iter, it;
        if(Estimation_.size() == 0 || count <= GetMinFreq())
        {
            Estimation_.push_back(make_pair(query, count));
            return true;
        }
        for(iter= Estimation_.begin();iter!=Estimation_.end();iter++)
        {
//            LOG(INFO) << iter->first<<" "<<iter->second<<" " <<query<< "  " << count << endl;
            if(iter->first == query && iter->second < count)
            {
                iter->second = count;
                if(iter == Estimation_.begin())
                    break;
                it = iter;it--;
                if(it->second < iter->second)
                {
                    string temp_str = it->first; uint32_t temp_int = it->second;
                    it->first = iter->first; it->second = iter->second;
                    iter->first = temp_str; iter->second = temp_int;
                }
                break;
            }
        }
        if(iter==Estimation_.end())
        {
            while(iter!=Estimation_.begin())
            {
                it = iter;
                iter--;
                if(iter->second >= count)
                {
                    Estimation_.insert(it, make_pair(query, count));
                    break;
                }
                else if(iter == Estimation_.begin())
                {
                    Estimation_.insert(iter,make_pair(query, count));
                    break;
                }
            }
        }
        return true;
    }

    std::list<std::pair<std::string, uint32_t> > Estimation_;
};

class QueryEstimationCacheElement
{
public:
    QueryEstimationCacheElement()
    {
    }
    bool insert(const std::string& time,
            std::list<std::pair<std::string, uint32_t> >&estimation)
    {
        if(timeCacheMap_.find(time) != timeCacheMap_.end())
            return false;
        *timeCacheMap_[time] = estimation;
        return true;
    }

    bool get(const std::string& time,
            std::list<std::pair<std::string, uint32_t> >& estimation)
    {
        LOG(INFO) << "QueryCacheElement: get" <<endl;
        if(timeCacheMap_.find(time) != timeCacheMap_.end())
        {
            estimation = *(timeCacheMap_[time]);
            return true;
        }
        return false;
    }
private:
    typedef std::list<std::pair<std::string, uint32_t> > EstimationType;

    boost::unordered_map<std::string, boost::shared_ptr<EstimationType> > timeCacheMap_;
};

class QueryEstimationCache
{
public:
    QueryEstimationCache()
    {
    }

    bool insertEstimationOfDay(const std::string& collection, const std::string& time,
            std::list<std::pair<std::string, uint32_t> >& estimation)
    {
        if(!checkCollection(collection))
            return false;
        return collectionCacheMap_[collection]->insert(time, estimation);
    }

    bool getEstimationOfDay(const std::string& collection, const std::string& time,
            std::list<std::pair<std::string, uint32_t> >& estimation)
    {
        if(collectionCacheMap_.find(collection) != collectionCacheMap_.end())
        {
                return collectionCacheMap_[collection]->get(time, estimation);
        }
        return false;
    }
private:
    bool checkCollection(const std::string& collection)
    {
        if(collectionCacheMap_.find(collection) == collectionCacheMap_.end())
        {
            return initCollectionCache(collection);
        }
        return true;
    }
    bool initCollectionCache(const std::string& collection)
    {
        boost::shared_ptr<QueryEstimationCacheElement> share(new QueryEstimationCacheElement());
        collectionCacheMap_[collection] = share;
        return true;
    }

    uint32_t CacheSize_;

    boost::unordered_map<std::string, boost::shared_ptr<QueryEstimationCacheElement> > collectionCacheMap_;
};

class LogServerSketchManager
{
public:
    LogServerSketchManager(std::string wd)
    {
        workdir_ = wd;
    }

    bool open()
    {
        return true;
    }

    void close()
    {

    }
bool injectUserQuery(const std::string & query,
            const std::string & collection,
            const std::string & hitnum,
            const std::string & page_start,
            const std::string & page_count,
            const std::string & duration,
            const std::string & timestamp)
    {
        //update sketch
        if(!checkSketch(collection))
            return false;
        uint64_t count = collectionSketchMap_[collection]->inc(query.c_str(), query.length());

        LOG(INFO) << "query: " << query << " count: " << count << endl;
        //update estimation
        if(!checkFreqEstimation(collection))
            return false;
        collectionFEMap_[collection]->Insert(query, count);

        //write file (for external sort)
        return true;
    }

    bool getFreqUserQueries(const std::string & collection,
            const std::string & begin_time,
            const std::string & end_time,
            const std::string & limit,
            std::list< std::map<std::string, std::string> > & results)
    {
        LOG(INFO) << collection << "   " << begin_time << "   " << end_time << "  " <<limit<<endl;
        vector<string> time_list;

        getTimeList(begin_time, end_time, time_list);
        vector<string>::iterator it;

        ptime td(day_clock::local_day(), hours(0));
        string today = to_iso_string(td);
        QueriesFrequencyEstimation* qfe = new QueriesFrequencyEstimation(boost::lexical_cast<uint32_t>(limit));
        std::map<std::string, uint32_t> Tmap;

        for(it=time_list.begin();it!=time_list.end();it++)
        {
            std::list< std::pair<std::string, uint32_t> > temp;
            if(queryEstimationCache_.getEstimationOfDay(collection, *it, temp))
            {
                std::list< std::pair<std::string, uint32_t> >::iterator itr;
                uint32_t i=0;
                for(itr=temp.begin();itr!=temp.end();itr++,i++)
                {
                    if(i>3*boost::lexical_cast<uint32_t>(limit)) break;
                    Tmap[itr->first] = Tmap[itr->first] + itr->second;
                    qfe->Insert(itr->first, Tmap[itr->first]);
                }
            }
            else if(*it == today)
            {
                if(collectionFEMap_.find(collection) != collectionFEMap_.end())
                {
                    std::list< std::pair<std::string, uint32_t> > es = collectionFEMap_[collection]->GetEstimation();
                    std::list< std::pair<std::string, uint32_t> >::iterator itr;
                    uint32_t i=0;
                    for(itr=es.begin();itr!=es.end();itr++, i++)
                    {
                        if(i>3*boost::lexical_cast<uint32_t>(limit)) break;
                        Tmap[itr->first] = Tmap[itr->first] + itr->second;
                        qfe->Insert(itr->first, Tmap[itr->first]);
                    }
                }
            }
        }

        std::list< std::pair<std::string, uint32_t> > es = qfe->GetEstimation();
        std::list< std::pair<std::string, uint32_t> >::iterator iter;
        for(iter=es.begin();iter!=es.end();iter++)
        {
            std::map<std::string, std::string> res;
            res["query"] = iter->first;
            res["count"] = boost::lexical_cast<std::string>(iter->second);
            results.push_back(res);
        }
        return true;
    }

private:

    bool getTimeList(const std::string& begin_time,
            const std::string& end_time,
            vector<string>& results)
    {
        ptime begin = from_iso_string(begin_time);
        ptime end = from_iso_string(end_time);

        if(begin > end) return false;
        days dd(1);
        ptime p(begin.date(), hours(0));
        while(p<end)
        {
            results.push_back(to_iso_string(p)); p = p + dd;
        }
        return true;
    }

    bool checkSketch(const std::string& collection)
    {

        if(collectionSketchMap_.find(collection) == collectionSketchMap_.end())
        {
            return initSketchFile(collection);
        }
        return true;
    }

    bool initSketchFile(const std::string& collection)
    {
        collectionSketchMap_[collection] = new madoka::Sketch();
        collectionSketchMap_[collection]->create();
        return true;
    }

    bool checkFreqEstimation(const std::string& collection)
    {
        if(collectionFEMap_.find(collection) == collectionFEMap_.end())
        {
            return initFreqEstimation(collection);
        }
        return true;
    }
    bool initFreqEstimation(const std::string& collection)
    {
        collectionFEMap_[collection] = new QueriesFrequencyEstimation();
        return true;
    }
    std::string workdir_;

    QueryEstimationCache queryEstimationCache_;

    std::map<std::string, madoka::Sketch*> collectionSketchMap_;
    std::map<std::string, QueriesFrequencyEstimation*> collectionFEMap_;
};
}
}//end of namespace sf1r

#endif
