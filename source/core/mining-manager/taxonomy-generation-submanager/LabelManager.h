///
/// @file LabelManager
/// @brief Provide save and load function for labels in documents.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-08-28
/// @date Updated 2010-03-17 00:58:09 Refactor


#ifndef LABELMANAGER_H_
#define LABELMANAGER_H_
#include <string>
#include <mining-manager/MiningManagerDef.h>
#include <mining-manager/doc_table.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/random.hpp>
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <am/external_sort/izene_sort.hpp>
#include <am/sequence_file/SimpleSequenceFile.hpp>
#include <am/sequence_file/SequenceFile.hpp>
#include <ir/ir_database/IRDatabase.hpp>
#include "TgTypes.h"
#include <util/error_handler.h>
#include <idmlib/util/file_object.h>
#include <boost/serialization/vector.hpp>
// #include <am/matrix/matrix_file_io.h>
// #include <idmlib/similarity/term_similarity.h>
namespace sf1r
{

template <typename block_type= uint32_t>
class SerBitset
{
    typedef boost::dynamic_bitset<block_type> bitset_type;
public:
    SerBitset()
    {
    }
    SerBitset( const bitset_type& bitset ):bitset_(bitset)
    {
    }
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        typedef std::vector<block_type> vec_type;
        vec_type vec( bitset_.num_blocks() );
        typename vec_type::iterator it = vec.begin();
        boost::to_block_range<uint32_t, typename bitset_type::allocator_type, typename vec_type::iterator >(bitset_, it);
        typename bitset_type::size_type size = bitset_.size();
        ar & size;
        ar & vec;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        typename bitset_type::size_type size;
        ar & size;
        typedef std::vector<block_type> vec_type;
        vec_type vec;
        ar & vec;
        bitset_ = bitset_type(vec.begin(), vec.end() );
        bitset_.resize(size);

    }
    BOOST_SERIALIZATION_SPLIT_MEMBER();

public:
    bitset_type bitset_;

};



/**
* @brief Label Manager manages the labels and other related data in TG.
*/
class ConceptIDManager;
class LabelManager : public izenelib::util::ErrorHandler
{
public:
    typedef uint32_t INFO_TYPE;
    typedef std::pair<uint32_t, uint32_t> id2count_t;


    typedef uint64_t hash_t;

    typedef izenelib::am::SSFType<uint32_t, std::pair<uint32_t, uint32_t> , uint8_t, false> Doc2LabelSSFType;


    typedef boost::tuple<izenelib::util::UString, std::vector<id2count_t>, uint8_t, uint8_t > Label2DocItem;
    typedef izenelib::am::SSFType<uint32_t, Label2DocItem , uint32_t, false> Label2DocSSFType;

    typedef boost::tuple<izenelib::util::UString, std::vector<id2count_t>, uint8_t, uint8_t, labelid_t, hash_t > SortedLabel2DocItem;
    typedef izenelib::am::SSFType<uint32_t, SortedLabel2DocItem , uint32_t, false> SortedLabel2DocSSFType;

    typedef izenelib::am::SSFType<hash_t, labelid_t , uint8_t, false> SavedLabelSSFType;

    typedef izenelib::am::SimpleSequenceFileMerger<hash_t, labelid_t, Label2DocItem, uint8_t, uint32_t> merger_t;
    typedef izenelib::am::SequenceFile<std::vector<uint32_t>
    , izenelib::am::tc_fixdb<uint32_t, std::vector<uint32_t> >
    , izenelib::am::CompressSeqFileSerializeHandler
    , izenelib::am::SeqFileCharCacheHandler
    > DocLabelInfoType;

    typedef std::vector<id2count_t> LabelDistributeType;
    typedef izenelib::am::SSFType<uint32_t, LabelDistributeType , uint32_t, false> LabelDistributeSSFType;

    typedef boost::function<void (uint32_t, izenelib::util::UString, std::vector<id2count_t>, uint8_t, uint8_t) >   TaskFunctionType;

    typedef std::vector<TaskFunctionType> TaskFunctionListType;
// typedef idmlib::sim::TermSimilarity<izenelib::am::MatrixFileFLIo> TermSimilarityType;
    struct LABEL_TYPE
    {
        static const uint8_t COMMON = 0;
        static const uint8_t PEOP = 1;
        static const uint8_t LOC = 2;
        static const uint8_t ORG = 3;
    };


    LabelManager(const std::string& path, bool no_doc_container = false);

    ~LabelManager();

    void SetDbSave(const std::string& collection_name);

    /**
    * @brief open this label manager.
    */
    void open();

    /**
    * @brief whether is open.
    */
    bool isOpen();


    void AddTask(TaskFunctionType function);

    void ClearAllTask();

    /**
    * @brief insert the label which appears in a list of documents.
    * @param labelStr label's string
    * @param docIdList the doc id list to the insert label
    * @param score label's normalized score
    */
    void insertLabel(
        const izenelib::util::UString& labelStr,
        const std::vector<id2count_t>& docItemList,
        uint8_t score,
        uint8_t type = LABEL_TYPE::COMMON);


    /**
    * @brief insert the label for a given document.
    * @param docId the doc id list to the insert label
    * @param labelStr label's string
    */
    void insertLabel(
        docid_t docId,
        const izenelib::util::UString& labelStr);

    bool getLabelType(uint32_t labelId, uint8_t& type);

    /**
    * @brief delete the given document.
    * @param docId the doc id.
    */
    void deleteDocLabels(docid_t docId);

    /**
    * @brief get all label id list by doc id list.
    * @param docIdList the doc id list we want to fetch.
    * @param labelIdList all label id list of list
    * @return if succ.
    */
    bool getLabelsByDocIdList(const std::vector<docid_t>& docIdList, std::vector<std::vector<labelid_t> >& labelIdList);

    /**
    * @brief get all label id list by doc id.
    * @param docIdList the doc id we want to fetch.
    * @param labelIdList all label id list.
    * @return if succ.
    */
    bool getLabelsByDocId(docid_t docId, std::vector<labelid_t>& labelIdList);

    bool getSortedLabelsByDocId(docid_t docId, std::vector<labelid_t>& labelIdList);

    /**
    * @brief convert label id to label string.
    * @param labelId label id
    * @param[out] labelStr returned labelStr.
    * @return if this label id exist.
    */
    bool getLabelStringByLabelId(labelid_t labelId, izenelib::util::UString& labelStr);

    inline bool getConceptStringByConceptId(labelid_t labelId, izenelib::util::UString& labelStr)
    {
        return getLabelStringByLabelId(labelId, labelStr);
    }


    /**
    * @brief convert label id list to label string list.
    * @param labelIdList label id list
    * @param[out] labelStrList returned labelStr list.
    * @return if at least one label id exist.
    */
    bool getLabelStringListByLabelIdList(const std::vector<labelid_t>& labelIdList, std::vector<izenelib::util::UString>& labelStrList);


    /**
    * @brief check if the label string in label id manager.
    * @param labelStr label string
    * @param[out] labelId returned label id.
    * @return if exist.
    */
    bool checkLabelIdByLabelString(const izenelib::util::UString& labelStr, labelid_t& labelId);

    /**
    * @brief build the doc to label list mapping.
    */
    void buildDocLabelMap();

    /**
    * @brief get label's document freq.
    * @param labelId the label id
    * @param[out] df the df.
    * @return if label id exist.
    */
    bool getLabelDF(labelid_t labelId, uint32_t& df);


    inline bool getConceptDFByConceptId(labelid_t labelId, uint32_t& df)
    {
        return getLabelDF(labelId, df);
    }

    /**
    * @brief set label's document freq.
    * @param labelId the label id
    * @param df the df.
    */
    void setLabelDF(labelid_t labelId, uint32_t df);



    void cleanLabelStream();

    uint32_t getTotalDocCount() const
    {
        return info_;
    }

    double getCacheHitRatio();

    uint32_t getMaxLabelID() const
    {
        //use df's container.
        return dfInfo_->getItemCount();
    }

    bool ExistsLabel(const izenelib::util::UString& text);

    /**
    * @brief flush all modification.
    */
    void flush();

    /**
    * @brief close.
    */
    void close();

    LabelDistributeSSFType::ReaderType* getLabelDistributeReader();

private:

    uint32_t getLabelId_(const izenelib::util::UString& text, uint32_t df, uint8_t type, uint8_t score);

    void insertToDocTable_( uint32_t labelId, std::vector<id2count_t>& labelItemList);

    void encode_(const std::vector<labelid_t>& labels, char** data, uint16_t& len);
    void decode_(char* const data, uint16_t len, std::vector<labelid_t>& labels);

    bool selectDocLabels(
        std::vector<id2count_t>& labelItemList,
        std::vector<uint32_t>& labelIdList,
        std::vector<uint32_t>& sorted_label);

    bool updateLabelDb_();

private:
    std::string path_;
    bool isOpen_;
    bool no_doc_container_;
    std::string collection_name_;
    INFO_TYPE info_;
    izenelib::am::tc_hash<bool, INFO_TYPE > serInfo_;

    uint32_t partialDocCount_;
    ConceptIDManager* idManager_;

    /**
    * @brief stores the document-label forward index.
    */
    DocTable<>* docLabelInfo_;
    izenelib::am::tc_hash<uint32_t, std::vector<uint32_t> >* sorted_label_;
    /**
    * @brief stores the label information used for query recommendation.
    */
    izenelib::am::SequenceFile<uint32_t>* dfInfo_;

    izenelib::am::SequenceFile<uint8_t>* typeInfo_;

    /**
    * @brief stores the labels in the file that will be replicated to BA to support query autofill.
    */
    std::ofstream labelStream_;

    izenelib::am::tc_hash<uint64_t,bool >* semHypernym_;
    izenelib::am::rde_hash<uint64_t,bool >* vocSemHypernym_;

    izenelib::am::tc_hash<hash_t,uint32_t >* labelMap_;;
//     Label2DocSSFType::WriterType* label2DocWriter_;
    std::list<std::pair<uint32_t, Label2DocItem> > label2DocWriter_;
    std::string savedLabelHashPath_;
    LabelDistributeSSFType::WriterType* ldWriter_;
    boost::shared_mutex docTableMutex_;
    boost::shared_mutex dfMutex_;
    uint32_t cacheHit_;
    uint32_t cacheNoHit_;
    idmlib::util::FileObject<std::vector<std::list<uint32_t> > > docItemListFile_;

    TaskFunctionListType task_list_;
//     TermSimilarityType* sim_;


};

}

#endif
