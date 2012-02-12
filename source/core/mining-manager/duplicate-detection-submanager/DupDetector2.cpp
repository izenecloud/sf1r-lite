#include "DupDetector2.h"
#include "dd_constants.h"
#include "charikar_algo.h"

#include <document-manager/DocumentManager.h>
#include <common/Utilities.h>

#include <util/ClockTimer.h>
#include <util/url_util.h>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/similarity/term_similarity.h>

// #define DUPD_GROUP_DEBUG;
// #define DUPD_TEXT_DEBUG;
// #define DUPD_DEBUG;
// #define DUPD_FP_DEBUG;
// #define NO_IN_SITE;
using namespace std;
namespace sf1r
{

DupDetector2::DupDetector2(const std::string& container)
    : container_(container), document_manager_(), analyzer_(NULL)
    , file_info_(new FileObjectType(container + "/file_obj"))
    , fp_only_(false)
    , maxk_(0), partition_num_(0)
    , algo_(NULL)
    , fp_storage_(container + "/fp")
    , group_(NULL)
    , processed_max_docid_(0)
    , processing_max_docid_(0)
{
    SetParameters_();
}

DupDetector2::DupDetector2(
        const std::string& container,
        const boost::shared_ptr<DocumentManager>& document_manager,
        const std::vector<std::string>& properties,
        idmlib::util::IDMAnalyzer* analyzer,
        bool fp_only)
    : container_(container), document_manager_(document_manager), analyzer_(analyzer)
    , file_info_(new FileObjectType(container + "/file_obj"))
    , fp_only_(fp_only)
    , maxk_(0), partition_num_(0)
    , algo_(NULL)
    , fp_storage_(container + "/fp")
    , group_(NULL)
    , processed_max_docid_(0)
    , processing_max_docid_(0)
{
    SetParameters_();
    for (uint32_t i = 0; i < properties.size(); i++)
    {
        dd_properties_.insert(properties[i], 0);
    }
}

void DupDetector2::SetParameters_()
{
    maxk_ = 6;
    partition_num_ = 9;
    trash_threshold_ = 0.02;
    trash_min_ = 200;
}

uint8_t DupDetector2::GetK_(uint32_t doc_length)
{
    // (document length, k) follows a log based regression function
    // while the document length gets larger, k value decreases.
    // 1 is added to doc_length because ln(0) is not defined (1 is a kind of smothing factor)
//   return 3;
    double y = -1.619429845 * log(double(doc_length + 1)) + 17.57504447;
    if (y <= 0) y = 0;
    uint8_t k = (uint8_t)y;
    if (y - double(k) >= 0.5) k++;
    if (k > 6) k = 6;

    return k;
}

DupDetector2::~DupDetector2()
{
    delete file_info_;
    if (algo_)
        delete algo_;
    if (group_)
        delete group_;
}

bool DupDetector2::Open()
{
    std::string fp_dir = container_ + "/fp";
    try
    {
        boost::filesystem::create_directories(container_);
        boost::filesystem::create_directories(fp_dir);
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return false;
    }

    if (file_info_->Load())
    {
        processed_max_docid_ = file_info_->GetValue();
        //     maxk_ = file_info_.GetValue().second;
    }
    algo_ = new CharikarAlgo(container_.c_str(), DdConstants::f, 0.95);
    FpTables tables;
    tables.GenTables(DdConstants::f, maxk_, partition_num_, table_list_);
    std::cout << "Generated " << table_list_.size() << " tables for maxk =" << (int) maxk_ << std::endl;

    std::string inject_file = container_ + "/inject.txt";
    std::string group_file = container_ + "/group";

    fp_storage_.GetFpByTableId(0, fp_vec_);
    processed_max_docid_ = fp_vec_.size();

    if (id_manager_ && boost::filesystem::exists(inject_file))
    {
        std::cout << "start inject data to DD result" << std::endl;
        boost::filesystem::remove_all(group_file);
        group_ = new GroupType(group_file);
        if (!group_->Load())
        {
            std::cerr << "Load group info failed" << std::endl;
        }
        std::ifstream ifs(inject_file.c_str());
        std::string line;
        std::vector<uint32_t> in_group;
        while (getline(ifs,line))
        {
            boost::algorithm::trim(line);
            if (line.empty() && !in_group.empty())
            {
                //process in_group
                for (uint32_t i = 1; i < in_group.size(); i++)
                {
                    group_->AddDoc(in_group[0], in_group[i]);
                }

                in_group.resize(0);
                continue;
            }
            if (!line.empty())
            {
                std::vector<std::string> vec;
                boost::algorithm::split(vec, line, boost::algorithm::is_any_of("\t"));
                //                 std::cout << "XXX " << vec[0] << std::endl;
                izenelib::util::UString str_docid(vec[0], izenelib::util::UString::UTF_8);

                uint32_t docid = 0;
                if (id_manager_->getDocIdByDocName(str_docid, docid, false))
                {
                    in_group.push_back(docid);
                }
                else
                {
                    std::cout << "docid " << vec[0] << " does not exists." << std::endl;
                }

            }

        }
        ifs.close();
        //process in_group
        if (!in_group.empty())
        {
            for (uint32_t i = 1; i < in_group.size(); i++)
            {
                group_->AddDoc(in_group[0], in_group[i]);
            }
        }
        group_->Flush();
        boost::filesystem::remove_all(inject_file);
        std::cout << "updated dd by " << inject_file << std::endl;
        //         boost::filesystem::remove_all(output_group_file);
        //         std::string reoutput_file = container_ + "/re_output_group.txt";
        //         OutputResult_(reoutput_file);
    }
    else
    {
        group_ = new GroupType(group_file);
        if (!group_->Load())
        {
            std::cerr << "Load group info failed" << std::endl;
        }
    }

    std::string continue_file = container_ + "/continue";
    if (boost::filesystem::exists(continue_file))
    {
        boost::filesystem::remove_all(continue_file);
        runDuplicateDetectionAnalysis(true);
    }
    return true;

}

bool DupDetector2::SkipTrash_(uint32_t total_count, uint32_t pairwise_count, uint32_t table_id)
{
    if (table_id == 0) return false; //first table.
    if (pairwise_count < trash_min_) return false;
    else return true;
//   double d = (double)pairwise_count/total_count;
//   if (d >= trash_threshold_)
//   {
//     return true;
//   }
//   return false;
}

void DupDetector2::FindDD_(const std::vector<FpItem>& data, const FpTable& table, uint32_t table_id)
{
    std::vector<FpItem> for_compare;
    bool is_first = true;
    std::vector<uint64_t> compare_value, last_compare_value;
    uint32_t this_max_docid = 0;
    uint32_t total_count = data.size();
    uint32_t dd_count = 0;
    for (uint32_t i = 0; i < data.size(); i++)
    {
        uint32_t docid = data[i].docid;
        if (docid == 0) continue;
//     if (i%10000==0) std::cout << "[dup-id] " << i << std::endl;
        table.GetMaskedBits(data[i].fp, compare_value);
        if (is_first)
        {
        }
        else
        {
            if (last_compare_value != compare_value)
            {
                //process for_compare
                if (this_max_docid > processed_max_docid_)
                {
                    if (!SkipTrash_(total_count, for_compare.size(), table_id))
                        PairWiseCompare_(for_compare, dd_count);
                }
                for_compare.clear();
                this_max_docid = 0;
            }
        }
        for_compare.push_back(data[i]);
        last_compare_value = compare_value;

        if (docid > this_max_docid)
        {
            this_max_docid = docid;
        }
        is_first = false;
    }
    if (this_max_docid > processed_max_docid_)
    {
        if (!SkipTrash_(total_count, for_compare.size(), table_id))
            PairWiseCompare_(for_compare, dd_count);
    }
    for_compare.clear();
    std::cout << "table id " << table_id << " find " << dd_count << " dd pairs." << std::endl;
}

void DupDetector2::PairWiseCompare_(const std::vector<FpItem>& for_compare, uint32_t& dd_count)
{
    if (for_compare.empty()) return;
#ifdef DUPD_GROUP_DEBUG
    std::cout << "[in-group] " << for_compare.size() << std::endl;
#endif
    for (uint32_t i = 0; i < for_compare.size(); i++)
    {
        for (uint32_t j = i + 1; j < for_compare.size(); j++)
        {
            if (for_compare[i].docid <= processed_max_docid_
                    && for_compare[j].docid <= processed_max_docid_) continue;
            if (IsDuplicated_(for_compare[i], for_compare[j]))
            {
                ++dd_count;
                boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
                group_->AddDoc(for_compare[i].docid, for_compare[j].docid);
            }
        }
    }
}

bool DupDetector2::IsDuplicated_(const FpItem& item1, const FpItem& item2)
{
    uint32_t hamming_dist = Utilities::calcHammingDist(item1.fp, item2.fp);
    bool result = false;

    // K should be selected by longer document. (longer document generates lower K value)
    // Therefore, std::max is used to get longer document.
    uint32_t doc_length = std::max(item1.length, item2.length);

    // We need to check the corpus is based on English alphabet.
    // Because GetK_ function is based on actual document length.
    // In here, 7 is multiplied to document length. (7 means the average word length in Enlgish)
//   bool isEnglish = true;
//   if (isEnglish)

    doc_length *= 7;
    uint8_t k = GetK_(doc_length);
    if (hamming_dist <= k)
    {
        result = true;
#ifdef NO_IN_SITE
        std::string url_property = "Url";
        std::string url1;
        std::string url2;
        GetPropertyString_(item1.docid, url_property, url1);
        GetPropertyString_(item2.docid, url_property, url2);

        std::string base_site1;
        std::string base_site2;
        if (!izenelib::util::UrlUtil::GetBaseSite(url1, base_site1))
        {
            std::cout << "get base site error for " << url1 << std::endl;
            return false;
        }
        if (!izenelib::util::UrlUtil::GetBaseSite(url2, base_site2))
        {
            std::cout << "get base site error for " << url2 << std::endl;
            return false;
        }
//     std::cout << "SITE: " << url1 << " -> " << base_site1 << std::endl;
//     std::cout << "SITE: " << url2 << " -> " << base_site2 << std::endl;
        if (base_site1 == base_site2) result = false;
#endif
    }
#ifdef DUPD_DEBUG
    if (result)
    {
        std::cout << "find dd: " << item1.docid << " , " << item2.docid << " : " << (uint32_t) k << "," << hammDist << std::endl;
    }
#endif
//   std::cout << "D" << item1.docid << ",D" << item2.docid << " " << c.GetCount() << " " << item1.length << "," << item2.length << " " << (int)result << std::endl;
    return result;
}

// void DupDetector2::DataDupd(const std::vector<std::pair<uint32_t, izenelib::util::CBitArray> >& data, uint8_t group_num, uint32_t min_docid)
// {
//   std::vector<std::pair<uint32_t, izenelib::util::CBitArray> > for_compare;
//   bool is_first = true;
//   uint64_t last_compare_value = 0;
//   uint32_t this_max_docid = 0;
//   for (uint32_t i = 0; i < data.size(); i++)
//   {
//     uint32_t docid = data[i].first;
//     uint64_t compare_value = compare_obj_[group_num].GetBitsValue(data[i].second);
//     if (is_first)
//     {
//     }
//     else
//     {
//
//       if (last_compare_value != compare_value)
//       {
//         //process for_compare
//         if (this_max_docid >= min_docid) PairWiseCompare_(for_compare, min_docid);
//         for_compare.resize(0);
//         this_max_docid = 0;
//       }
//     }
//     for_compare.push_back(data[i]);
//     last_compare_value = compare_value;
//
//     if (docid > this_max_docid)
//     {
//       this_max_docid = docid;
//     }
//     is_first = false;
//   }
//   if (this_max_docid >= min_docid) PairWiseCompare_(for_compare, min_docid);
//   for_compare.resize(0);
// }

uint32_t DupDetector2::GetProcessedMaxId_()
{
    return processed_max_docid_;
}

//bool DupDetector2::ProcessCollectionBySim_()
//{
//    process_start_docid_ = GetProcessedMaxId_() + 1;
//    processing_max_docid_ = document_manager_->getMaxDocId();
//    if (processing_max_docid_ < process_start_docid_)
//    {
//        std::cout << "No document need to processed: from " << process_start_docid_ << " to " << processing_max_docid_ << std::endl;
//        return true;
//    }
//    uint32_t total_doc_count = processing_max_docid_-process_start_docid_+1;
//    std::cout << "Will processing from " << process_start_docid_ << " to " << processing_max_docid_ << std::endl;
//
//    uint32_t process_count = 0;
//    Document doc;
//    typedef izenelib::am::SparseVector<double, uint32_t> SparseType;
//    izenelib::am::ssf::Writer<> matrix_writer1(container_ + "/matrix1");
//    matrix_writer1.Open();
//    izenelib::am::rde_hash<izenelib::util::UString, uint32_t > term_id_map;
//    izenelib::am::rde_hash<uint32_t, uint32_t> term_df_map;
//    uint32_t avail_term_id = 1;
//    uint32_t docid = 0;
//    for (docid = process_start_docid_; docid <= processing_max_docid_; docid++)
//    {
//        process_count++;
//        if (process_count % 1000 == 0)
//        {
//            MEMLOG("[DUPD2] inserted %d. docid: %d", process_count, docid);
//        }
//        bool b = document_manager_->getDocument(docid, doc);
//        if (!b) continue;
//        Document::property_iterator property_it = doc.propertyBegin();
//
//#ifdef DUPD_TEXT_DEBUG
//        std::cout << "[" << docid << "]" << std::endl;;
//#endif
//        SparseType sparse_vec;
//        while (property_it != doc.propertyEnd())
//        {
//            if (dd_properties_.find(property_it->first))
//            {
//                const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
//                std::vector<izenelib::util::UString> termStrList;
//                analyzer_->GetStringList(content, termStrList);
//#ifdef DUPD_TEXT_DEBUG
//                std::string content_str;
//                content.convertString(content_str, izenelib::util::UString::UTF_8);
//                std::cout << "[" << property_it->first << "] " << content_str << std::endl;
//#endif
//                for (uint32_t u = 0; u < termStrList.size(); u++)
//                {
//                    termStrList[u].toLowerString();
//                    if (termStrList[u]== izenelib::util::UString("-", izenelib::util::UString::UTF_8))
//                    {
//                        continue;
//                    }
//                    if (termStrList[u]== izenelib::util::UString(" ", izenelib::util::UString::UTF_8))
//                    {
//                        continue;
//                    }
//#ifdef DUPD_TEXT_DEBUG
//                    std::string display;
//                    termStrList[u].convertString(display, izenelib::util::UString::UTF_8);
//                    std::cout << display << ",";
//#endif
//                    uint32_t* p_term_id = term_id_map.find(termStrList[u]);
//                    uint32_t term_id = 0;
//                    if (!p_term_id)
//                    {
//                        term_id = avail_term_id;
//                        avail_term_id++;
//                        term_id_map.insert(termStrList[u], term_id);
//                    }
//                    else
//                    {
//                        term_id = *p_term_id;
//                    }
//                    sparse_vec.value.push_back(std::make_pair(term_id, 1.0));
//
//                }
//#ifdef DUPD_TEXT_DEBUG
//                std::cout << std::endl;
//#endif
//            }
//            property_it++;
//        }
//
//        uint32_t doc_length = sparse_vec.value.size();
//        idmlib::ssp::VectorUtil::Flush(sparse_vec);
//        for (uint32_t i = 0; i < sparse_vec.value.size(); i++)
//        {
//            sparse_vec.value[i].second /= doc_length;
//            uint32_t id = sparse_vec.value[i].first;
//            uint32_t* p_term_df = term_df_map.find(id);
//            if (!p_term_df)
//            {
//                term_df_map.insert(id, 1);
//            }
//            else
//            {
//                term_df_map.update(id, (*p_term_df) + 1);
//            }
//        }
//        matrix_writer1.Append(docid, sparse_vec);
//
//    }
//    matrix_writer1.Close();
//    //DO tdidf and normalization
//    izenelib::am::ssf::Writer<> matrix_writer2(container_ + "/matrix2");
//    matrix_writer2.Open();
//    izenelib::am::ssf::Reader<> matrix_reader(container_ + "/matrix1");
//    matrix_reader.Open();
//    SparseType sparse_vec;
//    while (matrix_reader.Next(docid, sparse_vec))
//    {
//        for (uint32_t i = 0; i < sparse_vec.value.size(); i++)
//        {
//            uint32_t* df = term_df_map.find(sparse_vec.value[i].first);
//            double idf = std::log(double(total_doc_count) / (*df));
//            sparse_vec.value[i].second *= idf;
//        }
//        SparseType normal_vec;
//        idmlib::ssp::Normalizer::Normalize(sparse_vec, normal_vec);
////       std::cout << "[N] ";
////       for (uint32_t i = 0; i < normal_vec.value.size(); i++)
////       {
////           std::cout << normal_vec.value[i].first << "-" << normal_vec.value[i].second << ",";
////       }
////       std::cout << std::endl;
//        matrix_writer2.Append(docid, normal_vec);
//    }
//    matrix_reader.Close();
//    matrix_writer2.Close();
//
//    typedef idmlib::ssp::Apss<uint32_t, uint32_t> ApssType;
//    ApssType apss(0.9, boost::bind(&DupDetector2::FindSim_, this, _1, _2, _3), avail_term_id);
//    SparseType normal_vec;
//    apss.Compute(container_ + "/matrix2", docid, normal_vec);
//
//    //output to text file for manually review
//    std::string output_txt_file = container_ + "/output_group.txt";
//    OutputResult_(output_txt_file);
//    return true;
//}

void DupDetector2::OutputResult_(const std::string& file)
{
    std::string title_property = "Title";
    std::string url_property = "Url";
    std::ofstream ofs(file.c_str());
    const std::vector<std::vector<uint32_t> >& group_info = group_->GetGroupInfo();
    std::cout << "[TOTAL GROUP SIZE] : " << group_info.size() << std::endl;
    for (uint32_t group_id = 0; group_id < group_info.size(); group_id++)
    {
        const std::vector<uint32_t>& in_group = group_info[group_id];
        for (uint32_t i = 0; i < in_group.size(); i++)
        {
            uint32_t docid = in_group[i];
            std::string title;
            std::string url;
            GetPropertyString_(docid, title_property, title);
            GetPropertyString_(docid, url_property, url);
            ofs << docid << "\t" << title << "\t" << url << std::endl;
        }
        ofs << std::endl << std::endl;
    }
    ofs.close();
}

void DupDetector2::FindSim_(uint32_t docid1, uint32_t docid2, double score)
{
    std::string title_property = "Title";
    std::string url_property = "Url";
    std::string url1;
    std::string url2;
    GetPropertyString_(docid1, url_property, url1);
    GetPropertyString_(docid2, url_property, url2);

    std::string base_site1;
    std::string base_site2;
    if (!izenelib::util::UrlUtil::GetBaseSite(url1, base_site1))
    {
        std::cout << "get base site error for " << url1 << std::endl;
        return;
    }
    if (!izenelib::util::UrlUtil::GetBaseSite(url2, base_site2))
    {
        std::cout << "get base site error for " << url2 << std::endl;
        return;
    }
//     std::cout << "SITE: " << url1 << " -> " << base_site1 << std::endl;
//     std::cout << "SITE: " << url2 << " -> " << base_site2 << std::endl;
    if (base_site1 == base_site2) return;


    std::string title1;
    std::string title2;
    GetPropertyString_(docid1, title_property, title1);
    GetPropertyString_(docid2, title_property, title2);

//     std::cout << "[DUPD2-FINDSIM] " << title1 << " *|* " << title2 << " : " << score << std::endl;

    boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
    group_->AddDoc(docid1, docid2);
}

bool DupDetector2::GetPropertyString_(uint32_t docid, const std::string& property, std::string& value)
{
    Document doc;
    if (!document_manager_->getDocument(docid, doc)) return false;
    Document::property_iterator property_it = doc.findProperty(property);
    if (property_it == doc.propertyEnd()) return false;
    const izenelib::util::UString& ustr = property_it->second.get<izenelib::util::UString>();
    ustr.convertString(value, izenelib::util::UString::UTF_8);
    return true;
}

bool DupDetector2::ProcessCollection()
{
    return ProcessCollectionBySimhash_();
//     return ProcessCollectionBySim_();
}

bool DupDetector2::ProcessCollectionBySimhash_()
{
    processing_max_docid_ = document_manager_->getMaxDocId();
    if (processing_max_docid_ <= processed_max_docid_)
    {
        std::cout << "No document need to processed: from " << processed_max_docid_ + 1 << " to " << processing_max_docid_ << std::endl;
        return true;
    }
    std::cout << "Will processing from " << processed_max_docid_ + 1 << " to " << processing_max_docid_ << std::endl;

    fp_vec_.resize(processing_max_docid_);
    uint32_t process_count = 0;
    Document doc;

    for (uint32_t docid = processed_max_docid_ + 1; docid <= processing_max_docid_; docid++)
    {
        process_count++;
        if (process_count % 1000 == 0)
        {
            MEMLOG("[DUPD] inserted %d. docid: %d", process_count, docid);
        }
        if (!document_manager_->getDocument(docid, doc))
            continue;

        Document::property_iterator property_it = doc.propertyBegin();

#ifdef DUPD_TEXT_DEBUG
        std::cout << "[" << docid << "] ";
#endif
        FpItem& fp_item = fp_vec_[docid - 1];
        std::vector<izenelib::util::UString> content_list;
        while (property_it != doc.propertyEnd())
        {
            if (dd_properties_.find(property_it->first))
            {
                const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
                content_list.push_back(content);
            }
            property_it++;
        }
        std::vector<uint64_t> signature;
        uint32_t text_len = getSignatureForMultiText(content_list, signature);
        if (text_len)
        {
            fp_item.docid = docid;
            fp_item.length = text_len;
            fp_item.fp.swap(signature);
        }
#ifdef DUPD_TEXT_DEBUG
        std::cout << std::endl;
#endif
    }

    return runDuplicateDetectionAnalysis();
}

/**
 * Get a duplicated document list to a given document.
 * @return if no dupDocs for this docId, return false, else return false;
 */
bool DupDetector2::getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList)
{

    boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
    group_->Find(docId, docIdList);
    if (!docIdList.empty())
    {
        for (uint32_t i = 0; i < docIdList.size(); i++)
        {
            if (docIdList[i] == docId)
            {
                docIdList.erase(docIdList.begin()+i);
                break;
            }
        }
    }
    return true;
}

/// Get a unique document id list with duplicated ones excluded.
bool DupDetector2::getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs)
{
    boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
    ///Get all the dup docs of the collection.
    cleanDocs.assign(docIdList.begin(), docIdList.end());
    for (uint32_t i = 0; i < cleanDocs.size(); ++i)
    {
        for (uint32_t j = i + 1; j < cleanDocs.size(); ++j)
        {
            if (isDuplicated(cleanDocs[i], cleanDocs[j]))
            {
                cleanDocs.erase(cleanDocs.begin()+j);
                --j;
            }
        }

    }

    return true;
}

/// Tell two documents belonging to the same collections are duplicated.
bool DupDetector2::isDuplicated(uint32_t docId1, uint32_t docId2)
{
    return group_->IsSameGroup(docId1, docId2);
}

//void DupDetector2::insertDocument(uint32_t docid, const std::vector<std::string>& v)
//{
//    if (docid <= fp_vec_.size() && docid > 0)
//    {
//        uint64_t signature;
//        //  CharikarAlgorithm algo;
//        algo_->generate_document_signature(v, &signature);
//        fp_vec_[docid - 1] = FpItem(docid, v.size(), signature);
//        if (docid > processing_max_docid_)
//        {
//            processing_max_docid_ = docid;
//        }
//    }
//    else
//    {
//        std::cerr << "not enough space for docid " << docid << std::endl;
//    }
//}

bool DupDetector2::runDuplicateDetectionAnalysis(bool force)
{
    if (!force && processed_max_docid_ == processing_max_docid_)
    {
        std::cout << "No data for DD" << std::endl;
        return true;
    }
    uint32_t table_count = table_list_.size();
    //only save one copy of all fps, sort it in every table time.
    std::vector<FpItem> processed;

    std::cout << "Processed doc count : " << processed_max_docid_ << std::endl;
    if (!fp_storage_.SetFpByTableId(0, fp_vec_))
    {
        std::cerr << "Save fps failed" << std::endl;
    }

#ifdef DUPD_FP_DEBUG
    for (uint32_t i = 0; i < fp_vec_.size(); i++)
    {
        const FpItem& item = fp_vec_[i];
        std::cout << "FP " << item.docid << "," << item.length << ",";
        item.fp.display(std::cout);
        std::cout << std::endl;
    }
#endif
    std::cout << "Now all doc count : " << fp_vec_.size() << std::endl;
    if (fp_only_)
    {
        processed_max_docid_ = processing_max_docid_;
        processing_max_docid_ = 0;
        return true;
    }

    std::cout << "Table count : " << table_count << std::endl;

    std::vector<FpItem> temp_fp_vec = fp_vec_;
    for (uint32_t tid = 0; tid < table_count; tid++)
    {
        MEMLOG("Processing table id : %d", tid);
        const FpTable& table = table_list_[tid];
        std::sort(temp_fp_vec.begin(), temp_fp_vec.end(), table);
        FindDD_(temp_fp_vec, table, tid);
    }

    uint32_t max = processing_max_docid_;
    processing_max_docid_ = 0;

    if (!group_->Flush())
    {
        std::cerr << "group flush error" << std::endl;
        return false;
    }
    processed_max_docid_ = max;
    file_info_->SetValue(processed_max_docid_);
    file_info_->Save();
    group_->PrintStat();
    //output to text file for manually review
//   std::string output_txt_file = container_ + "/output_group_dd.txt";
//   OutputResult_(output_txt_file);
    return true;
}

// bool DupDetector2::insertDocument_(uint32_t docid, const std::vector<std::string >& term_list)
// {
//
//     izenelib::util::CBitArray bitArray;
//     //  CharikarAlgorithm algo;
//     algo_->generate_document_signature(term_list, bitArray);
//
// //     std::cout << "[DUPD] " << docid << ",";
// //     bitArray.display(std::cout);
// //     std::cout << std::endl;
//
//     writer_->append(docid, bitArray);
//     if (docid > processed_max_docid_) processed_max_docid_ = docid;
//     return true;
//
// }
//
//
// /// Remove documents from a collection. From now, those documents should be excluded in the result.
// bool DupDetector2::removeDocument_(unsigned int docid)
// {
// //   if (!fp_index_->exist(docid))
// //     return false;
// //
// //     read_write_mutex_.lock_shared();
// //     fp_index_->del_doc(docid);
// //     read_write_mutex_.unlock_shared();
//  return true;
// }

uint32_t DupDetector2::getSignatureForText(
        const izenelib::util::UString& text,
        std::vector<uint64_t>& signature)
{
    std::vector<izenelib::util::UString> termList;
    analyzer_->GetFilteredStringList(text, termList);

    std::vector<std::string> strTermList(termList.size());
    for (uint32_t u = 0; u < termList.size(); u++)
    {
        termList[u].convertString(strTermList[u], izenelib::util::UString::UTF_8);
#ifdef DUPD_TEXT_DEBUG
        std::cout << strTermList[u] << ",";
#endif
    }
    if (!strTermList.empty())
    {
        //  CharikarAlgorithm algo;
        algo_->generate_document_signature(strTermList, signature);
    }
    return strTermList.size();
}

uint32_t DupDetector2::getSignatureForMultiText(
        const std::vector<izenelib::util::UString>& texts,
        std::vector<uint64_t>& signature)
{
    std::vector<izenelib::util::UString> termList;
    for (uint32_t u = 0; u < texts.size(); u++)
    {
        std::vector<izenelib::util::UString> tmpTermList;
        analyzer_->GetFilteredStringList(texts[u], tmpTermList);
        termList.insert(termList.begin(), tmpTermList.begin(), tmpTermList.end());
    }

    std::vector<std::string> strTermList(termList.size());
    for (uint32_t u = 0; u < termList.size(); u++)
    {
        termList[u].convertString(strTermList[u], izenelib::util::UString::UTF_8);
#ifdef DUPD_TEXT_DEBUG
        std::cout << strTermList[u] << ",";
#endif
    }
    if (!strTermList.empty())
    {
        //  CharikarAlgorithm algo;
        algo_->generate_document_signature(strTermList, signature);
    }
    return strTermList.size();
}

void DupDetector2::getKNNListBySignature(
        const std::vector<uint64_t>& signature,
        uint32_t count,
        std::vector<std::pair<uint32_t, FpItem> >& knn_list)
{
    knn_list.clear();
    knn_list.reserve(count + 1);

    for (uint32_t i = 0; i < fp_vec_.size(); i++)
    {
        uint32_t hamming_dist = Utilities::calcHammingDist(signature, fp_vec_[i].fp);
        knn_list.push_back(std::make_pair(hamming_dist, fp_vec_[i]));
        std::push_heap(knn_list.begin(), knn_list.end());
        if (knn_list.size() > count)
        {
            std::pop_heap(knn_list.begin(), knn_list.end());
            knn_list.pop_back();
        }
    }

    std::sort_heap(knn_list.begin(), knn_list.end());
}

}
