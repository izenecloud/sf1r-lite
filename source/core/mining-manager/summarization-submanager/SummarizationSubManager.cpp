#include "SummarizationSubManager.h"
#include "SummarizationStorage.h"
#include "CommentCacheStorage.h"
#include "splm.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAPool.h>

#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <common/ScdWriter.h>
#include <common/Utilities.h>
#include <idmlib/util/idm_analyzer.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

#include <glog/logging.h>

#include <iostream>
#include <algorithm>
#include "OpinionsManager.h"
#include <am/succinct/wat_array/wat_array.hpp>

#define OPINION_COMPUTE_THREAD_NUM 4
#define OPINION_COMPUTE_QUEUE_SIZE 200

using izenelib::util::UString;
using namespace izenelib::ir::indexmanager;
namespace bfs = boost::filesystem;

namespace sf1r
{

static const UString DOCID("DOCID", UString::UTF_8);

bool CheckParentKeyLogFormat(
        const SCDDocPtr& doc,
        const UString& parent_key_name)
{
    if (doc->size() != 2) return false;
    const UString& first = (*doc)[0].first;
    const UString& second = (*doc)[1].first;
    //FIXME case insensitive compare, but it requires extra string conversion,
    //which introduces unnecessary memory fragments
    return (first == DOCID && second == parent_key_name);
}

vector<std::pair<double,UString> > SegmentToSentece(UString Segment)
{
    vector<std::pair<double,UString> > Sentence;
    string temp ;
    Segment.convertString(temp, izenelib::util::UString::UTF_8);
    string dot=",";
    size_t templen = 0;
  
    while(!temp.empty())
    {
        size_t len1 = temp.find(",");
        size_t len2 = temp.find(".");
        if(len1 != string::npos || len2 != string::npos)
        {
            if(len1 == string::npos)
            {
                templen = len2;
            }
            else if(len2 == string::npos)
            {
                templen = len1;
            }
            else
            {
                templen = min(len1,len2);
            }
            if(temp.substr(0,templen).length()>0)
            {
                Sentence.push_back(std::make_pair(1.0,UString(temp.substr(0,templen), UString::UTF_8)) );
            }
            temp = temp.substr(templen + dot.length());
        }
        else
        {   
            if(temp.length()>0)
            {
               Sentence.push_back(std::make_pair(1.0,UString(temp, UString::UTF_8)));
            }
            break;
        }
    }
    return Sentence;
}
struct IsParentKeyFilterProperty
{
    const std::string& parent_key_property;

    IsParentKeyFilterProperty(const std::string& property)
        : parent_key_property(property)
    {}

    bool operator()(const QueryFiltering::FilteringType& filterType)
    {
        return boost::iequals(parent_key_property, filterType.property_);
    }
};


MultiDocSummarizationSubManager::MultiDocSummarizationSubManager(
        const std::string& homePath,
        SummarizeConfig schema,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager,
        idmlib::util::IDMAnalyzer* analyzer)
    : last_docid_path_(homePath + "/last_docid.txt")
    , schema_(schema)
    , document_manager_(document_manager)
    , index_manager_(index_manager)
    , analyzer_(analyzer)
    , comment_cache_storage_(new CommentCacheStorage(homePath))
    , summarization_storage_(new SummarizationStorage(homePath))
    , corpus_(new Corpus())
{
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    Opc_ = new OpinionsClassificationManager(cma_path, schema_.opinionWorkingPath);
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
    delete summarization_storage_;
    delete comment_cache_storage_;
    delete corpus_;
}

void MultiDocSummarizationSubManager::EvaluateSummarization()
{
    std::vector<docid_t> del_docid_list;
    document_manager_->getDeletedDocIdList(del_docid_list);
    for(unsigned int i = 0; i < del_docid_list.size();++i)
    {
        Document doc;
        document_manager_->getDocument(i, doc);
        Document::property_const_iterator kit = doc.findProperty(schema_.uuidPropName);
        if (kit == doc.propertyEnd()) continue;

        const UString& key = kit->second.get<UString>();
        if (key.empty()) continue;
        comment_cache_storage_->ExpelUpdate(Utilities::md5ToUint128(key), i);
    }

    float score;
    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = document_manager_->getNumericPropertyTable(schema_.scorePropName);
    for (uint32_t i = GetLastDocid_() + 1, count = 0; i <= document_manager_->getMaxDocId(); i++)
    {
        Document doc;
        document_manager_->getDocument(i, doc);
        Document::property_const_iterator kit = doc.findProperty(schema_.uuidPropName);
        if (kit == doc.propertyEnd()) continue;

        Document::property_const_iterator cit = doc.findProperty(schema_.contentPropName);
        if (cit == doc.propertyEnd()) continue;

        Document::property_const_iterator ait = doc.findProperty(schema_.advantagePropName);
        if (cit == doc.propertyEnd()) continue;

        Document::property_const_iterator dit = doc.findProperty(schema_.disadvantagePropName);
        if (cit == doc.propertyEnd()) continue;

        const UString& key = kit->second.get<UString>();
        if (key.empty()) continue;

        ContentType content ;//= cit->second.get<UString>();

        //const AdvantageType& advantage = ait->second.get<AdvantageType>();
        //const DisadvantageType& disadvantage = dit->second.get<DisadvantageType>();
        UString  us(cit->second.get<UString>());
        string str;
        us.convertString(str, izenelib::util::UString::UTF_8);
        std::pair<UString,UString> advantagepair=Opc_->test(str);

        AdvantageType advantage=advantagepair.first;

        DisadvantageType disadvantage=advantagepair.second;

        score = 0.0f;
        numericPropertyTable->getFloatValue(i, score);
        comment_cache_storage_->AppendUpdate(Utilities::md5ToUint128(key), i, content,
                advantage, disadvantage, score);

        if (++count % 100000 == 0)
        {
            LOG(INFO) << "Caching comments: " << count;
        }
    }

    SetLastDocid_(document_manager_->getMaxDocId());
    comment_cache_storage_->Flush(true);

    if (!schema_.scoreSCDPath.empty())
    {
        score_scd_writer_.reset(new ScdWriter(schema_.scoreSCDPath, UPDATE_SCD));
    }

    if (!schema_.opinionSCDPath.empty())
    {
        opinion_scd_writer_.reset(new ScdWriter(schema_.opinionSCDPath, UPDATE_SCD));
    }

    {
        boost::unique_lock<boost::mutex> g(waiting_opinion_lock_);
        can_quit_compute_ = false;
    }

    string OpPath = schema_.opinionWorkingPath;

    std::vector<UString> filters;
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    LOG(INFO)<<"OpPath"<<OpPath<<endl;

    try
    {
        ifstream infile;
        infile.open((OpPath + "/opinion_filter_data.txt").c_str(), ios::in);
        while(infile.good())
        {
            std::string line;
            getline(infile, line);
            if(line.empty())
            {
                continue;
            }
            filters.push_back(UString(line, UString::UTF_8));
        }
        infile.close();
    }
    catch(...)
    {
        LOG(ERROR) << "read opinion filter file error" << endl;
    }

    for(int i = 0; i < OPINION_COMPUTE_THREAD_NUM; i++)
    {
        std::string log_path = OpPath;
        log_path += "/opinion-log-";
        log_path.push_back('a' + i);
        boost::filesystem::path p(log_path);
        boost::filesystem::create_directory(p);

        std::string cma_path;
        LAPool::getInstance()->get_cma_path(cma_path);

        Ops_.push_back(new OpinionsManager( log_path, cma_path, OpPath));

        Ops_.back()->setSigma(0.1, 5, 0.6, 20);
        //////////////////////////
        Ops_.back()->setFilterStr(filters);

        opinion_compute_threads_.push_back(new boost::thread(&MultiDocSummarizationSubManager::DoComputeOpinion,
                    this, Ops_[i]));
    }

    LOG(INFO) << "====== Evaluating summarization begin ======" << std::endl;
    {
        CommentCacheStorage::DirtyKeyIteratorType dirtyKeyIt(comment_cache_storage_->dirty_key_db_);
        CommentCacheStorage::DirtyKeyIteratorType dirtyKeyEnd;
        for (uint32_t count = 0; dirtyKeyIt != dirtyKeyEnd; ++dirtyKeyIt)
        {
            const KeyType& key = dirtyKeyIt->first;

            CommentCacheItemType commentCacheItem;
            comment_cache_storage_->Get(key, commentCacheItem);
            if (commentCacheItem.empty())
            {
                summarization_storage_->Delete(key);
                continue;
            }

            Summarization summarization(commentCacheItem);
            DoEvaluateSummarization_(summarization, key, commentCacheItem);

            DoOpinionExtraction(summarization, key, commentCacheItem);
            if (++count % 1000 == 0)
            {
                std::cout << "\r === Evaluating summarization and opinion count: " << count << " ===" << std::flush;
            }
        }

        {
            boost::unique_lock<boost::mutex> g(waiting_opinion_lock_);
            can_quit_compute_ = true;
            waiting_opinion_cond_.notify_all();
        }

        DoWriteOpinionResult();

        summarization_storage_->Flush();
    } //Destroy dirtyKeyIterator before clearing dirtyKeyDB

    LOG(INFO) << "====== Evaluating summarization end ======" << std::endl;
    for(int i = 0; i < OPINION_COMPUTE_THREAD_NUM; i++)
    {
        delete opinion_compute_threads_[i];
        delete Ops_[i];
    }
    Ops_.clear();
    opinion_compute_threads_.clear();

    if (score_scd_writer_)
    {
        score_scd_writer_->Close();
        score_scd_writer_.reset();
    }
    if (opinion_scd_writer_)
    {
        opinion_scd_writer_->Close();
        opinion_scd_writer_.reset();
    }


    comment_cache_storage_->ClearDirtyKey();
    LOG(INFO) << "Finish evaluating summarization.";
}

void MultiDocSummarizationSubManager::DoComputeOpinion(OpinionsManager* Op)
{
    LOG(INFO) << "opinion compute thread started : " << (long)pthread_self() << endl;
    int count = 0;
    while(true)
    {
        assert(Op != NULL);
        WaitingComputeCommentItem opinion_data;
        {
            boost::unique_lock<boost::mutex> g(waiting_opinion_lock_);
            while(waiting_opinion_comments_.empty())
            {
                if(can_quit_compute_)
                {
                    LOG(INFO) << "!!!---compute thread:" << (long)pthread_self() << " finished. ---!!!" << endl;
                    return;
                }
                waiting_opinion_cond_.wait(g);
            }
            opinion_data = waiting_opinion_comments_.front();
            waiting_opinion_comments_.pop();
        }
        if(opinion_data.cached_comments.empty())
            continue;

        // std::vector<UString> Z;
        std::vector<UString> advantage_comments;
        std::vector<UString> disadvantage_comments;
        //Z.reserve(opinion_data.cached_comments.size());
        advantage_comments.reserve(opinion_data.cached_comments.size());
        disadvantage_comments.reserve(opinion_data.cached_comments.size());
        for (CommentCacheItemType::const_iterator it = opinion_data.cached_comments.begin();
                it != opinion_data.cached_comments.end(); ++it)
        {

            //Z.push_back((it->second).content);
            advantage_comments.push_back(it->second.advantage);
            disadvantage_comments.push_back(it->second.disadvantage);
            
        }

        //Op->setComment(Z);
        std::vector< std::pair<double, UString> > product_opinions;// = Op->getOpinion();
        Op->setComment(advantage_comments);
        std::vector< std::pair<double, UString> > advantage_opinions = Op->getOpinion();
        Op->setComment(disadvantage_comments);
        std::vector< std::pair<double, UString> > disadvantage_opinions = Op->getOpinion();
        if(advantage_opinions.empty()&&(!advantage_comments.empty()))
        {
            for(unsigned i=0;i<min(advantage_comments.size(),5);i++)
            {
                 std::vector< std::pair<double, UString> > temp=SegmentToSentece(advantage_comments[i]);
                 advantage_opinions.insert(advantage_opinions.end(),temp.begin(),temp.end());
                 if(advantage_opinions.size()>5)
                    break;
            }
        }
        if(disadvantage_opinions.empty()&&(!disadvantage_comments.empty()))
        {
            for(unsigned i=0;i<min(disadvantage_comments.size(),5);i++)
            {
                 std::vector< std::pair<double, UString> > temp=SegmentToSentece(disadvantage_comments[i]);
                 disadvantage_opinions.insert(disadvantage_opinions.end(),temp.begin(),temp.end());
                 if(disadvantage_opinions.size()>5)
                    break;
            }
        }
        
        if((!advantage_opinions.empty())||(!disadvantage_opinions.empty()))
        {
            OpinionResultItem item;
            item.key = opinion_data.key;


            item.result_advantage = advantage_opinions;
            item.result_disadvantage = disadvantage_opinions;

            item.summarization.swap(opinion_data.summarization);
            boost::unique_lock<boost::mutex> g(opinion_results_lock_);
            opinion_results_.push(item);
            opinion_results_cond_.notify_one();
        }
        if (++count % 100 == 0)
        {
            cout << "\r == thread:" << (long)pthread_self() << " computing opinion count: " << count << " ===" << std::flush;
        }
    }
}

void MultiDocSummarizationSubManager::DoOpinionExtraction(
        Summarization& summarization,
        const KeyType& key,
        const CommentCacheItemType& comment_cache_item)
{
    WaitingComputeCommentItem item;
    item.key = key;
    item.cached_comments = comment_cache_item;
    item.summarization.swap(summarization);
    boost::unique_lock<boost::mutex> g(waiting_opinion_lock_);
    while(waiting_opinion_comments_.size() > OPINION_COMPUTE_QUEUE_SIZE)
        waiting_opinion_cond_.timed_wait(g, boost::posix_time::millisec(500));
    waiting_opinion_comments_.push(item);
    waiting_opinion_cond_.notify_one();
}

void MultiDocSummarizationSubManager::DoWriteOpinionResult()
{
    int count = 0;
    while(true)
    {
        bool all_finished = false;
        OpinionResultItem result;
        {
            boost::unique_lock<boost::mutex> g(opinion_results_lock_);
            while(opinion_results_.empty())
            {
                // check if all thread finished.
                all_finished = true;
                for(size_t i = 0; i < opinion_compute_threads_.size(); ++i)
                {
                    if(!opinion_compute_threads_[i]->timed_join(boost::posix_time::millisec(1)))
                    {
                        // not finished
                        all_finished = false;
                        break;
                    }
                }
                if(!all_finished)
                {
                    opinion_results_cond_.timed_wait(g, boost::posix_time::millisec(5000));
                }
                else
                {
                    LOG(INFO) << "opinion result write finished." << endl;
                    return;
                }
            }
            result = opinion_results_.front();
            opinion_results_.pop();
        }
        if((!result.result_advantage.empty())||(!result.result_advantage.empty()))
        {
            if(!result.result_advantage.empty())
            {
                UString final_opinion_str = result.result_advantage[0].second;
                for(size_t i = 1; i < result.result_advantage.size(); ++i)
                {
                    final_opinion_str.append( UString(",", UString::UTF_8) );
                    final_opinion_str.append(result.result_advantage[i].second);
                }

                std::string key_str;
                key_str = Utilities::uint128ToUuid(result.key);
                UString key_ustr(key_str, UString::UTF_8);

                if (opinion_scd_writer_)
                {
                    Document doc;
                    doc.property("DOCID") = key_ustr;
                    doc.property(schema_.opinionPropName) = final_opinion_str;
                    opinion_scd_writer_->Append(doc);
                }
            }

            // result.summarization.updateProperty("overview", result.result_opinions);
            result.summarization.updateProperty("advantage", result.result_advantage);
            result.summarization.updateProperty("disadvantage", result.result_disadvantage);
            summarization_storage_->Update(result.key, result.summarization);

            if (++count % 1000 == 0)
            {
                cout << "\r== write opinion count: " << count << " ====" << std::flush;
            }
        }
    }
}

bool MultiDocSummarizationSubManager::DoEvaluateSummarization_(
        Summarization& summarization,
        const KeyType& key,
        const CommentCacheItemType& comment_cache_item)
{
#define MAX_SENT_COUNT 1000

    ScoreType total_score = 0;
    uint32_t count = 0;

    std::string key_str;
    key_str = Utilities::uint128ToUuid(key);
    UString key_ustr(key_str, UString::UTF_8);

    for (CommentCacheItemType::const_iterator it = comment_cache_item.begin();
            it != comment_cache_item.end(); ++it)
    {
        if (it->second.score)
        {
            ++count;
            total_score += (it->second).score;
        }
    }
    if (count)
    {
        std::vector<std::pair<double, UString> > score_list(1);
        double avg_score = (double)total_score / (double)count;
        score_list[0].first = avg_score;
        summarization.updateProperty("avg_score", score_list);

        if (score_scd_writer_)
        {
            Document doc;
            doc.property("DOCID") = key_ustr;
            doc.property(schema_.scorePropName) = UString(boost::lexical_cast<std::string>(avg_score), UString::UTF_8);
            score_scd_writer_->Append(doc);
        }
    }
    return true;
}

bool MultiDocSummarizationSubManager::GetSummarizationByRawKey(
        const UString& rawKey,
        Summarization& result)
{
    std::string key_str;
    rawKey.convertString(key_str, UString::UTF_8);
    return summarization_storage_->Get(Utilities::uuidToUint128(key_str), result);
}

void MultiDocSummarizationSubManager::AppendSearchFilter(
        std::vector<QueryFiltering::FilteringType>& filtingList)
{
    ///When search filter is based on ParentKey, get its associated values,
    ///and add those values to filter conditions.
    ///The typical situation of this happen when :
    ///SELECT * FROM comments WHERE product_type="foo"
    ///This hook will translate the semantic into:
    ///SELECT * FROM comments WHERE product_id="1" OR product_id="2" ...

    typedef std::vector<QueryFiltering::FilteringType>::iterator IteratorType;
    IteratorType it = std::find_if(filtingList.begin(),
            filtingList.end(), IsParentKeyFilterProperty(schema_.uuidPropName));
    if (it != filtingList.end())
    {
        const std::vector<PropertyValue>& filterParam = it->values_;
        if (!filterParam.empty())
        {
            try
            {
                const std::string& paramValue = get<std::string>(filterParam[0]);
                KeyType param = Utilities::uuidToUint128(paramValue);

                LogServerConnection& conn = LogServerConnection::instance();
                GetDocidListRequest req;
                UUID2DocidList resp;

                req.param_.uuid_ = param;
                conn.syncRequest(req, resp);
                if (req.param_.uuid_ != resp.uuid_) return;

                BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
                QueryFiltering::FilteringType filterRule;
                filterRule.operation_ = QueryFiltering::INCLUDE;
                filterRule.property_ = schema_.docidPropName;
                for (std::vector<KeyType>::const_iterator rit = resp.docidList_.begin();
                        rit != resp.docidList_.end(); ++rit)
                {
                    UString result(Utilities::uint128ToMD5(*rit), UString::UTF_8);
                    if (pBTreeIndexer->seek(schema_.docidPropName, result))
                    {
                        ///Protection
                        ///Or else, too many unexisted keys are added
                        PropertyValue v(result);
                        filterRule.values_.push_back(v);
                    }
                }
                //filterRule.logic_ = QueryFiltering::OR;
                filtingList.erase(it);
                //it->logic_ = QueryFiltering::OR;
                filtingList.push_back(filterRule);
            }
            catch (const boost::bad_get &)
            {
                filtingList.erase(it);
                return;
            }
        }
    }
}

uint32_t MultiDocSummarizationSubManager::GetLastDocid_() const
{
    std::ifstream ifs(last_docid_path_.c_str());

    if (!ifs) return 0;

    uint32_t docid;
    ifs >> docid;
    return docid;
}

void MultiDocSummarizationSubManager::SetLastDocid_(uint32_t docid) const
{
    std::ofstream ofs(last_docid_path_.c_str());

    if (ofs) ofs << docid;
}

}
