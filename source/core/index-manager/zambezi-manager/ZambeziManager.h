/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>
#include <configuration-manager/ZambeziConfig.h>
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <common/PropSharedLockSet.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace sf1r
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
    
namespace faceted
{
class GroupManager;
class AttrManager;
}

class ZambeziTokenizer;
class DocumentManager;

typedef  izenelib::ir::Zambezi::AttrScoreInvertedIndex AttrIndex;

class ZambeziManager
{
public:
    ZambeziManager(const ZambeziConfig& config);
    ~ZambeziManager();

    void init();

    bool open();

    template <class FilterType>
    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const FilterType& filter,
        uint32_t limit,
        const std::vector<std::string>& propertyList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores)
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
            property_index_map_[propertyList[0]].retrievalAndFiltering(tokens, filter, limit, true, docids, scores);// if need to use filter;
            LOG(INFO) << "zambezi returns docid num: " << docids.size()
                      << ", costs :" << timer.elapsed() << " seconds";
            return;
        }

        // only one property
        if (propertyList_.size() == 1)
        {    
            property_index_map_[propertyList_[0]].retrievalAndFiltering(tokens, filter, limit, true, docids, scores);// if need to use filter;
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
            property_index_map_[searchPropertyList[i]].retrievalAndFiltering(tokens, filter, limit, true, docidsList[i], scoresList[i]); // add new interface;
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

    inline std::map<std::string, AttrIndex>& getIndexMap()
    {
        return property_index_map_;
    }

    inline const std::vector<std::string>& getProperties()
    {
        return propertyList_;
    }

    bool isAttrTokenize()
    {
        return config_.hasAttrtoken;
    }

    void buildTokenizeDic();
    
    ZambeziTokenizer* getTokenizer();

private:
    void merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<uint32_t> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

private:
    const ZambeziConfig& config_;
    ZambeziTokenizer* zambeziTokenizer_;

    std::vector<std::string> propertyList_;

    std::map<std::string, AttrIndex> property_index_map_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H
