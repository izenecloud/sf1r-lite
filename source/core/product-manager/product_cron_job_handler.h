///
#ifndef SF1R_PRODUCTMANAGER_PRODUCTCRONJOGHANDLER_H
#define SF1R_PRODUCTMANAGER_PRODUCTCRONJOGHANDLER_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "pm_def.h"
#include "pm_types.h"

#include <util/singleton.h>
#include <util/cronexpression.h>

namespace sf1r
{

class ProductPriceTrend;

class ProductCronJobHandler : public boost::noncopyable
{
public:
    static ProductCronJobHandler* getInstance()
    {
        return izenelib::util::Singleton<ProductCronJobHandler>::get();
    }

public:
    void setParam(uint32_t wait_sec, uint32_t days);

    ProductCronJobHandler();

    ~ProductCronJobHandler();

    void addCollection(ProductPriceTrend* price_trend);

    bool cronStart(const std::string& cron_job);

    void runEvents();

private:
    void cronJob_();

private:
    std::vector<ProductPriceTrend *> price_trend_list_;
    uint32_t wait_seconds_;
    uint32_t wait_days_;
    izenelib::util::CronExpression cron_expression_;
    bool cron_started_;
    boost::mutex mtx_;
};

}
#endif
