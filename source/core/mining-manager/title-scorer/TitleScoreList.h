/**
 * @file TitleScoreList.h
 * @brief a list which stores a product tokenizer scores for each doc.
 */

#ifndef SF1R_TITLE_SCORE_LIST_H
#define SF1R_TITLE_SCORE_LIST_H

#include <common/inttypes.h>
#include <common/type_defs.h>
#include <string>
#include <vector>

namespace sf1r
{

class TitleScoreList
{
public:
    TitleScoreList(
        const std::string& dirPath,
        const std::string& propName,
        bool isDebug);

    const std::string& dirPath() const { return dirPath_; }
    const std::string& propName() const { return propName_; }

    bool open();
    
    void resize(unsigned int size);
    bool insert(unsigned int index, double value);
    double getScore(unsigned int index);
    docid_t getLastDocId_();

    bool saveDocumentScore(unsigned int last_doc);
    bool loadDocumentScore();
    bool clearDocumentScore();

private:
    const std::string dirPath_;

    const std::string propName_;

    std::vector<double> document_Scores_;

    docid_t lastDocScoreDocid_;

    bool isDebug_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_CLASSIFY_TABLE_H