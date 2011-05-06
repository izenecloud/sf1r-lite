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
        for(std::list<orderid_t>::iterator orderIt = orders.begin();
              orderIt != orders.end(); ++orderIt)
        {
            std::vector<itemid_t> items_in_order;
            if(_getOrder(*orderIt,items_in_order))
            {
                results.resize(results.size() + items_in_order.size());
                std::copy(items_in_order.begin(), items_in_order.end(), results.begin());
            }
        }
        results.remove_if( FilterItems(itemIdSet));
        return true;
    }
    else return false;
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
    fseek(order_key_, orderId, SEEK_SET);
    fread(&pos, 1, 4, order_key_);
    }
    size_t bytes_read;
    uint32_t vector_size;

    FILE* order_db = fopen(order_db_path_.c_str(), "rb" );
    fseek(order_db, pos, SEEK_SET);
    while ((bytes_read = fread(&orderId, 1, 4, order_db)) == 4)
    {
        bytes_read = fread(&vector_size, 1, 4, order_db);
        if (bytes_read != 4)
        {
            if (ferror(order_db))
                break;
            return -1;
        }
        items.resize(vector_size);
        bytes_read = fread(&(items[0]), 1, sizeof(itemid_t) * vector_size, order_db);
        if (bytes_read != sizeof(itemid_t) * vector_size)
        {
            if (ferror(order_db))
                break;
            fclose(order_db);
            return false;
        }
        fclose(order_db);
        return true;
    }
    if (ferror(order_db))
    {
        fclose(order_db);
        return false;
    }
    if (bytes_read != 0)
    {
        fclose(order_db);
        return false;
    }

    return false;
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
            itemid_t itemId = boost::lexical_cast< itemid_t >( *it );
            max_itemset.push_back(itemId);
        }
        _judgeFrequentItemset(max_itemset, frequentItemsets);
    }
    izenelib::util::ScopedWriteLock<izenelib::util::ReadWriteLock> lock(result_lock_);
    frequent_itemsets_.clear();
    frequent_itemsets_.swap(frequentItemsets);
}

void OrderManager::_judgeFrequentItemset(
    std::vector<itemid_t>& max_itemset , 
    FrequentItemSetResultType& frequent_itemsets)
{
    std::vector<itemid_t> sub_itemset(max_itemset.size());
    std::vector<itemid_t>::iterator last_item = max_itemset.begin();
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
