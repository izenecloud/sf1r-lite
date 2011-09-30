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
#include "QueryLogManager.h"
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <am/3rdparty/rde_hash.h>
#include <util/hashFunction.h>
#include <ctime>
#include <boost/tuple/tuple.hpp>
#include <algorithm>
#include <queue>
#include <vector>

#include <boost/bind.hpp>
#include <mining-manager/concept-id-manager.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/taxonomy-generation-submanager/LabelManager.h>
#include <mining-manager/auto-fill-submanager/AutoFillSubManager.h>
#include <mining-manager/util/FSUtil.hpp>
#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>
#include <boost/serialization/deque.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <am/cccr_hash/cccr_hash.h>
#include <am/sequence_file/SequenceFile.hpp>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/Indexer.h>
#include <ir/index_manager/index/TermIterator.h>
#include <ir/index_manager/index/TermReader.h>
#include <ir/index_manager/index/Term.h>
#include <ir/ir_database/IRDatabase.hpp>

#include <util/functional.h>

#include <idmlib/util/file_object.h>
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/util/directory_switcher.h>
namespace sf1r
{
namespace iii = izenelib::ir::indexmanager;


///@brief The structure used to store the offline data used for query recommendation., used to
///@brief manage collectino specific task.
class RecommendManager : public boost::noncopyable
{
    typedef uint32_t INFO_TYPE;
    typedef  boost::mpl::vector<uint8_t, uint8_t>  IR_DATA_TYPES;
    typedef  boost::mpl::vector<izenelib::am::SequenceFile<uint8_t> , izenelib::am::SequenceFile<uint8_t> >  IR_DB_TYPES;

    typedef izenelib::ir::irdb::IRDatabase< IR_DATA_TYPES, IR_DB_TYPES  >  MIRDatabase;
    typedef izenelib::ir::irdb::IRDocument< IR_DATA_TYPES >  MIRDocument;

    typedef izenelib::am::SSFType<uint8_t, izenelib::util::UString , uint32_t, false> PropertySSFType;
    typedef PropertySSFType::WriterType property_writer_t;
    typedef izenelib::am::SSFType<uint64_t, std::pair<izenelib::util::UString, uint32_t> , uint32_t, false> QueryLogSSFType;
    
public:
    RecommendManager(const std::string& path,
                     const std::string& collection_name,
                     const RecommendPara& recommend_param,
                     const MiningSchema& mining_schema,
                     const boost::shared_ptr<DocumentManager>& documentManager,
                     boost::shared_ptr<LabelManager> labelManager,
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


    void insertQuery(const izenelib::util::UString& queryStr, uint32_t freq = 1);

    
    void insertProperty(const izenelib::util::UString& queryStr, uint32_t docid);

    /**
    * @brief get the related queries from the data base.
    */
    uint32_t getRelatedConcepts(
        const izenelib::util::UString& query,
        uint32_t maxNum,
        std::deque<izenelib::util::UString>& queries);
        
    bool getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

    
//     void rebuild();

    bool RebuildForAll();

    bool RebuildForRecommend();

    bool RebuildForCorrection();

    bool RebuildForAutofill();

//     MIRDatabase* getNewDB() const;

    uint32_t GetMaxDocId() const
    {
        return max_docid_;
    }

    boost::shared_ptr<LabelManager> GetLabelManager()
    {
        return labelManager_;
    }

private:

    uint8_t LabelScore_(uint32_t df);

    uint8_t QueryLogScore_(uint32_t freq);

    bool AddRecommendItem_(MIRDatabase* db, uint32_t item_id, const izenelib::util::UString& text, uint8_t type, uint32_t score);


    bool getConceptStringByConceptId_(uint32_t id, izenelib::util::UString& ustr);

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
    RecommendPara recommend_param_;
    MiningSchema mining_schema_;
    INFO_TYPE info_;
    izenelib::am::tc_hash<bool, INFO_TYPE > serInfo_;
    boost::shared_ptr<DocumentManager> document_manager_;
    MIRDatabase* recommend_db_;
    ConceptIDManager* concept_id_manager_;
    boost::shared_ptr<LabelManager> labelManager_;
    boost::shared_ptr<AutoFillSubManager> autofill_;
    uint32_t max_labelid_in_recommend_;
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
