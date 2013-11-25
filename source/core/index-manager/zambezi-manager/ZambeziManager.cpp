#include "ZambeziManager.h"
#include <common/PropSharedLock.h>
#include "../zambezi-tokenizer/ZambeziTokenizer.h"
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <iostream>

using namespace sf1r;

ZambeziManager::ZambeziManager(
        const ZambeziConfig& config)
    : config_(config)
    , zambeziTokenizer_(NULL)
{
    init();
    
    if (!config_.hasAttrtoken)
        buildTokenizeDic();
}

ZambeziManager::~ZambeziManager()
{
    if (zambeziTokenizer_)
        delete zambeziTokenizer_;

    for (unsigned int i = 0; i < propertyList_.size(); ++i)
    {
        if (property_index_map_[propertyList_[i]] != NULL)
            delete property_index_map_[propertyList_[i]];
    }
}

void ZambeziManager::init()
{
    for (std::vector<ZambeziProperty>::const_iterator i = config_.properties.begin(); i != config_.properties.end(); ++i)
    {
        if (i->isTokenizer)
        {
            propertyList_.push_back(i->name);
            ZambeziBaseIndex* zambeziIndex = NULL;
            createZambeziIndex_(zambeziIndex, i->poolSize);
            property_index_map_.insert(std::make_pair(i->name, zambeziIndex));
        }
    }

    for (std::vector<ZambeziVirtualProperty>::const_iterator i = config_.virtualPropeties.begin(); i != config_.virtualPropeties.end(); ++i)
    {
        propertyList_.push_back(i->name);
        ZambeziBaseIndex* zambeziIndex = NULL;
        createZambeziIndex_(zambeziIndex, i->poolSize);
        property_index_map_.insert(std::make_pair(i->name, zambeziIndex));
    }
}

void ZambeziManager::createZambeziIndex_(ZambeziBaseIndex* &zambeziIndex, unsigned int poolSize)
{
    if (config_.indexType_ == ZambeziIndexType::DefultIndexType)
    {
        zambeziIndex = new AttrIndex(poolSize, config_.poolCount, config_.reverse);
    }
    else if (config_.indexType_ == ZambeziIndexType::PostionIndexType)
    {
        zambeziIndex = new PositionalIndex(izenelib::ir::Zambezi::NON_POSITIONAL, poolSize, config_.poolCount);
    }
}

void ZambeziManager::buildTokenizeDic()
{
    boost::filesystem::path cma_index_dic(config_.system_resource_path_);
    cma_index_dic /= boost::filesystem::path("dict");
    cma_index_dic /= boost::filesystem::path(config_.tokenPath);

    ZambeziTokenizer::TokenizerType type = ZambeziTokenizer::CMA_MAXPRE;

    zambeziTokenizer_ = new ZambeziTokenizer(); // this is true;
    zambeziTokenizer_->InitWithCMA_(type, cma_index_dic.c_str());
}

ZambeziTokenizer* ZambeziManager::getTokenizer()
{
    return zambeziTokenizer_;
}

bool ZambeziManager::open() 
{
    const std::string& basePath = config_.indexFilePath; //not init

    for (std::vector<std::string>::iterator i = propertyList_.begin(); i != propertyList_.end(); ++i)
    {

        LOG(INFO) << "index BASE PATH: " << basePath << std::endl;
        std::string path = basePath + "_" + *i; // index.bin_Title
        std::ifstream ifs(path.c_str(), std::ios_base::binary);

        if (!ifs)
        {
            LOG(WARNING) << "NEW zambezi or add new property!";
            return true; // nothing to load, if add new property, that need to rebuild;
        }

        LOG(INFO) << "loading zambezi index for propery: " << *i << ", path" << path;

        try
        {
            property_index_map_[*i]->load(ifs);
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << path;
            return false;
        }
    }

    LOG(INFO) << "Finished open zambezi index";

    return true;
}

void ZambeziManager::search(
        const std::vector<std::pair<std::string, int> >& tokens,
        uint32_t limit,
        const std::vector<std::string>& propertyList,
        std::vector<docid_t>& docids,
        std::vector<float>& scores)
{
    std::cout <<"[ZambeziManager::search] Search tokens: ";
    for (unsigned int i = 0; i < tokens.size(); ++i)
    {
        std::cout << tokens[i].first <<" , ";
    }
    std::cout << std::endl;

    izenelib::util::ClockTimer timer;
    // in one property
    if (propertyList.size() == 1)
    {    
        property_index_map_[propertyList[0]]->retrievalWithBuffer(kAlgorithm, tokens, limit, true, docids, scores);
        LOG(INFO) << "zambezi returns docid num: " << docids.size()
                  << ", costs :" << timer.elapsed() << " seconds";
        return;
    }

    // only one property
    if (propertyList_.size() == 1)
    {    
        property_index_map_[propertyList_[0]]->retrievalWithBuffer(kAlgorithm, tokens, limit, true, docids, scores);
        LOG(INFO) << "zambezi returns docid num: " << docids.size()
                  << ", costs :" << timer.elapsed() << " seconds";
        return;
    }

    std::vector<std::string> searchPropertyList = propertyList;
    if (searchPropertyList.empty())
        searchPropertyList = propertyList_;

    std::vector<std::vector<docid_t> > docidsList;
    docidsList.resize(searchPropertyList.size());
    std::vector<std::vector<float> > scoresList;
    scoresList.resize(searchPropertyList.size());

    for (unsigned int i = 0; i < searchPropertyList.size(); ++i)
    {
        property_index_map_[searchPropertyList[i]]->retrievalWithBuffer(kAlgorithm, tokens, limit, true, docidsList[i], scoresList[i]);
    }

    izenelib::util::ClockTimer timer_merge;

    for (unsigned int i = 0; i < docidsList.size(); ++i)
    {
        if (docidsList[i].size() != scoresList[i].size())
        {
            LOG(INFO) << "[ERROR] dismatch doclist and scorelist";
            return;
        }
    }

    merge_(docidsList, scoresList, docids, scores);

    LOG(INFO) << "zambezi merge " << docidsList.size()
              << " properties, costs :" << timer_merge.elapsed() << " seconds";

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}

// to do, add weight to each posting
void ZambeziManager::merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<float> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<float>& scores)
{
    // init
    unsigned int docidsListSize = docidsList.size();
    int totalCount = 0;
    int docCount = 0;

    std::list<pair<int, unsigned int> > existingList;
    for (unsigned int i = 0; i < docidsListSize; ++i)
    {
        if (docidsList[i].size() != 0)
        {
            totalCount += docidsList[i].size();
            existingList.push_back(std::make_pair(i, 0));
        }
    }
    docids.resize(totalCount);
    scores.resize(totalCount);
    
    // count
    std::vector<std::list<pair<int, unsigned int> >::iterator> minDocList;

    unsigned int min = -1;
    while(existingList.size() > 1)
    {
        if (config_.reverse)
            min = 0;
        else
            min = -1;

        for (std::list<pair<int, unsigned int> >::iterator i = existingList.begin(); i != existingList.end(); ++i)
        {
            if ((config_.reverse && min < docidsList[i->first][i->second]) || 
                (!config_.reverse && min > docidsList[i->first][i->second]))
            {
                min = docidsList[i->first][i->second];
                minDocList.clear();
                minDocList.push_back(i);
                continue;
            }
            if (min == docidsList[i->first][i->second])
                minDocList.push_back(i);
        }

        for (unsigned int i = 0; i < minDocList.size(); ++i)
        {
            int k = minDocList[i]->first;
            int j = minDocList[i]->second++;
            if (minDocList[i]->second == docidsList[k].size())
                existingList.erase(minDocList[i]);

            docids[docCount] = docidsList[k][j];
            scores[docCount] += scoresList[k][j];
        }
        docCount++;
    }

    if (existingList.size() == 1)
    {
        int k = existingList.begin()->first;
        for (unsigned int i = existingList.begin()->second; i < docidsList[k].size(); ++i)
        {
            docids[docCount] = docidsList[k][i];
            scores[docCount++] = scoresList[k][i];
        }
    }
    docids.resize(docCount);
    scores.resize(docCount);
}