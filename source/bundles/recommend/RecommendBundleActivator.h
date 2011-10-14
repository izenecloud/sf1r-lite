/**
 * @file RecommendBundleActivator.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_BUNDLE_ACTIVATOR_H
#define RECOMMEND_BUNDLE_ACTIVATOR_H

#include "RecommendBundleConfiguration.h"
#include "RecommendTaskService.h"
#include "RecommendSearchService.h"

#include <recommend-manager/RecTypes.h>
#include <directory-manager/DirectoryRotator.h>
#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>

#include <string>

namespace sf1r
{
using namespace izenelib::osgi;

class UserManager;
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class EventManager;
class OrderManager;
class RecommenderManager;

class RecommendBundleActivator : public IBundleActivator
{
public:
    RecommendBundleActivator();
    virtual ~RecommendBundleActivator();
    virtual void start(IBundleContext::ConstPtr context);
    virtual void stop(IBundleContext::ConstPtr context);

private:
    IBundleContext* context_;
    RecommendTaskService* taskService_;
    IServiceRegistration* taskServiceReg_;
    RecommendSearchService* searchService_;
    IServiceRegistration* searchServiceReg_;

    RecommendBundleConfiguration* config_;
    DirectoryRotator directoryRotator_;

    UserManager* userManager_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    CartManager* cartManager_;
    EventManager* eventManager_;
    OrderManager* orderManager_;
    RecommenderManager* recommenderManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;

    CoVisitManager* coVisitManager_;
    ItemCFManager* itemCFManager_;

    bool init_();
    bool openDataDirectory_(std::string& dataDir);
};

} // namespace sf1r

#endif // RECOMMEND_BUNDLE_ACTIVATOR_H
