#ifndef DRIVER_LOG_PROCESSOR_H_
#define DRIVER_LOG_PROCESSOR_H_

#include "LogServerStorage.h"

#include <string>
#include <iostream>

#include <util/singleton.h>
#include <util/driver/Controller.h>
#include <common/Keys.h>

#include <boost/unordered_map.hpp>

namespace sf1r
{

struct ProcessorKey
{
    std::string controller_;
    std::string action_;

    ProcessorKey(
            const std::string& controller,
            const std::string& action)
        : controller_(controller)
        , action_(action)
    {}
};
/// for boost
inline std::size_t hash_value(const ProcessorKey& key)
{
    std::size_t hash_value = 0;
    boost::hash_combine(hash_value, key.controller_);
    boost::hash_combine(hash_value, key.action_);

    return hash_value;
}
inline bool operator==(const ProcessorKey& a, const ProcessorKey& b)
{
    return a.controller_ == b.controller_
           && a.action_ == b.action_;
}


/**
 * Processors
 */
class Processor
{
public:
    virtual ~Processor() {}

    void setStorage(
            const LogServerStorage::DrumPtr& drum,
            const LogServerStorage::KVDBPtr& docidDB)
    {
        drum_ = drum;
        docidDB_ = docidDB;
    }

     /// @param request [IN][OUT]
    virtual void process(izenelib::driver::Request& request) {}

protected:
    LogServerStorage::DrumPtr drum_;
    LogServerStorage::KVDBPtr docidDB_;

};
typedef Processor* ProcessorPtr;

class DocVisitProcessor : public Processor
{
public:
    virtual void process(izenelib::driver::Request& request);
};

class RecVisitItemProcessor : public Processor
{
public:
    virtual void process(izenelib::driver::Request& request);
};

class RecPurchaseItemProcessor : public Processor
{
public:
    virtual void process(izenelib::driver::Request& request);
};

/**
 * Processor Factory
 */
class ProcessorFactory
{
public:
    ProcessorFactory();

    ~ProcessorFactory();

    static ProcessorFactory* get()
    {
        return izenelib::util::Singleton<ProcessorFactory>::get();
    }

    void init(
            LogServerStorage::DrumPtr drum,
            LogServerStorage::KVDBPtr docidDB);

    void destroy();


    ProcessorPtr getProcessor(
            const std::string& controller,
            const std::string& action) const;

private:
    typedef boost::unordered_map<ProcessorKey, ProcessorPtr> map_t;

    ProcessorPtr defaultProcessor_;
    map_t specialProcessors_;
};

}

#endif /* DRIVER_LOG_PROCESSOR_H_ */
