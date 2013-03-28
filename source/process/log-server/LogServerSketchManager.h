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
#include <boost/bind.hpp>
#include <util/scheduler.h>

using namespace boost::posix_time;
using namespace boost::gregorian;

namespace sf1r{
namespace logsketch{
class QueriesFrequencyEstimation
{
public:
    typedef std::list<std::pair<std::string, uint32_t> > EstimationType;
    typedef EstimationType::const_iterator ConstIterType;

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

    bool GetEstimation(ConstIterType & begin, ConstIterType & end)
    {
        begin = Estimation_.begin();
        end = Estimation_.end();
        return true;;
    }
private:

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
        EstimationType::iterator iter, it;
        if(Estimation_.size() == 0 || count <= GetMinFreq())
        {
            Estimation_.push_back(make_pair(query, count));
            return true;
        }
        for(iter= Estimation_.begin();iter!=Estimation_.end();iter++)
        {
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

    uint32_t MaxSize_;
    EstimationType Estimation_;
};

class QueryEstimationCacheElement
{
public:
    typedef std::list<std::pair<std::string, uint32_t> > EstimationType;
    typedef EstimationType::const_iterator ConstIterType;

    QueryEstimationCacheElement()
    {
    }
    bool insert(const std::string& time,
            const EstimationType& estimation)
    {
        if(timeCacheMap_.find(time) != timeCacheMap_.end())
            return false;
        timeCacheMap_[time] = estimation;
        return true;
    }

    bool get(const std::string& time,
            ConstIterType& begin,
            ConstIterType& end)
    {
        if(timeCacheMap_.find(time) != timeCacheMap_.end())
        {
            begin = timeCacheMap_[time].begin();
            end = timeCacheMap_[time].end();
            return true;
        }
        return false;
    }
private:
    boost::unordered_map<std::string, EstimationType> timeCacheMap_;
};

class QueryEstimationCache
{
public:
    typedef std::list< std::pair<std::string, uint32_t> > EstimationType;
    typedef EstimationType::const_iterator ConstIterType;

    QueryEstimationCache()
    {
    }

    bool insertEstimationOfDay(const std::string& collection,
            const std::string& time,
            const EstimationType& estimation)
    {
        if(!checkCollection(collection))
            return false;
        return collectionCacheMap_[collection]->insert(time, estimation);
    }

    bool getEstimationOfDay(const std::string& collection,
            const std::string& time,
            ConstIterType& begin,
            ConstIterType& end)
    {
        if(collectionCacheMap_.find(collection) != collectionCacheMap_.end())
        {
                return collectionCacheMap_[collection]->get(time, begin, end);
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

class LogServerSketchManager: public boost::noncopyable
{
public:
    typedef std::list< std::pair<std::string, uint32_t> > EstimationType;
    typedef EstimationType::const_iterator ConstIterType;

    LogServerSketchManager(std::string wd)
    {
        slidCronJobName_ = "EstimationSlidMaker";
        sortCronJobName_ = "LogExternalSorter";
        workdir_ = wd;
        open();
    }

    bool open()
    {
        bool res = izenelib::util::Scheduler::addJob(slidCronJobName_,
                24*60*60*1000, //each day
                0,  //start from now
                boost::bind(&LogServerSketchManager::makeEstimationSlid, this));
        if(!res)
            LOG(ERROR) <<"Failed in izenelib::util::Scheduler::addJob(), cron job name: " << slidCronJobName_ << endl;
        else
            LOG(INFO) <<"Create cron job: " << slidCronJobName_ <<endl;
        res = izenelib::util::Scheduler::addJob(sortCronJobName_,
                1000,  //each second
                0,   //start from now
                boost::bind(&LogServerSketchManager::externalSort, this));
        if(!res)
            LOG(ERROR) <<"Failed in izenelib::util::Scheduler::addJob(), cron job name: " << sortCronJobName_ << endl;
        else
            LOG(INFO) <<"Create cron job: " << sortCronJobName_ <<endl;
        return true;
    }

    void close()
    {
    }

    void makeEstimationSlid()
    {
    }

    void externalSort()
    {
        //LOG(INFO) << "It's time to external sort..." << endl;
    }

    bool injectUserQuery(const std::string & query,
            const std::string & collection,
            const std::string & hitnum,
            const std::string & page_start,
            const std::string & page_count,
            const std::string & duration,
            const std::string & timestamp)
    {
        if(!checkSketch(collection))
            return false;

        uint32_t count = collectionSketchMap_[collection]->inc(query.c_str(), query.length());

        LOG(INFO) << "query: " << query << " count: " << count << endl;

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
        vector<string>::iterator it;

        ConstIterType iter, end_iter;
        getTimeList(begin_time, end_time, time_list);

        ptime td(day_clock::local_day(), hours(0));
        string today = to_iso_string(td);

        QueriesFrequencyEstimation* qfe = new QueriesFrequencyEstimation(boost::lexical_cast<uint32_t>(limit));
        std::map<std::string, uint32_t> Tmap;

        for(it=time_list.begin();it!=time_list.end();it++)
        {
            if(queryEstimationCache_.getEstimationOfDay(collection, *it, iter, end_iter))
            {
                uint32_t i=0;
                for(;iter!=end_iter;iter++,i++)
                {
                    if(i>3*boost::lexical_cast<uint32_t>(limit)) break;
                    Tmap[iter->first] = Tmap[iter->first] + iter->second;
                    qfe->Insert(iter->first, Tmap[iter->first]);
                }
            }
            else if(*it == today)
            {
                if(collectionFEMap_.find(collection) != collectionFEMap_.end())
                {
                    collectionFEMap_[collection]->GetEstimation(iter, end_iter);
                    uint32_t i=0;
                    for(;iter!=end_iter;iter++, i++)
                    {
                        if(i>3*boost::lexical_cast<uint32_t>(limit)) break;
                        Tmap[iter->first] = Tmap[iter->first] + iter->second;
                        qfe->Insert(iter->first, Tmap[iter->first]);
                    }
                }
            }
        }

        qfe->GetEstimation(iter, end_iter);
        for(;iter!=end_iter;iter++)
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
    std::string sortCronJobName_;
    std::string slidCronJobName_;

    QueryEstimationCache queryEstimationCache_;
    std::map<std::string, madoka::Sketch*> collectionSketchMap_;
    std::map<std::string, QueriesFrequencyEstimation*> collectionFEMap_;
};
}
}//end of namespace sf1r

#endif
