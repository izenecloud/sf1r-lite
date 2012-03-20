#ifndef SF1R_B5MMANAGER_OFFERDB_H_
#define SF1R_B5MMANAGER_OFFERDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <glog/logging.h>

namespace sf1r {



    typedef izenelib::am::tc::BTree<std::string, std::string> OfferDb;
    //typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;

}

#endif

