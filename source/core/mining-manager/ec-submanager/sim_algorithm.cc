
#include <boost/scoped_ptr.hpp>
#include "sim_algorithm.h"
#include "ec_manager.h"
#include <am/sequence_file/ssfr.h>
#include <am/matrix/sparse_vector.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/TermIterator.h>
#include <ir/index_manager/index/TermReader.h>
#include <document-manager/DocumentManager.h>

#include <idmlib/semantic_space/apss.h>

using namespace sf1r;
using namespace sf1r::ec;
using namespace izenelib::ir::indexmanager;

SimAlgorithm::SimAlgorithm(const std::string& working_dir, EcManager* ec_manager, uint32_t start_docid, double sim_threshold)
:working_dir_(working_dir), ec_manager_(ec_manager), start_docid_(start_docid), sim_threshold_(sim_threshold)
{
}
        
bool SimAlgorithm::ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, uint32_t property_id, const std::string& property_name)
{
    uint32_t num_docs = index_reader->numDocs();
    std::cout<<"num_docs : "<<num_docs<<std::endl;
    boost::filesystem::remove_all(working_dir_);
    boost::filesystem::create_directories(working_dir_);
    //delete docs first
    std::vector<uint32_t> deleted;
    if(!document_manager->getDeletedDocIdList(deleted))
    {
        std::cout<<"getDeletedDocIdList failed"<<std::endl;
    }
    for(uint32_t i=0;i<deleted.size();i++)
    {
        if(deleted[i]<start_docid_) continue;
        ec_manager_->DeleteDoc(deleted[i]);
    }
    
    izenelib::am::ssf::Writer<> doc_vector_writer(working_dir_+"/doc_vector");
    doc_vector_writer.Open();
    boost::scoped_ptr<TermReader> term_reader( index_reader->getTermReader(1) );
    boost::scoped_ptr<TermIterator> term_iterator( term_reader->termIterator(property_name.c_str()) );
    while(term_iterator->next())
    {
        uint32_t df = term_iterator->termInfo()->docFreq();
        Term term(*term_iterator->term());
        uint32_t termid = term.value;
        term_reader->seek(&term);
        boost::scoped_ptr<TermDocFreqs> tf_reader(term_reader->termDocFreqs());
        while(tf_reader->next())
        {
            uint32_t docid = tf_reader->doc();
            uint32_t tf = tf_reader->freq();
            double tfidf = (double)tf * std::log( (double)num_docs/df );
            doc_vector_writer.Append(docid, std::make_pair(termid, tfidf) );
        }
    }
    doc_vector_writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint32_t>::Sort(doc_vector_writer.GetPath());
    izenelib::am::ssf::Writer<> apss_input(working_dir_+"/apss_input");
    apss_input.Open();
    uint32_t dimensions = 0;
    {
        boost::unordered_map<uint32_t, uint32_t> termid2seq;
        uint32_t seq_id = 1;
        izenelib::am::ssf::Joiner<uint32_t, uint32_t, std::pair<uint32_t, double> > joiner(doc_vector_writer.GetPath());
        joiner.Open();
        uint32_t docid;
        std::vector<std::pair<uint32_t, double> > term_vector;
        while(joiner.Next(docid, term_vector) )
        {
            double a = 0.0;
            for(uint32_t i=0;i<term_vector.size();i++)
            {
                a += term_vector[i].second*term_vector[i].second;
            }
            if(a==0.0) continue;
            a = std::sqrt(a);
            for(uint32_t i=0;i<term_vector.size();i++)
            {
                term_vector[i].second /= a;
                boost::unordered_map<uint32_t, uint32_t>::iterator it = termid2seq.find( term_vector[i].first);
                if(it==termid2seq.end())
                {
                    termid2seq.insert(std::make_pair(term_vector[i].first, seq_id) );
                    term_vector[i].first = seq_id;
                    ++seq_id;
                    
                }
                else
                {
                    term_vector[i].first = it->second;
                }
            }
            std::sort(term_vector.begin(), term_vector.end());
            izenelib::am::SparseVector<double, uint32_t> sparse_vec;
            sparse_vec.value = term_vector;
            apss_input.Append(docid, sparse_vec);
        }
        dimensions = termid2seq.size();
    }
    apss_input.Close();
    
    std::cout<<"start apss with "<<sim_threshold_<<","<<dimensions<<","<<start_docid_<<std::endl;
    idmlib::ssp::Apss<uint32_t, uint32_t> apss(sim_threshold_, boost::bind( &SimAlgorithm::FindSim_, this, _1, _2, _3), dimensions);
    izenelib::am::SparseVector<double, uint32_t> value;
    apss.ComputeWithStart(apss_input.GetPath(), start_docid_, value);
    
    
    //end, clean
//     boost::filesystem::remove_all(working_dir_);
    return true;
}

void SimAlgorithm::FindSim_(uint32_t docid1, uint32_t docid2, double score)
{
    std::cout<<"SimAlgorithm::FindSim_ : "<<docid1<<","<<docid2<<" - "<<score<<std::endl;
//     boost::unordered_map<GroupIdType, GroupDocsWithRes>& group = ec_manager_->group_;
//     boost::unordered_map<uint32_t, std::vector<GroupIdType> >& docid_group = ec_manager_->docid_group_;
    EcManager::d2g_iterator it1 = ec_manager_->d2g_find(docid1);
    EcManager::d2g_iterator it2 = ec_manager_->d2g_find(docid2);
//     boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it1 = docid_group.find(docid1);
//     boost::unordered_map<uint32_t, std::vector<GroupIdType> >::iterator it2 = docid_group.find(docid2);
    if(it1!=ec_manager_->d2g_end() && it2!=ec_manager_->d2g_end() )
    {
//         std::cout<<"Found two exist docids : "<<docid1<<","<<docid2<<std::endl;
        //to combine them
        AddTo_(docid2, it1->second);
        AddTo_(docid1, it2->second);
    }
    else if(it1!=ec_manager_->d2g_end())
    {
        AddTo_(docid2, it1->second);
    }
    else if(it2!=ec_manager_->d2g_end())
    {
        AddTo_(docid1, it2->second);
    }
    else//two new docs
    {
        std::vector<uint32_t> docid_list;
        std::vector<uint32_t> candidate_list(2);
        candidate_list[0] = docid1;
        candidate_list[1] = docid2;
        GroupIdType groupid;
        ec_manager_->AddNewGroup(docid_list, candidate_list, groupid);
    }
    
}

void SimAlgorithm::AddTo_(uint32_t docid, const std::vector<GroupIdType>& gid_list)
{
    if(!gid_list.empty())//one doc exist
    {
        std::vector<GroupIdType> insert_to;
        //select doc if exist, otherwise select the candidates.
        if(gid_list[0]!=0 ) insert_to.resize(1, gid_list[0]);
        else
        {
            insert_to.assign( gid_list.begin()+1, gid_list.end());
        }
        
        std::vector<uint32_t> candidate_list(1, docid);
        for(uint32_t i=0;i<insert_to.size();i++)
        {
            GroupIdType gid = insert_to[i];
            ec_manager_->AddDocInCandidate(gid, candidate_list);
        }
        
        //insert the docid into groups
//         std::vector<GroupIdType> group_value(1,0);
//         for(uint32_t i=0;i<insert_to.size();i++)
//         {
//             GroupIdType gid = insert_to[i];
//             EcManager::g2d_iterator g2d_it = ec_manager_->g2d_find(gid);
//             if(g2d_it==ec_manager_->g2d_end()) continue;
//             g2d_it->second.docs.candidate.push_back(insert_docid);
//             group_value.push_back(gid);
//         }
//         //insert docid2group map
//         ec_manager_->d2g_insert(std::make_pair(insert_docid, group_value) );
    }
}



