#ifndef SF1R_PRODUCTMANAGER_PRODUCTBACKUPREADER_H
#define SF1R_PRODUCTMANAGER_PRODUCTBACKUPREADER_H

#include <3rdparty/json/json.h>
#include <common/type_defs.h>
#include <boost/algorithm/string.hpp>

#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include "product_backup_def.h"

namespace sf1r
{
    

class ProductBackupReader
{
    
#define INVALID() std::cout<<"invalid json line : "<<line<<std::endl; return false;
#define EXPECT(json_value, json_type)   if(json_value.type()!=json_type)  {INVALID();}
    
public:
    
    ProductBackupReader(const std::string& file)
    :file_(file), ifs_(file.c_str()), DOCID("DOCID")
    {
    }

    ~ProductBackupReader()
    {
    }
    
    
    bool Next(ProductBackupItemType& ritem)
    {
        std::string line;
        while(getline( ifs_, line))
        {
            boost::algorithm::trim(line);
            if(line.length()!=0) break;;
        }
        if(line.empty()) return false;
        Json::Reader json_reader;
        Json::Value json_value;
        if(!json_reader.parse(line, json_value))
        {
            std::cout<<"Json parse error : "<<json_reader.getFormatedErrorMessages()<<std::endl;
            return false;
        }
        EXPECT(json_value, Json::objectValue);
        EXPECT(json_value["type"], Json::stringValue);
        std::string type = json_value["type"].asCString();
        if(type=="C") //price compare item
        {
            PriceComparisonItem item;
            EXPECT(json_value["value"], Json::objectValue);
            Json::Value& value = json_value["value"];
            EXPECT(value["type"], Json::intValue);
            item.type = value["type"].asInt();
            EXPECT(value["uuid"], Json::stringValue);
            std::string suuid = value["uuid"].asCString();
            item.uuid = izenelib::util::UString(suuid, izenelib::util::UString::UTF_8);
            EXPECT(value["docid_list"], Json::arrayValue);
            for(uint32_t i=0;i<value["docid_list"].size();i++)
            {
                std::string sdocid = value["docid_list"][i].asCString();
                izenelib::util::UString docid(sdocid, izenelib::util::UString::UTF_8);
                item.docid_list.push_back(docid);
            }
            ritem = item;
        }
        else if(type=="U") //document update item
        {
            ProductUpdateItem item;
            EXPECT(json_value["value"], Json::objectValue);
            Json::Value& obj = json_value["value"];
            for (Json::Value::iterator it = obj.begin(), itEnd = obj.end(); it != itEnd; ++it)
            {
                Json::Value& obj_item = (*it);
                EXPECT(obj_item, Json::stringValue);
                std::string property_name = it.key().asCString();
                std::string property_value = obj_item.asCString();
                item.doc.property(property_name) = izenelib::util::UString(property_value, izenelib::util::UString::UTF_8);
            }
            ritem = item;

        }
        else
        {
            INVALID();
        }
        return true;
    }
    
    void Close()
    {
        ifs_.close();
    }
    
    static bool GetAllBackupData(const std::string& path, ProductBackupDataType& data)
    {
        std::string DOCID = "DOCID";
        ProductBackupReader reader(path);
        ProductBackupItemType item;
        while(reader.Next(item))
        {
            if(item.which()==0)
            {
                PriceComparisonItem citem = boost::get<PriceComparisonItem>(item);
                std::string suuid;
                citem.uuid.convertString(suuid, izenelib::util::UString::UTF_8);
                ProductBackupDataType::iterator it = data.find(suuid);
                if(it==data.end())
                {
                    it = data.insert(std::make_pair(suuid, ProductBackupDataValueType())).first;
                }
                ProductBackupDataValueType& value = it->second;
                if(citem.type==1) //append
                {
                    value.docid_list.insert(value.docid_list.end(), citem.docid_list.begin(), citem.docid_list.end());
                    VectorDD_(value.docid_list);
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
                ProductBackupDataType::iterator it = data.find(suuid);
                if(it==data.end())
                {
                    it = data.insert(std::make_pair(suuid, ProductBackupDataValueType())).first;
                }
                ProductBackupDataValueType& value = it->second;
                value.doc_info = doc; //override all properties,  or keep the old properties which is not overrided?
            }
        }
        reader.Close();
        return true;
    }
    
    static bool CheckValidation(const ProductBackupDataType& data)
    {
        boost::unordered_map<std::string, std::string> docid2uuid;
        ProductBackupDataType::const_iterator it = data.begin();
        while(it!=data.end())
        {
            std::string suuid = it->first;
            const ProductBackupDataValueType& value = it->second;
            for(uint32_t i=0;i<value.docid_list.size();i++)
            {
                std::string sdocid;
                value.docid_list[i].convertString(sdocid, izenelib::util::UString::UTF_8);
                boost::unordered_map<std::string, std::string>::iterator map_it = docid2uuid.find(sdocid);
                if(map_it!=docid2uuid.end())
                {
                    std::cout<<"Find conflict error, docid : "<<sdocid<<std::endl;
                    return false;
                }
                docid2uuid.insert(std::make_pair(sdocid, suuid));
            }
            ++it;
        }
        return true;
    }
    
    static bool Compact(const std::string& path, bool check = true)
    {
        std::string DOCID = "DOCID";
        ProductBackupDataType data;
        if(!GetAllBackupData(path, data))
        {
            std::cout<<"Load data failed"<<std::endl;
            return false;
        }
        if(check)
        {
            if(!CheckValidation(data))
            {
                std::cout<<"check validation failed"<<std::endl;
                return false;
            }
        }
        ProductBackupWriter writer(path+".compact");
        ProductBackupDataType::iterator it = data.begin();
        while(it!=data.end())
        {
            std::string suuid = it->first;
            ProductBackupDataValueType& value = it->second;
            izenelib::util::UString uuid(suuid, izenelib::util::UString::UTF_8);
            writer.AddPriceComparisonItem(uuid, value.docid_list, 1);
            if(value.doc_info.hasProperty(DOCID) )
            {
                writer.AddProductUpdateItem(value.doc_info);
            }
            ++it;
        }
        writer.Close();
        return true;
    }
    
private:
    template <typename T>
    static void VectorRemove_(std::vector<T>& vec, const T& value)
    {
        vec.erase( std::remove(vec.begin(), vec.end(), value),vec.end());
    }
    
    template <typename T>
    static void VectorDD_(std::vector<T>& vec)
    {
        std::sort(vec.begin(), vec.end());
        typename std::vector<T>::iterator iter_end;
        iter_end = std::unique(vec.begin(), vec.end() );
        vec.erase(iter_end, vec.end() );
    }
        
private:
    std::string file_;
    std::ifstream ifs_;
    std::string DOCID;
};

}

#endif

