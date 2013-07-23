///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2013.07.18
///
#ifndef TITLE_SCORE_MININGTASK_H
#define TITLE_SCORE_MININGTASK_H

#include "../MiningTask.h"
#include "TitleScoreList.h"
#include <common/type_defs.h>
#include <string>

namespace sf1r
{

class DocumentManager;
class ProductTokenizer;

class TitleScoreMiningTask: public MiningTask
{
public:
    explicit TitleScoreMiningTask(boost::shared_ptr<DocumentManager>& document_manager
                    , ProductTokenizer* tokenizer
                    , TitleScoreList* titleScoreList);

    ~TitleScoreMiningTask();

    bool buildDocument(docid_t docID, const Document& doc);
    
    bool preProcess();

    bool postProcess();

    docid_t getLastDocId();

private:
    boost::shared_ptr<DocumentManager> document_manager_;
    
    TitleScoreList* titleScoreList_;

    ProductTokenizer* tokenizer_;
};
}

#endif
