/**
 * @file CategoryClassifyMiningTask.h
 * @brief mining task to classify category for each docid.
 */

#ifndef SF1R_CATEGORY_CLASSIFY_MINING_TASK_H
#define SF1R_CATEGORY_CLASSIFY_MINING_TASK_H

#include "../MiningTask.h"
#include <string>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class DocumentManager;
class CategoryClassifyTable;
class NumericPropertyTableBase;

class CategoryClassifyMiningTask : public MiningTask
{
public:
    CategoryClassifyMiningTask(
        DocumentManager& documentManager,
        CategoryClassifyTable& classifyTable,
        const std::string& targetCategoryPropName,
        const boost::shared_ptr<const NumericPropertyTableBase>& priceTable);

    virtual bool buildDocument(docid_t docID, const Document& doc);

    virtual bool preProcess();

    virtual bool postProcess();

    virtual docid_t getLastDocId() { return startDocId_; }

private:
    bool ruleByOriginalCategory_(
        const Document& doc,
        std::string& classifyCategory);

    bool ruleBySource_(
        const Document& doc,
        std::string& classifyCategory);

    bool classifyByTitle_(
        const std::string& title,
        docid_t docID,
        std::string& classifyCategory,
        bool& isRule);

private:
    DocumentManager& documentManager_;

    CategoryClassifyTable& classifyTable_;

    const std::string targetCategoryPropName_;

    const boost::shared_ptr<const NumericPropertyTableBase> priceTable_;

    docid_t startDocId_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_CLASSIFY_MINING_TASK_H
