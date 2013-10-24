/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>
#include <search-manager/NumericPropertyTableBuilder.h>
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <common/PropSharedLockSet.h>
#include <string>
#include <vector>

namespace sf1r
{

namespace faceted
{
class GroupManager;
class AttrManager;
}

class DocumentManager;
class ZambeziConfig;

typedef  izenelib::ir::Zambezi::AttrScoreInvertedIndex AttrIndex;

class ZambeziManager
{
public:
    ZambeziManager(
            const ZambeziConfig& config,
            faceted::AttrManager* attrManager,
            NumericPropertyTableBuilder* numericTableBuilder);

    void init();

    bool open();

    bool open_1();

    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const boost::function<bool(uint32_t)>& filter,
        uint32_t limit,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const boost::function<bool(uint32_t)>& filter,
        uint32_t limit,
        const std::vector<std::string>& propertyList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);


    void NormalizeScore(
        std::vector<docid_t>& docids,
        std::vector<float>& scores,
        std::vector<float>& productScores,
        PropSharedLockSet &sharedLockSet);

private:
    void merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<uint32_t> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

private:
    const ZambeziConfig& config_;

    faceted::AttrManager* attrManager_;

    std::vector<std::string> propertyList_;

    izenelib::ir::Zambezi::AttrScoreInvertedIndex indexer_;

    std::map<std::string, AttrIndex> property_index_map_;

    NumericPropertyTableBuilder* numericTableBuilder_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H
