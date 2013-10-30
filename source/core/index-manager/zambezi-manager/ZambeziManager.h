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

typedef  izenelib::ir::Zambezi::AttrScoreInvertedIndex AttrIndex;
const unsigned int MAXDOCID = 268435456; //
class ZambeziManager
{
public:
    ZambeziManager(const ZambeziConfig& config);

    void init();

    bool open();

    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const boost::function<bool(uint32_t)>& filter,
        uint32_t limit,
        const std::vector<std::string>& propertyList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

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

private:
    void merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<uint32_t> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

private:
    const ZambeziConfig& config_;

    std::vector<std::string> propertyList_;

    std::map<std::string, AttrIndex> property_index_map_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H
