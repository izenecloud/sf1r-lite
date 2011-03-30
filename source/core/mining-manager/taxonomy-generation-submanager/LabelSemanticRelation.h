///
/// @file   LabelSemanticRelation.h
/// @brief  The one of the core classes to identify whether one phrase is meaningful.
/// @author Jia Guo
/// @date   2010-01-04
/// @date   2010-01-04
///
#ifndef LABELSEMANTICRELATION_H_
#define LABELSEMANTICRELATION_H_
#include <string>
#include <mining-manager/MiningManagerDef.h>
#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <cache/IzeneCache.h>
#include <util/izene_serialization.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "GraphIndexerManager.hpp"
namespace sf1r
{


class LabelSemanticRelation
{
public:
    LabelSemanticRelation(const std::string& name);
    ~LabelSemanticRelation();

    void open();
    void flush();
    void close();

    void addSiblingLabelPair(uint32_t leftLabelId, uint32_t rightLabelId, uint32_t count = 1);
    void addFathershipLabelPair(uint32_t fatherLabelId, uint32_t childLabelId, uint32_t count = 1);

    void getSiblingRelativeLables(uint32_t labelId, std::vector<std::pair<uint32_t, uint32_t> >& result);
    void getChildRelativeLables(uint32_t labelId, std::vector<std::pair<uint32_t, uint32_t> >& result);

private:
    LabelRepTable labelRepTable_;
    izenelib::am::tc_hash<uint32_t, std::vector<uint32_t> > semanticChildLabelList_;
};

class LabelSemanticRelationExtractor
{
public:
    LabelSemanticRelationExtractor(const LabelSemanticRelation& lsr);
    void processSentence(const std::vector<termid_t>& sentence);
private:
    LabelSemanticRelation lsr_;
};

}
#endif
