#include "dup_detector_wrapper.h"

#include <document-manager/DocumentManager.h>
#include <idmlib/util/idm_analyzer.h>

// #define DUPD_GROUP_DEBUG;
// #define DUPD_TEXT_DEBUG;
// #define DUPD_DEBUG;
// #define DUPD_FP_DEBUG;
// #define NO_IN_SITE;
using namespace std;
namespace sf1r
{

DupDetectorWrapper::DupDetectorWrapper(const std::string& container)
    : container_(container)
    , document_manager_()
    , analyzer_(NULL)
    , file_info_(new FileObjectType(container + "/last_docid"))
    , fp_only_(false)
    , group_table_(NULL)
    , dd_(NULL)
{
}

DupDetectorWrapper::DupDetectorWrapper(
        const std::string& container,
        const boost::shared_ptr<DocumentManager>& document_manager,
        const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& id_manager,
        const std::vector<std::string>& properties,
        idmlib::util::IDMAnalyzer* analyzer,
        bool fp_only)
    : container_(container)
    , document_manager_(document_manager)
    , id_manager_(id_manager)
    , analyzer_(analyzer)
    , file_info_(new FileObjectType(container + "/last_docid"))
    , fp_only_(fp_only)
    , group_table_(NULL)
    , dd_(NULL)
{
    for (uint32_t i = 0; i < properties.size(); i++)
    {
        dd_properties_.insert(properties[i], 0);
    }
}

DupDetectorWrapper::~DupDetectorWrapper()
{
    delete file_info_;
    if (group_table_)
        delete group_table_;
    if (dd_)
        delete dd_;
}

bool DupDetectorWrapper::Open()
{
    std::string inject_file = container_ + "/inject.txt";
    std::string group_table_file = container_ + "/group_table";

    if (!fp_only_)
    {
        if (id_manager_ && boost::filesystem::exists(inject_file))
        {
            std::cout << "start inject data to DD result" << std::endl;
            boost::filesystem::remove_all(group_table_file);
            group_table_ = new GroupTableType(group_table_file);
            if (!group_table_->Load())
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
                        group_table_->AddDoc(in_group[0], in_group[i]);
                    }

                    in_group.clear();
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
                    group_table_->AddDoc(in_group[0], in_group[i]);
                }
            }
            group_table_->Flush();
            boost::filesystem::remove_all(inject_file);
            std::cout << "updated dd by " << inject_file << std::endl;
            //         boost::filesystem::remove_all(output_group_file);
            //         std::string reoutput_file = container_ + "/re_output_group.txt";
            //         OutputResult_(reoutput_file);
        }
        else
        {
            group_table_ = new GroupTableType(group_table_file);
            if (!group_table_->Load())
            {
                std::cerr << "Load group info failed" << std::endl;
            }
        }
    }

    dd_ = new DDType(container_, group_table_, true, fp_only_);
    if (!dd_->Open()) return false;

    std::string continue_file = container_ + "/continue";
    if (boost::filesystem::exists(continue_file))
    {
        boost::filesystem::remove_all(continue_file);
        dd_->RunDdAnalysis(true);
    }
    return true;

}

bool DupDetectorWrapper::ProcessCollection()
{
    if (!dd_) return false;

    uint32_t processed_max_docid = 0;
    if (file_info_->Load())
    {
        processed_max_docid = file_info_->GetValue();
    }
    uint32_t processing_max_docid = document_manager_->getMaxDocId();
    if (processing_max_docid <= processed_max_docid)
    {
        std::cout << "No document need to processed: from " << processed_max_docid + 1 << " to " << processing_max_docid << std::endl;
        return true;
    }
    std::cout << "Will processing from " << processed_max_docid + 1 << " to " << processing_max_docid << std::endl;

    uint32_t process_count = 0;
    Document doc;

    dd_->IncreaseCacheCapacity(processing_max_docid - processed_max_docid);
    for (uint32_t docid = processed_max_docid + 1; docid <= processing_max_docid; docid++)
    {
        ++process_count;
        if (process_count % 1000 == 0)
        {
            MEMLOG("[DUPD] inserted %d. docid: %d", process_count, docid);
        }
        if (!document_manager_->getDocument(docid, doc))
            continue;

#ifdef DUPD_TEXT_DEBUG
        std::cout << "[" << docid << "] ";
#endif

        std::vector<std::string> strTermList;
        for (Document::property_iterator property_it = doc.propertyBegin();
                property_it != doc.propertyEnd(); ++property_it)
        {
            if (dd_properties_.find(property_it->first))
            {
                const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();

                std::vector<izenelib::util::UString> termList;
                analyzer_->GetFilteredStringList(content, termList);
                for (uint32_t u = 0; u < termList.size(); u++)
                {
                    strTermList.push_back(std::string());
                    termList[u].convertString(strTermList.back(), izenelib::util::UString::UTF_8);
#ifdef DUPD_TEXT_DEBUG
                    std::cout << strTermList.back() << ",";
#endif
                }
            }
        }
        if (!strTermList.empty())
        {
            dd_->InsertDoc(docid, strTermList);
        }
#ifdef DUPD_TEXT_DEBUG
        std::cout << std::endl;
#endif
    }
    file_info_->SetValue(processing_max_docid);
    file_info_->Save();

    return dd_->RunDdAnalysis();
}

/**
 * Get a duplicated document list to a given document.
 * @return if no dupDocs for this docId, return false, else return false;
 */
bool DupDetectorWrapper::getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList)
{
    return dd_->GetDuplicatedDocIdList(docId, docIdList);
}

/// Get a unique document id list with duplicated ones excluded.
bool DupDetectorWrapper::getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs)
{
    return dd_->GetUniqueDocIdList(docIdList, cleanDocs);
}

uint32_t DupDetectorWrapper::getSignatureForText(
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
        dd_->GetCharikarAlgo()->generate_document_signature(strTermList, signature);
    }
    return strTermList.size();
}

void DupDetectorWrapper::getKNNListBySignature(
        const std::vector<uint64_t>& signature,
        uint32_t count,
        uint32_t start,
        uint32_t max_hamming_dist,
        std::vector<uint32_t>& docIdList,
        std::vector<float>& rankScoreList,
        std::size_t& totalCount)
{
    std::vector<std::pair<uint32_t, uint32_t> > knn_list;

    dd_->GetKNNListBySignature(signature, count + start, max_hamming_dist, knn_list);
    for (uint32_t i = start; i < knn_list.size(); i++)
    {
        docIdList.push_back(knn_list[i].second);
        rankScoreList.push_back(float(1) - float(knn_list[i].first) / float(idmlib::dd::DdConstants::f));
    }
    totalCount = knn_list.size() - start;
}

}
