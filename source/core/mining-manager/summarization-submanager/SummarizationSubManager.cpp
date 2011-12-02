#include "SummarizationSubManager.h"
#include "ParentKeyStorage.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <common/ScdParser.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <glog/logging.h>

#include <iostream>
#include <vector>

using namespace izenelib::ir::indexmanager;
namespace bfs = boost::filesystem;

namespace sf1r
{

static const UString DOCID("DOCID", UString::UTF_8);

bool CheckParentKeyLogFormat(
        const SCDDocPtr& doc,
        const UString& parent_key_name)
{
    if (doc->size() != 2) return false;
    const UString& first = (*doc)[0].first;
    const UString& second = (*doc)[1].first;
    //TODO case insensitive compare, but it requires extra string conversion,
    //which introduces unnecessary memory fragments
    return (first == DOCID && second == parent_key_name);
}

MultiDocSummarizationSubManager::MultiDocSummarizationSubManager(
        const std::string& homePath,
        SummarizeConfig schema,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager)
    : schema_(schema)
    , document_manager_(document_manager)
    , index_manager_(index_manager)
    , parent_key_ustr_name_(schema_.parentKey, UString::UTF_8)
{
    if (!schema_.parentKeyLogPath.empty())
        boost::filesystem::create_directories(schema_.parentKeyLogPath);

    parent_key_storage_ = new ParentKeyStorage(homePath + "/parentkey");
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
    delete parent_key_storage_;
}

void MultiDocSummarizationSubManager::EvaluateSummarization()
{
    BuildIndexOfParentKey_();
    BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
    typedef BTreeIndexerManager::Iterator<UString> IteratorType;
    IteratorType it = pBTreeIndexer->begin<UString>(schema_.foreignKeyPropName);
    IteratorType itEnd = pBTreeIndexer->end<UString>();
    for (; it != itEnd; ++it)
    {
        //const std::vector<uint32_t>& docs = it->second;
        //const UString& key = it->first;
    }
}

void MultiDocSummarizationSubManager::BuildIndexOfParentKey_()
{
    if (schema_.parentKeyLogPath.empty()) return;
    ScdParser parser(UString::UTF_8);
    std::vector<std::string> scdList;
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(schema_.parentKeyLogPath); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename();
            if (parser.checkSCDFormat(fileName))
            {
                scdList.push_back(itr->path().string());
            }
            else
            {
                LOG(WARNING) << "SCD File not valid " << fileName;
            }
        }
    }

    std::vector<std::string>::iterator scd_it = scdList.begin();

    for (; scd_it != scdList.end(); ++scd_it)
    {
        size_t pos = scd_it->rfind("/") + 1;
        string filename = scd_it->substr(pos);

        LOG(INFO) << "Processing SCD file. " << bfs::path(*scd_it).stem();

        switch (parser.checkSCDType(*scd_it))
        {
        case INSERT_SCD:
            DoInsertBuildIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Indexing Finished";
            break;
        case DELETE_SCD:
            DoDelBuildIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Delete Finished";
            break;
        case UPDATE_SCD:
            DoUpdateIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Update Finished";
            break;
        default:
            break;
        }
        parser.load(*scd_it);
    }
    parent_key_storage_->Flush();

    bfs::path bkDir = bfs::path(schema_.parentKeyLogPath) / "backup";
    bfs::create_directory(bkDir);
    LOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;

    for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
    {
        try
        {
            bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
        }
        catch (bfs::filesystem_error& e)
        {
            LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
        }
    }
}

void MultiDocSummarizationSubManager::DoInsertBuildIndexOfParentKey_(
        const std::string& fileName)
{
    ScdParser parser(UString::UTF_8);
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        if (*doc_iter == NULL)
        {
            LOG(WARNING) << "SCD File not valid.";
            return;
        }
        SCDDocPtr doc = (*doc_iter);
        if (!CheckParentKeyLogFormat(doc, parent_key_ustr_name_))
            continue;
        parent_key_storage_->AppendUpdate((*doc)[1].second, (*doc)[0].second);
    }
}

void MultiDocSummarizationSubManager::DoUpdateIndexOfParentKey_(
        const std::string& fileName)
{
}

void MultiDocSummarizationSubManager::DoDelBuildIndexOfParentKey_(
        const std::string& fileName)
{
}

}