#ifndef SF1R_B5MMANAGER_PRODUCTDB_H_
#define SF1R_B5MMANAGER_PRODUCTDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include <document-manager/Document.h>
#include "b5m_types.h"
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <glog/logging.h>

namespace sf1r {


    class ProductProperty
    {
    public:
        typedef std::map<std::string, int32_t> SourceMap;
        typedef izenelib::util::UString UString;

        ProductPrice price;
        SourceMap source;
        int32_t itemcount;

        ProductProperty();
        ProductProperty(const Document& doc);

        template<class Archive> 
        void serialize(Archive& ar, const unsigned int version) 
        {
            ar & price & source & itemcount;
        }

        std::string GetSourceString() const;

        izenelib::util::UString GetSourceUString() const;

        ProductProperty& operator+=(const ProductProperty& other);

        ProductProperty& operator-=(const ProductProperty& other);

        std::string ToString() const;
    };

    //typedef izenelib::am::tc::BTree<std::string, ProductProperty> ProductDb;
    typedef izenelib::am::leveldb::Table<std::string, ProductProperty> ProductDb;

}

#endif

