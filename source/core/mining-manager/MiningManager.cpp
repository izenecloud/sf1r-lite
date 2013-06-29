#include <configuration-manager/PropertyConfig.h>
#include <document-manager/DocumentManager.h>
#include <common/PropSharedLockSet.h>
#include <common/QueryNormalizer.h>
#include "MiningManager.h"
#include "MiningQueryLogHandler.h"
#include "MiningTaskBuilder.h"
#include "MiningTask.h"

#include "duplicate-detection-submanager/dup_detector_wrapper.h"

#include "auto-fill-submanager/AutoFillSubManager.h"

#include "query-correction-submanager/QueryCorrectionSubmanager.h"
#include "query-recommend-submanager/QueryRecommendSubmanager.h"
#include "query-recommend-submanager/RecommendManager.h"
#include "query-recommend-submanager/QueryRecommendRep.h"

#include "taxonomy-generation-submanager/TaxonomyGenerationSubManager.h"
#include "taxonomy-generation-submanager/label_similarity.h"
//#include "taxonomy-generation-submanager/TaxonomyRep.h"
#include "taxonomy-generation-submanager/TaxonomyInfo.h"
#include "taxonomy-generation-submanager/LabelManager.h"

#include "similarity-detection-submanager/PrunedSortedTermInvertedIndexReader.h"
#include "similarity-detection-submanager/DocWeightListPrunedInvertedIndexReader.h"
#include "similarity-detection-submanager/SimilarityIndex.h"

#include "summarization-submanager/SummarizationSubManager.h"

#include "faceted-submanager/ontology_manager.h"
#include "group-manager/GroupManager.h"
#include "group-manager/GroupFilterBuilder.h"
#include "group-manager/GroupFilter.h"
#include "attr-manager/AttrManager.h"
#include "faceted-submanager/ctr_manager.h"

#include "util/convert_ustr.h"
#include "group-label-logger/GroupLabelLogger.h"
#include "group-label-logger/GroupLabelKnowledge.h"
#include "merchant-score-manager/MerchantScoreManager.h"
#include "custom-rank-manager/CustomDocIdConverter.h"
#include "custom-rank-manager/CustomRankManager.h"
#include "product-scorer/ProductScorerFactory.h"
#include "product-score-manager/ProductScoreManager.h"
#include "product-score-manager/OfflineProductScorerFactoryImpl.h"
#include "product-score-manager/ProductScoreTable.h"
#include "product-ranker/ProductRankerFactory.h"

#include "tdt-submanager/NaiveTopicDetector.hpp"

#include "suffix-match-manager/SuffixMatchManager.hpp"
#include "suffix-match-manager/FilterManager.h"
#include "suffix-match-manager/IncrementalFuzzyManager.hpp"
#include "suffix-match-manager/FMIndexManager.h"

#include "product-classifier/SPUProductClassifier.hpp"
#include "product-classifier/QueryCategorizer.hpp"
#include "query-intent/QueryIntentManager.h"

#include <search-manager/SearchManager.h>
#include <search-manager/NumericPropertyTableBuilderImpl.h>
#include <search-manager/RTypeStringPropTableBuilder.h>
#include <index-manager/IndexManager.h>
#include <common/SearchCache.h>

#include <idmlib/tdt/integrator.h>
#include <idmlib/util/container_switch.h>
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/semantic_space/esa/DocumentRepresentor.h>
#include <idmlib/semantic_space/esa/ExplicitSemanticInterpreter.h>
#include <idmlib/similarity/all-pairs-similarity-search/data_set_iterator.h>
#include <idmlib/similarity/all-pairs-similarity-search/all_pairs_search.h>
#include <idmlib/similarity/all-pairs-similarity-search/all_pairs_output.h>
#include <idmlib/similarity/term_similarity.h>
#include <idmlib/tdt/tdt_types.h>
#include <directory-manager/DirectoryCookie.h>

#include <b5m-manager/product_matcher.h> //idmlib/util/CollectionUtil.h make conflicts on scd parser

#include <ir/index_manager/index/IndexReader.h>
#include <ir/id_manager/IDManager.h>
#include <la-manager/LAManager.h>

#include <am/3rdparty/rde_hash.h>
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <util/driver/Request.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/timer.hpp>
#include <boost/scoped_ptr.hpp>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>

#include <fstream>
#include <iterator>
#include <set>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <algorithm>
#include <memory> // auto_ptr


namespace sf1r
{

bool greater_pair(const std::pair<std::string, double>& obj1, const std::pair<std::string, double>& obj2)
{
    return obj1.second > obj2.second;
}

using namespace boost::filesystem;
using namespace izenelib::ir::idmanager;
namespace bfs = boost::filesystem;

std::string MiningManager::system_resource_path_;
std::string MiningManager::system_working_path_;

MiningManager::MiningManager(
        const std::string& collectionDataPath,
        const CollectionPath& collectionPath,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<LAManager>& laManager,
        const boost::shared_ptr<IndexManager>& index_manager,
        const boost::shared_ptr<SearchManager>& searchManager,
        const boost::shared_ptr<SearchCache>& searchCache,
        const boost::shared_ptr<IDManager>& idManager,
        const std::string& collectionName,
        const DocumentSchema& documentSchema,
        const MiningConfig& miningConfig,
        const MiningSchema& miningSchema,
        const IndexBundleSchema& indexSchema)
    : collectionDataPath_(collectionDataPath)
    , queryDataPath_(collectionPath.getQueryDataPath())
    , collectionPath_(collectionPath)
    , collectionName_(collectionName)
    , documentSchema_(documentSchema)
    , miningConfig_(miningConfig)
    , indexSchema_ (indexSchema)
    , mining_schema_(miningSchema)
    , analyzer_(NULL)
    , c_analyzer_(NULL)
    , kpe_analyzer_(NULL)
    , document_manager_(documentManager)
    , laManager_(laManager)
    , index_manager_(index_manager)
    , searchManager_(searchManager)
    , searchCache_(searchCache)
    , tgInfo_(NULL)
    , idManager_(idManager)
    , numericTableBuilder_(NULL)
    , rtypeStringPropTableBuilder_(NULL)
    , groupManager_(NULL)
    , attrManager_(NULL)
    , groupFilterBuilder_(NULL)
    , merchantScoreManager_(NULL)
    , customDocIdConverter_(NULL)
    , customRankManager_(NULL)
    , offlineScorerFactory_(NULL)
    , productScoreManager_(NULL)
    , groupLabelKnowledge_(NULL)
    , productScorerFactory_(NULL)
    , productRankerFactory_(NULL)
    , queryIntentManager_(NULL)
    , tdt_storage_(NULL)
    , topicDetector_(NULL)
    , summarizationManagerTask_(NULL)
    , suffixMatchManager_(NULL)
    , incrementalManager_(NULL)
    , product_categorizer_(NULL)
    , kvManager_(NULL)
    , miningTaskBuilder_(NULL)
    , hasDeletedDocDuringMining_(false)
{
}

MiningManager::~MiningManager()
{
    if (miningTaskBuilder_) delete miningTaskBuilder_;
    if (analyzer_) delete analyzer_;
    if (c_analyzer_) delete c_analyzer_;
    if (kpe_analyzer_) delete kpe_analyzer_;
    if (productRankerFactory_) delete productRankerFactory_;
    if (queryIntentManager_) delete queryIntentManager_;
    if (productScorerFactory_) delete productScorerFactory_;
    if (groupLabelKnowledge_) delete groupLabelKnowledge_;
    if (productScoreManager_) delete productScoreManager_;
    if (offlineScorerFactory_) delete offlineScorerFactory_;
    if (customRankManager_) delete customRankManager_;
    if (customDocIdConverter_) delete customDocIdConverter_;
    if (merchantScoreManager_) delete merchantScoreManager_;
    if (groupFilterBuilder_) delete groupFilterBuilder_;
    if (groupManager_) delete groupManager_;
    if (attrManager_) delete attrManager_;
    if (numericTableBuilder_) delete numericTableBuilder_;
    if (rtypeStringPropTableBuilder_) delete rtypeStringPropTableBuilder_;
    if (tdt_storage_) delete tdt_storage_;
    if (topicDetector_) delete topicDetector_;
    if (suffixMatchManager_) delete suffixMatchManager_;
    if (incrementalManager_) delete incrementalManager_;
    if (product_categorizer_) delete product_categorizer_;
    if (kvManager_) delete kvManager_;

    close();
}

void MiningManager::close()
{
    for (GroupLabelLoggerMap::iterator it = groupLabelLoggerMap_.begin();
            it != groupLabelLoggerMap_.end(); ++it)
    {
        delete it->second;
    }
    groupLabelLoggerMap_.clear();
    MiningQueryLogHandler* handler = MiningQueryLogHandler::getInstance();
    handler->deleteCollection(collectionName_);
}

bool MiningManager::open()
{
    close();

    std::cout << "DO_TG : " << (int) mining_schema_.tg_enable << " - " << (int) mining_schema_.tg_kpe_only << std::endl;
    std::cout << "DO_DUPD : " << (int) mining_schema_.dupd_enable << " - " << (int) mining_schema_.dupd_fp_only << std::endl;
    std::cout << "DO_SIM : " << (int) mining_schema_.sim_enable << std::endl;
    std::cout << "DO_FACETED : " << (int) mining_schema_.faceted_enable << std::endl;
    std::cout << "DO_GROUP : " << (int) mining_schema_.group_enable << std::endl;
    std::cout << "DO_ATTR : " << (int) mining_schema_.attr_enable << std::endl;
    std::cout << "DO_TDT : " << (int) mining_schema_.tdt_enable << std::endl;
    std::cout << "DO_IISE : " << (int) mining_schema_.ise_enable << std::endl;

    /** Global variables **/
    try
    {
        bfs::path cookiePath(bfs::path(basicPath_) / "cookie");
        directory::DirectoryCookie cookie(cookiePath);
        if (cookie.read())
        {
            if (!cookie.valid())
            {
                LOG(INFO) << "Mining data is broken.";
                boost::filesystem::remove_all(basicPath_);
                LOG(INFO) << "Mining data deleted.";
            }
        }

        std::string prefix_path  = collectionDataPath_;
        FSUtil::createDir(prefix_path);

        kpe_res_path_ = system_resource_path_+"/kpe";
        rig_path_ = system_resource_path_+"/sim/rig";
        /** analyzer */

        std::string kma_path;
        LAPool::getInstance()->get_kma_path(kma_path);
        std::string cma_path;
        LAPool::getInstance()->get_cma_path(cma_path);
        std::string jma_path;
        LAPool::getInstance()->get_jma_path(jma_path);

        IDMAnalyzerConfig config1 = idmlib::util::IDMAnalyzerConfig::GetCommonConfig(kma_path,"",jma_path);
        analyzer_ = new idmlib::util::IDMAnalyzer(config1);
        if (!analyzer_->LoadT2SMapFile(kpe_res_path_+"/cs_ct"))
        {
            return false;
        }

        IDMAnalyzerConfig config2 = idmlib::util::IDMAnalyzerConfig::GetCommonConfig(kma_path,cma_path,jma_path);
        c_analyzer_ = new idmlib::util::IDMAnalyzer(config2);
        if (!c_analyzer_->LoadT2SMapFile(kpe_res_path_+"/cs_ct"))
        {
            return false;
        }

        IDMAnalyzerConfig config3 = idmlib::util::IDMAnalyzerConfig::GetCommonTgConfig(kma_path,"",jma_path);
        kpe_analyzer_ = new idmlib::util::IDMAnalyzer(config3);
        if (!kpe_analyzer_->LoadT2SMapFile(kpe_res_path_+"/cs_ct"))
        {
            return false;
        }
        /**Miningtask Builder*/
        if (mining_schema_.suffixmatch_schema.suffix_match_enable ||
            mining_schema_.group_enable ||
            mining_schema_.attr_enable )
        {
            miningTaskBuilder_ = new MiningTaskBuilder( document_manager_);
        }

        product_categorizer_ = new QueryCategorizer;

        /** TG */
        if (mining_schema_.tg_enable)
        {
            tg_path_ = prefix_path + "/tg";
            tg_label_path_ = tg_path_ + "/label";
            tg_label_sim_path_ = tg_path_+"/sim";
            tg_label_sim_table_path_ = tg_path_+"/sim_table";
            FSUtil::createDir(tg_path_);
            FSUtil::createDir(tg_label_path_);
            FSUtil::createDir(tg_label_sim_path_);

            labelManager_.reset(new LabelManager(tg_path_ + "/label", mining_schema_.tg_kpe_only));
            labelManager_->open();
            labelManager_->SetDbSave(collectionName_);

            tgManager_.reset(new TaxonomyGenerationSubManager(miningConfig_.taxonomy_param,labelManager_, analyzer_));

            label_sim_collector_.reset(new SimCollectorType(tg_label_sim_table_path_, 10));
            if (!label_sim_collector_->Open())
            {
                std::cerr<<"open label sim collector failed"<<std::endl;
            }
        }

        /** Recommend */
        FSUtil::createDir(queryDataPath_);
        ///TODO Yingfeng
        uint32_t logdays = 7;

        qcManager_.reset(new QueryCorrectionSubmanager(queryDataPath_, miningConfig_.query_correction_param.enableEK,
                    miningConfig_.query_correction_param.enableCN,miningConfig_.query_correction_param.fromDb));
        rmDb_.reset(new RecommendManager(collectionName_, collectionPath_, mining_schema_, miningConfig_, document_manager_,
                    qcManager_, analyzer_, logdays));

        if (!rmDb_->open())
        {
            std::cerr<<"open query recommend manager failed"<<std::endl;
            rmDb_->close();
            return false;
        }

        /** log manager */
        MiningQueryLogHandler* handler = MiningQueryLogHandler::getInstance();
        if (!handler->cronStart(miningConfig_.recommend_param.cron))
        {
            std::cout<<"Init query long cron task failed."<<std::endl;
            return false;
        }
        handler->addCollection(collectionName_, rmDb_);

        qrManager_.reset(new QueryRecommendSubmanager(rmDb_, queryDataPath_ + "/recommend_inject.txt"));
        qrManager_->Load();

        /** DUPD */
        if (mining_schema_.dupd_enable)
        {
            dupd_path_ = prefix_path + "/dupd/";
            FSUtil::createDir(dupd_path_);
            dupManager_.reset(new DupDType(dupd_path_, document_manager_, idManager_, mining_schema_.dupd_properties, c_analyzer_, mining_schema_.dupd_fp_only));
            if (!dupManager_->Open())
            {
                std::cerr<<"open DD failed"<<std::endl;
                return false;
            }
        }

        /** SIM */
        if (mining_schema_.sim_enable)
        {
            sim_path_ = prefix_path + "/sim";
            FSUtil::createDir(sim_path_);

            if (!miningConfig_.similarity_param.enable_esa)
            {
                std::string hdb_path = sim_path_ + "/hdb";
                boost::filesystem::create_directories(hdb_path);
                similarityIndex_.reset(new sim::SimilarityIndex(sim_path_, hdb_path));
            }
            else
            {
                similarityIndexEsa_.reset(new idmlib::sim::DocSimOutput(sim_path_+"/esa"));
            }
        }

        /** faceted */
        if (mining_schema_.faceted_enable)
        {
            faceted_path_ = prefix_path + "/faceted";
            FSUtil::createDir(faceted_path_);
            faceted_.reset(new faceted::OntologyManager(faceted_path_, document_manager_, mining_schema_.faceted_properties, analyzer_));
            faceted_->Open();

        }

        /** CTR */
        if (true)
        {
            std::string ctr_path = prefix_path + "/ctr";
            FSUtil::createDir(ctr_path);

            size_t docNum = 0;
            if (index_manager_)
            {
                docNum = index_manager_->getIndexReader()->maxDoc();
            }

            ctrManager_.reset(new faceted::CTRManager(ctr_path, docNum));
            if (!ctrManager_->open())
            {
                std::cerr << "open CRT failed" << std::endl;
                return false;
            }
        }

        if (numericTableBuilder_) delete numericTableBuilder_;
        numericTableBuilder_ = new NumericPropertyTableBuilderImpl(
            *document_manager_, ctrManager_.get());

        if (rtypeStringPropTableBuilder_) delete rtypeStringPropTableBuilder_;
        rtypeStringPropTableBuilder_ = new RTypeStringPropTableBuilder(*document_manager_);

        /** group */
        if (mining_schema_.group_enable)
        {
            if (groupManager_) delete groupManager_;
            std::string groupPath = prefix_path + "/group";

            groupManager_ = new faceted::GroupManager(
                    mining_schema_.group_config_map,
                    *document_manager_, groupPath);
            if (! groupManager_->open())
            {
                std::cerr << "open GROUP failed" << std::endl;
                return false;
            }
            std::vector<MiningTask*>& miningTaskList = groupManager_->getGroupMiningTask();
            for (std::vector<MiningTask*>::const_iterator i = miningTaskList.begin(); i != miningTaskList.end(); ++i)
            {
                miningTaskBuilder_->addTask(*i);
            }
            miningTaskList.clear();
        }

        /** attr */
        if (mining_schema_.attr_enable)
        {
            if (attrManager_) delete attrManager_;
            std::string attrPath = prefix_path + "/attr";

            attrManager_ = new faceted::AttrManager(mining_schema_.attr_property, attrPath, *document_manager_.get());
            if (! attrManager_->open())
            {
                std::cerr << "open ATTR failed" << std::endl;
                return false;
            }
            MiningTask* miningTask = attrManager_->getAttrMiningTask();
            miningTaskBuilder_->addTask(miningTask);
        }

        if (groupManager_ || attrManager_)
        {
            if (groupFilterBuilder_) delete groupFilterBuilder_;

            groupFilterBuilder_ = new faceted::GroupFilterBuilder(
                mining_schema_.group_config_map,
                groupManager_,
                attrManager_,
                numericTableBuilder_);
        }

        /** group label log */
        if (mining_schema_.group_enable)
        {
            std::string logPath = prefix_path + "/group_label_log";
            try
            {
                FSUtil::createDir(logPath);
            }
            catch (FileOperationException& e)
            {
                LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
                return false;
            }

            for (GroupConfigMap::const_iterator it = mining_schema_.group_config_map.begin();
                    it != mining_schema_.group_config_map.end(); ++it)
            {
                if (! it->second.isStringType())
                    continue;

                const std::string& propName = it->first;
                if (!groupLabelLoggerMap_[propName])
                {
                    std::auto_ptr<GroupLabelLogger> loggerPtr(new GroupLabelLogger(logPath, propName));
                    groupLabelLoggerMap_[propName] = loggerPtr.release();
                }
                else
                {
                    std::cerr << "the label logger on group property " << propName
                        << " is already opened before" << std::endl;
                    return false;
                }
            }
        }

        if (customDocIdConverter_) delete customDocIdConverter_;
        customDocIdConverter_ = new CustomDocIdConverter(*idManager_);

        const ProductRankingConfig& rankConfig =
            mining_schema_.product_ranking_config;

        if (!initMerchantScoreManager_(rankConfig) ||
            !initGroupLabelKnowledge_(rankConfig) ||
            !initProductScorerFactory_(rankConfig) ||
            !initProductRankerFactory_(rankConfig))
            return false;
        
        if (queryIntentManager_) delete queryIntentManager_;
        queryIntentManager_ = new QueryIntentManager();
        
        /** tdt **/
        if (mining_schema_.tdt_enable)
        {
            if (mining_schema_.tdt_config.perform_tdt_task)
            {
                tdt_path_ = prefix_path + "/tdt";
                boost::filesystem::create_directories(tdt_path_);
                std::string tdt_storage_path = tdt_path_+"/storage";
                tdt_storage_ = new TdtStorageType(tdt_storage_path);
                if (!tdt_storage_->Open())
                {
                    LOG(ERROR) << "tdt init failed";
                    return false;
                }
            }
            boost::filesystem::path cma_tdt_dic(system_resource_path_);
            cma_tdt_dic /= boost::filesystem::path("dict");
            cma_tdt_dic /= boost::filesystem::path(mining_schema_.tdt_config.tdt_tokenize_dicpath);
            topicDetector_ = new NaiveTopicDetector(
                system_resource_path_, cma_tdt_dic.c_str(),mining_schema_.tdt_config.enable_semantic,mining_schema_.tdt_config.tdt_type);
        }


        /** Summarization */
        if (mining_schema_.summarization_enable && !mining_schema_.summarization_schema.isSyncSCDOnly)
        {
            summarization_path_ = prefix_path + "/summarization";
            boost::filesystem::create_directories(summarization_path_);
            summarizationManagerTask_ =
                new MultiDocSummarizationSubManager(summarization_path_, collectionName_,
                                                    collectionPath_.getScdPath(),
                                                    mining_schema_.summarization_schema,
                                                    document_manager_,
                                                    index_manager_,
                                                    c_analyzer_);
            miningTaskBuilder_->addTask(summarizationManagerTask_);
            if (!mining_schema_.summarization_schema.uuidPropName.empty())
            {
                //searchManager_->set_filter_hook(boost::bind(&MultiDocSummarizationSubManager::AppendSearchFilter, summarizationManager_, _1));
            }
        }

        /** Suffix Match */
        if (mining_schema_.suffixmatch_schema.suffix_match_enable)
        {
            LOG(INFO) << "suffix match enabled.";
            suffix_match_path_ = prefix_path + "/suffix_match";
            suffixMatchManager_ = new SuffixMatchManager(suffix_match_path_,
                    mining_schema_.suffixmatch_schema.suffix_match_tokenize_dicpath,
                    system_resource_path_,
                    document_manager_, groupManager_, attrManager_, numericTableBuilder_);
            suffixMatchManager_->addFMIndexProperties(mining_schema_.suffixmatch_schema.searchable_properties, FMIndexManager::LESS_DV);
            suffixMatchManager_->addFMIndexProperties(mining_schema_.suffixmatch_schema.suffix_match_properties, FMIndexManager::COMMON, true);

            //product_categorizer_->SetSuffixMatchManager(suffixMatchManager_);
            product_categorizer_->SetDocumentManager(document_manager_);
            // reading suffix config and load filter data here.
            boost::shared_ptr<FilterManager>& filter_manager = suffixMatchManager_->getFilterManager();
            filter_manager->setGroupFilterProperties(mining_schema_.suffixmatch_schema.group_filter_properties);
            filter_manager->setAttrFilterProperties(mining_schema_.suffixmatch_schema.attr_filter_properties);
            filter_manager->setStrFilterProperties(mining_schema_.suffixmatch_schema.str_filter_properties);
            filter_manager->setDateFilterProperties(mining_schema_.suffixmatch_schema.date_filter_properties);
            const std::vector<NumericFilterConfig>& number_config_list = mining_schema_.suffixmatch_schema.num_filter_properties;
            std::vector<std::string> number_props;
            number_props.reserve(number_config_list.size());
            std::vector<int32_t> number_amp_list;
            number_amp_list.reserve(number_config_list.size());
            for (size_t i = 0; i < number_config_list.size(); ++i)
            {
                number_props.push_back(number_config_list[i].property);
                number_amp_list.push_back(number_config_list[i].amplifier);
            }
            filter_manager->setNumFilterProperties(number_props, number_amp_list);
            filter_manager->loadFilterId();

            if (mining_schema_.suffixmatch_schema.suffix_incremental_enable)
            {
                incrementalManager_ = new IncrementalFuzzyManager(suffix_match_path_,
                        mining_schema_.suffixmatch_schema.suffix_match_tokenize_dicpath,
                        mining_schema_.suffixmatch_schema.suffix_match_properties,
                        document_manager_, idManager_, laManager_, indexSchema_,
                        groupManager_, attrManager_, numericTableBuilder_);
                incrementalManager_->Init();
                boost::shared_ptr<FilterManager>& filter_manager_inc = incrementalManager_->getFilterManager();
                filter_manager_inc->setGroupFilterProperties(mining_schema_.suffixmatch_schema.group_filter_properties);

                filter_manager_inc->setAttrFilterProperties(mining_schema_.suffixmatch_schema.attr_filter_properties);
                filter_manager_inc->setStrFilterProperties(mining_schema_.suffixmatch_schema.str_filter_properties);
                filter_manager_inc->setDateFilterProperties(mining_schema_.suffixmatch_schema.date_filter_properties);

                filter_manager_inc->setNumFilterProperties(number_props, number_amp_list);
                filter_manager_inc->loadFilterId();
                if (!filter_manager_inc->loadFilterList())
                    filter_manager_inc->generatePropertyId();
            }

            if (mining_schema_.suffixmatch_schema.suffix_match_enable)
            {
                suffixMatchManager_->buildMiningTask();
                MiningTask* miningTask = suffixMatchManager_->getMiningTask();
                miningTaskBuilder_->addTask(miningTask);

                if (mining_schema_.suffixmatch_schema.suffix_incremental_enable)
                {
                    if (cronExpression_.setExpression(miningConfig_.fuzzyIndexMerge_param.cron))
                    {
                        string cronJobName = "fuzzy_index_merge";

                        bool result = izenelib::util::Scheduler::addJob(cronJobName,
                        60*1000, // each minute
                        0, // start from now
                        boost::bind(&MiningManager::updateMergeFuzzyIndex, this, _1));
                        if (result)
                        {
                            LOG (INFO) << "ADD cron job for fuzzy_index_merge ......";
                        }
                        else
                            return false;
                    }
                }
            }
        }
        if (mining_schema_.product_matcher_enable)
        {
            std::vector<std::string> restrict_vector;
            //restrict_vector.push_back("^手机$");
            //restrict_vector.push_back("^数码相机/单反相机/摄像机");
            //restrict_vector.push_back("^MP3/MP4/iPod/录音笔$");
            //restrict_vector.push_back("^笔记本电脑$");
            //restrict_vector.push_back("^平板电脑/MID$");
            //restrict_vector.push_back("^台式机/一体机/服务器.*$");
            //restrict_vector.push_back("^电脑硬件/显示器/电脑周边.*$");
            //restrict_vector.push_back("^网络设备/网络相关.*$");
            //restrict_vector.push_back("^闪存卡/U盘/存储/移动硬盘.*$");
            //restrict_vector.push_back("^办公设备/耗材/相关服务>打印机$");
            //restrict_vector.push_back("^办公设备/耗材/相关服务>投影机$");
            //restrict_vector.push_back("^办公设备/耗材/相关服务>扫描仪$");
            //restrict_vector.push_back("^办公设备/耗材/相关服务>复合复印机$");
            //restrict_vector.push_back("^大家电.*$");
            for (uint32_t i=0;i<restrict_vector.size();i++)
            {
                match_category_restrict_.push_back(boost::regex(restrict_vector[i]));
            }
            std::string res_path = system_resource_path_+"/product-matcher";
            ProductMatcher* matcher = ProductMatcherInstance::get();
            if (!matcher->Open(res_path))
            {
                std::cerr<<"product matcher open failed"<<std::endl;
            }
            else
            {
                matcher->SetUsePriceSim(false);
                //matcher->SetCategoryMaxDepth(2);
            }
            product_categorizer_->SetProductMatcher(matcher);
            if (suffixMatchManager_)
                suffixMatchManager_->setProductMatcher(matcher);

            //SPUProductClassifier* product_classifier = SPUProductClassifier::Get();
            //product_classifier->Open(system_resource_path_+"/spu-classifier");
            //product_categorizer_->SetSPUProductClassifier(product_classifier);
            //product_categorizer_->SetWorkingMode(mining_schema_.product_categorizer_mode);
            //test
            std::ifstream ifs("./querylog.txt");
            std::string line;
            while (getline(ifs, line))
            {
                boost::algorithm::trim(line);
                UString query(line, UString::UTF_8);
                UString category;
                if (GetProductFrontendCategory(query, category))
                {
                    std::string scategory;
                    category.convertString(scategory, UString::UTF_8);
                    LOG(ERROR) << "!!!!!!!!!!!query category " << line << "," << scategory;
                }
            }
            ifs.close();
        }

        /** KV */
        kv_path_ = prefix_path + "/kv";
        boost::filesystem::create_directories(kv_path_);
        kvManager_ = new KVSubManager(kv_path_);
        if (!kvManager_->open())
        {
            LOG(ERROR) << "kv manager open fail";
            delete kvManager_;
            kvManager_ = NULL;
        }
    }
    catch (NotEnoughMemoryException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (MiningConfigException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (FileOperationException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (ResourceNotFoundException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (FileCorruptedException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (boost::filesystem::filesystem_error& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    catch (izenelib::ir::indexmanager::IndexManagerException& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    return true;
}

bool MiningManager::DoMiningCollectionFromAPI()
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    TimestampReqLog reqlog;
    reqlog.timestamp = Utilities::createTimeStamp();
    if (!DistributeRequestHooker::get()->prepare(Req_WithTimestamp, reqlog))
    {
        LOG(INFO) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = false;
    try
    {
        ret = DoMiningCollection(reqlog.timestamp);
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        ret = false;
    }

    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
    return ret;
}

void MiningManager::DoContinue()
{
    //do mining continue;
    if (NodeManagerBase::get()->isDistributed())
    {
        LOG(INFO) << "continue not allowed in distributed sf1r.";
        return;
    }
    try
    {
        std::string continue_file = collectionDataPath_+"/continue";
        if (boost::filesystem::exists(continue_file))
        {
            boost::filesystem::remove_all(continue_file);
            DoMiningCollection(0);
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

bool MiningManager::DOMiningTask()
{
    if (miningTaskBuilder_)
    {
        miningTaskBuilder_->buildCollection();
    }
    return true;
}

bool MiningManager::DoMiningCollection(int64_t timestamp)
{
    MEMLOG("[Mining] DoMiningCollection");
    //do TG
    if (mining_schema_.tg_enable)
    {
        MEMLOG("[Mining] TG starting..");
        if (doTgInfoInit_())
        {
            izenelib::am::rde_hash<std::string, bool> tg_properties;
            for (uint32_t i=0;i<mining_schema_.tg_properties.size();i++)
            {
                tg_properties.insert(mining_schema_.tg_properties[i], 0);
            }
            uint32_t max_docid = tgInfo_->GetMaxDocId();
            uint32_t dm_maxid = document_manager_->getMaxDocId();
            uint32_t process_count = 0;

            std::string rig_path = system_resource_path_+"/sim/rig";
            std::cout<<"rig path : "<<rig_path<<std::endl;
            LabelSimilarity label_sim(tg_label_sim_path_, rig_path, label_sim_collector_);
            if (!label_sim.Open(dm_maxid))
            {
                std::cerr<<"open label sim failed : "<<dm_maxid<<std::endl;
                return false;
            }

            LabelManager::TaskFunctionType label_sim_task = boost::bind(&LabelSimilarity::Append, &label_sim, _1, _2, _3, _4, _5);
            labelManager_->AddTask(label_sim_task);

            Document doc;
            std::cout<<"Will processing from "<<max_docid+1<<" to "<<dm_maxid<<std::endl;
            for (uint32_t docid = max_docid+1; docid<=dm_maxid; docid++)
            {
                bool b = document_manager_->getDocument(docid, doc);
                if (!b) continue;
                document_manager_->getRTypePropertiesForDocument(docid, doc);
                Document::property_iterator property_it = doc.propertyBegin();
                while (property_it != doc.propertyEnd())
                {
                    if (tg_properties.find(property_it->first))
                    {
                        const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
                        tgInfo_->addDocument(docid, content);
                    }
                    property_it++;
                }
                process_count++;
                if (process_count % 1000 == 0)
                {
                    MEMLOG("[TG] inserted %d. docid: %d", process_count, docid);
                }
            }
            tgInfo_->finish();
            doTgInfoRelease_();
            if (!label_sim.Compute())
            {
                std::cerr<<"label sim compute failed"<<std::endl;
            }
//             printSimilarLabelResult_(1);
//             printSimilarLabelResult_(2);
        }
        labelManager_->ClearAllTask();
        MEMLOG("[Mining] TG finished.");
        LOG(INFO) << "Mining runEvents time: " << timestamp;
        MiningQueryLogHandler::getInstance()->runEvents(timestamp);
    }
    //do DUPD
    if (mining_schema_.dupd_enable)
    {
        MEMLOG("[Mining] DUPD starting..");
        dupManager_->ProcessCollection();
        MEMLOG("[Mining] DUPD finished.");
    }

    if (mining_schema_.faceted_enable)
    {
        faceted_->ProcessCollection(false);
    }

    //do ctr
    if (ctrManager_)
    {
        size_t docNum = 0;
        if (index_manager_)
        {
            docNum = index_manager_->getIndexReader()->maxDoc();
        }

        ctrManager_->updateDocNum(docNum);
    }

    // calculate product score
    if (mining_schema_.product_ranking_config.isEnable)
    {
        productScoreManager_->buildCollection();
    }

    //do tdt
    if (mining_schema_.tdt_enable && mining_schema_.tdt_config.perform_tdt_task)
    {
        idmlib::tdt::Storage* next_storage = tdt_storage_->Next();
        if (!next_storage)
        {
            std::cerr<<"can not get next tdt storage"<<std::endl;
        }
        else
        {
            std::string tdt_working_path = tdt_path_+"/working";
            idmlib::tdt::Integrator<DocumentManager> tdt_manager(tdt_working_path, rig_path_, kpe_res_path_, analyzer_);
            DocumentManager* p_dm = document_manager_.get();
            std::cout<<"got dm"<<std::endl;
            tdt_manager.SetDataSource(p_dm);
            tdt_manager.Process(next_storage);
            if (!tdt_storage_->EnsureSwitch())
            {
                std::cerr<<"can not switch tdt storage"<<std::endl;
            }
            //         boost::gregorian::date start = boost::gregorian::from_string("2011-03-01");
            //         boost::gregorian::date end = boost::gregorian::from_string("2011-03-31");
            //         std::vector<izenelib::util::UString> topic_list;
            //         GetTdtInTimeRange(start, end, topic_list);
            //         for (uint32_t i=0;i<topic_list.size();i++)
            //         {
            //             std::string str;
            //             topic_list[i].convertString(str, izenelib::util::UString::UTF_8);
            //             std::cout<<"find topic in date range : "<<str<<std::endl;
            //         }
        }
    }

    //do Similarity
    if (mining_schema_.sim_enable)
    {
        MEMLOG("[Mining] SIM starting..");
        if (!miningConfig_.similarity_param.enable_esa)
        {
            computeSimilarity_(index_manager_->getIndexReader(), mining_schema_.sim_properties);
        }
        else
        {
            computeSimilarityESA_(mining_schema_.sim_properties);
        }
        MEMLOG("[Mining] SIM finished.");
    }

    if (mining_schema_.suffixmatch_schema.suffix_match_enable)
    {
        if (mining_schema_.suffixmatch_schema.suffix_incremental_enable)
        {
            uint32_t last_docid = 0;
            uint32_t docNum = 0;
            uint32_t maxDoc = 0;
            incrementalManager_->getLastDocid(last_docid);
            incrementalManager_->getDocNum(docNum);
            incrementalManager_->getMaxNum(maxDoc);
            /**
            @brief: if there is -D SCD, the fuzzy index will merge and rebuid;
            @for incremental is for -U and -R scd;
            **/
            if (last_docid == document_manager_->getMaxDocId()) /// for -D SCD and -R SCD
            {
                if (document_manager_->last_delete_docid_.size() > 0) /// merge and rebuild fm-index;
                {
                    SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();
                    bool isIncre = false;
                    miningTask->setTaskStatus(isIncre);
                }
                else if (document_manager_->isThereRtypePro())
                {
                    std::vector<string> unchangedProperties;
                    const std::vector<std::pair<int32_t, std::string> >& prop_list
                    = suffixMatchManager_->getFilterManager()->getProp_list();

                    for (std::vector<std::pair<int32_t, std::string> >::const_iterator i = prop_list.begin()
                        ; i != prop_list.end(); ++i)
                    {
                        std::set<string>::iterator iter = document_manager_->RtypeDocidPros_.find((*i).second);
                        if (iter == document_manager_->RtypeDocidPros_.end())
                        {
                            unchangedProperties.push_back((*i).second);
                        }
                    }
                    /// all just rebuid filter ...
                    if (suffixMatchManager_->getMiningTask()->getLastDocId() - 1 == document_manager_->getMaxDocId()) /// only fm-index;
                    {
                        SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();
                        bool isIncre = false;
                        miningTask->setTaskStatus(isIncre);
                    }
                    else /// each just update it's filter;
                    {
                        /*
                        @brief: because the fm-index will not rebuild, so even the filter docid num is
                        more than fm-index docid num, there is no effect.
                        */
                        SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();
                        bool isIncre = false;
                        miningTask->isRtypeIncremental_ = true;
                        miningTask->setTaskStatus(isIncre);//

                        incrementalManager_->updateFilterForRtype(); // use extra filter update for incre;
                    }
                }
            }
            else // -U SCD or continue
            {
                SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();

                if (docNum + (document_manager_->getMaxDocId() - last_docid) < maxDoc) // not merge
                {
                    bool isIncre = true;
                    miningTask->setTaskStatus(isIncre);
                }
                else // merge
                {
                    incrementalManager_->reset();
                    bool isIncre = false;
                    miningTask->setTaskStatus(isIncre);
                }
            }
        }
    }

    DOMiningTask();

    if (mining_schema_.suffixmatch_schema.suffix_match_enable)
    {
        if (mining_schema_.suffixmatch_schema.suffix_incremental_enable)
        {
            uint32_t last_docid = 0;
            uint32_t docNum = 0;
            uint32_t maxDoc = 0;
            incrementalManager_->getLastDocid(last_docid);
            incrementalManager_->getDocNum(docNum);
            incrementalManager_->getMaxNum(maxDoc);
            if (last_docid == document_manager_->getMaxDocId())
            {
                if (document_manager_->last_delete_docid_.size() > 0) /// merge and rebuild fm-index;
                {
                    incrementalManager_->reset();
                }
                else if (document_manager_->isThereRtypePro())
                {
                    // do nothing
                    SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();
                    miningTask->isRtypeIncremental_ = false;
                }
            }
            else
            {
                if (docNum + (document_manager_->getMaxDocId() - last_docid) < maxDoc) // not merge
                {
                    LOG(INFO) << "Build incrmental index ..." <<endl;
                    incrementalManager_->doCreateIndex();
                    incrementalManager_->updateFilterForRtype();//this is in case there is Rtype docid in -U scd;
                }
                else // merge
                {
                    incrementalManager_->setLastDocid(document_manager_->getMaxDocId());//right
                }
            }
        }
    }

    hasDeletedDocDuringMining_ = false;
    LOG (INFO) << "Clear Rtype Docid List";
    document_manager_->RtypeDocidPros_.clear();
    document_manager_->last_delete_docid_.clear();
    return true;
}

void MiningManager::onIndexUpdated(size_t docNum)
{
    if (ctrManager_)
    {
        ctrManager_->updateDocNum(docNum);
    }
}

bool MiningManager::getMiningResult(const KeywordSearchActionItem& actionItem , KeywordSearchResult& miaInput)
{
    CREATE_PROFILER(prepareData, "MIAProcess",
                    "ProcessGetMiningResults : prepare data from sia output");
    CREATE_PROFILER(tg, "MIAProcess", "ProcessGetMiningResults : doing tg");
    CREATE_PROFILER(qr, "MIAProcess",
                    "ProcessGetMiningResults : doing query recommendation");
    CREATE_PROFILER(dupd, "MIAProcess",
                    "ProcessGetMiningResults : doing duplication detection");
    CREATE_PROFILER(sim, "MIAProcess",
                    "ProcessGetMiningResults : doing similarity detection");
    CREATE_PROFILER(faceted, "MIAProcess",
                    "ProcessGetMiningResults : doing faceted");

    //std::cout << "[MiningManager::getMiningResult]" << std::endl;
    bool bResult = true;
    try
    {
        START_PROFILER(qr);
        if (actionItem.requireRelatedQueries_)
        {
            //izenelib::util::ClockTimer clocker;
            addQrResult_(miaInput);
            //std::cout<<"QR ALL COST: "<<clocker.elapsed()<<" seconds."<<std::endl;
        }
        STOP_PROFILER(qr);

        START_PROFILER(tg);
        if (mining_schema_.tg_enable && !mining_schema_.tg_kpe_only)
        {
            izenelib::util::ClockTimer clocker;
            addTgResult_(miaInput);
            std::cout<<"TG ALL COST: "<<clocker.elapsed()<<" seconds."<<std::endl;
        }
        STOP_PROFILER(tg);

        START_PROFILER(dupd);
        //get dupd result
        if (mining_schema_.dupd_enable && !mining_schema_.dupd_fp_only)
        {
            addDupResult_(miaInput);
        }
        STOP_PROFILER(dupd);

        START_PROFILER(sim);
        // get similar result
        if (mining_schema_.sim_enable)
        {
            addSimilarityResult_(miaInput);
        }

        STOP_PROFILER(sim);

        START_PROFILER(faceted);
        if (mining_schema_.faceted_enable)
        {
            addFacetedResult_(miaInput);
        }

        STOP_PROFILER(faceted);

    }
    catch (NotEnoughMemoryException& ex)
    {
        LOG(ERROR) << ex.toString();
        bResult = false;
    }
    catch (MiningConfigException& ex)
    {
        LOG(ERROR) << ex.toString();
        bResult = false;
    }
    catch (FileOperationException& ex)
    {
        LOG(ERROR) << ex.toString();
        bResult = false;
    }
    catch (ResourceNotFoundException& ex)
    {
        LOG(ERROR) << ex.toString();
        bResult = false;
    }
    catch (FileCorruptedException& ex)
    {
        LOG(ERROR) << ex.toString();
        bResult = false;
    }
    catch (boost::filesystem::filesystem_error& ex)
    {
        LOG(ERROR) << ex.what();
        bResult = false;
    }
    catch (izenelib::ir::indexmanager::IndexManagerException& ex)
    {
        LOG(ERROR) << ex.what();
        bResult = false;
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << ex.what();
        bResult = false;
    }
    //get query recommend result
    REPORT_PROFILE_TO_FILE("PerformanceQueryResult.MiningManager")
    return bResult;
}

bool MiningManager::getSimilarImageDocIdList(
        const std::string& targetImageURI,
        SimilarImageDocIdList& imageDocIdList
)
{
    return false;
    if (mining_schema_.ise_enable)
    {
        std::cout << "[MiningManager::getSimilarImageDocIdList]" << std::endl;
        bool bResult = true;
        try
        {
            //ise_->query(targetImageURI, imageDocIdList.docIdList_,imageDocIdList.imageIdList_);
        }
        catch (NotEnoughMemoryException& ex)
        {
            LOG(ERROR) << ex.toString();
            bResult = false;
        }
        catch (MiningConfigException& ex)
        {
            LOG(ERROR) << ex.toString();
            bResult = false;
        }
        catch (FileOperationException& ex)
        {
            LOG(ERROR) << ex.toString();
            bResult = false;
        }
        catch (ResourceNotFoundException& ex)
        {
            LOG(ERROR) << ex.toString();
            bResult = false;
        }
        catch (FileCorruptedException& ex)
        {
            LOG(ERROR) << ex.toString();
            bResult = false;
        }
        catch (boost::filesystem::filesystem_error& ex)
        {
            LOG(ERROR) << ex.what();
            bResult = false;
        }
        catch (izenelib::ir::indexmanager::IndexManagerException& ex)
        {
            LOG(ERROR) << ex.what();
            bResult = false;
        }
        catch (std::exception& ex)
        {
            LOG(ERROR) << ex.what();
            bResult = false;
        }
        std::cout << "[getSimilarImageDocIdList finished]" << std::endl;
        return bResult;
    }
    else
    {
        return true;
    }
} // end - getSimilarImageDocIdList()

// void MiningManager::getMiningStatus(Status& status)
// {
//     if (status_)
//     {
//         status = *status_;
//     }
// }

// bool MiningManager::getQuerySpecificTaxonomy_(
//         const std::vector<docid_t>& docIdList,
//         const izenelib::util::UString& queryStr,
//         uint32_t totalCount,
//         uint32_t  numberFromSia,
//         TaxonomyRep& taxonomyRep,
//         ne_result_list_type& neList)
// {
//     std::cout << "[MiningManager::getQuerySpecificTaxonomy]" << std::endl;
//     struct timeval tv_start;
//     struct timeval tv_end;
//     //struct timezone tz;
//     gettimeofday(&tv_start, NULL);
//     boost::timer timer;
//     bool ret = tgManager_->getQuerySpecificTaxonomyInfo(docIdList,
//                queryStr, totalCount, numberFromSia, taxonomyRep, neList);
//     double cpu_time = timer.elapsed();
//     gettimeofday(&tv_end, NULL);
//     double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
//             + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
//     //time_t second = end - start;
//     std::cout << "TG cost CPU " << cpu_time << " seconds." << std::endl;
//     std::cout << "TG all cost " << timespend << " seconds." << std::endl;
//     return ret;
//
// }

bool MiningManager::getRecommendQuery_(const izenelib::util::UString& queryStr,
                                       const std::vector<docid_t>& topDocIdList,
                                       QueryRecommendRep & recommendRep)
{
    //struct timeval tv_start;
    //struct timeval tv_end;
    //gettimeofday(&tv_start, NULL);
    qrManager_->getRecommendQuery(queryStr, topDocIdList,
                                  miningConfig_.recommend_param.recommend_num, recommendRep);
    //gettimeofday(&tv_end, NULL);
    //double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    //std::cout << "QR all cost " << timespend << " seconds." << std::endl;
    return true;
}

bool MiningManager::getReminderQuery(
        std::vector<izenelib::util::UString>& popularQueries,
        std::vector<izenelib::util::UString>& realtimeQueries)
{
    return true;
}

bool MiningManager::getUniqueDocIdList(
    const std::vector<uint32_t>& docIdList,
    std::vector<uint32_t>& cleanDocs)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return false;

    return dupManager_->getUniqueDocIdList(docIdList, cleanDocs);

}

bool MiningManager::getUniquePosList(
    const std::vector<uint32_t>& docIdList,
    std::vector<std::size_t>& uniquePosList)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return false;

    return dupManager_->getUniquePosList(docIdList, uniquePosList);

}

bool MiningManager::getDuplicateDocIdList(uint32_t docId, std::vector<
        uint32_t>& docIdList)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return false;

    return dupManager_->getDuplicatedDocIdList(docId, docIdList);
}

bool MiningManager::computeSimilarity_(izenelib::ir::indexmanager::IndexReader* pIndexReader, const std::vector<std::string>& property_names)
{
//   if (pIndexReader->maxDoc()> similarityIndex_->GetMaxDocId())
    if (true)
    {
        std::cout << "Start to compute similarity index, please wait..."<< std::endl;
        // we should be able get information from index manager by property name only
        std::vector<uint32_t> property_ids(property_names.size());
        PropertyConfigBase property_config;
        for (uint32_t i=0;i<property_ids.size();i++)
        {
            PropertyConfigBase byName;
            byName.propertyName_ = property_names[i];
            DocumentSchema::const_iterator it(documentSchema_.find(byName));
            if (it != documentSchema_.end())
                property_config = *it;
            else assert(false);
            property_ids[i] = property_config.propertyId_;
        }

        // customize the concrete reader
        typedef sim::PrunedSortedTermInvertedIndexReader base_reader_type;
        typedef sim::DocWeightListPrunedInvertedIndexReader<base_reader_type>
        doc_pruned_reader_type;
        typedef boost::shared_ptr<doc_pruned_reader_type>
        doc_pruned_reader_type_ptr;
        std::vector<doc_pruned_reader_type_ptr> readers;

        for (size_t i = 0; i < property_ids.size(); i++)
        {
            std::cout << "SIM Getting property: " << property_names[i] << std::endl;
            float fieldAveLength=pIndexReader->getAveragePropertyLength(property_ids[i]);
            std::cout<<"Average Length: "<<fieldAveLength<<std::endl;
            boost::shared_ptr<base_reader_type> reader(new base_reader_type(
                        miningConfig_.similarity_param.termnum_limit,
                        miningConfig_.similarity_param.docnum_limit,
                        pIndexReader, property_ids[i], property_names[i],
                        pIndexReader->numDocs(),
                        fieldAveLength
                    ));
            doc_pruned_reader_type_ptr docPrunedReader(new doc_pruned_reader_type(
                        reader, miningConfig_.similarity_param.docnum_limit));
            readers.push_back(docPrunedReader);

        }

        similarityIndex_->index(readers, pIndexReader->numDocs());

        std::cout << "Finish computing similarity index." << std::endl;
    }
    return true;
}

bool MiningManager::computeSimilarityESA_(const std::vector<std::string>& property_names)
{
    using namespace idmlib::ssp;
    using namespace idmlib::sim;

    std::cout << "Start to compute similarity index (ESA), please wait..."<< std::endl;
    try
    {
        std::string colBasePath = sim_path_;
        size_t pos = colBasePath.find("/collection-data");
        colBasePath = colBasePath.replace(pos, colBasePath.length(), ""); // xxx
//      CollectionMeta colMeta;
//      SF1Config::get()->getCollectionMetaByName(collectionName_,colMeta);
//      colBasePath = colMeta.getCollectionPath().getBasePath();

        std::string cmaPath;
        LAPool::getInstance()->get_cma_path(cmaPath);

        std::string esaSimPath = sim_path_+"/esa";
        std::string esaSimTmpPath = esaSimPath+"/tmp";

//        cout <<cmaPath<<endl;
//        cout <<colBasePath<<endl;
//        cout <<esaSimPath<<endl;

        // document representation
        size_t maxDoc = 0;
        DocumentRepresentor docRepresentor(colBasePath, cmaPath, esaSimTmpPath, property_names, maxDoc);
        docRepresentor.represent();

        // interpretation
        std::string wikiIndexdir = system_resource_path_ + "/sim/esa";
        cout <<wikiIndexdir<<endl;
        ExplicitSemanticInterpreter esInter(wikiIndexdir, esaSimTmpPath);
        if (!esInter.getWikiIndexState())
        {
            std::cerr << "WikiIndex load failed! \n"<< std::endl;
            return false;
        }
        esInter.interpret(10000, maxDoc);

        // all pairs similarity search
        boost::shared_ptr<DocSimOutput> output(new DocSimOutput(esaSimPath));
        float thresholdSim = 0.8; // TODO
        AllPairsSearch allPairs(output, thresholdSim);

        /// single input file (build inverted index in memory)
        ///string datafile = esaSimTmpPath+"/doc_int.vec1";
        ///boost::shared_ptr<DataSetIterator> dataSetIterator(new SparseVectorSetIterator(datafile));
        ///allPairs.findAllSimilarPairs(dataSetIterator, maxDoc);

        /// multiple input file
        std::vector<boost::shared_ptr<DataSetIterator> > dataSetIteratorList;
        {
            directory_iterator iterEnd;
            for (directory_iterator iter(esaSimTmpPath); iter != iterEnd; iter ++)
            {
                string datafile = iter->path().string();
                if (datafile.find("doc_int.vec") != string::npos)
                {
                    //cout << "file path : "<<datafile << endl;
                    boost::shared_ptr<DataSetIterator> dataSetIterator(new SparseVectorSetIterator(datafile));
                    dataSetIteratorList.push_back(dataSetIterator);
                }
            }
        }
        allPairs.findAllSimilarPairs(dataSetIteratorList, maxDoc);

        // remove tmp files
        boost::filesystem::remove_all(esaSimTmpPath);
    }
    catch (std::exception& ex)
    {
        std::cerr<<"Exception:"<<ex.what()<<std::endl;
    }
    std::cout << "Finish computing similarity index (ESA)." << std::endl;

    return true;
}

bool MiningManager::getSimilarDocIdList(uint32_t documentId, uint32_t maxNum,
                                        std::vector<std::pair<uint32_t, float> >& result)
{
    if (!similarityIndex_ && !similarityIndexEsa_)
    {
        // TODO display warning message
        cerr << "[MiningManager] Warning - similarity index is not executed"
             << endl;
        return false;
    }

    result.clear();
    if (!miningConfig_.similarity_param.enable_esa)
    {
        similarityIndex_->getSimilarDocIdScoreList(documentId, maxNum,
                std::back_inserter(result));
    }
    else
    {
        similarityIndexEsa_->getSimilarDocIdScoreList(documentId, maxNum,
                result);
    }

    return true;
}
bool MiningManager::getSimilarLabelList(uint32_t label_id, std::vector<uint32_t>& sim_list)
{
    if (!label_sim_collector_) return false;
    return label_sim_collector_->GetContainer()->Get(label_id, sim_list);

}

bool MiningManager::getSimilarLabelStringList(uint32_t label_id, std::vector<izenelib::util::UString>& sim_list)
{
    if (!labelManager_) return false;
    std::vector<uint32_t> sim_id_list;
    if (!getSimilarLabelList(label_id, sim_id_list)) return false;
    for (uint32_t i=0;i<sim_id_list.size();i++)
    {
        izenelib::util::UString sim_text;
        if (labelManager_->getLabelStringByLabelId(sim_id_list[i], sim_text))
        {
            sim_list.push_back(sim_text);
        }
        else
        {
            std::cerr<<"get label string for "<<sim_id_list[i]<<" failed"<<std::endl;
        }
    }
    return true;
}

bool MiningManager::getLabelListWithSimByDocId(uint32_t docid,
        std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list)
{
    std::vector<std::pair<uint32_t, izenelib::util::UString> > present_label_list;
    if (!getLabelListByDocId(docid, present_label_list)) return false;
    for (uint32_t i=0;i<present_label_list.size();i++)
    {
        std::vector<izenelib::util::UString> sim_list;
        if (!getSimilarLabelStringList(present_label_list[i].first, sim_list)) continue;
        if (sim_list.empty()) continue;
        label_list.push_back(std::make_pair(present_label_list[i].second, sim_list));
    }
    return true;
}

void MiningManager::printSimilarLabelResult_(uint32_t label_id)
{
    izenelib::util::UString sim_text;
    std::string str;
    if (labelManager_->getLabelStringByLabelId(label_id, sim_text))
    {
        sim_text.convertString(str, izenelib::util::UString::UTF_8);
        std::cout<<"[sim]["<<str<<"] ";
        std::vector<izenelib::util::UString> sim_list;
        if (getSimilarLabelStringList(label_id, sim_list))
        {
            for (uint32_t i=0;i<sim_list.size();i++)
            {
                sim_list[i].convertString(str, izenelib::util::UString::UTF_8);
                std::cout<<str<<",";
            }
        }
        std::cout<<std::endl;
    }
    else
    {
        std::cerr<<"get label string for "<<label_id<<" failed"<<std::endl;
    }
}

bool MiningManager::getLabelListByDocId(uint32_t docid, std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list)
{
    if (!labelManager_) return false;
    std::vector<uint32_t> label_id_list;
    if (!labelManager_->getSortedLabelsByDocId(docid, label_id_list))
    {
        return false;
    }
    for (uint32_t i=0;i<label_id_list.size();i++)
    {
        izenelib::util::UString text;
        if (labelManager_->getLabelStringByLabelId(label_id_list[i], text))
        {
            label_list.push_back(std::make_pair(label_id_list[i], text));
        }
    }
    return true;
}

// void MiningManager::tgTermList_(const izenelib::util::UString& text, std::vector<TgTerm>& termList)
// {
//
//
//
// }

// void MiningManager::normalTermList(const izenelib::util::UString& text, std::vector<sf1r::termid_t>& termList)
// {
//     termList.clear();
//     idManager_->getAnalysisTermIdList2(text, termList);
//
// }

bool MiningManager::addQrResult_(KeywordSearchResult& miaInput)
{
    //std::cout << "[MiningManager::getQRResult] "<<miaInput.rawQueryString_ << std::endl;
    miaInput.relatedQueryList_.clear();
    miaInput.rqScore_.clear();
    bool ret = false;
    QueryRecommendRep recommendRep;
    ret = getRecommendQuery_(izenelib::util::UString(miaInput.rawQueryString_,
                             miaInput.encodingType_), miaInput.topKDocs_, recommendRep);
    //std::cout << "Get " << recommendRep.recommendQueries_.size() << " related keywords" << std::endl;
    miaInput.relatedQueryList_ = recommendRep.recommendQueries_;
    miaInput.rqScore_ = recommendRep.scores_;

    if (!ret)
    {
        std::string msg = "error at getting related queries";
        if (recommendRep.error_.length() > 0)
        {
            msg = recommendRep.error_;
        }
        miaInput.error_ += "[QR: "+msg+"]";
    }

    return ret;
}

bool MiningManager::addTgResult_(KeywordSearchResult& miaInput)
{
    std::cout << "[MiningManager::getTGResult]" << std::endl;
    if (miaInput.topKDocs_.empty()) return true;

    izenelib::util::UString query(miaInput.rawQueryString_, miaInput.encodingType_);

    bool ret = tgManager_->GetConceptsByDocidList(miaInput.topKDocs_, query, miaInput.totalCount_, miaInput.tg_input);
    return ret;

}

bool MiningManager::addDupResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(dup_mtx_);
    if (!dupManager_) return true;
    miaInput.numberOfDuplicatedDocs_.resize(miaInput.topKDocs_.size());
    std::vector<count_t>::iterator result =
        miaInput.numberOfDuplicatedDocs_.begin();

    std::vector<docid_t> tmpDupDocs;
    for (std::vector<docid_t>::const_iterator it = miaInput.topKDocs_.begin();
            it != miaInput.topKDocs_.end(); ++it)
    {
        tmpDupDocs.clear(); // use resize to reserve the memory
        dupManager_->getDuplicatedDocIdList(*it, tmpDupDocs);
        *(result++) = tmpDupDocs.size();
    }

    return true;

}

bool MiningManager::addSimilarityResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(sim_mtx_);
    if (!similarityIndex_ && !similarityIndexEsa_) return true;
    miaInput.numberOfSimilarDocs_.resize(miaInput.topKDocs_.size());

    for (uint32_t i = 0; i < miaInput.topKDocs_.size(); i++)
    {
        uint32_t docid = miaInput.topKDocs_[i];
        uint32_t count = 0;
        if (!miningConfig_.similarity_param.enable_esa)
        {
            count = similarityIndex_->getSimilarDocNum(docid);
        }
        else
        {
            count = similarityIndexEsa_->getSimilarDocNum(docid);
        }
        miaInput.numberOfSimilarDocs_[i] = count;
    }

    return true;
}

bool MiningManager::addFacetedResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(sim_mtx_);
    if (!faceted_) return true;
    faceted_->GetSearcher()->GetRepresentation(miaInput.topKDocs_, miaInput.onto_rep_);

    return true;
}

bool MiningManager::visitDoc(uint32_t docId)
{
    if (!ctrManager_)
    {
        return true;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = ctrManager_->update(docId);
    if (!ret)
    {
        LOG(INFO) << "visitDoc failed to update ctrManager_ : " << docId;
    }

    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::clickGroupLabel(
    const std::string& query,
    const std::string& propName,
    const std::vector<std::string>& groupPath
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    faceted::PropValueTable::pvid_t pvId = propValueId_(propName, groupPath);
    if (pvId == 0)
    {
        return false;
    }

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = logger->logLabel(query, pvId);
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::getFreqGroupLabel(
    const std::string& query,
    const std::string& propName,
    int limit,
    std::vector<std::vector<std::string> >& pathVec,
    std::vector<int>& freqVec
)
{
    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    if (! logger->getFreqLabel(query, limit, pvIdVec, freqVec))
    {
        LOG(ERROR) << "error in GroupLabelLogger::getFreqLabel(), query: " << query;
        return false;
    }

    if (! groupManager_)
    {
        LOG(ERROR) << "the GroupManager is not initialized";
        return false;
    }

    const faceted::PropValueTable* propValueTable = groupManager_->getPropValueTable(propName);
    if (! propValueTable)
    {
        LOG(ERROR) << "the PropValueTable is not initialized for group property: " << propName;
        return false;
    }

    for (std::vector<faceted::PropValueTable::pvid_t>::const_iterator idIt = pvIdVec.begin();
            idIt != pvIdVec.end(); ++idIt)
    {
        std::vector<izenelib::util::UString> ustrPath;
        propValueTable->propValuePath(*idIt, ustrPath);

        std::vector<std::string> path;
        for (std::vector<izenelib::util::UString>::const_iterator ustrIt = ustrPath.begin();
             ustrIt != ustrPath.end(); ++ustrIt)
        {
            std::string str;
            ustrIt->convertString(str, UString::UTF_8);
            path.push_back(str);
        }

        pathVec.push_back(path);
    }

    return true;
}

faceted::PropValueTable::pvid_t MiningManager::propValueId_(
        const std::string& propName,
        const std::vector<std::string>& groupPath
) const
{
    faceted::PropValueTable::pvid_t pvId = 0;
    const faceted::PropValueTable* propValueTable = NULL;

    if (groupManager_ &&
        (propValueTable = groupManager_->getPropValueTable(propName)))
    {
        std::vector<izenelib::util::UString> ustrPath;
        convert_to_ustr_vector(groupPath, ustrPath);
        pvId = propValueTable->propValueId(ustrPath);

        if (pvId == 0)
        {
            std::string pathStr;
            for (std::vector<std::string>::const_iterator it = groupPath.begin();
                    it != groupPath.end(); ++it)
            {
                pathStr += *it;
                pathStr += '>';
            }
            LOG(WARNING) << "no pvid_t exists for group label path: " << pathStr;
        }
    }
    else
    {
        LOG(WARNING) << "PropValueTable is not initialized for group property: " << propName;
    }

    return pvId;
}

bool MiningManager::setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::vector<std::string> >& groupLabels
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    for (std::vector<std::vector<std::string> >::const_iterator it = groupLabels.begin();
        it != groupLabels.end(); ++it)
    {
        faceted::PropValueTable::pvid_t pvId = propValueId_(propName, *it);
        if (pvId == 0)
        {
            return false;
        }

        pvIdVec.push_back(pvId);
    }
    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = logger->setTopLabel(query, pvIdVec);
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::getMerchantScore(
    const std::vector<std::string>& merchantNames,
    MerchantStrScoreMap& merchantScoreMap
) const
{
    if (! merchantScoreManager_)
        return false;

    if (merchantNames.empty())
    {
        merchantScoreManager_->getAllStrScore(merchantScoreMap);
    }
    else
    {
        merchantScoreManager_->getStrScore(merchantNames, merchantScoreMap);
    }

    return true;
}

bool MiningManager::setMerchantScore(const MerchantStrScoreMap& merchantScoreMap)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    if (! merchantScoreManager_)
    {
        return false;
    }
    NoAdditionReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    merchantScoreManager_->setScore(merchantScoreMap);

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

bool MiningManager::setCustomRank(
    const std::string& query,
    const CustomRankDocStr& customDocStr
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    CustomRankDocId customDocId;

    bool convertResult = customDocIdConverter_ &&
        customDocIdConverter_->convert(customDocStr, customDocId);

    if (!convertResult)
    {
        return false;
    }

    NoAdditionReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool setResult = customRankManager_ &&
        customRankManager_->setCustomValue(query, customDocStr);

    DISTRIBUTE_WRITE_FINISH(convertResult && setResult);

    return convertResult && setResult;
}

bool MiningManager::getCustomRank(
    const std::string& query,
    std::vector<Document>& topDocList,
    std::vector<Document>& excludeDocList
)
{
    CustomRankDocId customDocId;

    return customRankManager_ &&
           customRankManager_->getCustomValue(query, customDocId) &&
           getDocList_(customDocId.topIds, topDocList) &&
           getDocList_(customDocId.excludeIds, excludeDocList);
}

bool MiningManager::getCustomQueries(std::vector<std::string>& queries)
{
    return customRankManager_ &&
           customRankManager_->getQueries(queries);
}

bool MiningManager::getDocList_(
    const std::vector<docid_t>& docIdList,
    std::vector<Document>& docList
)
{
    Document doc;

    for (std::vector<docid_t>::const_iterator it = docIdList.begin();
        it != docIdList.end(); ++it)
    {
        docid_t docId = *it;

        if (document_manager_->getDocument(docId, doc))
        {
            document_manager_->getRTypePropertiesForDocument(docId, doc);
            docList.push_back(doc);
        }
        else
        {
            LOG(WARNING) << "in getDocList_(), docid_t " << docId
                         << " does not exist";
        }
    }

    return true;
}

bool MiningManager::GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list)
{
    idmlib::tdt::TimeIdType start_date;
    if (!idmlib::util::TimeUtil::GetDateByUString(start, start_date))
    {
        std::string start_str;
        start.convertString(start_str, izenelib::util::UString::UTF_8);
        std::cout<<"parse date error on "<<start_str<<std::endl;
        return false;
    }
    idmlib::tdt::TimeIdType end_date;
    if (!idmlib::util::TimeUtil::GetDateByUString(end, end_date))
    {
        std::string end_str;
        end.convertString(end_str, izenelib::util::UString::UTF_8);
        std::cout<<"parse date error on "<<end_str<<std::endl;
        return false;
    }
    return GetTdtInTimeRange(start_date, end_date, topic_list);
}

bool MiningManager::GetTdtInTimeRange(const idmlib::tdt::TimeIdType& start, const idmlib::tdt::TimeIdType& end, std::vector<izenelib::util::UString>& topic_list)
{
    if (!mining_schema_.tdt_enable || !tdt_storage_) return false;
    idmlib::tdt::Storage* storage = tdt_storage_->Current();
    if (!storage)
    {
        std::cerr<<"can not get current storage for tdt"<<std::endl;
        return false;
    }
    return storage->GetTopicsInTimeRange(start, end, topic_list);
}

bool MiningManager::GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& info)
{
    if (!mining_schema_.tdt_enable || !tdt_storage_) return false;
    idmlib::tdt::Storage* storage = tdt_storage_->Current();
    if (!storage)
    {
        std::cerr<<"can not get current storage for tdt"<<std::endl;
        return false;
    }
    return storage->GetTopicInfo(text, info);
}

bool MiningManager::GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit)
{
    if (!mining_schema_.tdt_enable) return false;
    return topicDetector_->GetTopics(content, topic_list,limit);
}

void MiningManager::GetRefinedQuery(const izenelib::util::UString& query, izenelib::util::UString& result)
{
    if (!qcManager_) return;
    qcManager_->getRefinedQuery(query, result);
}

void MiningManager::InjectQueryCorrection(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    if (!qcManager_) return;
    qcManager_->Inject(query, result);
}

void MiningManager::FinishQueryCorrectionInject()
{
    if (!qcManager_) return;
    qcManager_->FinishInject();
}

void MiningManager::InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    if (!qrManager_) return;
    qrManager_->Inject(query, result);
}

void MiningManager::FinishQueryRecommendInject()
{
    if (!qrManager_) return;
    qrManager_->FinishInject();
}

bool MiningManager::GetSummarizationByRawKey(
        const izenelib::util::UString& rawKey,
        Summarization& result)
{
    if (!summarizationManagerTask_) return false;
    return summarizationManagerTask_->GetSummarizationByRawKey(rawKey,result);
}

uint32_t MiningManager::GetSignatureForQuery(
        const KeywordSearchActionItem& item,
        std::vector<uint64_t>& signature)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return 0;

    izenelib::util::UString text(item.env_.queryString_, izenelib::util::UString::UTF_8);
    return dupManager_->getSignatureForText(text, signature);
}

bool MiningManager::GetKNNListBySignature(
        const std::vector<uint64_t>& signature,
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList,
        std::size_t& totalCount,
        uint32_t knnTopK,
        uint32_t knnDist,
        uint32_t start)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return false;

    if (signature.empty()) return false;

    return dupManager_->getKNNListBySignature(signature, knnTopK, start, knnDist, docIdList, rankScoreList, totalCount);
}

namespace
{
struct IsDeleted
{
    IsDeleted(const boost::shared_ptr<DocumentManager>& doc_manager)
        :inner_doc_manager_(doc_manager)
    {
    }
    bool operator()(const std::pair<double, uint32_t>& single_res) const
    {
        return inner_doc_manager_->isDeleted(single_res.second);
    }
private:
    const boost::shared_ptr<DocumentManager>& inner_doc_manager_;
};
}

bool MiningManager::GetSuffixMatch(
        const SearchKeywordOperation& actionOperation,
        uint32_t max_docs,
        bool use_fuzzy,
        uint32_t start,
        const std::vector<QueryFiltering::FilteringType>& filter_param,
        std::vector<uint32_t>& docIdList,
        std::vector<float>& rankScoreList,
        std::vector<float>& customRankScoreList,
        std::size_t& totalCount,
        faceted::GroupRep& groupRep,
        sf1r::faceted::OntologyRep& attrRep,
        UString& analyzedQuery,
        std::string& pruneQueryString_)
{
    if (!mining_schema_.suffixmatch_schema.suffix_match_enable || !suffixMatchManager_)
        return false;

    izenelib::util::UString queryU(actionOperation.actionItem_.env_.queryString_, izenelib::util::UString::UTF_8);
    std::vector<std::pair<double, uint32_t> > res_list;

    const std::vector<string>& search_in_properties = actionOperation.actionItem_.searchPropertyList_;

    size_t orig_max_docs = max_docs;

    if (!use_fuzzy)
    {
        totalCount = suffixMatchManager_->longestSuffixMatch(
                actionOperation.actionItem_.env_.queryString_,
                search_in_properties,
                max_docs,
                res_list);
    }
    else
    {
        if (actionOperation.actionItem_.groupParam_.groupProps_.size() > 0 ||
            actionOperation.actionItem_.groupParam_.isAttrGroup_)
        {
            // need do counter.
            max_docs = mining_schema_.suffixmatch_schema.suffix_groupcounter_topk;
            LOG(INFO) << "topk doc num is :" << max_docs;
        }

        LOG(INFO) << "suffix searching using fuzzy mode " << endl;

        std::string pattern_orig = actionOperation.actionItem_.env_.queryString_;

        if (pattern_orig.empty())
            return 0;

        std::string pattern = pattern_orig;
        boost::to_lower(pattern);

        LOG(INFO) << "original query string: " << pattern_orig;

        std::list<std::pair<UString, double> > major_tokens;
        std::list<std::pair<UString, double> > minor_tokens;
        suffixMatchManager_->GetTokenResults(pattern, major_tokens, minor_tokens, analyzedQuery);
        for (std::list<std::pair<UString, double> >::iterator i = major_tokens.begin(); i != major_tokens.end(); ++i)
        {
            std::string key;
            (i->first).convertString(key, izenelib::util::UString::UTF_8);
            cout << key << " " << i->second << endl;
        }
        cout<<"-----"<<endl;
        for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin(); i != minor_tokens.end(); ++i)
        {
            std::string key;
            (i->first).convertString(key, izenelib::util::UString::UTF_8);
            cout << key << " " << i->second << endl;
        }

        const double rank_boundary = 0.8;

        totalCount = suffixMatchManager_->AllPossibleSuffixMatch(
            actionOperation.actionItem_.languageAnalyzerInfo_.synonymExtension_,
            major_tokens,
            minor_tokens,
            search_in_properties,
            max_docs,
            actionOperation.actionItem_.searchingMode_.filtermode_,
            filter_param,
            actionOperation.actionItem_.groupParam_,
            res_list,
            rank_boundary);

        if (mining_schema_.suffixmatch_schema.suffix_incremental_enable)
        {
            std::vector<uint32_t> _docIdList;
            std::vector<double> _rankScoreList;

            incrementalManager_->fuzzySearch(actionOperation.actionItem_.env_.queryString_, search_in_properties,
                    _docIdList, _rankScoreList, filter_param, actionOperation.actionItem_.groupParam_);

            uint32_t max_count = std::min(max_docs, (uint32_t)_docIdList.size());

            izenelib::util::ClockTimer timer;

            WordPriorityQueue_ topk_seedbigram(max_count);

            for (size_t i = 0; i < _docIdList.size(); ++i)
            {
                topk_seedbigram.insert(std::make_pair(_rankScoreList[i], _docIdList[i]));
            }

            max_count += res_list.size();
            std::vector<std::pair<double, uint32_t> > inc_res_list(max_count);
            for (uint32_t i = 0; i < max_count; ++i)
            {
                inc_res_list[max_count - i - 1] = topk_seedbigram.pop();
            }

            totalCount += _docIdList.size();

            std::merge(res_list.begin(), res_list.end(), inc_res_list.begin() + res_list.size(), inc_res_list.end(), inc_res_list.begin(), std::greater<std::pair<double, uint32_t> >());
            inc_res_list.swap(res_list);

            LOG(INFO) << "[]TOPN and cost:" << timer.elapsed() << " seconds" << std::endl;
        }

        if ((groupManager_ || attrManager_) && groupFilterBuilder_)
        {
            PropSharedLockSet propSharedLockSet;
            boost::scoped_ptr<faceted::GroupFilter> groupFilter(
                    groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_, propSharedLockSet));

            if (groupFilter)
            {
                for (size_t i = 0; i < res_list.size(); ++i)
                {
                    groupFilter->test(res_list[i].second);
                }
                groupFilter->getGroupRep(groupRep, attrRep);
            }
        }
    }

    if (!totalCount ||res_list.empty()) return false;

    //We do not use this post delete filtering because deleted documents should never be searched from
    //suffix index in normal cases, while if there are deleted documents before mining finished, these documents
    //should be abled to searched as well.
    //We ignore the exception case that SF1 quit before mining finished
    //if (!hasDeletedDocDuringMining_)
    //    res_list.erase(std::remove_if (res_list.begin(), res_list.end(), IsDeleted(document_manager_)), res_list.end());
    res_list.resize(std::min(orig_max_docs, res_list.size()));

    docIdList.resize(res_list.size());
    rankScoreList.resize(res_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        rankScoreList[i] = res_list[i].first;
        docIdList[i] = res_list[i].second;
    }

    searchManager_->fuzzySearchRanker_.rank(actionOperation, start, docIdList,
            rankScoreList, customRankScoreList);

    cout<<"return true"<<endl;
    return true;
}

bool MiningManager::GetProductCategory(
        const std::string& squery,
        int limit,
        std::vector<std::vector<std::string> >& pathVec)
{
    return product_categorizer_->GetProductCategory(squery, limit, pathVec);
}

bool MiningManager::GetProductCategory(const UString& query, UString& backend)
{
    if (mining_schema_.product_matcher_enable)
    {
        ProductMatcher* matcher = ProductMatcherInstance::get();
        Document doc;
        doc.property("Title") = query;
        ProductMatcher::Product result_product;
        if (matcher->Process(doc, result_product))
        {
            const std::string& category_name = result_product.scategory;
            if (!category_name.empty())
            {
                bool valid = true;
                if (!match_category_restrict_.empty())
                {
                    valid = false;
                    for (uint32_t i=0;i<match_category_restrict_.size();i++)
                    {
                        if (boost::regex_match(category_name, match_category_restrict_[i]))
                        {
                            valid = true;
                            break;
                        }
                    }
                }
                if (valid)
                {
                    backend = UString(result_product.scategory, UString::UTF_8);
                    return true;
                }
            }
        }
    }
    return false;
}

bool MiningManager::GetProductFrontendCategory(
        const izenelib::util::UString& query,
        int limit,
        std::vector<UString>& frontends)
{
    if (mining_schema_.product_matcher_enable)
    {
        ProductMatcher* matcher = ProductMatcherInstance::get();
        Document doc;
        doc.property("Title") = query;
        std::vector<ProductMatcher::Product> result_products;
        if (matcher->Process(doc, (uint32_t)limit, result_products))
        {
            for (uint32_t i = 0; i < result_products.size(); ++i)
            {
                const std::string& category_name = result_products[i].scategory;
                if (!category_name.empty())
                {
                    bool valid = true;
                    if (!match_category_restrict_.empty())
                    {
                        valid = false;
                        for (uint32_t i = 0; i < match_category_restrict_.size(); ++i)
                        {
                            if (boost::regex_match(category_name, match_category_restrict_[i]))
                            {
                                valid = true;
                                break;
                            }
                        }
                    }
                    if (valid && !result_products[i].fcategory.empty())
                    {
                        frontends.push_back(UString(result_products[i].fcategory, UString::UTF_8));
                    }
                }
            }
        }
    }
    if (!frontends.empty()) return true;
    return false;
}
bool MiningManager::GetProductFrontendCategory(const izenelib::util::UString& query, UString& frontend)
{
    std::vector<UString> frontends;
    GetProductFrontendCategory(query,1, frontends);
    if (!frontends.empty())
    {
        frontend = frontends[0];
        return true;
    }
    return false;
}

bool MiningManager::SetKV(const std::string& key, const std::string& value)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    if (!kvManager_)
    {
        return false;
    }

    NoAdditionReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    kvManager_->update(key, value);

    DISTRIBUTE_WRITE_FINISH(true);

    return true;
}

bool MiningManager::GetKV(const std::string& key, std::string& value)
{
    if (kvManager_)
    {
        return kvManager_->get(key, value);
    }
    else
    {
        return false;
    }
}

bool MiningManager::doTgInfoInit_()
{
    if (!tgInfo_)
    {
        tgInfo_ = new TaxonomyInfo(tg_path_,miningConfig_.taxonomy_param, labelManager_, kpe_analyzer_, system_resource_path_);
        tgInfo_->setKPECacheSize(1000000);
        if (!tgInfo_->Open())
        {
            delete tgInfo_;
            tgInfo_ = NULL;
            return false;
        }
    }
    return true;
}

void MiningManager::doTgInfoRelease_()
{
    if (tgInfo_)
    {
        delete tgInfo_;
        tgInfo_ = NULL;
    }
}

const faceted::PropValueTable* MiningManager::GetPropValueTable(const std::string& propName) const
{
    if (! groupManager_)
        return NULL;

    return groupManager_->getPropValueTable(propName);
}

bool MiningManager::getProductScore(
        const std::string& docIdStr,
        const std::string& scoreTypeName,
        score_t& scoreValue)
{
    docid_t docId = 0;

    if (!customDocIdConverter_ ||
        !customDocIdConverter_->convertDocId(docIdStr, docId))
        return false;

    if (scoreTypeName == faceted::CTRManager::kCtrPropName)
    {
        if (!ctrManager_)
            return false;

        scoreValue = ctrManager_->getClickCountByDocId(docId);
        return true;
    }

    ProductScoreType scoreType =
        mining_schema_.product_ranking_config.getScoreType(scoreTypeName);

    const ProductScoreTable* productScoreTable =
        productScoreManager_->getProductScoreTable(scoreType);

    if (productScoreTable == NULL)
    {
        LOG(WARNING) << "unknown score type: " << scoreTypeName;
        return false;
    }

    scoreValue = productScoreTable->getScoreHasLock(docId);
    return true;
}

bool MiningManager::initMerchantScoreManager_(const ProductRankingConfig& rankConfig)
{
    const ProductScoreConfig& merchantScoreConfig =
        rankConfig.scores[MERCHANT_SCORE];

    if (merchantScoreConfig.propName.empty() || !groupManager_)
        return true;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path scoreDir(parentDir / "merchant_score");
    bfs::create_directories(scoreDir);

    faceted::PropValueTable* merchantValueTable =
        groupManager_->getPropValueTable(merchantScoreConfig.propName);
    const ProductScoreConfig& categoryScoreConfig =
        rankConfig.scores[CATEGORY_SCORE];
    faceted::PropValueTable* categoryValueTable =
        groupManager_->getPropValueTable(categoryScoreConfig.propName);

    if (merchantScoreManager_) delete merchantScoreManager_;

    merchantScoreManager_ = new MerchantScoreManager(
        merchantValueTable, categoryValueTable);

    bfs::path scorePath = scoreDir / "score.bin";
    if (!merchantScoreManager_->open(scorePath.string()))
    {
        LOG(ERROR) << "open " << scorePath << " failed";
        return false;
    }

    return true;
}

bool MiningManager::initGroupLabelKnowledge_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable || !groupManager_)
        return true;

    if (groupLabelKnowledge_) delete groupLabelKnowledge_;

    const ProductScoreConfig& categoryScoreConfig =
        rankConfig.scores[CATEGORY_SCORE];

    const faceted::PropValueTable* categoryValueTable =
        groupManager_->getPropValueTable(categoryScoreConfig.propName);

    const bfs::path resourcePath(system_resource_path_);
    const bfs::path categoryBoostDir(resourcePath / "category-boost");
    if (bfs::exists(categoryBoostDir) && categoryValueTable)
    {
        groupLabelKnowledge_ = new GroupLabelKnowledge(collectionName_,
                                                       *categoryValueTable);

        if (!groupLabelKnowledge_->open(categoryBoostDir.string()))
        {
            LOG(ERROR) << "error in opening " << categoryBoostDir;
        }
    }

    return true;
}

bool MiningManager::initProductScorerFactory_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable)
        return true;

    LOG(INFO) << rankConfig.toStr();

    if (productScorerFactory_) delete productScorerFactory_;
    if (productScoreManager_) delete productScoreManager_;
    if (offlineScorerFactory_) delete offlineScorerFactory_;
    if (customRankManager_) delete customRankManager_;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path customRankDir(parentDir / "custom_rank");
    bfs::create_directories(customRankDir);

    const bfs::path customRankPath(customRankDir / "custom.db");
    customRankManager_ = new CustomRankManager(customRankPath.string(),
                                               *customDocIdConverter_,
                                               document_manager_.get());

    offlineScorerFactory_ = new OfflineProductScorerFactoryImpl(
        numericTableBuilder_);

    const bfs::path scoreDir(parentDir / "product_score");
    productScoreManager_ = new ProductScoreManager(
        rankConfig,
        miningConfig_.product_ranking_param,
        *offlineScorerFactory_,
        *document_manager_,
        collectionName_,
        scoreDir.string(),
        searchCache_);

    if (!productScoreManager_->open())
    {
        LOG(ERROR) << "open " << scoreDir << " failed";
        return false;
    }

    productScorerFactory_ = new ProductScorerFactory(rankConfig, *this);
    return true;
}

bool MiningManager::initProductRankerFactory_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable)
        return true;

    const std::string& offerCountPropName =
        rankConfig.scores[OFFER_ITEM_COUNT_SCORE].propName;
    const std::string& diversityPropName =
        rankConfig.scores[DIVERSITY_SCORE].propName;
    const score_t randomScoreWeight =
        rankConfig.scores[RANDOM_SCORE].weight;

    if (offerCountPropName.empty() &&
        diversityPropName.empty() &&
        randomScoreWeight == 0)
        return true;

    const std::string& categoryPropName =
        rankConfig.scores[CATEGORY_SCORE].propName;

    const faceted::PropValueTable* categoryValueTable =
        GetPropValueTable(categoryPropName);

    const boost::shared_ptr<const NumericPropertyTableBase>& offerItemCountTable =
        numericTableBuilder_->createPropertyTable(offerCountPropName);

    if (!offerCountPropName.empty() && !offerItemCountTable)
    {
        LOG(ERROR) << "the NumericPropertyTableBase is not initialized for property: "
                   << offerCountPropName;
        return false;
    }

    const faceted::PropValueTable* diversityValueTable =
        GetPropValueTable(diversityPropName);

    if (!diversityPropName.empty() && !diversityValueTable)
    {
        LOG(ERROR) << "the PropValueTable is not initialized for property: "
                   << diversityPropName;
        return false;
    }

    if (productRankerFactory_) delete productRankerFactory_;
    productRankerFactory_ = new ProductRankerFactory(rankConfig,
                                                     categoryValueTable,
                                                     offerItemCountTable,
                                                     diversityValueTable,
                                                     merchantScoreManager_);
    return true;
}

void MiningManager::flush()
{
    LOG(INFO) << "begin flush in MiningManager .... ";
    for (GroupLabelLoggerMap::iterator it = groupLabelLoggerMap_.begin();
            it != groupLabelLoggerMap_.end(); ++it)
    {
        it->second->flush();
    }

    LOG(INFO) << "GroupLabelLoggerMap flushed .... ";
    if (customRankManager_) customRankManager_->flush();
    if (merchantScoreManager_) merchantScoreManager_->flush();
    if (kvManager_) kvManager_->flush();
    if (labelManager_) labelManager_->flush();
    if (label_sim_collector_) label_sim_collector_->Flush();
    if (rmDb_) rmDb_->flush();
    if (idManager_) idManager_->flush();
    LOG(INFO) << "MiningManager flush finished.";
}

void MiningManager::updateMergeFuzzyIndex(int calltype)
{
    if (cronExpression_.matches_now() || calltype > 0)
    {
        static const string cronJobName = "fuzzy_index_merge";
        if (calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(cronJobName, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << cronJobName;
            }
            else
            {
                LOG(INFO) << "cron job on replica ignored. ";
            }
            return;
        }

        DISTRIBUTE_WRITE_BEGIN;
        DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;
        CronJobReqLog reqlog;
        reqlog.cron_time = Utilities::createTimeStamp();
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName << std::endl;
            return;
        }

        if (!incrementalManager_->isEmpty())
        {
            LOG (INFO) << "update cron-job with fm-index";
            SuffixMatchMiningTask* miningTask = suffixMatchManager_->getMiningTask();
            bool isIncre = false;
            miningTask->setTaskStatus(isIncre);

            suffixMatchManager_->updateFmindex();
            incrementalManager_->reset();

            docid_t docid;
            docid = miningTask->getLastDocId();
            incrementalManager_->setLastDocid(docid);
        }
        DISTRIBUTE_WRITE_FINISH(true);
    }
}

}
