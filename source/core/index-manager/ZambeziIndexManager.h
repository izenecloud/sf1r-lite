/**
 * @file ZambeziIndexManager.h
 * @brief build zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_INDEX_MANAGER_H
#define SF1R_ZAMBEZI_INDEX_MANAGER_H

#include "IIncSupportedIndex.h"
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>

namespace sf1r
{
class ZambeziConfig;

class ZambeziIndexManager : public IIncSupportedIndex
{
public:
    ZambeziIndexManager(
        const ZambeziConfig& config,
        izenelib::ir::Zambezi::AttrScoreInvertedIndex& indexer);

    virtual bool isRealTime() { return false; }
    virtual void flush(bool force) {}
    virtual void optimize(bool wait) {}

    virtual void preBuildFromSCD(std::size_t total_filesize) {}
    virtual void postBuildFromSCD(time_t timestamp);

    virtual void preMining() {}
    virtual void postMining() {}

    virtual void finishIndex() {}

    virtual void finishRebuild() {}

    virtual void preProcessForAPI() {}
    virtual void postProcessForAPI() {}

    virtual bool insertDocument(
        const Document& doc,
        time_t timestamp);

    virtual bool updateDocument(
        const Document& olddoc,
        const Document& old_rtype_doc,
        const Document& newdoc,
        int updateType,
        time_t timestamp);

    virtual void removeDocument(docid_t docid, time_t timestamp) {}

private:
    bool buildDocument_(const Document& doc);

private:
    const ZambeziConfig& config_;

    izenelib::ir::Zambezi::AttrScoreInvertedIndex& indexer_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_INDEX_MANAGER_H
