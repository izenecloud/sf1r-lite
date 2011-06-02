/*
 * QueryDataBase.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "RecommendManager.h"
#include <mining-manager/util/TermUtil.hpp>
#include <common/SFLogger.h>
#include <cmath>
#include <log-manager/LogManager.h>
#include <log-manager/UserQuery.h>
#include <query-manager/QMCommonFunc.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
using namespace sf1r;

RecommendManager::RecommendManager(
    const std::string& path,
    const std::string& collection_name,
    const RecommendPara& recommend_param,
    const MiningSchema& mining_schema,
    const boost::shared_ptr<DocumentManager>& documentManager,
    boost::shared_ptr<LabelManager> labelManager,
    idmlib::util::IDMAnalyzer* analyzer,
    uint32_t logdays):
        path_(path),
        isOpen_(false),
        collection_name_(collection_name),
        recommend_param_(recommend_param),
        mining_schema_(mining_schema),
        info_(0),
        serInfo_(path_+"/ser_info"),
        document_manager_(documentManager),
        recommend_db_(NULL), concept_id_manager_(NULL),
        labelManager_(labelManager),
        analyzer_(analyzer),
        reminder_(NULL),
        logdays_(logdays),
        dir_switcher_(path),
        max_docid_(0), max_docid_file_(path+"/max_id")
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
    if ( max_docid_file_.Load() )
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
    if ( !dir_switcher_.Open() )
    {
        return false;
    }
    std::string current_recommend_path;
    if (!dir_switcher_.GetCurrent(current_recommend_path) )
    {
        return false;
    }
    std::cout<<"open recommend manager on "<<current_recommend_path<<std::endl;
    recommend_db_ = new MIRDatabase(current_recommend_path);
    recommend_db_->setCacheSize<0>(100000000);
    recommend_db_->setCacheSize<1>(0);
    recommend_db_->open();
    std::string path_tocreate = current_recommend_path+"/concept-id";
    boost::filesystem::create_directories(path_tocreate);
    concept_id_manager_ = new ConceptIDManager(path_tocreate);
    if (!concept_id_manager_->Open())
    {
        //TODO
    }
    reminderStorage_ = new ReminderStorageType(path_+"/reminder_storage");
    reminderStorage_->setCacheSize(10);
    reminderStorage_->open();
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
        if ( reminderStorage_!= NULL )
        {
            reminderStorage_->close();
            delete reminderStorage_;
            reminderStorage_ = NULL;
        }
        isOpen_ = false;
    }
}

void RecommendManager::initReminder_()
{
    if ( reminder_ == NULL )
    {
        boost::filesystem::remove_all( path_+"/reminder" );
        boost::filesystem::create_directories( path_+"/reminder" );
        reminder_ = new ReminderType(analyzer_, path_+"/reminder", recommend_param_.realtime_num, recommend_param_.popular_num);
    }
}


bool RecommendManager::RebuildForAll()
{
    bool succ = RebuildForRecommend();
    if (!succ) return false;
//     succ = RebuildForReminder();
//     if (!succ) return false;
    succ = RebuildForCorrection();
    if (!succ) return false;
    succ = RebuildForAutofill();
    if (!succ) return false;
    return true;
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

bool RecommendManager::AddRecommendItem_(MIRDatabase* db, uint32_t item_id, const izenelib::util::UString& text, uint8_t type, uint32_t score)
{
    MIRDocument doc;
    std::vector<termid_t> termIdList;
    analyzer_->GetIdListForMatch(text, termIdList);
    if ( termIdList.size() == 0 ) return false;

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

bool RecommendManager::RebuildForRecommend()
{
    std::string newPath;
    if (!dir_switcher_.GetNextWithDelete(newPath) )
    {
        return false;
    }
    std::cout<<"switching recommend manager to "<<newPath<<std::endl;
    MIRDatabase* new_db = new MIRDatabase(newPath);
    new_db->setCacheSize<0>(100000000);
    new_db->setCacheSize<1>(0);
    new_db->open();
    std::string path_tocreate = newPath+"/concept-id";
    boost::filesystem::create_directories(path_tocreate);
    ConceptIDManager* new_concept_id_manager = new ConceptIDManager(path_tocreate);
    if (!new_concept_id_manager->Open() )
    {
        //TODO
    }
    std::cout<<newPath<<" opened"<<std::endl;


    uint32_t item_id = 1;
    if ( mining_schema_.recommend_tg && labelManager_) //use tg's kp
    {
        uint32_t max_labelid = labelManager_->getMaxLabelID();
        izenelib::util::UString label_str;
        uint32_t df = 0;
        for (uint32_t i=1; i<=max_labelid; i++)
        {
            bool b1 = labelManager_->getLabelStringByLabelId(i, label_str);
            bool b2 = labelManager_->getLabelDF(i, df);
            if (b1 && b2)
            {
                bool succ = AddRecommendItem_(new_db, item_id, label_str, 0, LabelScore_(df) );
                if ( succ )
                {
                    item_id++;
                }
            }
        }
    }
    max_labelid_in_recommend_ = item_id-1;
    if ( mining_schema_.recommend_querylog) //use query log
    {
        boost::posix_time::ptime time_now = boost::posix_time::second_clock::local_time();
        uint32_t days = logdays_;
        boost::gregorian::days dd(days);
        boost::posix_time::ptime p = time_now-dd;
        std::string time_string = boost::posix_time::to_iso_string(p);
        std::string freq_sql = "select query, count(*) as freq from user_queries where collection='"+collection_name_+"' and TimeStamp >='"+time_string+"' group by query";
        std::list<std::map<std::string, std::string> > freq_records;
        UserQuery::find_by_sql(freq_sql, freq_records);
        std::list<std::map<std::string, std::string> >::iterator it = freq_records.begin();

        for ( ;it!=freq_records.end();++it )
        {
            izenelib::util::UString uquery( (*it)["query"], izenelib::util::UString::UTF_8);
            if ( QueryUtility::isRestrictWord( uquery ) )
            {
                continue;
            }
//         std::cout<<"{{{242 "<<(*it)["freq"]<<std::endl;
            uint32_t freq = boost::lexical_cast<uint32_t>( (*it)["freq"] );
            bool succ = AddRecommendItem_(new_db, item_id, uquery, 1, QueryLogScore_(freq) );
            if ( succ )
            {
                new_concept_id_manager->Put(item_id-max_labelid_in_recommend_, uquery);
                item_id++;
            }
        }
    }

    if ( mining_schema_.recommend_properties.size()>0 && document_manager_)
    {
        izenelib::am::rde_hash<std::string, bool> recommend_properties;
        for (uint32_t i=0;i<mining_schema_.recommend_properties.size();i++)
        {
            recommend_properties.insert( mining_schema_.recommend_properties[i], 0);
        }

        uint32_t process_count = 0;
        Document doc;
        for ( uint32_t docid = max_docid_+1; docid<document_manager_->getMaxDocId(); docid++)
        {
            bool b = document_manager_->getDocument(docid, doc);
            if (!b) continue;
            Document::property_iterator property_it = doc.propertyBegin();
            while (property_it != doc.propertyEnd() )
            {
                if ( recommend_properties.find( property_it->first)!= NULL)
                {
                    const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
                    bool succ = AddRecommendItem_(new_db, item_id, content, 2, 40 );
                    if ( succ )
                    {
                        new_concept_id_manager->Put(item_id-max_labelid_in_recommend_, content);
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
    return true;
}

bool RecommendManager::RebuildForReminder()
{
    boost::posix_time::ptime time_now = boost::posix_time::second_clock::local_time();
    uint32_t days = logdays_;
    boost::gregorian::days dd(days+1);
    boost::posix_time::ptime p = time_now-dd;
    std::string time_string = boost::posix_time::to_iso_string(p);
    std::string now_time_string = boost::posix_time::to_iso_string(time_now);
//   std::cout<<"{{{322 "<<now_time_string.substr(0,8)<<std::endl;
    uint32_t now_day = boost::lexical_cast<uint32_t>( now_time_string.substr(0,8) );

    std::string slice_sql = "select substr(TimeStamp,1,8) as day, query , count(*) as freq from user_queries where collection='"+collection_name_+"' and TimeStamp >='"+time_string+"' group by day, query order by day asc";
    std::list<std::map<std::string, std::string> > slice_records;
    UserQuery::find_by_sql(slice_sql, slice_records);
    std::list<std::map<std::string, std::string> >::iterator it = slice_records.begin();

    std::list<std::pair<izenelib::util::UString,uint32_t> > logItems;
    uint32_t last_day = 0;
    for ( ;it!=slice_records.end();++it )
    {
//       std::cout<<"{{{333 "<<(*it)["day"]<<","<<(*it)["freq"]<<std::endl;
        uint32_t day = boost::lexical_cast<uint32_t>( (*it)["day"] );
        uint32_t freq = boost::lexical_cast<uint32_t>( (*it)["freq"] );
        izenelib::util::UString uquery( (*it)["query"], izenelib::util::UString::UTF_8);


        if ( day != last_day && logItems.size()>0 )
        {
            bool isLast = false;
            if ( now_day >= last_day )
            {
                if ( now_day - last_day <= 1 ) isLast = true;
            }
//             std::cout<<"*XX* "<<now_day<<" "<<last_day<<" "<<isLast<<std::endl;
            insertQuerySlice( last_day, logItems, isLast );
            logItems.resize(0);
        }

        if ( !QueryUtility::isRestrictWord( uquery ) )
        {
            logItems.push_back( std::make_pair(uquery, freq) );
        }


        last_day = day;
    }
    if ( logItems.size()>0 )
    {
        bool isLast = false;
        if ( now_day >= last_day )
        {
            if ( now_day - last_day <= 1 ) isLast = true;
        }
//         std::cout<<"*XX* "<<now_day<<" "<<last_day<<" "<<isLast<<std::endl;
        insertQuerySlice( last_day, logItems, isLast );
    }
    if ( reminder_ != NULL )
    {
        reminder_->indexQuery();
        std::vector<izenelib::util::UString> popularQueries;
        std::vector<izenelib::util::UString> realTimeQueries;
        reminder_->getPopularQuery( popularQueries );
        reminder_->getRealTimeQuery( realTimeQueries );
        {
            boost::lock_guard<boost::shared_mutex> lock(reminderMutex_);
            reminderStorage_->update( 1, popularQueries );
            reminderStorage_->update( 2, realTimeQueries );
            reminderStorage_->flush();
        }
        std::string str;
        std::cout<<"[Popular query:]"<<std::endl;
        for (uint32_t i=0;i<popularQueries.size();i++)
        {
            popularQueries[i].convertString( str, izenelib::util::UString::UTF_8);
            std::cout<<str<<std::endl;
        }
        std::cout<<"[Realtime query:]"<<std::endl;
        for (uint32_t i=0;i<realTimeQueries.size();i++)
        {
            realTimeQueries[i].convertString( str, izenelib::util::UString::UTF_8);
            std::cout<<str<<std::endl;
        }
        delete reminder_;
        reminder_ = NULL;
        boost::filesystem::remove_all( path_+"/reminder" );
    }
    return true;
}

bool RecommendManager::RebuildForCorrection()
{
    boost::posix_time::ptime time_now = boost::posix_time::second_clock::local_time();
    uint32_t days = logdays_;
    boost::gregorian::days dd(days);
    boost::posix_time::ptime p = time_now-dd;
    std::string time_string = boost::posix_time::to_iso_string(p);
    std::string query_sql = "select query, count(*) as freq from user_queries where collection='"+collection_name_+"' and TimeStamp >='"+time_string+"' group by query";
    std::list<std::map<std::string, std::string> > query_records;
    UserQuery::find_by_sql(query_sql, query_records);
    std::list<std::map<std::string, std::string> >::iterator it = query_records.begin();

    std::list<std::pair<izenelib::util::UString,uint32_t> > logItems;
    for ( ;it!=query_records.end();++it )
    {
        izenelib::util::UString uquery( (*it)["query"], izenelib::util::UString::UTF_8);
        if ( QueryUtility::isRestrictWord( uquery ) )
        {
            continue;
        }
//       std::cout<<"{{{421 "<<(*it)["freq"]<<std::endl;
        uint32_t freq = boost::lexical_cast<uint32_t>( (*it)["freq"] );
        logItems.push_back( std::make_pair(uquery, freq) );
    }
    if (logItems.size()>0)
    {
        QueryCorrectionSubmanager::getInstance().updateCogramAndDict(collection_name_, logItems);
    }
    return true;
}

bool RecommendManager::RebuildForAutofill()
{
    return true;
}




void RecommendManager::insertQuerySlice(uint32_t timeId, const std::list<std::pair<izenelib::util::UString,uint32_t> >& logItems, bool isLastestTimeId)
{
    if ( logItems.size()==0 ) return;
    initReminder_();
    reminder_->indexQueryLog(timeId, logItems, isLastestTimeId);
}

void RecommendManager::getReminderQuery(std::vector<izenelib::util::UString>& popularQueries, std::vector<izenelib::util::UString>& realTimeQueries)
{
    if ( reminderStorage_ == NULL ) return;
    boost::shared_lock<boost::shared_mutex> mLock(reminderMutex_);
    reminderStorage_->get(1, popularQueries);
    reminderStorage_->get(2, realTimeQueries);
}

uint32_t RecommendManager::getRelatedConcepts(const izenelib::util::UString& queryStr, uint32_t maxNum, std::deque<izenelib::util::UString>& queries)
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
            sflog->error(SFL_MINE, e.what());
            return 0;
        }
        if (!b ) continue;
//         while( termDocFreq->next() )
        for (uint32_t j=0;j<docIdList.size();j++)
        {
            docid_t docId=docIdList[j];
//             docid_t docId=termDocFreq->doc();
            uint8_t labelScore;
            db->getDocData<0>(docId,labelScore);
            if ( (pos = posMap.find(docId)) == NULL)
            {
                posMap.insert(docId, seq.size() );
                seq.push_back( std::make_pair( docId, labelScore ) );
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
        getConceptStringByConceptId_(seq[i].first, queryStr);
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

bool RecommendManager::getConceptStringByConceptId_(uint32_t id, izenelib::util::UString& ustr)
{
    uint32_t label_count = recommend_db_->numDocuments() - concept_id_manager_->size();
    if ( id <= label_count )
    {
        if ( labelManager_ )
        {
            return labelManager_->getConceptStringByConceptId(id, ustr);
        }
        else
        {
            return false;
        }
    }
    else
    {
        uint32_t qpid = id - label_count;
        return concept_id_manager_->GetStringById(qpid, ustr);
    }
}


