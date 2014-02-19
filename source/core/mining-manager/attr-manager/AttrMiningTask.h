///
/// @file AttrMiningTask.h
/// @brief prcessCollection for Attr;
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.28
///
#ifndef ATTR_MINING_TASK_H
#define ATTR_MINING_TASK_H
#include "../MiningTask.h"
#include "AttrTable.h"
#include <configuration-manager/AttrConfig.h>
#include <common/TableSwapper.h>
#include <string>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN

class AttrMiningTask: public MiningTask
{
public:
    AttrMiningTask(DocumentManager& documentManager
        , AttrTable& attrTable
        , std::string dirPath
        , const AttrConfig& attrConfig);

    bool preProcess(int64_t timestamp);
    bool postProcess();
    bool buildDocument(docid_t docID, const Document& doc);
    docid_t getLastDocId();

private:
    sf1r::DocumentManager& documentManager_;
    const std::string propName_;
    TableSwapper<AttrTable> swapper_;
    std::string dirPath_;
    const AttrConfig& attrConfig_;
};

NS_FACETED_END
#endif
