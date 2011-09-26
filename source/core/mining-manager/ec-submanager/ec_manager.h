/// Electronic commerce manager

#ifndef SF1R_EC_ECMANAGER_H_
#define SF1R_EC_ECMANAGER_H_


#include "ec_types.h"
#include "ec_def.h"
#include "sim_algorithm.h"
#include <document-manager/Document.h>
#include <document-manager/DocumentManager.h>
#include <string>
#include <vector>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <idmlib/util/file_object.h>
#include <idmlib/util/idm_analyzer.h>
namespace sf1r { namespace ec {

    
    
/**
 * @brief The interface of duplicate detection.
 * It manages all the offline and online algorithms in duplication detection which
 * aims to eliminate the duplicate and near-duplicate documents from the collection and
 * search result.
  */
class EcManager
{
    //for exists group, groupid must > 0
    
//     typedef izenelib::am::SimpleSequenceFileWriter<uint32_t, izenelib::util::CBitArray, uint8_t> WriterType;
//     typedef izenelib::am::SimpleSequenceFileReader<uint32_t, izenelib::util::CBitArray, uint8_t> ReaderType;
//     typedef izenelib::am::SimpleSequenceFileSorter<uint32_t, izenelib::util::CBitArray, uint8_t> SorterType;
//     typedef sf1r::GroupTable GroupType;

public:
    typedef boost::unordered_map<GroupIdType, GroupDocsWithRes> g2d_type;
    typedef boost::unordered_map<uint32_t, std::vector<GroupIdType> > d2g_type;
    typedef g2d_type::iterator g2d_iterator;
    typedef d2g_type::iterator d2g_iterator;
    
    EcManager(const std::string& container);
    
    EcManager(const std::string& container, idmlib::util::IDMAnalyzer* analyzer, idmlib::util::IDMAnalyzer* cma_analyzer,  const std::string& kpe_resource_path);
    ~EcManager();
    bool Open();
    bool Flush();
    bool ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_propert);
    
    g2d_iterator g2d_find(const GroupIdType& gid)
    {
        return group_.find(gid);
    }
    
    g2d_iterator g2d_begin()
    {
        return group_.begin();
    }
    
    g2d_iterator g2d_end()
    {
        return group_.end();
    }
    
    std::pair<g2d_iterator, bool> g2d_insert(const g2d_type::value_type& value)
    {
        return group_.insert(value);
    }
     
    
    d2g_iterator d2g_find(uint32_t docid)
    {
        return docid_group_.find(docid);
    }
    
    d2g_iterator d2g_begin()
    {
        return docid_group_.begin();
    }
    
    d2g_iterator d2g_end()
    {
        return docid_group_.end();
    }
    
    std::pair<d2g_iterator, bool> d2g_insert(const d2g_type::value_type& value)
    {
        return docid_group_.insert(value);
    }
    
    bool ClearGroup();
    
    bool GetAllGroupIdList(std::vector<GroupIdType>& id_list);
    
    bool AddDocInGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list);
    bool AddDocInCandidate(const GroupIdType& id, const std::vector<uint32_t>& docid_list);
    
    bool RemoveDocInGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list, bool gen_new_group = true);
    bool RemoveDocInCandidate(const GroupIdType& id, const std::vector<uint32_t>& docid_list);
    
    bool AddNewGroup(const std::vector<uint32_t>& docid_list, const std::vector<uint32_t>& candidate_list, GroupIdType& groupid);
    
    bool DeleteDoc(uint32_t docid);
    
//     bool SetGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list);
//     
//     bool AddGroup(const std::vector<uint32_t>& docid_list, GroupIdType& groupid);
    
    bool GetGroup(const GroupIdType& id, std::vector<uint32_t>& docid_list);
    
    bool GetGroupCandidate(const GroupIdType& id, std::vector<uint32_t>& candidate_docid_list);
    
    bool GetGroupAll(const GroupIdType& id, std::vector<uint32_t>& docid_list, std::vector<uint32_t>& candidate_docid_list);
    
    /// return status, 0 means not belong to any group, 1 means it's a candidate in that group, 2 means it's in group
    bool GetGroupId(uint32_t docid, GroupIdType& groupid);

    /**
     * @brief Get a unique document id list with duplicated ones excluded.
     */
//     bool GetUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs);
// 
//     /**
//      * @brief Get a duplicated document list to a given document.
//      */
//     bool GetDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList);
// 
//     /**
//      * @brief Tell two documents belonging to the same collections are duplicated, call GetGroupID()
//      */
//     bool IsDuplicated( uint32_t docid1, uint32_t docid2);


    void SetIDManager(const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& id_manager)
    {
        id_manager_ = id_manager;
    }
    
    const std::string& ErrorMsg() const
    {
        return error_msg_;
    }
    
    


private:
    
    void Evaluate_();
    
    bool EvaluateInSameGroup_(uint32_t docid1, uint32_t docid2);
    
    template<typename T>
    void VectorRemove_(std::vector<T>& vec, const T& value)
    {
        vec.erase( std::remove(vec.begin(), vec.end(), value),vec.end());
    }
    
    uint32_t GetProcessedMaxId_();

    GroupIdType NextGroupId_();
    
    GroupIdType NextGroupIdCheck_();
    void NextGroupIdEnsure_();
    
    void ChangeGroup_(uint32_t docid, const GroupIdType& groupid);
    
    void FindSim_(uint32_t docid1, uint32_t docid2, double score);
    
    void OutputResult_(const std::string& file);



private:

    std::string container_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> id_manager_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::util::IDMAnalyzer* cma_analyzer_;
    std::string kpe_resource_path_;
    g2d_type group_;

    ///for docid in group, the vector size==1. for docid in candidate and if only in one group, the vector = {0, groupid}.
    d2g_type docid_group_;
    
    GroupIdType available_groupid_;
    std::vector<GroupIdType> available_groupid_list_;
    
    uint32_t max_docid_;
    
    std::string error_msg_;


};

}
}

#endif
