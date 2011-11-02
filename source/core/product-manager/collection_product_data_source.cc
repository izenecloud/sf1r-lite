#include "collection_product_data_source.h"
#include <document-manager/DocumentManager.h>
#include <index-manager/IndexManager.h>


using namespace sf1r;
using namespace izenelib::ir::indexmanager;
using namespace izenelib::ir::idmanager;

CollectionProductDataSource::CollectionProductDataSource(const boost::shared_ptr<DocumentManager>& document_manager, const boost::shared_ptr<IndexManager>& index_manager, const boost::shared_ptr<IDManager>& id_manager, const PMConfig& config, const std::set<PropertyConfig, PropertyComp>& schema)
: document_manager_(document_manager), index_manager_(index_manager), id_manager_(id_manager), config_(config), schema_(schema)
{
}
    
CollectionProductDataSource::~CollectionProductDataSource()
{
}

bool CollectionProductDataSource::GetDocument(uint32_t docid, PMDocumentType& doc)
{
    return document_manager_->getDocument(docid, doc);
}

void CollectionProductDataSource::GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid)
{
    IndexReader* index_reader = index_manager_->getIndexReader();
    
    uint32_t num_docs = index_reader->numDocs();
    uint32_t bits_num = num_docs + 1;
    BitVector bit_vector(bits_num);
    PropertyType value(uuid);
    index_manager_->getDocsByNumericValue(1, config_.uuid_property_name, value, bit_vector);
    
    for(uint32_t i=1;i<=num_docs;i++)
    {
        if(i==exceptid) continue;
        if(bit_vector.test(i))
        {
            docid_list.push_back(i);
        }
    }
    
//     EWAHBoolArray<uword32> docid_set;
//     bit_vector.compressed(docid_set);
//     EWAHBoolArrayBitIterator<uword32> it = docid_set.bit_iterator();
//     while(it.next())
//     {
//         docid_list.push_back(it.doc());
//     }
//     docid_list.erase( std::remove(docid_list.begin(), docid_list.end(), exceptid),docid_list.end());
}

bool CollectionProductDataSource::UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid)
{
    if(docid_list.empty()) return false;
    PropertyConfig property_config;
    property_config.propertyName_ = config_.uuid_property_name;
    std::set<PropertyConfig, PropertyComp>::iterator iter = schema_.find(property_config);
    if(iter==schema_.end()) return false;
    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
//     {
//         std::cout<<"XXXXProperty:"<<indexerPropertyConfig.getPropertyId()<<","<<indexerPropertyConfig.getName()
//         <<","<<(int)(indexerPropertyConfig.isIndex())<<","<<(int)(indexerPropertyConfig.isFilter())<<std::endl;
//     }
    std::vector<PMDocumentType> doc_list(docid_list.size());
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        if(!GetDocument(docid_list[i], doc_list[i]))
        {
            return false;
        }
    }
    std::vector<izenelib::util::UString> uuid_list(doc_list.size());
    for(uint32_t i=0;i<doc_list.size();i++)
    {
        PMDocumentType::property_const_iterator it = doc_list[i].findProperty(config_.uuid_property_name);
        if(it == doc_list[i].propertyEnd())
        {
            return false;
        }
        uuid_list[i] = it->second.get<izenelib::util::UString>();
    }
    
    //update DM first
    uint32_t index = 0;
    for(;index<docid_list.size();index++)
    {
        PMDocumentType newdoc;
        newdoc.setId(docid_list[index]);
        newdoc.property(config_.uuid_property_name) = uuid;
        if(!document_manager_->updatePartialDocument(newdoc))
        {
            //rollback in DM
            for(uint32_t i=0;i<=index;i++)
            {
                if(!document_manager_->updateDocument(doc_list[i]))
                {
                    //
                }
            }
            return false;
        }
    }
    //update IM
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        IndexerDocument oldIndexDocument;
        oldIndexDocument.setDocId(docid_list[i], 1);
        oldIndexDocument.insertProperty(indexerPropertyConfig, uuid_list[i]);
        IndexerDocument indexDocument;
        indexDocument.setDocId(docid_list[i], 1);
        indexDocument.insertProperty(indexerPropertyConfig, uuid);
//         {
//             std::string suid1;
//             std::string suid2;
//             uuid_list[i].convertString(suid1, izenelib::util::UString::UTF_8);
//             uuid.convertString(suid2, izenelib::util::UString::UTF_8);
//             std::cout<<"Update uuid in doc "<<docid_list[i]<<", from "<<suid1<<" to "<<suid2<<std::endl;
//         }
        if(!index_manager_->updateRtypeDocument(oldIndexDocument, indexDocument))
        {
            //TODO how to rollback in IM?
        }
    }
    return true;
}

bool CollectionProductDataSource::SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid)
{
    PropertyConfig property_config;
    property_config.propertyName_ = config_.uuid_property_name;
    std::set<PropertyConfig, PropertyComp>::iterator iter = schema_.find(property_config);
    if(iter==schema_.end()) return false;
    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
    doc.insertProperty(indexerPropertyConfig, uuid);
    return true;
}

bool CollectionProductDataSource::GetInternalDocidList(const std::vector<izenelib::util::UString>& sdocid_list, std::vector<uint32_t>& docid_list)
{
    docid_list.resize(sdocid_list.size());
    for(uint32_t i=0;i<sdocid_list.size();i++)
    {
        if(!id_manager_->getDocIdByDocName(sdocid_list[i], docid_list[i],false))
        {
            docid_list.clear();
            std::string sdocid;
            sdocid_list[i].convertString(sdocid, izenelib::util::UString::UTF_8);
            SetError_("Can not get docid for "+sdocid);
            return false;
        }
    }
    return true;
}


