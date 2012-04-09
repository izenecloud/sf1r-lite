#include "ec_manager.h"

#include <common/Utilities.h>
#include <boost/unordered_set.hpp>

using namespace sf1r::ec;

EcManager::EcManager(const std::string& container)
:container_(container)
, analyzer_(NULL), cma_analyzer_(NULL), kpe_resource_path_("")
, available_groupid_(1), available_groupid_list_(), max_docid_(0)
{
}

EcManager::EcManager(const std::string& container, idmlib::util::IDMAnalyzer* analyzer, idmlib::util::IDMAnalyzer* cma_analyzer, const std::string& kpe_resource_path)
:container_(container)
, analyzer_(analyzer), cma_analyzer_(cma_analyzer), kpe_resource_path_(kpe_resource_path)
, available_groupid_(1), available_groupid_list_(), max_docid_(0)
{
}


EcManager::~EcManager()
{
}

bool EcManager::Open()
{
    boost::filesystem::create_directories(container_);
    std::string g2d_map_file = container_+"/g2d_map";


    if (boost::filesystem::exists(g2d_map_file))
    {
        GroupIdType max_gid = 0;
        uint32_t step = 100000;
        boost::dynamic_bitset<uint32_t> gid_flag(step);
        izenelib::am::ssf::Reader<> reader(g2d_map_file);
        if (!reader.Open()) return false;
        std::cout<<"[EcManager] loading groups : "<<reader.Count()<<std::endl;
        GroupIdType gid;
        GroupDocsWithRes value;
        while (reader.Next(gid, value))
        {
            if (gid>max_gid)
            {
                max_gid = gid;
            }

            if (gid<=gid_flag.size())
            {
                gid_flag.set(gid-1);
            }
            else
            {
                uint32_t a = gid/step;
                uint32_t b = gid - a*step;
                if (b>0) ++a;
                gid_flag.resize(a*step);
            }
            gid_flag.set(gid-1);
            g2d_insert(std::make_pair(gid, value));
            for (uint32_t i=0;i<value.docs.doc.size();i++)
            {
                uint32_t docid = value.docs.doc[i];
                d2g_iterator dgit = d2g_find(docid);
                if (dgit== d2g_end())
                {
                    std::vector<GroupIdType> insert_value(1, gid);
                    d2g_insert(std::make_pair(docid, insert_value));
                }
                else
                {
                    std::vector<GroupIdType>& find_value = dgit->second;
                    if (find_value.empty()) find_value.push_back(gid);
                    else if (find_value[0] != 0)
                    {
                        //error, one doc belongs to two groups
                        std::cout<<"EcManager docid "<<docid<<" error"<<std::endl;
                    }
                    else
                    {
                        find_value[0] = gid;
                    }
                }
            }
            //process candidate
            for (uint32_t i=0;i<value.docs.candidate.size();i++)
            {
                uint32_t docid = value.docs.candidate[i];
                d2g_iterator dgit = d2g_find(docid);
                if (dgit== d2g_end())
                {
                    std::vector<GroupIdType> insert_value(2, 0);
                    insert_value[1] = gid;
                    d2g_insert(std::make_pair(docid, insert_value));
                }
                else
                {
                    std::vector<GroupIdType>& find_value = dgit->second;
                    if (find_value.empty())
                    {
                        find_value.resize(2, 0);
                        find_value[1] = gid;
                    }
                    else
                    {
                        find_value.push_back(gid);
                    }
                }
            }

        }
        reader.Close();
        available_groupid_ = max_gid+1;
        for (uint32_t gid=1;gid<=max_gid;gid++)
        {
            if (!gid_flag[gid-1])
            {
                available_groupid_list_.push_back(gid);
            }
        }
    }
    std::string docid_file = container_+"/max_docid";
    izenelib::am::ssf::Util<>::Load(docid_file, max_docid_);
    Evaluate_();
    return true;
}

bool EcManager::Flush()
{
    std::string g2d_map_file = container_+"/g2d_map";
    boost::filesystem::remove_all(g2d_map_file);
    izenelib::am::ssf::Writer<> writer(g2d_map_file);
    if (!writer.Open()) return false;
    g2d_iterator it = g2d_begin();
    while (it!= g2d_end())
    {
        writer.Append(it->first, it->second);
        ++it;
    }
    writer.Close();
    std::string docid_file = container_+"/max_docid";
    izenelib::am::ssf::Util<>::Save(docid_file, max_docid_);
    return true;
}

bool EcManager::ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_propert)
{
    //...
    uint32_t start_docid = max_docid_+1;
    std::string working_dir = container_+"/working";
    SimAlgorithm algo(working_dir, this, start_docid, 0.7, analyzer_, cma_analyzer_, kpe_resource_path_);
    if (!algo.ProcessCollection(document_manager, index_reader, title_property, content_propert)) return false;
    max_docid_ = document_manager->getMaxDocId();
    if (!Flush()) return false;
    Evaluate_();
    return true;
}

bool EcManager::ClearGroup()
{
    group_.clear();
//     group_candidate_.clear();
    docid_group_.clear();
    available_groupid_ = 1;
    available_groupid_list_.clear();
    Flush();
    return true;
}

bool EcManager::GetAllGroupIdList(std::vector<GroupIdType>& id_list)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.begin();
    while (it!= group_.end())
    {
        id_list.push_back(it->first);
        ++it;
    }
    return true;
}

bool EcManager::AddDocInGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list)
{
//     std::cout<<"AddDocInGroup "<<id<<std::endl;
//     for (uint32_t i=0;i<docid_list.size();i++)
//     {
//         std::cout<<"["<<docid_list[i]<<"]"<<std::endl;
//     }
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator git = group_.find(id);
    if (git == group_.end())
    {
        char buffer [255];
        sprintf (buffer, "can not find tid : %d", id);
        error_msg_ = buffer;
        return false;
    }
    GroupDocsWithRes& ginfo = git->second;
    GroupDocs& docs = ginfo.docs;
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        std::vector<uint32_t>::iterator it = std::find(docs.doc.begin(), docs.doc.end(), docid);
        if (it!=docs.doc.end())
        {
            //find the same in exist list.
            char buffer [255];
            sprintf (buffer, "find same id in list : %d", docid);
            error_msg_ = buffer;
            return false;
        }
        boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator dgit = docid_group_.find(docid);
        if (dgit!=docid_group_.end())
        {
            std::vector<GroupIdType>& glist = dgit->second;
            if (!glist.empty() && glist[0] != 0)
            {
                char buffer [255];
                sprintf (buffer, "docid %d belongs to other group, should delete it from that group firstly.", docid);
                error_msg_ = buffer;
                return false;
            }
        }
    }
    //real insert
    docs.doc.insert(docs.doc.end(), docid_list.begin(), docid_list.end());
    //clean candidate
    docs.candidate.clear();
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it = docid_group_.find(docid);
        if (it!=docid_group_.end())
        {
            std::vector<GroupIdType>& groups = it->second;
            for (uint32_t j=0;j<groups.size();j++)
            {
                GroupIdType& groupid = groups[j];
                if (groupid==0) continue;
                boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.find(groupid);
                if (it==group_.end()) continue;
                GroupDocsWithRes& info = it->second;
                //it will not appears in doc
//                 VectorRemove_(info.docs.doc, docid);
                VectorRemove_(info.docs.candidate, docid);
                if (info.docs.doc.empty() && info.docs.candidate.empty())
                {
                    //empty, del this group
                    group_.erase(groupid);
                    available_groupid_list_.push_back(groupid);
                }
            }
            docid_group_.erase(docid);
        }

        std::vector<GroupIdType> value(1, id);
        docid_group_.insert(std::make_pair(docid, value));
    }

    return true;
}

bool EcManager::AddDocInCandidate(const GroupIdType& id, const std::vector<uint32_t>& docid_list)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator git = group_.find(id);
    if (git == group_.end())
    {
        //do not find, return;
        return false;
    }
    GroupDocsWithRes& ginfo = git->second;
    GroupDocs& docs = ginfo.docs;
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        std::vector<uint32_t>::iterator it = std::find(docs.candidate.begin(), docs.candidate.end(), docid);
        if (it!=docs.candidate.end())
        {
            //find the same in exist list.
            return false;
        }
        boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator dgit = docid_group_.find(docid);
        if (dgit!=docid_group_.end())
        {
            std::vector<GroupIdType>& glist = dgit->second;
            if (!glist.empty() && glist[0] != 0)
            {
                //this docid belongs to other group, should delete it from that group firstly.
                return false;
            }
        }
    }
    //real insert
    docs.candidate.insert(docs.candidate.end(), docid_list.begin(), docid_list.end());

    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it = docid_group_.find(docid);
        if (it!=docid_group_.end())
        {
            std::vector<GroupIdType>& groups = it->second;
            if (groups.empty())
            {
                groups.resize(2,0);
                groups[1] = id;
            }
            else
            {
                groups.push_back(id);
            }
        }
        else
        {
            std::vector<GroupIdType> value(2, 0);
            value[1] = id;
            docid_group_.insert(std::make_pair(docid, value));
        }


    }
    return true;
}

bool EcManager::RemoveDocInGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list, bool gen_new_group)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator git = group_.find(id);
    if (git == group_.end())
    {
        //do not find, return;
        char buffer [255];
        sprintf (buffer, "can not find tid : %d", id);
        error_msg_ = buffer;
        return false;
    }
    GroupDocsWithRes& ginfo = git->second;
    GroupDocs& docs = ginfo.docs;
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        std::vector<uint32_t>::iterator it = std::find(docs.doc.begin(), docs.doc.end(), docid_list[i]);
        if (it==docs.doc.end())
        {
            //not found in exist list.
            char buffer [255];
            sprintf (buffer, "docid %d not found in exist list", docid_list[i]);
            error_msg_ = buffer;
            return false;
        }
    }
    //real remove
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        VectorRemove_(docs.doc, docid);
        //removed doc should be a seperate group
        docid_group_.erase(docid);
        if (gen_new_group)
        {
            std::vector<uint32_t> new_candidate(1, docid);
            GroupIdType new_id = 0;
            if (!AddNewGroup(std::vector<uint32_t>(), new_candidate, new_id))
            {
                return false;
            }
        }
        //update doc-group map
//         std::vector<GroupIdType> value(2, 0);
//         value[1] = new_id;
//         docid_group_.insert(std::make_pair(docid, value));
    }
    //check if this group is empty
    if (docs.doc.empty() && docs.candidate.empty())
    {
        group_.erase(id);
        available_groupid_list_.push_back(id);
    }
    //clean candidate
//     docs.candidate.clear();
    return true;

}

bool EcManager::RemoveDocInCandidate(const GroupIdType& id, const std::vector<uint32_t>& docid_list)
{
    g2d_iterator git = g2d_find(id);
    if (git == g2d_end())
    {
        //do not find, return;
        return false;
    }
    GroupDocsWithRes& ginfo = git->second;
    GroupDocs& docs = ginfo.docs;
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        std::vector<uint32_t>::iterator it = std::find(docs.candidate.begin(), docs.candidate.end(), docid_list[i]);
        if (it==docs.doc.end())
        {
            //not found in exist list.
            return false;
        }
    }
    //real remove
    for (uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        VectorRemove_(docs.candidate, docid);
        d2g_iterator dit = d2g_find(docid);
        if (dit==d2g_end()) continue;
        std::vector<GroupIdType>& gid_list = dit->second;
        VectorRemove_(gid_list, id);
        //del the docid key if gid_list is empty
        if (gid_list.empty())
        {
            docid_group_.erase(docid);
        }
        else if (gid_list.size()==1 && gid_list[0] == 0)
        {
            docid_group_.erase(docid);
        }
    }
    if (docs.doc.empty() && docs.candidate.empty())
    {
        group_.erase(id);
        available_groupid_list_.push_back(id);
    }
    return true;
}

bool EcManager::AddNewGroup(const std::vector<uint32_t>& docid_list, const std::vector<uint32_t>& candidate_list, GroupIdType& groupid)
{
    GroupIdType gid = NextGroupIdCheck_();
    GroupDocsWithRes empty_value;
    group_.erase(gid);
    g2d_insert(std::make_pair(gid, empty_value));
    if (!AddDocInGroup(gid, docid_list))
    {
        group_.erase(gid);
        return false;
    }
    if (!AddDocInCandidate(gid, candidate_list))
    {
        group_.erase(gid);
        return false;
    }
    NextGroupIdEnsure_();
    groupid = gid;
    return true;
}

bool EcManager::DeleteDoc(uint32_t docid)
{
    //use direct operations
    d2g_iterator it = d2g_find(docid);
    if (it==d2g_end())
    {
        return true;
    }
    std::vector<GroupIdType>& gid_list = it->second;
    if (!gid_list.empty())
    {
        if (gid_list[0]!=0)
        {
            std::vector<uint32_t> remove_source(1, docid);
            return RemoveDocInGroup(gid_list[0], remove_source, false);
        }
        else
        {
            std::vector<uint32_t> remove_source(1, docid);
            for (uint32_t i=1;i<gid_list.size();i++)
            {
                RemoveDocInCandidate(gid_list[i], remove_source);
            }
            docid_group_.erase(docid);
        }
    }
    else
    {
        docid_group_.erase(docid);
    }
    return true;
}

// bool EcManager::SetGroup(const GroupIdType& id, const std::vector<uint32_t>& docid_list)
// {
//     //delete first
//
//     for (uint32_t i=0;i<docid_list.size();i++)
//     {
//         uint32_t docid = docid_list[i];
//         boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it = docid_group_.find(docid);
//         if (it!=docid_group_.end())
//         {
//             std::vector<GroupIdType>& groups = it->second;
//             for (uint32_t j=0;j<groups.size();j++)
//             {
//                 GroupIdType& groupid = groups[j];
//                 if (groupid==0) continue;
//                 boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.find(groupid);
//                 if (it==group_.end()) continue;
//                 GroupDocsWithRes& info = it->second;
//                 VectorRemove_(info.docs.doc, docid);
//                 VectorRemove_(info.docs.candidate, docid);
//                 if (info.doc.empty() && info.candidate.size() <= 1)
//                 {
//                     //TODO del this group
//                 }
//             }
//         }
//         docid_group_.erase(docid);
//         std::vector<GroupIdType> value(1, groupid);
//         docid_group_.insert(std::make_pair(docid, value));
//     }
//
//     std::vector<uint32_t> old_docid_list;
//     std::vector<uint32_t> old_candidate_docid_list;
//     GetGroupAll(id, old_docid_list, old_candidate_docid_list);
//     for (uint32_t i=0;i<old_docid_list;i++)
//     {
//         ChangeGroup_(old_docid_list[i], 0);
//     }
//     for (uint32_t i=0;i<old_candidate_docid_list;i++)
//     {
//         ChangeGroup_(old_candidate_docid_list[i], 0);
//     }
//     group_candidate_.erase(id);
//     group_.erase(id);
//     group_.insert(std::make_pair(id, docid_list));
//     for (uint32_t i=0;i<docid_list;i++)
//     {
//         ChangeGroup_(docid_list[i], id);
//     }
//     return true;
// }
//
// void EcManager::ChangeGroup_(uint32_t docid, const GroupIdType& groupid)
// {
//     docid_group_.erase(docid);
//
// }
//
// bool EcManager::AddGroup(const std::vector<uint32_t>& docid_list, GroupIdType& groupid)
// {
//     groupid = NextGroupId_();
//     return SetGroup(groupid, docid_list);
// }

GroupIdType EcManager::NextGroupId_()
{
    GroupIdType result = 0;
    if (!available_groupid_list_.empty())
    {
        result = available_groupid_list_[0];
        available_groupid_list_.erase(available_groupid_list_.begin());
    }
    else
    {
        result = available_groupid_;
        ++available_groupid_;
    }
    return result;
}

GroupIdType EcManager::NextGroupIdCheck_()
{
    GroupIdType result = 0;
    if (!available_groupid_list_.empty())
    {
        result = available_groupid_list_[0];
    }
    else
    {
        result = available_groupid_;
    }
    return result;
}

void EcManager::NextGroupIdEnsure_()
{
    if (!available_groupid_list_.empty())
    {
        available_groupid_list_.erase(available_groupid_list_.begin());
    }
    else
    {
        ++available_groupid_;
    }
}



bool EcManager::GetGroup(const GroupIdType& id, std::vector<uint32_t>& docid_list)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.find(id);
    if (it==group_.end())
    {
        char buffer [255];
        sprintf (buffer, "can not find tid : %d", id);
        error_msg_ = buffer;
        return false;
    }
    docid_list.assign(it->second.docs.doc.begin(), it->second.docs.doc.end());
    return true;
}

bool EcManager::GetGroupCandidate(const GroupIdType& id, std::vector<uint32_t>& candidate_docid_list)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.find(id);
    if (it==group_.end())
    {
        char buffer [255];
        sprintf (buffer, "can not find tid : %d", id);
        error_msg_ = buffer;
        return false;
    }
    candidate_docid_list.assign(it->second.docs.candidate.begin(), it->second.docs.candidate.end());
    return true;
}

bool EcManager::GetGroupAll(const GroupIdType& id, std::vector<uint32_t>& docid_list, std::vector<uint32_t>& candidate_docid_list)
{
    boost::unordered_map<GroupIdType, GroupDocsWithRes>::iterator it = group_.find(id);
    if (it==group_.end())
    {
        char buffer [255];
        sprintf (buffer, "can not find tid : %d", id);
        error_msg_ = buffer;
        return false;
    }
    docid_list.assign(it->second.docs.doc.begin(), it->second.docs.doc.end());
    candidate_docid_list.assign(it->second.docs.candidate.begin(), it->second.docs.candidate.end());
//     std::cout<<"GetGroupAll "<<id<<std::endl;
//     for (uint32_t i=0;i<docid_list.size();i++)
//     {
//         std::cout<<"["<<docid_list[i]<<"]"<<std::endl;
//     }
    return true;
}

/// return status, 0 means not belong to any group, 1 means it's a candidate in that group, 2 means it's in group
bool EcManager::GetGroupId(uint32_t docid, GroupIdType& groupid)
{
    boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it = docid_group_.find(docid);
    if (it==docid_group_.end())
    {
        return false;
    }
    std::vector<GroupIdType>& vec = it->second;
    if (vec.empty()) return false;
    if (vec[0]==0) return false;
    groupid = vec[0];
    return true;
}

void EcManager::Evaluate_()
{
    if (!id_manager_) return;
    std::string import_file = container_+"/evaluate.txt";
    if (!boost::filesystem::exists(import_file)) return;
    std::ifstream ifs(import_file.c_str());
    std::string line;
    std::vector<uint32_t> in_group;
    uint32_t standard_count = 0;
    uint32_t correct_count = 0;
    while (true)
    {
        bool bbreak = false;
        if (!getline(ifs,line))
        {
            line = "";
            bbreak = true;
        }
        boost::algorithm::trim(line);
        if (line.length()==0 && in_group.size()>0)
        {
            //process in_group
            for (uint32_t i=0;i<in_group.size();i++)
            {
                for (uint32_t j=1;j<in_group.size();j++)
                {
                    if (EvaluateInSameGroup_(in_group[i], in_group[j]))
                    {
                        ++correct_count;
                    }
                    ++standard_count;
                }
            }

            in_group.resize(0);
        }
        if (line.length()>0)
        {
            std::size_t a = line.find('\t');
            if (a!=std::string::npos)
            {
                std::string s_docid = line.substr(0, a);
                uint32_t docid = 0;
                if (id_manager_->getDocIdByDocName(Utilities::md5ToUint128(s_docid), docid, false))
                {
                    in_group.push_back(docid);
                }
                else
                {
//                     std::cout<<"docid "<<s_docid<<" does not exists."<<std::endl;
                }
            }
        }
        if (bbreak) break;
    }
    ifs.close();
    double recall = (double)correct_count/standard_count;
    std::cout<<"[Evaluate_] recall : "<<recall<<std::endl;

}

bool EcManager::EvaluateInSameGroup_(uint32_t docid1, uint32_t docid2)
{
    d2g_iterator it1 = d2g_find(docid1);
    d2g_iterator it2 = d2g_find(docid2);
    if (it1 == d2g_end() || it2 == d2g_end()) return false;
    boost::unordered_set<GroupIdType> set;
    std::vector<GroupIdType>& list1 = it1->second;
    std::vector<GroupIdType>& list2 = it2->second;
    for (uint32_t i=0;i<list1.size();i++)
    {
        GroupIdType gid = list1[i];
        if (gid==0) continue;
        set.insert(gid);
        g2d_iterator it = g2d_find(gid);
        if (it==g2d_end()) continue;
        GroupDocs& docs = it->second.docs;
        for (uint32_t j=0;j<docs.candidate.size();j++)
        {
            uint32_t docid = docs.candidate[j];
            d2g_iterator dit = d2g_find(docid);
            if (dit==d2g_end()) continue;
            std::vector<GroupIdType>& list = dit->second;
            for (uint32_t k=0;k<list.size();k++)
            {
                GroupIdType gids = list[k];
                if (gids==0) continue;
                set.insert(gids);
            }
        }

    }
    for (uint32_t i=0;i<list2.size();i++)
    {
        if (list2[i]==0) continue;
        boost::unordered_set<GroupIdType>::iterator it = set.find(list2[i]);
        if (it!=set.end())
        {
            return true;
        }
    }
    return false;
}

/**
    * @brief Get a unique document id list with duplicated ones excluded.
    */
// bool EcManager::GetUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs)
// {
// }
//
// /**
//     * @brief Get a duplicated document list to a given document.
//     */
// bool EcManager::GetDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList)
// {
// }
//
// /**
//     * @brief Tell two documents belonging to the same collections are duplicated, call GetGroupID()
//     */
// bool EcManager::IsDuplicated(uint32_t docid1, uint32_t docid2)
// {
// }
