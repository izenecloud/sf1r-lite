///
/// @file RecommendManager.h
/// @brief storage all labels and user submitted queries. can obtain related queries from this class.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-03-26 Jia Guo
///

#ifndef RECOMMENDMANAGER_H_
#define RECOMMENDMANAGER_H_
#include <string>

#include <mining-manager/MiningManagerDef.h>

#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>

#include <am/3rdparty/rde_hash.h>
#include <am/sequence_file/SequenceFile.hpp>
#include <util/hashFunction.h>
#include <util/functional.h>
#include <ir/ir_database/IRDatabase.hpp>

#include <idmlib/util/file_object.h>
#include <idmlib/util/directory_switcher.h>

#include <boost/bind.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include <ctime>
#include <algorithm>
#include <queue>
#include <vector>

namespace idmlib{namespace util{
class IDMAnalyzer;
}}

namespace sf1r
{
namespace iii = izenelib::ir::indexmanager;

///@brief The structure used to store the offline data used for query recommendation., used to
///@brief manage collectino specific task.
class ConceptIDManager;
class DocumentManager;
class QueryCorrectionSubmanager;
class AutoFillSubManager;
class RecommendManager : public boost::noncopyable
{
    typedef uint32_t INFO_TYPE;
    typedef  boost::mpl::vector<uint8_t, uint8_t>  IR_DATA_TYPES;
    typedef  boost::mpl::vector<izenelib::am::SequenceFile<uint8_t> , izenelib::am::SequenceFile<uint8_t> >  IR_DB_TYPES;

    typedef izenelib::ir::irdb::IRDatabase< IR_DATA_TYPES, IR_DB_TYPES  >  MIRDatabase;
    typedef izenelib::ir::irdb::IRDocument< IR_DATA_TYPES >  MIRDocument;

    typedef boost::tuple<uint32_t, uint32_t, izenelib::util::UString> QueryLogType;
    typedef std::pair<uint32_t, izenelib::util::UString> PropertyLabelType;

public:
    RecommendManager(
        const std::string& path,
        const std::string& collection_name,
        const MiningSchema& mining_schema,
        const boost::shared_ptr<DocumentManager>& documentManager,
        boost::shared_ptr<QueryCorrectionSubmanager> query_correction,
        idmlib::util::IDMAnalyzer* analyzer_,
        uint32_t logdays);
    ~RecommendManager();
    /**
    * @brief Open the data.
    */
    bool open();
    /**
    * @brief Close the data base.
    */
    void close();

    void insertQuery(
        const izenelib::util::UString& queryStr, 
        uint32_t freq = 1);


    void insertProperty(
        const izenelib::util::UString& queryStr, 
        uint32_t docid);

    /**
    * @brief get the related queries from the data base.
    */
    uint32_t getRelatedConcepts(
        const izenelib::util::UString& query,
        uint32_t maxNum,
        std::deque<izenelib::util::UString>& queries);

    bool getAutoFillList(
        const izenelib::util::UString& query, 
        std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

    void RebuildForAll();

    void RebuildForRecommend(
        const std::list<QueryLogType>& queryList, 
        const std::list<PropertyLabelType>& labelList);

    void RebuildForCorrection(
        const std::list<QueryLogType>& queryList, 
        const std::list<PropertyLabelType>& labelList);

    void RebuildForAutofill(
        const std::list<QueryLogType>& queryList, 
        const std::list<PropertyLabelType>& labelList);

    uint32_t GetMaxDocId() const
    {
        return max_docid_;
    }

private:

    uint8_t LabelScore_(uint32_t df);

    uint8_t QueryLogScore_(uint32_t freq);

    bool AddRecommendItem_(
        MIRDatabase* db, 
        uint32_t item_id, 
        const izenelib::util::UString& text, 
        uint8_t type, 
        uint32_t score);

    uint32_t getRelatedOnes_(
        MIRDatabase* db,
        const std::vector<termid_t>& termIdList,
        const std::vector<double>& weightList,
        uint32_t maxNum,
        izenelib::am::rde_hash<uint64_t>& obtIdList ,
        std::deque<izenelib::util::UString>& queries);

private:

    std::string path_;
    bool isOpen_;
    std::string collection_name_;
    MiningSchema mining_schema_;
    INFO_TYPE info_;
    izenelib::am::tc_hash<bool, INFO_TYPE > serInfo_;
    boost::shared_ptr<DocumentManager> document_manager_;
    MIRDatabase* recommend_db_;
    ConceptIDManager* concept_id_manager_;
    boost::shared_ptr<AutoFillSubManager> autofill_;
    boost::shared_ptr<QueryCorrectionSubmanager> query_correction_;
    idmlib::util::IDMAnalyzer* analyzer_;
    uint32_t logdays_;
    boost::shared_mutex mutex_;
    boost::shared_mutex reminderMutex_;
    boost::mutex offlineMutex_;
    idmlib::util::DirectorySwitcher dir_switcher_;
    uint32_t max_docid_;
    idmlib::util::FileObject<uint32_t> max_docid_file_;

};

}

#endif
