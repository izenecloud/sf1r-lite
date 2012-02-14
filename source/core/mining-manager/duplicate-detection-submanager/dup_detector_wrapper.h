/**
 * @file core/mining-manager/duplicate-detection-submanager/DupDetectorWrapper.h
 * @author Jinglei
 * @date Created <2009-1-19 13:49:09>
 */

#ifndef DUPDETECTOR2_H_
#define DUPDETECTOR2_H_

#include "DupTypes.h"
#include "group_table.h"
#include "fp_storage.h"
#include "fp_tables.h"
#include <string>
#include <vector>

#include <idmlib/duplicate-detection/dup_detector.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>


namespace idmlib
{
namespace util
{
class IDMAnalyzer;
}
}

namespace sf1r
{
/**
 * @brief The interface of duplicate detection.
 * It manages all the offline and online algorithms in duplication detection which
 * aims to eliminate the duplicate and near-duplicate documents from the collection and
 * search result.
  */
class DocumentManager;
class DupDetectorWrapper
{
    typedef idmlib::dd::DupDetector<uint32_t, uint32_t> DDType;
    typedef DDType::GroupTableType GroupTableType;

public:
    DupDetectorWrapper(const std::string& container);

    DupDetectorWrapper(
            const std::string& container,
            const boost::shared_ptr<DocumentManager>& document_manager,
            const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& id_manager,
            const std::vector<std::string>& properties,
            idmlib::util::IDMAnalyzer* analyzer,
            bool fp_only);
    ~DupDetectorWrapper();

    bool Open();

    bool ProcessCollection();

    /**
     * @brief Get a unique document id list with duplicated ones excluded.
     */
    bool getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs);

    /**
     * @brief Get a duplicated document list to a given document.
     */
    bool getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList);

    uint32_t getSignatureForText(
            const izenelib::util::UString& text,
            std::vector<uint64_t>& signature);

    void getKNNListBySignature(
            const std::vector<uint64_t>& signature,
            uint32_t count,
            uint32_t start,
            std::vector<uint32_t>& docIdList,
            std::vector<float>& rankScoreList,
            std::size_t& totalCount);

private:
    bool GetPropertyString_(uint32_t docid, const std::string& property, std::string& value);

private:
    std::string container_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> id_manager_;
    izenelib::am::rde_hash<std::string, bool> dd_properties_;
    idmlib::util::IDMAnalyzer* analyzer_;

    bool fp_only_;
    GroupTableType* group_table_;
    DDType* dd_;

    boost::shared_mutex read_write_mutex_;
};

}

#endif /* DUPDETECTOR_H_ */
