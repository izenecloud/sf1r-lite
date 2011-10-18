#include "collection_product_data_source.h"
#include <document-manager/DocumentManager.h>
#include <index-manager/IndexManager.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

CollectionProductDataSource::CollectionProductDataSource(const boost::shared_ptr<DocumentManager>& document_manager, const boost::shared_ptr<IndexManager>& index_manager, const PMConfig& config)
: document_manager_(document_manager), index_manager_(index_manager), config_(config)
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

