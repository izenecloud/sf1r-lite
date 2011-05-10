#include "OrderManager.h"

#include <idmlib/itemset/all-maximal-sets-satelite.h>
#include <idmlib/itemset/data-source-iterator.h>
#include <util/nextsubset.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/vector.hpp>

#include <fstream>
#include <memory>
#include <algorithm>
#include <map>

using namespace std;

namespace sf1r
{

class FilterItems
{
    ItemIdSet& itemIdSet_;
public:
    FilterItems(ItemIdSet& itemIdSet):itemIdSet_(itemIdSet){}

    bool operator()(itemid_t itemId)
    {
        return itemIdSet_.find(itemId) == itemIdSet_.end() ? false: true;
    }
};

/**
 * @return true for the frequency value of @p p1 is higher than that of @p p2,
 *         if their frequency values are equal, true if the items in @p p1 compares
 *         lexicographically less than the items in @p p2.
 * @pre the items in @p p1 and @p p2 are already sorted by item id
 */
bool itemSetCompare (const std::pair<std::vector<itemid_t>, size_t >& p1, const std::pair<std::vector<itemid_t>, size_t >& p2)
{
    if (p1.second > p2.second)
        return true;

    if (p1.second < p2.second)
        return false;

    return std::lexicographical_compare(p1.first.begin(), p1.first.end(),
                                        p2.first.begin(), p2.first.end());
}

/**
 * @return true for the same items and frequency value.
 * @pre the items in @p p1 and @p p2 are already sorted by item id
 */
bool itemSetPredicate (const std::pair<std::vector<itemid_t>, size_t >& p1, const std::pair<std::vector<itemid_t>, size_t >& p2)
{
    if (p1.second != p2.second || p1.first.size() != p2.first.size())
        return false;

    return std::equal(p1.first.begin(), p1.first.end(), p2.first.begin());
}

OrderManager::OrderManager(const std::string& path)
    :item_order_index_(path+"/index")
    ,order_key_path_(boost::filesystem::path(boost::filesystem::path(path)/"orderdb.key").string())
    ,order_db_path_(boost::filesystem::path(boost::filesystem::path(path)/"orderdb.data").string())
    ,max_itemsets_results_path_(boost::filesystem::path(boost::filesystem::path(path)/"maxitemsets.txt").string())
    ,frequent_itemsets_results_path_(boost::filesystem::path(boost::filesystem::path(path)/"frequentitemsets.db").string())
    ,threshold_(5)
{
    _restoreFreqItemsetDb();

    struct stat statbuf;
    bool creating = stat(order_key_path_.c_str(), &statbuf);
    order_key_ = fopen(order_key_path_.c_str(), creating ? "w+b" : "r+b");
    creating = stat(order_db_path_.c_str(), &statbuf);
    order_db_ = fopen(order_db_path_.c_str(), "ab");
}

OrderManager::~OrderManager()
{
    fclose(order_key_);
    fclose(order_db_);
}

void OrderManager::addOrder(
	sf1r::orderid_t orderId,
	std::list<sf1r::itemid_t>& items
)
{
    item_order_index_.add(orderId, items);
    _writeRecord(orderId, items);
}

bool OrderManager::getFreqItemSets(
    std::list<sf1r::itemid_t>& items, 
    std::list<sf1r::itemid_t>& results)
{
    std::list<orderid_t> orders;
    if(item_order_index_.get( items, orders))
    {
        ItemIdSet itemIdSet(items.begin(),items.end());
        FilterItems filterItems(itemIdSet);
        std::map<itemid_t, int> itemFreqMap;
        for(std::list<orderid_t>::iterator orderIt = orders.begin();
              orderIt != orders.end(); ++orderIt)
        {
            std::vector<itemid_t> items_in_order;
            if(_getOrder(*orderIt,items_in_order))
            {
                for(std::vector<itemid_t>::const_iterator itemIt = items_in_order.begin();
                    itemIt != items_in_order.end(); ++itemIt)
                {
                    if(filterItems(*itemIt) == false)
                    {
                        ++itemFreqMap[*itemIt];
                    }
                }
            }
        }
        for(std::map<itemid_t, int>::const_iterator mapIt = itemFreqMap.begin();
            mapIt != itemFreqMap.end(); ++mapIt)
        {
            results.push_back(mapIt->first);
        }
        return true;
    }

    return false;
}

void OrderManager::getAllFreqItemSets(
    FrequentItemSetResultType& freq_itemsets,
    size_t threshold
)
{
    izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(result_lock_);
    for(FrequentItemSetResultType::iterator freqIt = frequent_itemsets_.begin();
          freqIt != frequent_itemsets_.end(); ++freqIt)
    {
        if(freqIt->second >= threshold)
        {
            freq_itemsets.push_back(*freqIt);
        }
        else
        {
            // assume frequent_itemsets_ is sorted by frequence
            break;
        }
    }
}

void OrderManager::flush()
{
    item_order_index_.flush();
    fflush(order_key_);
    fflush(order_db_);
}

void OrderManager::buildFreqItemsets()
{
    _findMaxItemsets();
    _findFrequentItemsets();
    _saveFreqItemsetDb();
}

void OrderManager::_writeRecord(
        sf1r::orderid_t orderId,
        std::list<sf1r::itemid_t>& items)
{
    {
    ///write keys
    izenelib::util::ScopedWriteLock<izenelib::util::ReadWriteLock> lock(db_lock_);	
    off_t pos = ftell(order_db_);
    fseek(order_key_, orderId*sizeof(off_t), SEEK_SET);
    fwrite(&pos, 1, sizeof(off_t), order_key_);
    }
    ///write values
    fwrite(&orderId, sizeof(sf1r::orderid_t), 1, order_db_); // recordId
    uint32_t size = items.size();
    fwrite(&size, sizeof(uint32_t), 1, order_db_); // size
    for(std::list<sf1r::itemid_t>::iterator iter = items.begin(); 
          iter != items.end(); ++iter)
    {
        sf1r::itemid_t itemId = *iter;
        fwrite(&itemId, sizeof(sf1r::itemid_t), 1, order_db_); //itemId
    }
}

bool OrderManager::_getOrder(
	sf1r::orderid_t orderId,
	std::vector<sf1r::itemid_t>& items
)
{
    off_t pos;
    {
    izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(db_lock_);
    fseek(order_key_, orderId*sizeof(off_t), SEEK_SET);
    fread(&pos, 1, sizeof(off_t), order_key_);
    }

    FILE* order_db = fopen(order_db_path_.c_str(), "rb" );
    fseek(order_db, pos, SEEK_SET);

    sf1r::orderid_t tempOrderId = 0;
    bool result = false;
    size_t bytes_read;
    if ((bytes_read = fread(&tempOrderId, 1, 4, order_db)) == 4
        && tempOrderId == orderId)
    {
        uint32_t vector_size;
        bytes_read = fread(&vector_size, 1, 4, order_db);
        if (bytes_read == 4)
        {
            items.resize(vector_size);
            bytes_read = fread(&(items[0]), 1, sizeof(itemid_t) * vector_size, order_db);
            if (bytes_read == sizeof(itemid_t) * vector_size)
            {
                result = true;
            }
        }
    }

    fclose(order_db);
    return result;
}

void OrderManager::_findMaxItemsets()
{
    std::fstream max_itemsets_results;
    max_itemsets_results.open (max_itemsets_results_path_.c_str(), fstream::out/* | fstream::app*/);
    std::auto_ptr<idmlib::DataSourceIterator> data(idmlib::DataSourceIterator::Get(order_db_path_.c_str()));	
    idmlib::AllMaximalSetsSateLite ap;
    size_t numItems = item_order_index_.getNumItems();
    bool result = ap.FindAllMaximalSets(
        data.get(),
        numItems+1,
        1000000000/*max_items_in_ram*/,
        max_itemsets_results);
    if (!result) 
    {
        LOG(ERROR) << "IO ERROR:: " << data->GetErrorMessage();
    }
}

void OrderManager::_findFrequentItemsets()
{
    std::fstream max_itemsets_results;
    max_itemsets_results.open (max_itemsets_results_path_.c_str(), fstream::in);
    std::string line;
    FrequentItemSetResultType frequentItemsets;
    while(std::getline(max_itemsets_results, line))
    {
        std::vector<std::string> item_strs;
        boost::algorithm::split( item_strs, line, boost::algorithm::is_any_of(" ") );
        if(item_strs.size() <= 1) continue; 
        std::vector<itemid_t> max_itemset;
        max_itemset.reserve(item_strs.size());
        for(std::vector<std::string>::iterator it = item_strs.begin();
              it != item_strs.end(); ++it)
        {
            try
            {
                itemid_t itemId = boost::lexical_cast< itemid_t >( *it );
                max_itemset.push_back(itemId);
            }catch( const boost::bad_lexical_cast & ){}
        }
        _judgeFrequentItemset(max_itemset, frequentItemsets);
    }
    std::sort(frequentItemsets.begin(), frequentItemsets.end(), itemSetCompare);
    FrequentItemSetResultType::iterator uniqueEnd = std::unique(frequentItemsets.begin(), frequentItemsets.end(), itemSetPredicate);
    frequentItemsets.resize(uniqueEnd - frequentItemsets.begin());
    izenelib::util::ScopedWriteLock<izenelib::util::ReadWriteLock> lock(result_lock_);
    frequent_itemsets_.swap(frequentItemsets);
}

void OrderManager::_judgeFrequentItemset(
    std::vector<itemid_t>& max_itemset , 
    FrequentItemSetResultType& frequent_itemsets)
{
    std::vector<itemid_t> sub_itemset(max_itemset.size());
    std::vector<itemid_t>::iterator last_item = sub_itemset.begin();
    while ( 
        ( last_item = izenelib::util::next_subset( max_itemset.begin(), max_itemset.end(),
                     sub_itemset.begin(), last_item ) )
            != sub_itemset.begin() ) {
        size_t size = last_item - sub_itemset.begin();
        if(size < 2) continue;
        std::vector<itemid_t> result(sub_itemset.begin(), last_item);
        size_t freq = item_order_index_.get(result);
        if(freq >= threshold_)
        {
            frequent_itemsets.push_back(make_pair(result, freq));
        }
    }
}

bool OrderManager::_saveFreqItemsetDb() const 
{
    try
    {
        std::ofstream ofs(frequent_itemsets_results_path_.c_str());
        if (ofs)
        {
            boost::archive::xml_oarchive oa(ofs);
            oa << boost::serialization::make_nvp(
                "FreqItemSets", frequent_itemsets_
            );
        }

        return ofs;
    }
    catch(boost::archive::archive_exception& e)
    {
        LOG(ERROR) << "IO ERROR:: Can not save frequent itemset DB";
        return false;
    }
}

bool OrderManager::_restoreFreqItemsetDb() 
{
    try
    {
        std::ifstream ifs(frequent_itemsets_results_path_.c_str());
        if (ifs)
        {
            boost::archive::xml_iarchive ia(ifs);
            ia >> boost::serialization::make_nvp(
                    "FreqItemSets", frequent_itemsets_
                    );
        }
        return ifs;
    }
    catch(boost::archive::archive_exception& e)
    {
        LOG(ERROR) << "IO ERROR:: Can not restore frequent itemset DB";
        return false;
    }
}

}
