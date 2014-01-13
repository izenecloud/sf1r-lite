/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>
#include <configuration-manager/ZambeziConfig.h>
#include <ir/Zambezi/ZambeziIndex.hpp>
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <ir/Zambezi/PositionalInvertedIndex.hpp>
#include <common/PropSharedLockSet.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace sf1r
{
    
namespace faceted
{
class GroupManager;
class AttrManager;
}

class ZambeziTokenizer;
class DocumentManager;

typedef izenelib::ir::Zambezi::AttrScoreInvertedIndex AttrIndex;
typedef izenelib::ir::Zambezi::PositionalInvertedIndex PositionalIndex;
typedef izenelib::ir::Zambezi::ZambeziIndex ZambeziBaseIndex;

class ZambeziManager
{
public:
    ZambeziManager(const ZambeziConfig& config);
    ~ZambeziManager();

    void init();

    bool open();

    void search(
        izenelib::ir::Zambezi::Algorithm algorithm,
        const std::vector<std::pair<std::string, int> >& tokens,
        uint32_t limit,
        const std::vector<std::string>& propertyList,
        std::vector<docid_t>& docids,
        std::vector<float>& scores);

    inline std::map<std::string, ZambeziBaseIndex*>& getIndexMap()
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
        const std::vector<std::vector<float> >& scoresList,
        const std::vector<float>& weightList,
        std::vector<docid_t>& docids,
        std::vector<float>& scores);

    void createZambeziIndex_(ZambeziBaseIndex* &zambeziIndex,
                    unsigned int poolSize);

private:
    const ZambeziConfig& config_;

    ZambeziTokenizer* zambeziTokenizer_;

    std::vector<std::string> propertyList_;

    std::map<std::string, ZambeziBaseIndex*> property_index_map_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H
