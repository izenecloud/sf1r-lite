/**
 * @file core/mining-manager/duplicate-detection-submanager/DupDetectorWrapper.h
 * @author Jinglei
 * @date Created <2009-1-19 13:49:09>
 */

#ifndef DUPDETECTOR2_H_
#define DUPDETECTOR2_H_

#include <ir/id_manager/IDManager.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <idmlib/util/file_object.h>

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
    typedef idmlib::util::FileObject<uint32_t> FileObjectType;

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
     * For example, given @p docIdList "1 2 3 4 5 6", and "2 4 6" are duplicated
     * docs, then @p cleanDocs would be "1 3 5".
     */
    bool getUniqueDocIdList(
        const std::vector<uint32_t>& docIdList,
        std::vector<uint32_t>& cleanDocs);

    /**
     * @brief Get the unique docid positions in original list.
     * For example, given @p docIdList "1 2 3 4 5 6", and "2 4 6" are duplicated
     * docs, then @p uniquePosList would be "0 2 4".
     */
    bool getUniquePosList(
        const std::vector<uint32_t>& docIdList,
        std::vector<std::size_t>& uniquePosList);

    /**
     * @brief Get a duplicated document list to a given document.
     */
    bool getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList);

    uint32_t getSignatureForText(
            const izenelib::util::UString& text,
            std::vector<uint64_t>& signature);

    bool getKNNListBySignature(
            const std::vector<uint64_t>& signature,
            uint32_t count,
            uint32_t start,
            uint32_t max_hamming_dist,
            std::vector<uint32_t>& docIdList,
            std::vector<float>& rankScoreList,
            std::size_t& totalCount);

private:
    std::string container_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> id_manager_;
    izenelib::am::rde_hash<std::string, bool> dd_properties_;
    idmlib::util::IDMAnalyzer* analyzer_;
    FileObjectType* file_info_;

    bool fp_only_;
    GroupTableType* group_table_;
    DDType* dd_;

    boost::shared_mutex read_write_mutex_;
};

}

#endif /* DUPDETECTOR_H_ */
