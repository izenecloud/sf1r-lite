#ifndef SF1R_PRODUCTMANAGER_PRODUCTBACKUP_H
#define SF1R_PRODUCTMANAGER_PRODUCTBACKUP_H

#include <common/type_defs.h>


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include "product_backup_writer.h"
#include "product_backup_reader.h"

namespace sf1r
{

class ProductManager;

///@brief Not thread safe, controlled by ProductManager
class ProductBackup
{

    typedef ProductBackupWriter WriterType;
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
        ProductBackupDataType data;
        if(!ProductBackupReader::GetAllBackupData(file_, data))
        {
            std::cout<<"ProductBackupReader::GetAllBackupData failed"<<std::endl;
            return false;
        }
        if(!ProductBackupReader::CheckValidation(data))
        {
            std::cout<<"ProductBackupReader::CheckValidation failed"<<std::endl;
            return false;
        }

        ProductBackupDataType::iterator it = data.begin();
        while(it!=data.end())
        {
            izenelib::util::UString uuid(it->first, izenelib::util::UString::UTF_8);
            ProductBackupDataValueType& value = it->second;
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
        WriterType* writer = GetWriter_();
        if(writer==NULL)
        {
            return false;
        }
        return writer->AddPriceComparisonItem(uuid, docid_list, type);

    }

    bool AddProductUpdateItem(const PMDocumentType& doc)
    {
        WriterType* writer = GetWriter_();
        if(writer==NULL)
        {
            return false;
        }
        return writer->AddProductUpdateItem(doc);

    }

private:
    WriterType* GetWriter_()
    {
        if(writer_==NULL)
        {
            writer_ = new WriterType(file_);
            if(!writer_->Open())
            {
                delete writer_;
                writer_ = NULL;
            }
        }
        return writer_;
    }

private:
    std::string file_;
    WriterType* writer_;

};

}

#endif
