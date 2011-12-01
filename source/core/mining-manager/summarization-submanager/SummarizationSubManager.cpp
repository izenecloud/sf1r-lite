#include "SummarizationSubManager.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <iostream>

using namespace izenelib::ir::indexmanager;
using izenelib::util::UString;
namespace sf1r
{
MultiDocSummarizationSubManager::MultiDocSummarizationSubManager(
    const std::string& homePath,
    SummarizeConfig schema,
    boost::shared_ptr<DocumentManager> document_manager,
    boost::shared_ptr<IndexManager> index_manager)
    :schema_(schema)
    ,document_manager_(document_manager)
    ,index_manager_(index_manager)
{
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
}

void MultiDocSummarizationSubManager::ComputeSummarization()
{
    BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
    typedef BTreeIndexerManager::Iterator<UString> IteratorType;
    IteratorType it = pBTreeIndexer->begin<UString>(schema_.externalKeyPropName);
    IteratorType itEnd = pBTreeIndexer->end<UString>();
    for(; it != itEnd; ++it)
    {
        //const std::vector<uint32_t>& docs = it->second;
        //const UString& key = it->first;
    }

}

}
