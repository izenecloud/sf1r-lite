#ifndef SF1R_PRODUCTMANAGER_PRODUCTBACKUPWRITER_H
#define SF1R_PRODUCTMANAGER_PRODUCTBACKUPWRITER_H

#include <3rdparty/json/json.h>
#include <boost/algorithm/string.hpp>
#include <common/type_defs.h>

#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include "product_backup_def.h"

namespace sf1r
{
    
class ProductBackupWriter
{
    
    
public:
    
    ProductBackupWriter(const std::string& file)
    :file_(file)
    {
        
    }
    
    ~ProductBackupWriter()
    {
        
    }
    
    bool Open()
    {
        ofs_.open(file_.c_str(), std::ofstream::app);
        if(ofs_.fail()) return false;
        return true;
    }

    
    bool AddPriceComparisonItem(const izenelib::util::UString& uuid, const std::vector<izenelib::util::UString>& docid_list, int type)
    {
        Json::Value json_value;
        Json::FastWriter writer;
        //create json_value from parameters
        json_value["type"] = "C";
        Json::Value& value = json_value["value"];
        value["type"] = Json::Value::Int(type);
        std::string suuid;
        uuid.convertString(suuid, izenelib::util::UString::UTF_8);
        value["uuid"] = suuid;
        Json::Value& doclist_value = value["docid_list"];
        doclist_value = Json::arrayValue;
        doclist_value.resize(docid_list.size());
        for(uint32_t i=0;i<docid_list.size();i++)
        {
            std::string sdocid;
            docid_list[i].convertString(sdocid, izenelib::util::UString::UTF_8);
            doclist_value[i] = sdocid;
        }
       
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        ofs_<<str_value<<std::endl;
        if(ofs_.fail()) return false;
        return true;
    }
    
    bool AddProductUpdateItem(const PMDocumentType& doc)
    {
        Json::Value json_value;
        Json::FastWriter writer;
        //create json_value from parameters
        json_value["type"] = "U";
        Json::Value& value = json_value["value"];
        PMDocumentType::property_const_iterator it = doc.propertyBegin();
        while(it!=doc.propertyEnd())
        {
            std::string key = it->first;
            const izenelib::util::UString& uvalue = it->second.get<izenelib::util::UString>();
            std::string svalue;
            uvalue.convertString(svalue, izenelib::util::UString::UTF_8);
            value[key] = svalue;
            ++it;
        }
       
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        ofs_<<str_value<<std::endl;
        if(ofs_.fail()) return false;
        return true;
    }
    
    void Close()
    {
        ofs_.close();
    }
        
private:
    std::string file_;
    std::ofstream ofs_;
    
};

}

#endif

