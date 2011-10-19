#include "product_manager.h"
#include "product_data_source.h"
#include "operation_processor.h"
#include "uuid_generator.h"

using namespace sf1r;

ProductManager::ProductManager(ProductDataSource* data_source, OperationProcessor* op_processor, const PMConfig& config)
:data_source_(data_source), op_processor_(op_processor), uuid_gen_(new UuidGenerator()), config_(config)
{
}

ProductManager::~ProductManager()
{
    delete uuid_gen_;
}


bool ProductManager::HookInsert(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document)
{
    izenelib::util::UString uuid = uuid_gen_->Gen(doc);
    if(!data_source_->SetUuid(index_document, uuid)) return false;
    PMDocumentType new_doc(doc);
    doc.property(config_.uuid_property_name) = uuid;
    new_doc.property(config_.docid_property_name) = uuid;
    SetItemCount_(new_doc, 1);
    op_processor_->Append(1, new_doc);
    return true;
}

bool ProductManager::HookUpdate(PMDocumentType& to, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type)
{
    uint32_t fromid = index_document.getId(); //oldid
    PMDocumentType from;
    if(!data_source_->GetDocument(fromid, from)) return false;
    izenelib::util::UString from_uuid;
    if(!GetUuid_(from, from_uuid)) return false;
    std::vector<uint32_t> docid_list;
    data_source_->GetDocIdList(from_uuid, docid_list, fromid); // except from.docid
    if(docid_list.empty()) // the from doc is unique, so delete it and insert 'to'
    {
        if(!data_source_->SetUuid(index_document, from_uuid)) return false;
        PMDocumentType new_doc(to);
        to.property(config_.uuid_property_name) = from_uuid;
        new_doc.property(config_.docid_property_name) = from_uuid;
        SetItemCount_(new_doc, 1);
        op_processor_->Append( 2, new_doc);// if r_type, only numberic properties in 'to'
    }
    else
    {
        //need not to update(insert) to.uuid, 
        ProductPrice from_price;
        ProductPrice to_price;
        if(!GetPrice_(fromid, from_price)) return false;
        if(!GetPrice_(to, to_price)) return false;
        if(!data_source_->SetUuid(index_document, from_uuid)) return false;
        to.property(config_.uuid_property_name) = from_uuid;
        //update price only
        if(from_price!=to_price)
        {
            PMDocumentType diff_properties;
            ProductPrice price(to_price);
            GetPrice_(docid_list, price);
            diff_properties.property(config_.price_property_name) = price.ToUString();
            diff_properties.property(config_.docid_property_name) = from_uuid;
            //auto r_type? itemcount no need?
//                 diff_properties.property(config_.itemcount_property_name) = izenelib::util::UString(boost::lexical_cast<std::string>(docid_list.size()+1), izenelib::util::UString::UTF_8);
            
            op_processor_->Append( 2, diff_properties );
        }
    }
    return true;
}

bool ProductManager::HookDelete(uint32_t docid)
{
    PMDocumentType from;
    if(!data_source_->GetDocument(docid, from)) return false;
    izenelib::util::UString from_uuid;
    if(!GetUuid_(from, from_uuid)) return false;
    std::vector<uint32_t> docid_list;
    data_source_->GetDocIdList(from_uuid, docid_list, docid); // except from.docid
    if(docid_list.empty()) // the from doc is unique, so delete it in A
    {
        PMDocumentType del_doc;
        del_doc.property(config_.docid_property_name) = from_uuid;
        op_processor_->Append(3, del_doc);
    }
    else
    {
        PMDocumentType diff_properties;
        ProductPrice price;
        GetPrice_(docid_list, price);
        diff_properties.property(config_.price_property_name) = price.ToUString();
        diff_properties.property(config_.docid_property_name) = from_uuid;
        SetItemCount_(diff_properties, docid_list.size());
        op_processor_->Append(2, diff_properties);
    }
    return true;
}

void ProductManager::GenOperations()
{
    op_processor_->Finish();

    /// M
    /*
    void ProductManager::onProcessed(bool success)
    {
        // action after SCDs have been processed
    }

    DistributedProcessSynchronizer dsSyn;
    std::string scdDir = "/tmp/hdfs/scd"; // generated scd files in scdDir
    dsSyn.generated(scdDir);
    dsSyn.watchProcess(boost::bind(&ProductManager::onProcessed, this,_1));
    */

    /// A
    /*
    class Receiver {
    public:
        Receiver()
        {
            dsSyn.watchGenerate(boost::bind(&Receiver::onGenerated, this,_1));
        }

        void Receiver::onGenerated(const std::string& s)
        {
            // process SCDs

            dsSyn.processed(true);
        }

        DistributedProcessSynchronizer dsSyn;
    }
    */
}

bool ProductManager::AddGroup(const std::vector<uint32_t>& docid_list, izenelib::util::UString& gen_uuid)
{
    std::cout<<"ProductManager::AddGroup"<<std::endl;
    if(docid_list.empty()) return false;
    std::vector<PMDocumentType> doc_list(docid_list.size());
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        if(!data_source_->GetDocument(docid_list[i], doc_list[i]))
        {
            error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
            return false;
        }
    }
    std::vector<izenelib::util::UString> uuid_list(doc_list.size());
    for(uint32_t i=0;i<doc_list.size();i++)
    {
        if(!GetUuid_(doc_list[i], uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid_list[i]);
            return false;
        }
//         if(!GetDOCID_(doc_list[i], str_docid_list[i]))
//         {
//             error_ = "Can not get DOCID in document "+boost::lexical_cast<std::string>(docid_list[i]);
//             return false;
//         }
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid_list[i], same_docid_list, docid_list[i]);
        if(!same_docid_list.empty())
        {
            error_ = "Document id "+boost::lexical_cast<std::string>(docid_list[i])+" belongs to other group";
            return false;
        }
    }
    //validation finished here.
    PMDocumentType output(doc_list[0]);
    output.property(config_.docid_property_name) = izenelib::util::UString("", izenelib::util::UString::UTF_8);
    SetItemCount_(output, doc_list.size());
    output.eraseProperty(config_.uuid_property_name);
    izenelib::util::UString uuid = uuid_gen_->Gen(output);
    output.property(config_.docid_property_name) = uuid;
    //update DM and IM first
    if(!data_source_->UpdateUuid(docid_list, uuid))
    {
        error_ = "Update uuid failed";
        return false;
    }
    //output generated here.
    for(uint32_t i=0;i<uuid_list.size();i++)
    {
        PMDocumentType del_doc;
        del_doc.property(config_.docid_property_name) = uuid_list[i];
        op_processor_->Append(3, del_doc);
    }
    op_processor_->Append(1, output);
    gen_uuid = uuid;
    //update doc in DM and IM
    return true;
}

bool ProductManager::GetPrice_(uint32_t docid, ProductPrice& price)
{
    PMDocumentType doc;
    if(!data_source_->GetDocument(docid, doc)) return false;
    return GetPrice_(doc, price);
}

bool ProductManager::GetPrice_(const PMDocumentType& doc, ProductPrice& price)
{
    Document::property_const_iterator it = doc.findProperty(config_.price_property_name);
    if(it == doc.propertyEnd())
    {
        return false;
    }
    izenelib::util::UString price_str = it->second.get<izenelib::util::UString>();
    if(!price.Parse(price_str)) return false;
    return true;
}

void ProductManager::GetPrice_(const std::vector<uint32_t>& docid_list, ProductPrice& price)
{
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        ProductPrice p;
        if(GetPrice_(docid_list[i], p))
        {
            price += p;
        }
    }
}

bool ProductManager::GetUuid_(const PMDocumentType& doc, izenelib::util::UString& uuid)
{
    PMDocumentType::property_const_iterator it = doc.findProperty(config_.uuid_property_name);
    if(it == doc.propertyEnd())
    {
        return false;
    }
    uuid = it->second.get<izenelib::util::UString>();
    return true;
}

bool ProductManager::GetDOCID_(const PMDocumentType& doc, izenelib::util::UString& docid)
{
    PMDocumentType::property_const_iterator it = doc.findProperty(config_.docid_property_name);
    if(it == doc.propertyEnd())
    {
        return false;
    }
    docid = it->second.get<izenelib::util::UString>();
    return true;
}

void ProductManager::SetItemCount_(PMDocumentType& doc, uint32_t item_count)
{
    doc.property(config_.itemcount_property_name) = izenelib::util::UString(boost::lexical_cast<std::string>(item_count), izenelib::util::UString::UTF_8);
}


