/*
 *  AdClickPredictor.h
 */

#ifndef SF1_AD_CLICK_PREDICTOR_H_
#define SF1_AD_CLICK_PREDICTOR_H_

#include <util/singleton.h>
#include <idmlib/ctr/AdPredictor.hpp>
#include <glog/logging.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <fstream>

namespace sf1r{

class AdClickPredictor {
public:
    typedef boost::shared_lock<boost::shared_mutex> readLock;
    typedef boost::unique_lock<boost::shared_mutex> writeLock;
    typedef idmlib::AdPredictor AdPredictorType;

    AdClickPredictor()
    {
    }
/*
    AdClickPredictor(const std::string& path)
        : workingPath_(path)
    {
        predictor_.reset(new AdPredictorType);
        dataPath_ = path + "/data/";
        modelPath_ = path + "/model/predictor.bin";
    }
*/
    ~AdClickPredictor()
    {
    }

    static AdClickPredictor* get()
    {
        return izenelib::util::Singleton<AdClickPredictor>::get();
    }

    void init(const std::string& path)
    {
        predictor_.reset(new AdPredictorType);

        dataPath_ = path + "/data/";
        modelPath_ = path + "/model/predictor.bin";

        boost::filesystem::create_directories(path + "/model");
        boost::filesystem::create_directories(dataPath_ + "backup");
    }

    double predict(const std::vector<std::pair<std::string, std::string> >& assignment)
    {
        readLock lock(rwMutex_);
        return predictor_->predict(assignment);
    }

    bool preProcess();

    bool trainFromFile(const std::string& filename);

    bool train();

    bool postProcess();

    bool load();

private:
    std::string workingPath_;

    std::string dataPath_;

    std::string modelPath_;

    boost::shared_mutex rwMutex_;

    boost::shared_ptr<AdPredictorType> predictor_;

    boost::shared_ptr<AdPredictorType> learner_;
};
} // namespace sf1r
#endif
