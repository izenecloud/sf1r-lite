#ifndef TEST_DUMP_INDEX_H
#define TEST_DUMP_INDEX_H

//#include "../common/RecTypes.h"
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/Indexer.h>
#include <ir/index_manager/index/TermIterator.h>
#include <ir/index_manager/index/TermReader.h>
#include <ir/index_manager/index/Term.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionMeta.h>

#include <boost/shared_ptr.hpp>
#include <list>
#include <vector>
#include <string>

namespace sf1r
{

namespace iii = izenelib::ir::indexmanager;

class TestDumpIndex
{
    typedef std::vector<uint32_t>::const_iterator DataVectorIterator;
public:
    TestDumpIndex(const std::string& path, int64_t memorySize);

    void flush();

    bool getForTest(const uint32_t& termid, std::vector<uint32_t>& docs);
    bool get(const uint32_t& termid, const std::string& propname, uint32_t propid, std::vector<uint32_t>& docs);

    void dumpToFile(const std::string& basepath);
    void dumpThread(const std::string& basepath, const std::string& property, uint32_t propertyId);

    void prepareDocCommon(uint32_t docId, const std::vector<uint32_t> termid_list,
        iii::IndexerDocument& document);
    bool addDoc(uint32_t docId, std::vector<uint32_t>& termid_list);
    void delDoc(uint32_t docid);
    void updateDoc(uint32_t oldId,
        uint32_t docId, std::vector<uint32_t>& termid_list);
    void optimizeIndex();

    uint32_t get(std::vector<uint32_t>& itemIds, const std::string& propname, uint32_t propid);

    size_t itemFreq(uint32_t itemId, const std::string& propname);

    size_t getNumItems(const std::string& propname);

    boost::shared_ptr<iii::Indexer> indexer_;
    CollectionMeta collmeta_;
};

} // namespace sf1r

#endif // TEST_DUMP_INDEX_H

