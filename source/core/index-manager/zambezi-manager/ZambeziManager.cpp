#include "ZambeziManager.h"
/*
#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include "../attr-manager/AttrTable.h"
*/

#include <common/PropSharedLock.h>
#include <configuration-manager/ZambeziConfig.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <fstream>
#include <math.h>
#include <algorithm>

using namespace sf1r;

namespace
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(
        const ZambeziConfig& config)
    : config_(config)
    , indexer_(config_.poolSize, config_.poolCount, config_.reverse)
{
    void init();
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

bool ZambeziManager::open_1()
{
    const std::string& basePath = config_.indexFilePath;

    for (std::vector<std::string>::iterator i = propertyList_.begin(); i != propertyList_.end(); ++i)
    {
        std::string path = basePath + "_" + *i; // index.bin_Title
        std::ifstream ifs(path.c_str(), std::ios_base::binary);

        if (! ifs)
            return false;// orignal return true;

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

    LOG(INFO) << "Finished loading zambezi index";

    return true;
}

bool ZambeziManager::open()
{
    const std::string& path = config_.indexFilePath;
    std::ifstream ifs(path.c_str(), std::ios_base::binary);

    if (! ifs)
        return true;

    LOG(INFO) << "loading zambezi index path: " << path;

    try
    {
        indexer_.load(ifs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << path;
        return false;
    }

    LOG(INFO) << "finished loading zambezi index, total doc num: "
              << indexer_.totalDocNum();

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
        return;
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

}

void ZambeziManager::search(
    const std::vector<std::pair<std::string, int> >& tokens,
    const boost::function<bool(uint32_t)>& filter,
    uint32_t limit,
    std::vector<docid_t>& docids,
    std::vector<uint32_t>& scores)
{
    izenelib::util::ClockTimer timer;

    indexer_.retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docids, scores);

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}
