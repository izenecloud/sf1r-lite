#include "ZambeziManager.h"
#include <common/PropSharedLock.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <iostream>

using namespace sf1r;

namespace
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(
        const ZambeziConfig& config)
    : config_(config)
{
    init();
}

void ZambeziManager::init()
{
    for (std::vector<zambeziProperty>::const_iterator i = config_.properties.begin(); i != config_.properties.end(); ++i)
    {
        propertyList_.push_back(i->name);
        property_index_map_.insert(std::make_pair(i->name, AttrIndex(i->poolSize, config_.poolCount, config_.reverse)));
    }

    for (std::vector<zambeziVirtualProperty>::const_iterator i = config_.virtualPropeties.begin(); i != config_.virtualPropeties.end(); ++i)
    {
        propertyList_.push_back(i->name);
        property_index_map_.insert(std::make_pair(i->name, AttrIndex(i->poolSize, config_.poolCount, config_.reverse)));
    }
}

bool ZambeziManager::open() 
{
    const std::string& basePath = config_.indexFilePath; //not init

    LOG(INFO) << "index BASE PATH: " << basePath << std::endl;
    for (std::vector<std::string>::iterator i = propertyList_.begin(); i != propertyList_.end(); ++i)
    {
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
            property_index_map_[*i].load(ifs);
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
    const boost::function<bool(uint32_t)>& filter,
    uint32_t limit,
    const std::vector<std::string>& propertyList,
    std::vector<docid_t>& docids,
    std::vector<uint32_t>& scores)
{
    izenelib::util::ClockTimer timer;
    if (propertyList.size() == 1)
    {    
        property_index_map_[propertyList[0]].retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docids, scores);// if need to use filter;
        LOG(INFO) << "zambezi returns docid num: " << docids.size()
                  << ", costs :" << timer.elapsed() << " seconds";
    }

    std::vector<std::string> searchPropertyList = propertyList;
    if (searchPropertyList.empty())
        searchPropertyList = propertyList_;

    std::vector<std::vector<docid_t> > docidsList;
    docidsList.resize(searchPropertyList.size());
    std::vector<std::vector<uint32_t> > scoresList;
    scoresList.resize(searchPropertyList.size());

    for (unsigned int i = 0; i < searchPropertyList.size(); ++i)
    {
        property_index_map_[searchPropertyList[i]].retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docidsList[i], scoresList[i]); // add new interface;
    }
    
    merge_(docidsList, scoresList, docids, scores);

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}

void ZambeziManager::merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<uint32_t> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores)
{
    unsigned int BitLen = (MAXDOCID + 7) >> 3;
    char* BitMap = new char[BitLen];
    memset(BitMap, 0, BitLen * sizeof(char));

    for (unsigned int j = 0; j < docidsList.size(); ++j)
    {
        for (unsigned int i = 0; i < docidsList[j].size(); ++i)
        {
            unsigned int u = docidsList[j][i];
            uint32_t s = scoresList[j][i];
            if((BitMap[u>>3] & (0x80 >> (u&7))) == 0)
            {
                BitMap[u>>3] |= 0x80 >> (u&7);
                docids.push_back(u);
                scores.push_back(s);
            }
        }
    }
    delete [] BitMap;
}