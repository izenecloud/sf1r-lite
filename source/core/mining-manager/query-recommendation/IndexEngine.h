#ifndef INDEXE_ENGINE_H
#define INDEXE_ENGINE_H

#include <mining-manager/concept-id-manager.h>
#include <ir/ir_database/IRDatabase.hpp>
#include <am/sequence_file/SequenceFile.hpp>

#include <boost/unordered_map.hpp>

#include "tokenize/Tokenizer.h"
#include "StringUtil.h"

namespace sf1r
{
namespace Recommend
{

class IndexEngine
{
    typedef boost::mpl::vector<uint8_t, uint8_t> IR_DATA_TYPES;
    typedef boost::mpl::vector<izenelib::am::SequenceFile<uint8_t>, izenelib::am::SequenceFile<uint8_t> > IR_DB_TYPES;
    typedef izenelib::ir::irdb::IRDatabase<IR_DATA_TYPES, IR_DB_TYPES> MIRDatabase;
    typedef izenelib::ir::irdb::IRDocument<IR_DATA_TYPES> MIRDocument;

public:
    IndexEngine(const std::string& dir);
    ~IndexEngine();

public:
    void insert(Tokenize::TermVector& tv, const std::string& str, uint32_t count);
    void search(Tokenize::TermVector& tv, FreqStringVector& strs);
    void flush();

private:
    void open_();
    void close_();
private:
    boost::unordered_map<std::string, std::pair<uint32_t, Tokenize::TermVector> > userQueries_;
    MIRDatabase* db_;
    sf1r::ConceptIDManager* idManager_;
    const std::string dir_;
};

}
}
#endif
