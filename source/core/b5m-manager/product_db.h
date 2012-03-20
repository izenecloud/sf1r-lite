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


    struct ProductProperty
    {
        typedef std::map<std::string, int> SourceMap;
        typedef izenelib::util::UString UString;
        ProductProperty()
        :itemcount(0)
        {
        }

        ProductProperty(const Document& doc)
        :itemcount(1)
        {
            UString usource;
            UString uprice;
            doc.getProperty("Source", usource);
            doc.getProperty("Price", uprice);
            price.Parse(uprice);
            std::string ssource;
            usource.convertString(ssource, UString::UTF_8);
            if(ssource.length()>0)
            {
                SourceMap::iterator it = source.find(ssource);
                if(it==source.end())
                {
                    source.insert(std::make_pair(ssource, 1));
                }
                else
                {
                    it->second += 1;
                }
            }
        }

        ProductPrice price;
        SourceMap source;
        uint32_t itemcount;

        template<class Archive> 
        void serialize(Archive& ar, const unsigned int version) 
        {
            ar & price & source & itemcount;
        }

        std::string GetSourceString() const
        {
            std::string result;
            for(SourceMap::const_iterator it = source.begin(); it!=source.end(); ++it)
            {
                if(it->second>0)
                {
                    if(!result.empty()) result+=",";
                    result += it->first;
                }
            }
            return result;
        }

        izenelib::util::UString GetSourceUString() const
        {
            return izenelib::util::UString(GetSourceString(), izenelib::util::UString::UTF_8);
        }

        ProductProperty& operator+=(const ProductProperty& other)
        {
            price += other.price;
            for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
            {
                SourceMap::iterator it = source.find(oit->first);
                if(it==source.end())
                {
                    source.insert(std::make_pair(oit->first, 1));
                }
                else
                {
                    it->second += 1;
                }
            }
            itemcount+=1;
            return *this;
        }

        ProductProperty& operator-=(const ProductProperty& other)
        {
            for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
            {
                SourceMap::iterator it = source.find(oit->first);
                if(it==source.end())
                {
                    //invalid data
                    LOG(WARNING)<<"invalid data"<<std::endl;
                }
                else
                {
                    it->second -= 1;
                    if(it->second<0)
                    {
                        //invalid data
                        LOG(WARNING)<<"invalid data"<<std::endl;
                        it->second = 0;
                    }
                }
            }
            if(itemcount>1)
            {
                itemcount-=1;
            }
            else
            {
                //invalid data
                LOG(WARNING)<<"invalid data"<<std::endl;
            }
            //TODO how to process price?

            return *this;
        }
    };

    typedef izenelib::am::tc::BTree<std::string, ProductProperty> ProductDb;
    //typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;

}

#endif

