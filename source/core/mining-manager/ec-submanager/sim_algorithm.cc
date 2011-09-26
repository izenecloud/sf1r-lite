
#include <boost/scoped_ptr.hpp>
#include "sim_algorithm.h"
#include "ec_manager.h"
#include <am/sequence_file/ssfr.h>
#include <am/matrix/sparse_vector.h>
#include <util/id_util.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/TermIterator.h>
#include <ir/index_manager/index/TermReader.h>
#include <document-manager/DocumentManager.h>

#include <idmlib/semantic_space/apss.h>
#include <idmlib/keyphrase-extraction/kpe_task.h>

using namespace sf1r;
using namespace sf1r::ec;
using namespace izenelib::ir::indexmanager;
using namespace idmlib::kpe;

SimAlgorithm::SimAlgorithm(const std::string& working_dir, EcManager* ec_manager, uint32_t start_docid, double sim_threshold
, idmlib::util::IDMAnalyzer* analyzer, idmlib::util::IDMAnalyzer* cma_analyzer, const std::string& kpe_resource_path)
:working_dir_(working_dir), ec_manager_(ec_manager)
, analyzer_(analyzer), cma_analyzer_(cma_analyzer), kpe_resource_path_(kpe_resource_path)
, start_docid_(start_docid)
, sim_threshold_(sim_threshold), tfidf_ratio_(0.7)
, writer_(NULL), kpe_writer_(NULL), max_kpid_(0)
{
    
    kpe_ratio_ = 1.0-tfidf_ratio_;
    tfidf_threshold_ = (sim_threshold_-kpe_ratio_)/tfidf_ratio_;
    kpe_threshold_ = (sim_threshold_-tfidf_ratio_)/kpe_ratio_;
    if(tfidf_threshold_<0.0) tfidf_threshold_ = 0.0;
    if(kpe_threshold_<0.0) kpe_threshold_ = 0.0;
    
    //manually set
    sim_threshold_ = 0.8;
    tfidf_threshold_ = 0.8;
    kpe_threshold_ = 0.6;
}

void SimAlgorithm::Continue_()
{
//     {
//         writer_ = new izenelib::am::ssf::Writer<>(working_dir_+"/candidate_writer");
//         writer_->Open();
//         {
//             //import kpe candidate
//             izenelib::am::ssf::Reader<> reader(working_dir_+"/kpe_candidate_writer");
//             reader.Open();
//             uint64_t id = 0;
//             std::pair<double, uint32_t> value;
//             while( reader.Next(id, value) )
//             {
//                 writer_->Append(id, value);
//             }
//             reader.Close();
//         }
//         uint32_t dimensions = 0;
//         std::string path = working_dir_+"/apss_input";
//         {
//             uint32_t docid = 0;
//             izenelib::am::SparseVector<double, uint32_t> vec;
//             izenelib::am::ssf::Reader2<> reader(path);
//             reader.Open();
//             while(reader.Next(docid, vec))
//             {
//                 for(uint32_t i=0;i<vec.value.size();i++)
//                 {
//                     if(vec.value[i].first>dimensions)
//                     {
//                         dimensions = vec.value[i].first;
//                     }
//                 }
//             }
//             reader.Close();
//         }
//         std::cout<<"start tfidf apss with "<<tfidf_threshold_<<","<<dimensions<<","<<start_docid_<<std::endl;
//         idmlib::ssp::Apss<uint32_t, uint32_t> apss(tfidf_threshold_, boost::bind( &SimAlgorithm::FindTfidfSim_, this, _1, _2, _3), dimensions);
//         izenelib::am::SparseVector<double, uint32_t> value;
//         apss.ComputeWithStart(path, start_docid_, value);
//         writer_->Close();
//         path = writer_->GetPath();
//         delete writer_;
//         writer_ = NULL;
//         izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(path);
//         
//         return;
//     }

    //split
//     {
//         izenelib::am::ssf::Writer<> writer(working_dir_+"/tfidf_candidate_writer");
//         izenelib::am::ssf::Reader<> reader(working_dir_+"/candidate_writer");
//         reader.Open();
//         writer.Open();
//         uint64_t id = 0;
//         std::pair<double, uint32_t> value;
//         while( reader.Next(id, value) )
//         {
//             if(value.second==1)
//             {
//                 writer.Append(id, value);
//             }
//         }
//         reader.Close();
//         writer.Close();
//         return ;
//     }
    
    
    {
        std::string path = working_dir_+"/kpe_candidate_writer";
        izenelib::am::ssf::Reader<> reader(path);
        reader.Open();
        std::cout<<"candidate_writer count : "<<reader.Count()<<std::endl;
        reader.Close();
        {
            uint64_t process_count = 0;
            izenelib::am::ssf::Joiner<uint32_t, uint64_t, std::pair<double, uint32_t> > joiner(path);
            joiner.Open();
            uint64_t id;
            std::vector<std::pair<double, uint32_t> > score_list;
            while( joiner.Next(id, score_list) )
            {
                process_count++;
                if(process_count%10000==0)
                {
                    std::cout<<"processed "<<process_count<<std::endl;
                }
                double score = 0.0;
                std::pair<uint32_t, uint32_t> docid_pair = izenelib::util::IdUtil::Get32(id);
                if(score_list.size()==1)
                {
                    score = score_list[0].first;
                }
                else if(score_list.size()==2)
                {
                    if(score_list[0].second == 1) //tfidf type
                    {
                        score = score_list[0].first*tfidf_ratio_ + score_list[1].first*kpe_ratio_;
                    }
                    else
                    {
                        score = score_list[1].first*tfidf_ratio_ + score_list[0].first*kpe_ratio_;
                    }
                }
                if(score >= sim_threshold_ )
                {
                    FindSim_(docid_pair.first, docid_pair.second, score);
                }
            }
        }
        return;
    }
    
    writer_ = new izenelib::am::ssf::Writer<>(working_dir_+"/candidate_writer");
    writer_->Open();
    uint32_t dimensions = 0;
    std::string path = working_dir_+"/apss_kpe";
    {
        uint32_t docid = 0;
        izenelib::am::SparseVector<double, uint32_t> vec;
        izenelib::am::ssf::Reader2<> reader(path);
        reader.Open();
        while(reader.Next(docid, vec))
        {
            for(uint32_t i=0;i<vec.value.size();i++)
            {
                if(vec.value[i].first>dimensions)
                {
                    dimensions = vec.value[i].first;
                }
            }
        }
        reader.Close();
    }
    std::cout<<"start kpe apss with "<<kpe_threshold_<<","<<dimensions<<","<<start_docid_<<std::endl;
    idmlib::ssp::Apss<uint32_t, uint32_t> apss(kpe_threshold_, boost::bind( &SimAlgorithm::FindKpeSim_, this, _1, _2, _3), dimensions);
    izenelib::am::SparseVector<double, uint32_t> value;
    apss.ComputeWithStart(path, start_docid_, value);
    CombineSim_();
}
        
bool SimAlgorithm::ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_property)
{
    std::cout<<"[SimAlgorithm::ProcessCollection]"<<std::endl;
    std::string continue_file = working_dir_+"/continue";
    if( boost::filesystem::exists(continue_file) )
    {
        Continue_();
        return true;
    }

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
    boost::scoped_ptr<TermIterator> term_iterator( term_reader->termIterator(title_property.second.c_str()) );
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
    writer_ = new izenelib::am::ssf::Writer<>(working_dir_+"/candidate_writer");
    writer_->Open();
    std::cout<<"start tfidf apss with "<<tfidf_threshold_<<","<<dimensions<<","<<start_docid_<<std::endl;
    idmlib::ssp::Apss<uint32_t, uint32_t> apss(tfidf_threshold_, boost::bind( &SimAlgorithm::FindTfidfSim_, this, _1, _2, _3), dimensions);
    izenelib::am::SparseVector<double, uint32_t> value;
    apss.ComputeWithStart(apss_input.GetPath(), start_docid_, value);
    
    //TODO do kpe task to calculate sim value
//     KpeTask_(document_manager, title_property, content_property);
    CombineSim_();
    //end, clean
//     boost::filesystem::remove_all(working_dir_);
    return true;
}

void SimAlgorithm::KpeTask_(const boost::shared_ptr<DocumentManager>& document_manager, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_property)
{
    std::string working_path = working_dir_+"/kpe_working";
    
    idmlib::util::IDMIdManager* id_manager = new idmlib::util::IDMIdManager(working_path+"/idmanager");
    KpeKnowledge* knowledge = new KpeKnowledge(analyzer_);
    knowledge->Load(kpe_resource_path_);
    KpeTask::DocKpCallback doc_callback = boost::bind( &SimAlgorithm::KpeCallback_, this, _1, _2);
    kpe_writer_ = new izenelib::am::ssf::Writer<>(working_dir_+"/apss_kpe");
    kpe_writer_->Open();
    uint32_t process_max_docid = document_manager->getMaxDocId();
    uint32_t step = 500000;
    uint32_t start_docid = start_docid_;
    while(true)
    {
        boost::filesystem::remove_all(working_path);
        KpeTask task(working_path, analyzer_, cma_analyzer_, id_manager, knowledge);
        task.SetCallback(doc_callback);
        
        uint32_t process_count = 0;
        
        Document doc;
        for( uint32_t docid = start_docid; docid<=process_max_docid; docid++)
        {
            StringType title;
            StringType content;
            process_count++;
            start_docid = docid;
            if(process_count==step) break;
            if( process_count %1000 == 0 )
            {
                MEMLOG("[EC-SIM-KPE] inserted %d. docid: %d", process_count, docid);
            }
            bool b = document_manager->getDocument(docid, doc);
            if(!b) continue;
            Document::property_iterator property_it = doc.propertyBegin();

            while(property_it != doc.propertyEnd() )
            {
                std::string str_property = property_it->first;
                if( str_property == title_property.second)
                {
                    title = property_it->second.get<izenelib::util::UString>();
                }
                else if( str_property == content_property.second)
                {
                    content = property_it->second.get<izenelib::util::UString>();
                }
                
                property_it++;
            }
            task.Insert(docid, title, content);
        }
        task.Close();
        if(start_docid == process_max_docid) break;
    }
    delete knowledge;
    delete id_manager;
    kpe_writer_->Close();
    std::string path = kpe_writer_->GetPath();
    delete kpe_writer_;
    kpe_writer_ = NULL;
    uint32_t dimensions = max_kpid_;
    std::cout<<"start kpe apss with "<<kpe_threshold_<<","<<dimensions<<","<<start_docid_<<std::endl;
    idmlib::ssp::Apss<uint32_t, uint32_t> apss(kpe_threshold_, boost::bind( &SimAlgorithm::FindKpeSim_, this, _1, _2, _3), dimensions);
    izenelib::am::SparseVector<double, uint32_t> value;
    apss.ComputeWithStart(path, start_docid_, value);
    max_kpid_ = 0;
}

void SimAlgorithm::CombineSim_()
{
    writer_->Close();
    std::string path = writer_->GetPath();
    delete writer_;
    writer_ = NULL;
    izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(path);
    {
        izenelib::am::ssf::Joiner<uint32_t, uint64_t, std::pair<double, uint32_t> > joiner(path);
        joiner.Open();
        uint64_t id;
        std::vector<std::pair<double, uint32_t> > score_list;
        while( joiner.Next(id, score_list) )
        {
            double score = 0.0;
            std::pair<uint32_t, uint32_t> docid_pair = izenelib::util::IdUtil::Get32(id);
            if(score_list.size()==1)
            {
                score = score_list[0].first;
            }
            else if(score_list.size()==2)
            {
                if(score_list[0].second == 1) //tfidf type
                {
                    score = score_list[0].first*tfidf_ratio_ + score_list[1].first*kpe_ratio_;
                }
                else
                {
                    score = score_list[1].first*tfidf_ratio_ + score_list[0].first*kpe_ratio_;
                }
            }
            if(score >= sim_threshold_ )
            {
                FindSim_(docid_pair.first, docid_pair.second, score);
            }
        }
    }
    
}

void SimAlgorithm::KpeCallback_(uint32_t docid, const std::vector<DocKpItem>& kp_list)
{
    double a = 0.0;
    for(uint32_t i=0;i<kp_list.size();i++)
    {
        a += kp_list[i].score*kp_list[i].score;
    }
    if(a==0.0) return;
    a = std::sqrt(a);
    izenelib::am::SparseVector<double, uint32_t> sparse_vec;
    for(uint32_t i=0;i<kp_list.size();i++)
    {
        double score = kp_list[i].score/a;
        uint32_t kpid = kp_list[i].id;
        sparse_vec.value.push_back(std::make_pair(kpid, score) );
        if(kpid>max_kpid_) max_kpid_ = kpid;
    }
    std::sort(sparse_vec.value.begin(), sparse_vec.value.end());
    kpe_writer_->Append(docid, sparse_vec);
}

void SimAlgorithm::FindTfidfSim_(uint32_t docid1, uint32_t docid2, double score)
{
    uint64_t id = izenelib::util::IdUtil::Get64(docid1, docid2);
    std::pair<double ,uint32_t> value( score, 1);// score and type;
    writer_->Append(id, value);
    
}

void SimAlgorithm::FindKpeSim_(uint32_t docid1, uint32_t docid2, double score)
{
    uint64_t id = izenelib::util::IdUtil::Get64(docid1, docid2);
    std::pair<double ,uint32_t> value( score, 2);// score and type;
    writer_->Append(id, value);

    
}

void SimAlgorithm::FindSim_(uint32_t docid1, uint32_t docid2, double score)
{
    //     std::cout<<"SimAlgorithm::FindSim_ : "<<docid1<<","<<docid2<<" - "<<score<<std::endl;
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



