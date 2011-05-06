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

#include <string>
#include <vector>
#include <set>

namespace sf1r
{
typedef std::vector<std::pair<std::vector<itemid_t>, size_t > > FrequentItemSetResultType;
class OrderManager
{
public:
    OrderManager(
        const std::string& path
    );

    ~OrderManager();

    void setMinThreshold(size_t threshold)
    {
        threshold_ = threshold;
    }

    void addOrder(
        sf1r::orderid_t orderId,
        std::list<sf1r::itemid_t>& items);

    bool getFreqItemSets(
        std::list<sf1r::itemid_t>& items, 
        std::list<sf1r::itemid_t>& results);


    FrequentItemSetResultType& 
    getAllFreqItemSets()
    {
        izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(result_lock_);
        return frequent_itemsets_;
    }

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
};

} // namespace sf1r

#endif // ORDER_MANAGER_H

