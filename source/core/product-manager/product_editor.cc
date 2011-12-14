#include "product_editor.h"
#include "product_data_source.h"
#include "operation_processor.h"
#include "uuid_generator.h"
#include <glog/logging.h>
using namespace sf1r;

#define PM_EDIT_INFO

ProductEditor::ProductEditor(ProductDataSource* data_source, OperationProcessor* op_processor,const PMConfig& config)
: data_source_(data_source), op_processor_(op_processor), config_(config), util_(config, data_source)
{
}

ProductEditor::~ProductEditor()
{
}

//update documents in A, so need transfer
bool ProductEditor::UpdateADoc(const PMDocumentType& doc)
{
    std::vector<PMDocumentType> doc_list;
    return AppendToGroup_(doc_list, doc);
}

//all intervention functions.
bool ProductEditor::AddGroup(const std::vector<uint32_t>& docid_list, PMDocumentType& info, const ProductEditOption& option)
{
    if (docid_list.size()<2)
    {
        error_ = "Docid list size must larger than 1";
        return false;
    }
    UString uuid;
    if( info.getProperty(config_.docid_property_name, uuid) )
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        if (!same_docid_list.empty())
        {
            std::string suuid;
            uuid.convertString(suuid, izenelib::util::UString::UTF_8);
            error_ = "UUID "+suuid+" was exists";
            return false;
        }
    }
    else
    {
        while(true)
        {
            UuidGenerator::Gen(uuid);
            std::vector<uint32_t> same_docid_list;
            data_source_->GetDocIdList(uuid, same_docid_list, 0);
            if (same_docid_list.empty())
            {
                break;
            }
        }
        info.property(config_.docid_property_name) = uuid;
    }
    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }
    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid_list[i], same_docid_list, docid);
        if (!same_docid_list.empty())
        {
            if( !option.force )
            {
#ifdef PM_EDIT_INFO
                for(uint32_t s=0;s<same_docid_list.size();s++)
                {
                    LOG(INFO)<<"Uuid in : "<<uuid_list[i]<<" , "<<same_docid_list[s]<<std::endl;
                }
#endif
                error_ = "Document id "+boost::lexical_cast<std::string>(docid_list[i])+" belongs to other group";
                return false;
            }
            else
            {
                LOG(WARNING)<<"Document id "<<docid_list[i]<<" belongs to other group";
            }
        }
    }
    
    return AppendToGroup_(doc_list, info);
}

bool ProductEditor::AppendToGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        if (same_docid_list.empty())
        {
            std::string suuid;
            uuid.convertString(suuid, izenelib::util::UString::UTF_8);
            error_ = "UUID "+suuid+" was not exists";
            return false;
        }
    }
    PMDocumentType info;
    info.property(config_.docid_property_name) = uuid;

    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }
    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid_list[i], same_docid_list, docid);
        if (!same_docid_list.empty())
        {
            if( !option.force )
            {
                error_ = "Document id "+boost::lexical_cast<std::string>(docid_list[i])+" belongs to other group";
                return false;
            }
        }
    }
    
    return AppendToGroup_(doc_list, info);
}

bool ProductEditor::AppendToGroup_(const std::vector<PMDocumentType>& doc_list,  const PMDocumentType& info)
{
    //do not make any check in this function
    UString uuid;
    info.getProperty(config_.docid_property_name, uuid);
    PMDocumentType new_doc(info);
    std::vector<uint32_t> uuid_docid_list;
    int type = 1; //insert
    {
        data_source_->GetDocIdList(uuid, uuid_docid_list, 0);
        if (!uuid_docid_list.empty())
        {
            type = 2; //update
        }
    }
    for(uint32_t i=0;i<doc_list.size();i++)
    {
        const PMDocumentType& doc = doc_list[i];
        uint32_t docid = doc.getId();
        UString doc_uuid;
        doc.getProperty(config_.uuid_property_name, doc_uuid);
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(doc_uuid, same_docid_list, docid);
        if( same_docid_list.empty())
        {
            //this docid not belongs to any group
            
            //need to delete
            PMDocumentType del_doc;
            del_doc.property(config_.docid_property_name) = doc_uuid;
#ifdef PM_EDIT_INFO
            LOG(INFO)<<"Output : "<<3<<" , "<<doc_uuid<<std::endl;
#endif
            op_processor_->Append(3, del_doc);
            util_.AddPrice(new_doc, doc);
            
        }
        else
        {
            ProductPrice update_price;
            util_.GetPrice(same_docid_list, update_price);
            PMDocumentType update_doc;
            update_doc.property(config_.docid_property_name) = doc_uuid;
#ifdef PM_EDIT_INFO
            LOG(INFO)<<"Output : "<<2<<" , "<<doc_uuid<<std::endl;
#endif
            op_processor_->Append(2, update_doc);
            util_.AddPrice(new_doc, doc);
        }
        
        //update managers.
        std::vector<uint32_t> update_docid_list(1, docid);
#ifdef PM_EDIT_INFO
        LOG(INFO)<<"Updating uuid : "<<docid<<" , "<<uuid<<std::endl;
#endif
        if (!data_source_->UpdateUuid(update_docid_list, uuid))
        {
            error_ = "Update uuid failed";
            return false;
        }
    }
    data_source_->Flush();
#ifdef PM_EDIT_INFO
    LOG(INFO)<<"Output : "<<type<<" , "<<uuid<<std::endl;
#endif
    op_processor_->Append(type, new_doc);
    return true;
}

bool ProductEditor::RemoveFromGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
    error_ = "Remove not supported yet";
    return false;
}
