#ifndef INDEXE_ENGINE_H
#define INDEXE_ENGINE_H

#include <util/DynamicBloomFilter.h>

#include <mining-manager/concept-id-manager.h>
#include <ir/ir_database/IRDatabase.hpp>
#include <am/sequence_file/SequenceFile.hpp>

#include <boost/unordered_map.hpp>

#include "tokenize/Tokenizer.h"
#include "StringUtil.h"

#include "UQCateEngine.h"

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
    typedef izenelib::util::DynamicBloomFilter<std::string> BloomFilter;

public:
    IndexEngine(const std::string& dir);
    ~IndexEngine();

public:
    void insert(const std::string& userQuery, uint32_t count);
    void search(const std::string& userQuery, FreqStringVector& byCate, FreqStringVector& byFreq, uint32_t N, const CateEqualer* equler, const std::string& original) const;

    void flush();
    void clear();
    
    void setTokenizer(Tokenize::Tokenizer* tokenizer)
    {
        tokenizer_ = tokenizer;
    }

private:
    void open_();
    void close_();
private:
    boost::unordered_map<std::string, uint32_t> userQueries_;
    MIRDatabase* db_;
    sf1r::ConceptIDManager* idManager_;
    BloomFilter* bf_;
    Tokenize::Tokenizer* tokenizer_;
    const std::string dir_;
    
    DISALLOW_COPY_AND_ASSIGN(IndexEngine);
};

}
}
#endif
