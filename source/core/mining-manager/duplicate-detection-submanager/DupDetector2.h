/**
 * @file core/mining-manager/duplicate-detection-submanager/DupDetector2.h
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

#include <ir/id_manager/IDManager.h>
#include <idmlib/util/file_object.h>

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
class CharikarAlgo;
class DupDetector2
{
//  typedef izenelib::am::SimpleSequenceFileWriter<uint32_t, izenelib::util::CBitArray, uint8_t> WriterType;
//  typedef izenelib::am::SimpleSequenceFileReader<uint32_t, izenelib::util::CBitArray, uint8_t> ReaderType;
//  typedef izenelib::am::SimpleSequenceFileSorter<uint32_t, izenelib::util::CBitArray, uint8_t> SorterType;
    typedef sf1r::GroupTable GroupType;
//   typedef FingerprintsCompare<64,3,6> FPCompareType;
    typedef idmlib::util::FileObject<uint32_t> FileObjectType;
public:
    DupDetector2(
        const std::string& container);
    DupDetector2(
        const std::string& container,
        const boost::shared_ptr<DocumentManager>& document_manager,
        const std::vector<std::string>& properties,
        idmlib::util::IDMAnalyzer* analyzer);
    ~DupDetector2();

    bool Open();

    /**
     * @brief Get a unique document id list with duplicated ones excluded.
     */
    bool getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs);

    /**
     * @brief Get a duplicated document list to a given document.
     */
    bool getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList);

    /**
     * @brief Tell two documents belonging to the same collections are duplicated.
     */
    bool isDuplicated( uint32_t docId1, uint32_t docId2);

    bool ProcessCollection();

//  void insertDocument(uint32_t docid, const std::vector<std::string>& v);

    bool runDuplicateDetectionAnalysis(bool force=false);

    /**
     * @brief Insert new documents to a collection
     */
//  bool insertDocuments(const std::vector<unsigned int>& docIdList,
//          const std::vector<std::vector<unsigned int> >& documentList);

    void SetIDManager(const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& id_manager)
    {
        id_manager_ = id_manager;
    }

    uint32_t getSignatureForText(
            const izenelib::util::UString& text,
            std::vector<uint64_t>& signature);

    void getKNNListBySignature(
            const std::vector<uint64_t>& signature,
            uint32_t count,
            std::vector<std::pair<uint32_t, FpItem> >& knn_list);

    inline uint32_t getSignatureAndKNNList(
            const izenelib::util::UString& text,
            uint32_t count,
            std::vector<uint64_t>& signature,
            std::vector<std::pair<uint32_t, FpItem> >& knn_list)
    {
        uint32_t text_len = getSignatureForText(text, signature);
        if (text_len)
            getKNNListBySignature(signature, count, knn_list);

        return text_len;
    }

private:

    bool ProcessCollectionBySimhash_();

//  bool ProcessCollectionBySim_();

    uint32_t GetProcessedMaxId_();

    void FindDD_(const std::vector<FpItem>& object, uint32_t table_id);

    bool SkipTrash_(uint32_t total_count, uint32_t pairwise_count, uint32_t table_id);

    void PairWiseCompare_(const std::vector<FpItem>& for_compare, uint32_t& dd_count);

    bool IsDuplicated_(const FpItem& item1, const FpItem& item2);

    uint8_t GetK_(uint32_t doc_length);

    void SetParameters_();

    void FindSim_(uint32_t docid1, uint32_t docid2, double score);

    bool GetPropertyString_(uint32_t docid, const std::string& property, std::string& value);

    void OutputResult_(const std::string& file);


//     void DataDupd(const std::vector<std::pair<uint32_t, izenelib::util::CBitArray> >& data, uint8_t group_num, uint32_t min_docid);


private:

    std::string container_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> id_manager_;
    izenelib::am::rde_hash<std::string, bool> dd_properties_;
    idmlib::util::IDMAnalyzer* analyzer_;
    FileObjectType* file_info_;
    //parameters
    uint8_t maxk_;
    uint8_t partition_num_;
    double trash_threshold_;
    uint32_t trash_min_;

    /**
    * @brief the near duplicate detection algorithm chosen.
    */
    CharikarAlgo* algo_;

    std::vector<FpTable> table_list_;
    FpStorage fp_storage_;
    std::vector<FpItem> fp_vec_;

    GroupType* group_;

    uint32_t processed_max_docid_;
    uint32_t processing_max_docid_;

    boost::shared_mutex read_write_mutex_;

};

}

#endif /* DUPDETECTOR_H_ */
