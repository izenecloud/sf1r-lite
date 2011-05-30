/**
 * @file OrderManager.h
 * @author Yingfeng Zhang
 * @date 2011-05-06
 */

#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "RecTypes.h"
#include "ItemIndex.h"

#include <util/ThreadModel.h>
#include <idmlib/resys/ItemRescorer.h>

#include <string>
#include <vector>
#include <set>

#include <boost/thread/mutex.hpp>

namespace sf1r
{
class ItemManager;

typedef std::vector<std::pair<std::vector<itemid_t>, size_t > > FrequentItemSetResultType;
class OrderManager
{
public:
    OrderManager(
        const std::string& path,
        const ItemManager* itemManager
    );

    ~OrderManager();

    void setMinThreshold(size_t threshold)
    {
        threshold_ = threshold;
    }

    void addOrder(std::list<sf1r::itemid_t>& items);

    /**
     * Get @p howmany items which is most frequently appeared with @p items in the same order.
     * @param howmany the number of items to get
     * @param items the input items
     * @param results the items returned
     * @param rescorer if not NULL, the items filtered by @p rescorer would not appear in @p results
     */
    bool getFreqItemSets(
        int howmany, 
        std::list<sf1r::itemid_t>& items, 
        std::list<sf1r::itemid_t>& results,
        idmlib::recommender::ItemRescorer* rescorer = NULL);

    /**
     * Get @p howmany item sets which frequency is not less than @p threshold.
     * @param howmany the number of item sets to get
     * @param threshold the minimum frequency value
     * @param results the item sets returned
     * @note to get latest result, you have to call @c buildFreqItemsets() beforehand.
     */
    void getAllFreqItemSets(
        int howmany, 
        size_t threshold,
        FrequentItemSetResultType& results);

    /**
     * Flush the orders and items into disk files.
     * @note you need not call this function explicitly,
     *       as in @c getFreqItemSets() and @c buildFreqItemsets(),
     *       the latest orders and items could be fetched in realtime.
     */
    void flush();

    void buildFreqItemsets();

private:
    bool _saveFreqItemsetDb() const ;

    bool _restoreFreqItemsetDb();

    void _writeRecord(
        sf1r::orderid_t orderId,
        std::list<sf1r::itemid_t>& items);

    bool _getOrder(
        sf1r::orderid_t orderId,
        std::vector<sf1r::itemid_t>& items);

    void _findMaxItemsets();

    void _findFrequentItemsets();

    void _judgeFrequentItemset(
        std::vector<itemid_t>& max_itemset , 
        FrequentItemSetResultType& frequent_itemsets);

    orderid_t _newOrderId();

private:
    ItemIndex item_order_index_;
    std::string order_key_path_;
    std::string order_db_path_;
    std::string max_itemsets_results_path_;
    std::string frequent_itemsets_results_path_;	
    size_t threshold_;
    izenelib::util::ReadWriteLock db_lock_;
    FILE* order_key_;
    FILE* order_db_;
    izenelib::util::ReadWriteLock result_lock_;
    FrequentItemSetResultType frequent_itemsets_;
    const ItemManager* itemManager_; // for max item id
    std::string orderIdPath_;
    orderid_t orderId_;
    boost::mutex orderIdMutex_;
};

} // namespace sf1r

#endif // ORDER_MANAGER_H

