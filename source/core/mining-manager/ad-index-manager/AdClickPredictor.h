/*
 *  AdClickPredictor.h
 */

#ifndef SF1_AD_CLICK_PREDICTOR_H_
#define SF1_AD_CLICK_PREDICTOR_H_

#include <util/singleton.h>
#include <idmlib/ctr/AdPredictor.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <vector>

namespace sf1r
{

class AdClickPredictor {
public:
    typedef boost::shared_lock<boost::shared_mutex> readLock;
    typedef boost::unique_lock<boost::shared_mutex> writeLock;
    typedef idmlib::AdPredictor AdPredictorType;
    typedef std::vector<std::pair<std::string, std::string> > AssignmentT;

    AdClickPredictor()
    {
    }
    ~AdClickPredictor()
    {
        stop();
    }

    static AdClickPredictor* get()
    {
        return izenelib::util::Singleton<AdClickPredictor>::get();
    }

    void init(const std::string& path);
    void stop();

    void update(const AssignmentT& assignment_left, const AssignmentT& assignment_right, bool click)
    {
        writeLock lock(rwMutex_);
        predictor_->update(assignment_left, assignment_right, click);
    }

    void update(const AssignmentT& assignment, bool click)
    {
        writeLock lock(rwMutex_);
        predictor_->update(assignment, click);
    }

    double predict(const AssignmentT& assignment_left,
        const AssignmentT& assignment_right)
    {
        readLock lock(rwMutex_);
        return predictor_->predict(assignment_left, assignment_right);
    }

    double predict(const AssignmentT& assignment)
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
