#ifndef SF1R_PRODUCTMANAGER_PRODUCTBACKUP_H
#define SF1R_PRODUCTMANAGER_PRODUCTBACKUP_H

#include <common/type_defs.h>


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include <boost/variant.hpp>
#include <boost/serialization/variant.hpp>
#include <am/sequence_file/ssfr.h>

namespace sf1r
{
    
class ProductManager;

///@brief Not thread safe, controlled by ProductManager
class ProductBackup
{
    struct PriceComparisonItem
    {
        izenelib::util::UString uuid;
        std::vector<izenelib::util::UString> docid_list;
        int type; // 1 means append, 2 means remove
        
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & uuid & docid_list & type;
        }
    };
    
    struct ProductUpdateItem
    {
        PMDocumentType doc;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & doc;
        }
    };
    
    typedef boost::variant<PriceComparisonItem, ProductUpdateItem> ItemType;
    
    
    struct ValueType
    {
        std::vector<izenelib::util::UString> docid_list;
        PMDocumentType doc_info;
    };
    
public:
    
    ProductBackup(const std::string& file)
    :file_(file), writer_(NULL)
    {
    }

    ~ProductBackup()
    {
    }
    
//     bool Open()
//     {
//         data_.clear();
//         izenelib::am::ssf::Reader<> reader(file_);
//         if(!reader.Open()) return false;
//         izenelib::util::UString key;//uuid
//         ValueType value;
//         while(reader.Next(key, value))
//         {
//             std::string str_key;
//             key.convertString(str_key, izenelib::util::UString::UTF_8);
//             data_.insert(std::make_pair(str_key, value));
//         }
//         reader.Close();
//         
//     }
    
    bool Recover(ProductManager* pm)
    {
        std::string DOCID = "DOCID";
        izenelib::am::ssf::Reader<> reader(file_);
        if(!reader.Open()) return false;
        ItemType item;
        boost::unordered_map<std::string, ValueType> data;
        while(reader.Next(item))
        {
            if(item.which()==0)
            {
                PriceComparisonItem citem = boost::get<PriceComparisonItem>(item);
                std::string suuid;
                citem.uuid.convertString(suuid, izenelib::util::UString::UTF_8);
                boost::unordered_map<std::string, ValueType>::iterator it = data.find(suuid);
                if(it==data.end())
                {
                    it = data.insert(std::make_pair(suuid, ValueType())).first;
                }
                ValueType& value = it->second;
                if(citem.type==1) //append
                {
                    value.docid_list.insert(value.docid_list.end(), citem.docid_list.begin(), citem.docid_list.end());
                }
                else //remove
                {
                    for(uint32_t i=0;i<citem.docid_list.size();i++)
                    {
                        VectorRemove_(value.docid_list, citem.docid_list[i]);
                    }
                }
            }
            else
            {
                ProductUpdateItem uitem = boost::get<ProductUpdateItem>(item);
                Document& doc = uitem.doc;
                izenelib::util::UString uuid = doc.property(DOCID).get<izenelib::util::UString>();
                std::string suuid;
                uuid.convertString(suuid, izenelib::util::UString::UTF_8);
                boost::unordered_map<std::string, ValueType>::iterator it = data.find(suuid);
                if(it==data.end())
                {
                    it = data.insert(std::make_pair(suuid, ValueType())).first;
                }
                ValueType& value = it->second;
                value.doc_info = doc;
            }
        }
        reader.Close();
        
        boost::unordered_map<std::string, ValueType>::iterator it = data.begin();
        while(it!=data.end())
        {
            izenelib::util::UString uuid(it->first, izenelib::util::UString::UTF_8);
            ValueType& value = it->second;
            Document& doc = value.doc_info;
            Document::property_const_iterator pit = doc.findProperty(DOCID);
            if(pit == doc.propertyEnd())
            {
                doc.property(DOCID) = uuid;
            }
            if(!pm->AddGroupWithInfo(value.docid_list, doc, false)) //do not backup again
            {
                std::cout<<pm->GetLastError()<<std::endl;
                return false;
            }
            ++it;
        }
        return true;
    }
    
    bool AddPriceComparisonItem(const izenelib::util::UString& uuid, const std::vector<izenelib::util::UString>& docid_list, int type)
    {
        PriceComparisonItem citem;
        citem.uuid = uuid;
        citem.docid_list = docid_list;
        citem.type = type;
        ItemType item = citem;
        return WriteItem_(item);

        
    }
    
    bool AddProductUpdateItem(const PMDocumentType& doc)
    {
        ProductUpdateItem uitem;
        uitem.doc = doc;
        ItemType item = uitem;
        return WriteItem_(item);
        
        
        
    }
    
private:
    izenelib::am::ssf::Writer<>* GetWriter_()
    {
        if(writer_==NULL)
        {
            writer_ = new izenelib::am::ssf::Writer<>(file_);
            if(!writer_->Open())
            {
                delete writer_;
                writer_ = NULL;
            }
        }
        return writer_;
    }
    
    
    bool WriteItem_(const ItemType& item)
    {
        izenelib::am::ssf::Writer<>* writer = GetWriter_();
        if(writer==NULL)
        {
            return false;
        }
        if(!writer->Append(item)) return false;
        writer->Flush();
        return true;
    }
    
    template <typename T>
    void VectorRemove_(std::vector<T>& vec, const T& value)
    {
        vec.erase( std::remove(vec.begin(), vec.end(), value),vec.end());
    }
    
        
private:
    std::string file_;
    izenelib::am::ssf::Writer<>* writer_;
    
};

}

#endif

