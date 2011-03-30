/**
 * @file core/mining-manager/duplicate-detection-submanager/DupDetector2.h
 * @author Jinglei
 * @date Created <2009-1-19 13:49:09>
 */

#ifndef DUPDETECTOR2_H_
#define DUPDETECTOR2_H_


#include "DupTypes.h"
#include <document-manager/Document.h>
#include <document-manager/DocumentManager.h>
#include "charikar_algo.h"
#include "group_table.h"
#include "fp_storage.h"
#include "fp_tables.h"
#include <string>
#include <vector>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <idmlib/util/file_object.h>
#include <idmlib/util/idm_analyzer.h>
namespace sf1r
{

/**
 * @brief The interface of duplicate detection.
 * It manages all the offline and online algorithms in duplication detection which
 * aims to eliminate the duplicate and near-duplicate documents from the collection and
 * search result.
  */
class DupDetector2
{
    typedef izenelib::am::SimpleSequenceFileWriter<uint32_t, izenelib::util::CBitArray, uint8_t> WriterType;
    typedef izenelib::am::SimpleSequenceFileReader<uint32_t, izenelib::util::CBitArray, uint8_t> ReaderType;
    typedef izenelib::am::SimpleSequenceFileSorter<uint32_t, izenelib::util::CBitArray, uint8_t> SorterType;
    typedef sf1r::GroupTable GroupType;
//   typedef FingerprintsCompare<64,3,6> FPCompareType;
    typedef idmlib::util::FileObject<uint32_t > FileObjectType;
public:
    DupDetector2(const std::string& container);
    DupDetector2(const std::string& container, const boost::shared_ptr<DocumentManager>& document_manager, const std::vector<std::string>& properties, idmlib::util::IDMAnalyzer* analyzer);
    ~DupDetector2();

//   void SetDocAvgLength(uint32_t avg)
//   {
//     avg_ = avg;
//   }

    bool Open();

//   uint32_t GetMaxDocId()
//   {
//     return max_docid_;
//   }

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

    void insertDocument(uint32_t docid, const std::vector<std::string>& v);

    bool runDuplicateDetectionAnalysis(bool force=false);

    void Resize(uint32_t size);

    /**
     * @brief Insert new documents to a collection
     */
// 	bool insertDocuments(const std::vector<unsigned int>& docIdList,
// 	        const std::vector<std::vector<unsigned int> >& documentList);




private:
    uint32_t GetProcessedMaxId_();

    void FindDD_(const std::vector<FpItem>& object, const FpTable& table, uint32_t table_id);

    bool SkipTrash_(uint32_t total_count, uint32_t pairwise_count, uint32_t table_id);

    void PairWiseCompare_(const std::vector<FpItem>& for_compare, uint32_t& dd_count);

    bool IsDuplicated_(const FpItem& item1, const FpItem& item2);

    uint8_t GetK_(uint32_t doc_length);

    void SetParameters_();


//     void DataDupd(const std::vector<std::pair<uint32_t, izenelib::util::CBitArray> >& data, uint8_t group_num, uint32_t min_docid);


private:

    std::string container_;
    boost::shared_ptr<DocumentManager> document_manager_;
    izenelib::am::rde_hash<std::string, bool> dd_properties_;
    idmlib::util::IDMAnalyzer* analyzer_;
    FileObjectType* file_info_;
    //parameters
//     uint32_t avg_;
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
    std::vector<FpItem> vec_;

    GroupType* group_;

    uint32_t process_start_docid_;
    uint32_t process_max_docid_;

    uint32_t max_docid_;

    boost::shared_mutex read_write_mutex_;

};

}

#endif /* DUPDETECTOR_H_ */
