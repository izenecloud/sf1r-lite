#ifndef PRODUCT_FORWARD_MININGTASK_H
#define PRODUCT_FORWARD_MININGTASK_H

#include "../MiningTask.h"
#include "ProductForwardManager.h"
#include <common/type_defs.h>
#include <string>

namespace sf1r
{

class DocumentManager;
class ProductTokenizer;

class ProductForwardMiningTask: public MiningTask
{
public:
    explicit ProductForwardMiningTask(boost::shared_ptr<DocumentManager>& document_manager
                    , ProductTokenizer* tokenizer
                    , ProductForwardManager* forward);

    ~ProductForwardMiningTask();

    bool buildDocument(docid_t docID, const Document& doc);
    
    bool preProcess(int64_t timestamp);

    bool postProcess();

    docid_t getLastDocId();

private:
    boost::shared_ptr<DocumentManager> document_manager_;
    
    ProductForwardManager* forward_index_;

    ProductTokenizer* tokenizer_;

    std::vector<std::string> tmp_index_;
};
}

#endif
