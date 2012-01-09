/**
 * @file RecommendBundleActivator.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_BUNDLE_ACTIVATOR_H
#define RECOMMEND_BUNDLE_ACTIVATOR_H

#include "RecommendTaskService.h"
#include "RecommendSearchService.h"

#include <recommend-manager/RecTypes.h>
#include <directory-manager/DirectoryRotator.h>
#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>

namespace sf1r
{
using namespace izenelib::osgi;

class IndexSearchService;
class RecommendBundleConfiguration;
class UserManager;
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class OrderManager;
class EventManager;
class RateManager;
class RecommenderFactory;
class ItemIdGenerator;

class RecommendBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
public:
    RecommendBundleActivator();
    virtual ~RecommendBundleActivator();
    virtual void start(IBundleContext::ConstPtr context);
    virtual void stop(IBundleContext::ConstPtr context);
    virtual bool addingService(const ServiceReference& ref);
    virtual void removedService(const ServiceReference& ref);

private:
    bool init_(IndexSearchService* indexSearchService);
    bool createDataDir_();
    bool openDataDirectory_(std::string& dataPath);
    void createSCDDir_();
    void createUser_();
    void createItem_(IndexSearchService* indexSearchService);
    void createMining_();
    void createEvent_();
    void createOrder_();
    void createRecommender_();
    void createService_();

private:
    IBundleContext* context_;
    boost::scoped_ptr<ServiceTracker> indexSearchTracker_;
    boost::scoped_ptr<RecommendTaskService> taskService_;
    boost::scoped_ptr<IServiceRegistration> taskServiceReg_;
    boost::scoped_ptr<RecommendSearchService> searchService_;
    boost::scoped_ptr<IServiceRegistration> searchServiceReg_;

    boost::shared_ptr<RecommendBundleConfiguration> config_;
    DirectoryRotator directoryRotator_;
    boost::filesystem::path dataDir_;

    boost::scoped_ptr<UserManager> userManager_;
    boost::scoped_ptr<ItemManager> itemManager_;
    boost::scoped_ptr<VisitManager> visitManager_;
    boost::scoped_ptr<PurchaseManager> purchaseManager_;
    boost::scoped_ptr<CartManager> cartManager_;
    boost::scoped_ptr<OrderManager> orderManager_;
    boost::scoped_ptr<EventManager> eventManager_;
    boost::scoped_ptr<RateManager> rateManager_;
    boost::scoped_ptr<RecommenderFactory> recommenderFactory_;
    boost::scoped_ptr<ItemIdGenerator> itemIdGenerator_;

    boost::scoped_ptr<CoVisitManager> coVisitManager_;
    boost::scoped_ptr<ItemCFManager> itemCFManager_;
};

} // namespace sf1r

#endif // RECOMMEND_BUNDLE_ACTIVATOR_H
