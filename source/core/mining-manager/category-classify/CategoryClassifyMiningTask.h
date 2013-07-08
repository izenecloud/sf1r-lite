/**
 * @file CategoryClassifyMiningTask.h
 * @brief mining task to classify category for each docid.
 */

#ifndef SF1R_CATEGORY_CLASSIFY_MINING_TASK_H
#define SF1R_CATEGORY_CLASSIFY_MINING_TASK_H

#include "../MiningTask.h"
#include <fstream>
#include <string>

namespace sf1r
{
class DocumentManager;
class CategoryClassifyTable;

class CategoryClassifyMiningTask : public MiningTask
{
public:
    CategoryClassifyMiningTask(
        DocumentManager& documentManager,
        CategoryClassifyTable& classifyTable,
        const std::string& categoryPropName,
        bool isDebug);

    virtual bool buildDocument(docid_t docID, const Document& doc);

    virtual bool preProcess();

    virtual bool postProcess();

    virtual docid_t getLastDocId() { return startDocId_; }

private:
    void classifyByCategory_(
        const std::string& category,
        std::string& classifyCategory);

    void classifyByTitle_(
        const std::string& title,
        std::string& classifyCategory);

private:
    DocumentManager& documentManager_;

    CategoryClassifyTable& classifyTable_;

    const std::string categoryPropName_;

    docid_t startDocId_;

    bool isDebug_;

    std::ofstream debugStream_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_CLASSIFY_MINING_TASK_H
