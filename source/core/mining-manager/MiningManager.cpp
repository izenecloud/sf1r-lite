#include <configuration-manager/PropertyConfig.h>
#include <document-manager/DocumentManager.h>
#include "MiningManager.h"
#include "MiningQueryLogHandler.h"

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
#include "attr-manager/AttrManager.h"
#include "faceted-submanager/ctr_manager.h"

#include "group-label-logger/GroupLabelLogger.h"
#include "merchant-score-manager/MerchantScoreManager.h"
#include "custom-rank-manager/CustomRankManager.h"
#include "product-ranker/ProductRankerFactory.h"

#include <search-manager/SearchManager.h>
#include <index-manager/IndexManager.h>

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

#include <ir/index_manager/index/IndexReader.h>
#include <ir/id_manager/IDManager.h>

#include <am/3rdparty/rde_hash.h>
#include <util/ClockTimer.h>
#include <util/filesystem.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/timer.hpp>
#include <boost/scoped_ptr.hpp>

#include <fstream>
#include <iterator>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <algorithm>
#include <memory> // auto_ptr

using namespace boost::filesystem;
using namespace izenelib::ir::idmanager;
using namespace sf1r;
namespace bfs = boost::filesystem;

std::string MiningManager::system_resource_path_;
std::string MiningManager::system_working_path_;

MiningManager::MiningManager(
        const std::string& collectionDataPath,
        const std::string& queryDataPath,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& index_manager,
        const boost::shared_ptr<SearchManager>& searchManager,
        const boost::shared_ptr<IDManager>& idManager,
        const std::string& collectionName,
        const DocumentSchema& documentSchema,
        const MiningConfig& miningConfig,
        const MiningSchema& miningSchema)
    : collectionDataPath_(collectionDataPath)
    , queryDataPath_(queryDataPath)
    , collectionName_(collectionName)
    , documentSchema_(documentSchema)
    , miningConfig_(miningConfig)
    , mining_schema_(miningSchema)
    , analyzer_(NULL)
    , c_analyzer_(NULL)
    , kpe_analyzer_(NULL)
    , document_manager_(documentManager)
    , index_manager_(index_manager)
    , searchManager_(searchManager)
    , tgInfo_(NULL)
    , idManager_(idManager)
    , groupManager_(NULL)
    , attrManager_(NULL)
    , merchantScoreManager_(NULL)
    , customRankManager_(NULL)
    , productRankerFactory_(NULL)
    , tdt_storage_(NULL)
    , summarizationManager_(NULL)
{
}

MiningManager::~MiningManager()
{
    if (analyzer_) delete analyzer_;
    if (c_analyzer_) delete c_analyzer_;
    if (kpe_analyzer_) delete kpe_analyzer_;
    if (productRankerFactory_) delete productRankerFactory_;
    if (customRankManager_) delete customRankManager_;
    if (merchantScoreManager_) delete merchantScoreManager_;
    if (groupManager_) delete groupManager_;
    if (attrManager_) delete attrManager_;
    if (tdt_storage_) delete tdt_storage_;
    if (summarizationManager_) delete summarizationManager_;
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
                         miningConfig_.query_correction_param.enableCN));
        rmDb_.reset(new RecommendManager(queryDataPath_, collectionName_, mining_schema_, document_manager_,
                                         qcManager_, analyzer_, logdays));

        if(!rmDb_->open())
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

        /** group */
        if (mining_schema_.group_enable)
        {
            if (groupManager_) delete groupManager_;
            std::string groupPath = prefix_path + "/group";

            groupManager_ = new faceted::GroupManager(document_manager_.get(), groupPath);
            if (! groupManager_->open(mining_schema_.group_properties))
            {
                std::cerr << "open GROUP failed" << std::endl;
                return false;
            }
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
        }

        if (groupManager_ || attrManager_)
        {
            faceted::GroupFilterBuilder* filterBuilder = new faceted::GroupFilterBuilder(mining_schema_.group_properties,
                    groupManager_,
                    attrManager_,
                    searchManager_.get());
            searchManager_->setGroupFilterBuilder(filterBuilder);
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

            for (std::vector<GroupConfig>::const_iterator it = mining_schema_.group_properties.begin();
                    it != mining_schema_.group_properties.end(); ++it)
            {
                if (! it->isStringType())
                    continue;

                const std::string& propName = it->propName;
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

        /** merchant score */
        if (!mining_schema_.product_ranking_config.merchantPropName.empty() && groupManager_)
        {
            if (merchantScoreManager_) delete merchantScoreManager_;

            const bfs::path scoreDir = bfs::path(prefix_path) / "merchant_score";
            bfs::create_directories(scoreDir);

            const std::string& merchantProp = mining_schema_.product_ranking_config.merchantPropName;
            faceted::PropValueTable* merchantValueTable = groupManager_->getPropValueTable(merchantProp);

            const std::string& categoryProp = mining_schema_.product_ranking_config.categoryPropName;
            faceted::PropValueTable* categoryValueTable = groupManager_->getPropValueTable(categoryProp);

            merchantScoreManager_ = new MerchantScoreManager(merchantValueTable, categoryValueTable);

            const std::string scorePath = (scoreDir / "score.txt").string();
            if (! merchantScoreManager_->open(scorePath))
            {
                std::cerr << "open " << scorePath << " failed" << std::endl;
                return false;
            }
        }

        /** product ranking */
        if (mining_schema_.product_ranking_config.isEnable)
        {
            // custom rank manager
            if (customRankManager_) delete customRankManager_;

            const bfs::path customRankDir = bfs::path(prefix_path) / "custom_rank";
            bfs::create_directories(customRankDir);

            const std::string customRankPath = (customRankDir / "custom.db").string();
            customRankManager_ = new CustomRankManager(customRankPath);

            // product ranker factory
            if (productRankerFactory_) delete productRankerFactory_;

            productRankerFactory_ = new ProductRankerFactory(this);

            searchManager_->setCustomRankManager(customRankManager_);
            searchManager_->setProductRankerFactory(productRankerFactory_);
        }

        /** tdt **/
        if (mining_schema_.tdt_enable)
        {
            tdt_path_ = prefix_path + "/tdt";
            boost::filesystem::create_directories(tdt_path_);
            std::string tdt_storage_path = tdt_path_+"/storage";
            tdt_storage_ = new TdtStorageType(tdt_storage_path);
            if (!tdt_storage_->Open())
            {
                std::cerr<<"tdt init failed"<<std::endl;
                return false;
            }
        }


        /** Summarization */
        if (mining_schema_.summarization_enable)
        {
            summarization_path_ = prefix_path + "/summarization";
            boost::filesystem::create_directories(summarization_path_);
            summarizationManager_ =
                new MultiDocSummarizationSubManager(summarization_path_,
                                                    mining_schema_.summarization_schema,
                                                    document_manager_,
                                                    index_manager_,
                                                    c_analyzer_);

           if (!mining_schema_.summarization_schema.uuidPropName.empty())
           {
               //searchManager_->set_filter_hook(boost::bind(&MultiDocSummarizationSubManager::AppendSearchFilter, summarizationManager_, _1));
           }
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

void MiningManager::DoContinue()
{
    //do mining continue;
    try
    {
        std::string continue_file = collectionDataPath_+"/continue";
        if (boost::filesystem::exists(continue_file))
        {
            boost::filesystem::remove_all(continue_file);
            DoMiningCollection();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

bool MiningManager::DoMiningCollection()
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
        MiningQueryLogHandler::getInstance()->runEvents();
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

    //do group
    if (mining_schema_.group_enable)
    {
        groupManager_->processCollection();
    }

    //do attr
    if (mining_schema_.attr_enable)
    {
        attrManager_->processCollection();
    }

    //do tdt
    if (mining_schema_.tdt_enable)
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
            //         for(uint32_t i=0;i<topic_list.size();i++)
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

    // do Summarization
    if (mining_schema_.summarization_enable)
    {
        summarizationManager_->EvaluateSummarization();
    }
    return true;
}

void MiningManager::onIndexUpdated(size_t docNum)
{
    if (ctrManager_)
    {
        ctrManager_->updateDocNum(docNum);
    }
}

bool MiningManager::getMiningResult(KeywordSearchResult& miaInput)
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

    std::cout << "[MiningManager::getMiningResult]" << std::endl;
    bool bResult = true;
    try
    {
        START_PROFILER(qr);
        if (true)
        {
            izenelib::util::ClockTimer clocker;
            addQrResult_(miaInput);
            std::cout<<"QR ALL COST: "<<clocker.elapsed()<<" seconds."<<std::endl;
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
    std::cout << "[getMiningResult finished]" << std::endl;
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
//     if(status_)
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
    struct timeval tv_start;
    struct timeval tv_end;
    gettimeofday(&tv_start, NULL);
    qrManager_->getRecommendQuery(queryStr, topDocIdList,
                                  miningConfig_.recommend_param.recommend_num, recommendRep);
    gettimeofday(&tv_end, NULL);
    double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
                       + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    std::cout << "QR all cost " << timespend << " seconds." << std::endl;
    return true;
}

bool MiningManager::getReminderQuery(
        std::vector<izenelib::util::UString>& popularQueries,
        std::vector<izenelib::util::UString>& realtimeQueries)
{
    return true;
}

bool MiningManager::getUniqueDocIdList(const std::vector<uint32_t>& docIdList,
                                       std::vector<uint32_t>& cleanDocs)
{
    if (!mining_schema_.dupd_enable || !dupManager_)
        return false;

    return dupManager_->getUniqueDocIdList(docIdList, cleanDocs);

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
//   if(pIndexReader->maxDoc()> similarityIndex_->GetMaxDocId())
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
    std::cout << "[MiningManager::getQRResult] "<<miaInput.rawQueryString_ << std::endl;
    miaInput.relatedQueryList_.clear();
    miaInput.rqScore_.clear();
    bool ret = false;
    QueryRecommendRep recommendRep;
    ret = getRecommendQuery_(izenelib::util::UString(miaInput.rawQueryString_,
                             miaInput.encodingType_), miaInput.topKDocs_, recommendRep);
    std::cout << "Get " << recommendRep.recommendQueries_.size()
              << " related keywords" << std::endl;
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

    return ctrManager_->update(docId);
}

bool MiningManager::clickGroupLabel(
    const std::string& query,
    const std::string& propName,
    const std::vector<std::string>& groupPath
)
{
    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    faceted::PropValueTable::pvid_t pvId = propValueId_(propName, groupPath);
    if (pvId == 0)
        return false;

    return logger->logLabel(query, pvId);
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
        for (std::vector<std::string>::const_iterator it = groupPath.begin();
                it != groupPath.end(); ++it)
        {
            ustrPath.push_back(izenelib::util::UString(*it, UString::UTF_8));
        }

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
            return false;

        pvIdVec.push_back(pvId);
    }

    return logger->setTopLabel(query, pvIdVec);
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
    if (! merchantScoreManager_)
        return false;

    merchantScoreManager_->setScore(merchantScoreMap);
    return true;
}

bool MiningManager::setCustomRank(
    const std::string& query,
    const std::vector<std::string>& topDocIdList,
    const std::vector<std::string>& excludeDocIdList
)
{
    CustomRankValue customValue;

    return customRankManager_ &&
           convertDocIdList_(topDocIdList, customValue.topIds) &&
           convertDocIdList_(excludeDocIdList, customValue.excludeIds) &&
           customRankManager_->setCustomValue(query, customValue);
}

bool MiningManager::getCustomRank(
    const std::string& query,
    std::vector<Document>& topDocList,
    std::vector<Document>& excludeDocList
)
{
    CustomRankValue customValue;

    return customRankManager_ &&
           customRankManager_->getCustomValue(query, customValue) &&
           getDocList_(customValue.topIds, topDocList) &&
           getDocList_(customValue.excludeIds, excludeDocList);
}

bool MiningManager::getCustomQueries(std::vector<std::string>& queries)
{
    if (customRankManager_)
        return customRankManager_->getQueries(queries);

    return false;
}

bool MiningManager::convertDocIdList_(
    const std::vector<std::string>& strList,
    std::vector<docid_t>& idList
)
{
    for (std::vector<std::string>::const_iterator it = strList.begin();
        it != strList.end(); ++it)
    {
        const std::string& strId = *it;
        uint128_t convertId = Utilities::md5ToUint128(strId);
        docid_t docId = 0;

        if (! idManager_->getDocIdByDocName(convertId, docId, false))
        {
            LOG(WARNING) << "in convertDocIdList_(), DOCID " << strId << " does not exist";
            return false;
        }

        idList.push_back(docId);
    }

    return true;
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

        if (! document_manager_->getDocument(docId, doc))
        {
            LOG(WARNING) << "in getDocList_(), docid_t " << docId << " does not exist";
            continue;
        }

        docList.push_back(doc);
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
    if(!summarizationManager_) return false;
    return summarizationManager_->GetSummarizationByRawKey(rawKey,result);
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
