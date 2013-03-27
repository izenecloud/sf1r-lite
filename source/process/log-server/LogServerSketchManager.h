#ifndef LOG_SERVER_SKETCH_MANAGER_
#define LOG_SERVER_SKETCH_MANAGER_

#include <string>
#include <vector>
#include <map>
#include <list>
#include <util/sketch/madoka/sketch.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

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
    std::list<std::pair<std::string, uint32_t> >& GetEstimation()
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
        std::list<std::pair<std::string, uint32_t> >::iterator iter;
        for(iter= Estimation_.begin();iter!=Estimation_.end();iter++)
        {
            if(iter->first == query && iter->second < count)
            {
                iter->second = count;
                return true;
            }
            else if(iter->second < count)
            {
                Estimation_.insert(iter, make_pair(query, count));
                return true;
            }
        }
        Estimation_.push_back(make_pair(query, count));
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
        if(timeCacheMap_.find(time) == timeCacheMap_.end())
            return false;
        LOG(INFO) << "true" <<endl;
        estimation = *(timeCacheMap_[time]);
        return true;
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
        LOG(INFO) << "QueryEstimationCache: getEstimationOfDay"<<endl;
        std::list<std::string>::iterator it;
        for(it=collectionCacheL_.begin();it!=collectionCacheL_.end();it++)
        {
            if(*it == collection)
                return true;
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

    typedef QueryEstimationCacheElement* EleType;
    typedef std::pair<std::string, EleType> VType;
    std::list<std::string> collectionCacheL_;
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
            const uint32_t & hitnum,
            const uint32_t & page_start,
            const uint32_t & page_count,
            const std::string & duration,
            const std::string & timestamp)
    {
        //update sketch
        if(!checkSketch(collection))
            return false;
        uint64_t count = collectionSketchMap_[collection]->inc(query.c_str(), query.length());

        //update estimation
        if(!checkFreqEstimation(collection))
            return false;
        collectionFEMap_[collection]->Insert(query, count);

        //write file (for external sort)
        //to do
        return true;
    }

    bool getFreqUserQueries(const std::string & collection,
            const std::string & begin_time,
            const std::string & end_time,
            std::list< std::map<std::string, std::string> > & results)
    {
        LOG(INFO) << collection << "   " << begin_time << "   " << end_time << endl;
        //a temp sketch should be built
        //read result list from cache
        vector<string> time_list;

        getTimeList(begin_time.substr(1,begin_time.size()-1), end_time.substr(1,end_time.size()-1), time_list);
        vector<string>::iterator it;

        ptime td(day_clock::local_day(), hours(0));
        string today = to_iso_string(td);
        LOG(INFO) << time_list.size() <<endl;
        for(it=time_list.begin();it!=time_list.end();it++)
        {
            LOG(INFO) << *it << endl;
            std::list< std::pair<std::string, uint32_t> > temp;
            queryEstimationCache_.getEstimationOfDay(collection, *it, temp);
            if(*it == today)
            {
                if(collectionFEMap_.find(collection) != collectionFEMap_.end())
                    collectionFEMap_[collection]->GetEstimation();
            }
        }
        //read result list from file
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
            results.push_back(to_iso_string(p));
            p = p + dd;
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

    std::list<string> mylist_;
    QueryEstimationCache queryEstimationCache_;

    std::map<std::string, madoka::Sketch*> collectionSketchMap_;
    std::map<std::string, QueriesFrequencyEstimation*> collectionFEMap_;
};
}
}//end of namespace sf1r

#endif
