/*
 * QueryDataBase.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "RecommendManager.h"
#include <mining-manager/concept-id-manager.h>
#include <mining-manager/auto-fill-submanager/AutoFillSubManager.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
#include <mining-manager/util/TermUtil.hpp>

#include <document-manager/DocumentManager.h>
#include <common/SFLogger.h>
#include <log-manager/LogManager.h>
#include <log-manager/UserQuery.h>
#include <log-manager/PropertyLabel.h>
#include <query-manager/QMCommonFunc.h>

#include <idmlib/util/idm_analyzer.h>
#include <glog/logging.h>

#include <cmath>

using namespace sf1r;

RecommendManager::RecommendManager(
        const std::string& path,
        const std::string& collection_name,
        const MiningSchema& mining_schema,
        const boost::shared_ptr<DocumentManager>& documentManager,
        boost::shared_ptr<QueryCorrectionSubmanager> query_correction,
        idmlib::util::IDMAnalyzer* analyzer,
        uint32_t logdays
)
    : path_(path)
    , isOpen_(false)
    , collection_name_(collection_name)
    , mining_schema_(mining_schema)
    , info_(0)
    , serInfo_(path_+"/ser_info")
    , document_manager_(documentManager)
    , recommend_db_(NULL), concept_id_manager_(NULL)
    , autofill_(new AutoFillSubManager())
    , query_correction_(query_correction)
    , analyzer_(analyzer)
    , logdays_(logdays)
    , dir_switcher_(path)
    , max_docid_(0), max_docid_file_(path+"/max_id")
{
    open();
}

RecommendManager::~RecommendManager()
{
    close();
}

bool RecommendManager::open()
{
    if (isOpen_) return true;
    if (max_docid_file_.Load())
    {
        max_docid_ = max_docid_file_.GetValue();
    }
    serInfo_.setCacheSize(1);
    serInfo_.open();
    bool b = serInfo_.get(0, info_);
    if (!b)
    {
        serInfo_.update(0, info_);
    }
    serInfo_.flush();
    if (!dir_switcher_.Open())
    {
        return false;
    }
    std::string current_recommend_path;
    if (!dir_switcher_.GetCurrent(current_recommend_path))
    {
        return false;
    }
    //try {
        std::cout<<"open ir manager on "<<current_recommend_path<<std::endl;
        recommend_db_ = new MIRDatabase(current_recommend_path);
        recommend_db_->setCacheSize<0>(100000000);
        recommend_db_->setCacheSize<1>(0);
        recommend_db_->open();
    //}
    //catch(std::exception& ex)
    //{
        //LOG(ERROR)<<ex.what()<<std::endl;
        //return false;
    //}
    std::cout<<"open ir manager finished"<<std::endl;
    std::string path_tocreate = current_recommend_path+"/concept-id";
    boost::filesystem::create_directories(path_tocreate);
    concept_id_manager_ = new ConceptIDManager(path_tocreate);
    if (!concept_id_manager_->Open())
    {
        //TODO
    }
    autofill_->setCollectionName(collection_name_);
    if(!autofill_->Init(path_+"/autofill")) return false;
    isOpen_ = true;
    return true;
}

void RecommendManager::close()
{
    if (isOpen_)
    {
        serInfo_.close();
        if (recommend_db_ != NULL)
        {
            recommend_db_->close();
            delete recommend_db_;
            recommend_db_ = NULL;
        }
        if (concept_id_manager_ != NULL)
        {
            concept_id_manager_->Close();
            delete concept_id_manager_;
            concept_id_manager_ = NULL;
        }
        isOpen_ = false;
    }
}

void RecommendManager::RebuildForAll()
{
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime p = time_now - boost::gregorian::days(logdays_);
    std::string time_string = boost::posix_time::to_iso_string(p);
    typedef std::map<std::string, std::string> DbRecordType;

    std::vector<UserQuery> query_records;
    UserQuery::find(
            "query, max(hit_docs_num) as hit_docs_num, count(*) AS page_count",
            "collection = '" + collection_name_ + "' AND hit_docs_num > 0 AND TimeStamp >= '" + time_string + "'",
            "query",
            "",
            "",
            query_records);

    std::list<QueryLogType> queryList;
    for (std::vector<UserQuery>::const_iterator it = query_records.begin();
            it != query_records.end(); ++it)
    {
        if (it->getQuery().length() > 16)
            continue;
        izenelib::util::UString uquery(it->getQuery(), izenelib::util::UString::UTF_8);
        if (QueryUtility::isRestrictWord(uquery))
            continue;
        queryList.push_back(QueryLogType(it->getPageCount(), it->getHitDocsNum(), uquery));
    }

    std::vector<PropertyLabel> label_records;
    PropertyLabel::find(
            "label_name, sum(hit_docs_num) AS hit_docs_num",
            "collection = '" + collection_name_ + "'",
            "label_name",
            "",
            "",
            label_records);

    std::list<PropertyLabelType> labelList;
    for (std::vector<PropertyLabel>::const_iterator it = label_records.begin();
            it != label_records.end(); ++it)
    {
        if (it->getLabelName().length() > 16)
            continue;
        izenelib::util::UString plabel(it->getLabelName(), izenelib::util::UString::UTF_8);
        if (QueryUtility::isRestrictWord(plabel))
            continue;
        labelList.push_back(PropertyLabelType(it->getHitDocsNum(), plabel));
    }

    RebuildForRecommend(queryList, labelList);
    RebuildForCorrection(queryList, labelList);
    RebuildForAutofill(queryList, labelList);
}

uint8_t RecommendManager::LabelScore_(uint32_t df)
{
    uint8_t score = 10;
    double d = log((double)df) / log(1.1);
    d /= 2;
    if (d > 255)
    {
        score = (uint8_t)255;
    }
    else
    {
        score = (uint8_t)floor(d);
    }
    return score;
}

uint8_t RecommendManager::QueryLogScore_(uint32_t freq)
{
    uint8_t score = 10;
    double d = log((double)freq) / log(1.1);
    if (d > 255)
    {
        score = (uint8_t)255;
    }
    else
    {
        score = (uint8_t)floor(d);
    }
    return score;
}

bool RecommendManager::AddRecommendItem_(
        MIRDatabase* db,
        uint32_t item_id,
        const izenelib::util::UString& text,
        uint8_t type,
        uint32_t score)
{
    MIRDocument doc;
    std::vector<termid_t> termIdList;
    analyzer_->GetIdListForMatch(text, termIdList);
    if (termIdList.size() == 0) return false;

    //for test propose
//   std::string str;
//   text.convertString(str, izenelib::util::UString::UTF_8);
//   std::cout<<item_id<<","<<str<<",";
//   for(uint32_t i=0;i<termIdList.size();i++)
//   {
//     std::cout<<termIdList[i]<<"|";
//   }
//   std::cout<<std::endl;

    for (std::size_t j=0;j<termIdList.size();j++)
    {
        izenelib::ir::irdb::IRTerm term;
        term.termId_ = termIdList[j];
        term.field_ = "default";
        term.position_ = 0;
        doc.addTerm(term);

    }
    doc.setData<0>(score);
    doc.setData<1>(type);//label type
    db->addDocument(item_id, doc);
    return true;
}

void RecommendManager::RebuildForRecommend(
        const std::list<QueryLogType>& queryList,
        const std::list<PropertyLabelType>& labelList)
{
    std::string newPath;
    if (!dir_switcher_.GetNextWithDelete(newPath))
        return;

    std::cout << "switching recommend manager to " << newPath << std::endl;
    MIRDatabase* new_db = new MIRDatabase(newPath);
    new_db->setCacheSize<0>(100000000);
    new_db->setCacheSize<1>(0);
    new_db->open();
    std::string path_tocreate = newPath + "/concept-id";
    boost::filesystem::create_directories(path_tocreate);
    ConceptIDManager* new_concept_id_manager = new ConceptIDManager(path_tocreate);
    if (!new_concept_id_manager->Open())
    {
        //TODO
    }
    std::cout << newPath << " opened" << std::endl;

    uint32_t item_id = 1;

    if (mining_schema_.recommend_tg) //use tg's kp
    {
        std::list<PropertyLabelType>::const_iterator it = labelList.begin();

        for (; it != labelList.end(); ++it)
        {
            const izenelib::util::UString &uquery = it->second;
            const uint32_t &freq = it->first;
            if (AddRecommendItem_(new_db, item_id, uquery, 1, QueryLogScore_(freq)))
            {
                new_concept_id_manager->Put(item_id, uquery);
                item_id++;
            }
        }
    }

    if (mining_schema_.recommend_querylog) //use query log
    {
        std::list<QueryLogType>::const_iterator it = queryList.begin();

        for (; it != queryList.end(); ++it)
        {
            const izenelib::util::UString &uquery = it->get<2>();
            const uint32_t &freq = it->get<0>();
            if (AddRecommendItem_(new_db, item_id, uquery, 1, QueryLogScore_(freq)))
            {
                new_concept_id_manager->Put(item_id, uquery);
                item_id++;
            }
        }
    }

    if (mining_schema_.recommend_properties.size() > 0 && document_manager_)
    {
        izenelib::am::rde_hash<std::string, bool> recommend_properties;
        for (uint32_t i = 0; i < mining_schema_.recommend_properties.size(); i++)
        {
            recommend_properties.insert(mining_schema_.recommend_properties[i], 0);
        }

        uint32_t process_count = 0;
        Document doc;
        for (uint32_t docid = max_docid_+1; docid < document_manager_->getMaxDocId(); docid++)
        {
            bool b = document_manager_->getDocument(docid, doc);
            if (!b) continue;
            document_manager_->getRTypePropertiesForDocument(docid, doc);
            Document::property_iterator property_it = doc.propertyBegin();
            while (property_it != doc.propertyEnd())
            {
                if (recommend_properties.find(property_it->first) != NULL)
                {
                    const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
                    bool succ = AddRecommendItem_(new_db, item_id, content, 2, 40);
                    if (succ)
                    {
                        new_concept_id_manager->Put(item_id, content);
                        item_id++;
                    }
                }
                property_it++;
            }
            process_count++;
        }
        max_docid_file_.SetValue(max_docid_);
        max_docid_file_.Save();
    }
    new_db->flush();
    new_concept_id_manager->Flush();
    {
        boost::lock_guard<boost::shared_mutex> lock(mutex_);
        MIRDatabase* tmp = recommend_db_;
        recommend_db_ = new_db;
        new_db = tmp;
        ConceptIDManager* tmp2 = concept_id_manager_;
        concept_id_manager_ = new_concept_id_manager;
        new_concept_id_manager = tmp2;
    }
    try
    {
        dir_switcher_.SetNextValid();
        delete new_db;
        delete new_concept_id_manager;
    }
    catch (std::exception& exe)
    {}
}

void RecommendManager::RebuildForCorrection(
        const std::list<QueryLogType>& queryList,
        const std::list<PropertyLabelType>& labelList)
{
    query_correction_->updateCogramAndDict(queryList, labelList);
}

void RecommendManager::RebuildForAutofill(
        const std::list<QueryLogType>& queryList,
        const std::list<PropertyLabelType>& labelList)
{
    autofill_->buildIndex(queryList, labelList);
}

bool RecommendManager::getAutoFillList(
        const izenelib::util::UString& query,
        std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    return autofill_->getAutoFillList(query, list);
}

uint32_t RecommendManager::getRelatedConcepts(
        const izenelib::util::UString& queryStr,
        uint32_t maxNum,
        std::deque<izenelib::util::UString>& queries)
{

    izenelib::am::rde_hash<uint64_t> obtIdList;
    obtIdList.insert(TermUtil::makeQueryId(queryStr));
    std::vector<termid_t> termIdList;
    analyzer_->GetIdListForMatch(queryStr, termIdList);
    std::vector<double> weightList(termIdList.size(), 1.0);

    boost::shared_lock<boost::shared_mutex> mLock(mutex_);
    uint32_t num = getRelatedOnes_(recommend_db_, termIdList,weightList, maxNum, obtIdList , queries);
    return num;

}

uint32_t RecommendManager::getRelatedOnes_(
        MIRDatabase* db,
        const std::vector<termid_t>& termIdList,
        const std::vector<double>& weightList,
        uint32_t maxNum,
        izenelib::am::rde_hash<uint64_t>& obtIdList ,
        std::deque<izenelib::util::UString>& queries)
{
    // Now, we don't use term specific statistics. Those could
    // be added to this function after adjusting the online time
    // to an acceptable range. This could be done by modifying the
    // function to rank document at a time and consider different
    // term's contribution.

    typedef std::pair<uint32_t,uint32_t> value_type;
    typedef std::vector<value_type> sequence_type;
    typedef izenelib::util::second_greater<value_type> greater_than;
    typedef std::priority_queue<value_type, sequence_type, greater_than> heap_type;

    izenelib::am::rde_hash<uint32_t, uint32_t> posMap;
    uint32_t* pos;
    sequence_type seq;
    queries.clear();

//     struct timeval tvafter,tvpre;
//     struct timezone tz;
//     gettimeofday (&tvpre , &tz);
    std::deque<docid_t> docIdList;

    for (std::size_t i=0;i<termIdList.size();i++)
    {
        docIdList.clear();
//         boost::shared_ptr<iii::TermDocFreqs> termDocFreq;
        bool b = false;
        try
        {
            b = db->getMatchSet(termIdList[i], docIdList);
            //for test propose.
//             std::cout<<"[M]:";
//             for(uint32_t d = 0;d<docIdList.size();d++)
//             {
//               std::cout<<docIdList[d]<<",";
//             }
//             std::cout<<std::endl;
        }
        catch (std::exception& e)
        {
            LOG(ERROR) << e.what();
            return 0;
        }
        if (!b) continue;
        for (uint32_t j=0;j<docIdList.size();j++)
        {
            docid_t docId=docIdList[j];
//             docid_t docId=termDocFreq->doc();
            uint8_t labelScore;
            db->getDocData<0>(docId,labelScore);
            if ((pos = posMap.find(docId)) == NULL)
            {
                posMap.insert(docId, seq.size());
                seq.push_back(std::make_pair(docId, labelScore));
            }
            else
            {
                seq[*pos].second += labelScore;
            }
        }
    }
    std::sort(seq.begin(), seq.end(), greater_than());

    uint32_t i = 0;
    while (queries.size()< maxNum && i<seq.size())
    {
        izenelib::util::UString queryStr;
        concept_id_manager_->GetStringById(seq[i].first, queryStr);
        uint64_t queryId = TermUtil::makeQueryId(queryStr);
        if (obtIdList.find(queryId) == NULL)
        {
            queries.push_back(queryStr);
            obtIdList.insert(queryId);
        }
        i++;
    }

    return queries.size();
}
