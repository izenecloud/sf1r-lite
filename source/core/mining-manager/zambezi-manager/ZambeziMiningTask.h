/**
 * @file ZambeziMiningTask.h
 * @brief the mining task for zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MINING_TASK_H
#define SF1R_ZAMBEZI_MINING_TASK_H

#include "../MiningTask.h"
#include <ir/Zambezi/InvertedIndex.hpp>
#include <string>

namespace sf1r
{
class DocumentManager;

class ZambeziMiningTask : public MiningTask
{
public:
    ZambeziMiningTask(
        DocumentManager& documentManager,
        izenelib::ir::Zambezi::InvertedIndex& indexer,
        const std::string& indexPropName,
        const std::string& indexFilePath);

    virtual bool buildDocument(docid_t docID, const Document& doc);

    virtual bool preProcess();

    virtual bool postProcess();

    virtual docid_t getLastDocId() { return startDocId_; }

private:
    DocumentManager& documentManager_;

    izenelib::ir::Zambezi::InvertedIndex& indexer_;

    const std::string indexPropName_;

    const std::string indexFilePath_;

    docid_t startDocId_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MINING_TASK_H
