#ifndef SF1R_B5MMANAGER_PRODUCTDB_H_
#define SF1R_B5MMANAGER_PRODUCTDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include <document-manager/Document.h>
#include <document-manager/ScdDocument.h>
#include "b5m_types.h"
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <glog/logging.h>
#include <boost/unordered_set.hpp>

namespace sf1r {

    class B5mpDocGenerator
    {
    public:
        B5mpDocGenerator();
        void Gen(const std::vector<ScdDocument>& odocs, ScdDocument& pdoc, bool spu_only=false);

    private:
        boost::unordered_set<std::string> sub_doc_props_;
    };

    class ProductProperty
    {
    public:
        //typedef std::map<std::string, int32_t> SourceMap;
        typedef boost::unordered_set<std::string> SourceType;
        typedef Document::doc_prop_value_strtype StringType;
        typedef std::map<std::string, std::vector<std::string> > AttributeType;

        Document::doc_prop_value_strtype productid;
        ProductPrice price;
        SourceType source;
        int64_t itemcount;
        StringType oid;
        bool independent;
        AttributeType attribute;
        std::string date;

        ProductProperty();

        bool Parse(const Document& doc);

        void Set(Document& doc) const;

        void SetIndependent();


        std::string GetSourceString() const;

        izenelib::util::UString GetSourceUString() const;
        izenelib::util::UString GetAttributeUString() const;

        ProductProperty& operator+=(const ProductProperty& other);

        //ProductProperty& operator-=(const ProductProperty& other);

        std::string ToString() const;
    };


}

#endif

