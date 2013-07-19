///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2013.07.18
///
#ifndef TITLE_SCORE_MININGTASK_H
#define TITLE_SCORE_MININGTASK_H

#include "../MiningTask.h"
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
                    , std::string data_root_path
                    , ProductTokenizer* tokenizer);

    ~TitleScoreMiningTask();

    bool buildDocument(docid_t docID, const Document& doc);
    
    bool preProcess();

    bool postProcess();

    docid_t getLastDocId();

    const std::vector<double>& getDocumentScore();

    void saveDocumentScore();
    void loadDocumentScore();
    void clearDocumentScore();

    bool canUse();
private:

    bool couldUse_;
    boost::shared_ptr<DocumentManager> document_manager_;
    std::string data_root_path_;
    std::vector<double> document_Scores_; //from 1 - max

    docid_t lastDocScoreDocid_;
    ProductTokenizer* tokenizer_;
};
}

#endif
