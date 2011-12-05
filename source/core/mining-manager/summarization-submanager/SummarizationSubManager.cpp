#include "SummarizationSubManager.h"
#include "ParentKeyStorage.h"
#include "SummarizationStorage.h"
#include "splm.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <common/ScdParser.h>
#include <la/analyzer/MultiLanguageAnalyzer.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/compare.hpp>

#include <glog/logging.h>

#include <iostream>
#include <vector>
#include <algorithm>

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

struct IsParentKeyFilterProperty
{
    const std::string& parent_key_property;
    IsParentKeyFilterProperty(const std::string& property)
        :parent_key_property(property) {}
    bool operator() (QueryFiltering::FilteringType& filterType)
    {
        return boost::is_iequal()(parent_key_property,filterType.first.second);
    }
};


MultiDocSummarizationSubManager::MultiDocSummarizationSubManager(
        const std::string& homePath,
        SummarizeConfig schema,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager)
    : schema_(schema)
    , document_manager_(document_manager)
    , index_manager_(index_manager)
    , parent_key_ustr_name_(schema_.parentKey, UString::UTF_8)
    , corpus_(new Corpus())
{
    if (!schema_.parentKeyLogPath.empty())
        boost::filesystem::create_directories(schema_.parentKeyLogPath);

    parent_key_storage_ = new ParentKeyStorage(homePath + "/parentkey");
    summarization_storage_ = new SummarizationStorage(homePath + "/summarization");
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
    delete parent_key_storage_;
    delete summarization_storage_;
}

void MultiDocSummarizationSubManager::EvaluateSummarization()
{
    BuildIndexOfParentKey_();
    BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
    if (schema_.parentKeyLogPath.empty())
    {
        ///No parentKey, directly build summarization
        typedef BTreeIndexerManager::Iterator<UString> IteratorType;
        IteratorType it = pBTreeIndexer->begin<UString>(schema_.foreignKeyPropName);
        IteratorType itEnd = pBTreeIndexer->end<UString>();
        for (; it != itEnd; ++it)
        {
            std::vector<uint32_t> docs(it->second);
            std::sort(docs.begin(), docs.end());
            const UString& key = it->first;
            DoEvaluateSummarization_(key, docs);
        }
    }
    else
    {
        ///For each parentKey, get all its values(foreign key)
        ParentKeyStorage::ParentKeyIteratorType parentKeyIt(parent_key_storage_->parent_key_db_);
        ParentKeyStorage::ParentKeyIteratorType parentKeyEnd;
        for (; parentKeyIt != parentKeyEnd; ++parentKeyIt)
        {
            std::vector<uint32_t> docs;
            const std::vector<UString>& foreignKeys = parentKeyIt->second;
            std::vector<UString>::const_iterator fit = foreignKeys.begin();
            for (; fit != foreignKeys.end(); ++fit)
            {
                PropertyType foreignKey(*fit);
                pBTreeIndexer->getValue(schema_.foreignKeyPropName, foreignKey, docs);
            }
            std::sort(docs.begin(), docs.end());
            DoEvaluateSummarization_(*fit, docs);
        }
    }
    summarization_storage_->Flush	();
}

void MultiDocSummarizationSubManager::DoEvaluateSummarization_(
        const UString& key,
        const std::vector<uint32_t>& docs)
{
    Summarization summarization(docs);
    if(! summarization_storage_->IsRebuildSummarizeRequired(key, summarization))
        return;

    ilplib::langid::Analyzer* langIdAnalyzer = document_manager_->getLangId();

    corpus_->start_new_coll(key);

    for (uint32_t i = 0; i < docs.size(); i++)
    {
        Document doc;
        document_manager_->getDocument(docs[i], doc);
        Document::property_const_iterator it = doc.findProperty(schema_.contentPropName);
        if (it == doc.propertyEnd())
            continue;

        const UString& content = it->second.get<UString>();
        UString sentence;
        std::size_t startPos = 0;
        while (std::size_t len = langIdAnalyzer->sentenceLength(content, startPos))
        {
            sentence.assign(content, startPos, len);

            corpus_->start_new_sent(sentence);

            //TODO word-segmentation

            startPos += len;
        }
    }

    corpus_->start_new_sent();
    corpus_->start_new_coll();

    std::vector<std::pair<UString, std::vector<UString> > > summary_list;
    SPLM::generateSummary(summary_list, *corpus_);

    //TODO store the generated summary list
    summarization_storage_->Update(key, summarization);

    corpus_->reset();
}

void MultiDocSummarizationSubManager::AppendSearchFilter(
        std::vector<QueryFiltering::FilteringType>& filtingList)
{
    ///When search filter is based on ParentKey, get its associated values,
    ///and add those values to filter conditions.
    ///The typical situation of this happen when :
    ///SELECT * FROM comments WHERE product_type="XXX"
    ///This hook will translate the semantic into:
    ///SELECT * FROM comments WHERE product_id="1" OR product_id="2" ...

    typedef std::vector<QueryFiltering::FilteringType>::iterator IteratorType;
    IteratorType it = std::find_if (filtingList.begin(),
        filtingList.end(), IsParentKeyFilterProperty(schema_.parentKey));
    if (it != filtingList.end())
    {
        const std::vector<PropertyValue>& filterParam = it->second;
        if (!filterParam.empty())
        {
            try
            {
                const std::string & paramValue = get<std::string>(filterParam[0]);
                UString paramUStr(paramValue, UString::UTF_8);
                std::vector<UString> results;
                if (parent_key_storage_->Get(paramUStr, results))
                {
                    QueryFiltering::FilteringType filterRule;
                    filterRule.first.first = QueryFiltering::EQUAL;
                    filterRule.first.second = schema_.foreignKeyPropName;
                    std::vector<UString>::iterator rit = results.begin();
                    for(; rit!=results.end(); ++rit)
                    {
                        PropertyValue v(*rit);
                        filterRule.second.push_back(v);
                        filtingList.push_back(filterRule);
                    }
                }
            }
            catch (const boost::bad_get &)
            {
                filtingList.erase(it);
                return;
            }
        }
        filtingList.erase(it);
        return;
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
