/// @details
/// - Log
///     - 2009.08.27 change variable names. (topKDocs_ -> topKDocs_ , topKDocItemList_ -> topKDocItemList_) by Dohyun Yun
///     - 2009.11.25 refine the code writing -Jinglei

#include <common/SFLogger.h>
#include <log-manager/LogManager.h>
#include "util/FSUtil.hpp"
#include "util/TermUtil.hpp"
#include "util/MUtil.hpp"
#include "MiningManager.h"
#include "taxonomy-generation-submanager/TaxonomyRep.h"
#include "taxonomy-generation-submanager/TaxonomyInfo.h"
#include "taxonomy-generation-submanager/LabelManager.h"
#include "similarity-detection-submanager/PrunedSortedTermInvertedIndexReader.h"
#include "similarity-detection-submanager/DocWeightListPrunedInvertedIndexReader.h"
#include "LabelSynchronizer.h"

#include <directory-manager/DirectoryCookie.h>
#include <pwd.h>

#include <util/profiler/ProfilerGroup.h>
#include <ir/index_manager/index/IndexReader.h>
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

#include <fstream>
#include <iterator>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <algorithm>

using namespace boost::filesystem;

using namespace sf1r;
namespace bfs = boost::filesystem;
MiningManager::MiningManager(const std::string& collectionDataPath, const std::string& queryDataPath,
                             const boost::shared_ptr<LAManager>& laManager,
                             const boost::shared_ptr<DocumentManager>& documentManager,
                             const boost::shared_ptr<IndexManager>& index_manager,
                             const std::string& collectionName,
                             const schema_type& schema,
                             const MiningConfig& miningConfig,
                             const MiningSchema& miningSchema
                            )
        :collectionDataPath_(collectionDataPath), queryDataPath_(queryDataPath)
        ,collectionName_(collectionName), schema_(schema), miningConfig_(miningConfig), mining_schema_(miningSchema)
        , laManager_(laManager), document_manager_(documentManager), index_manager_(index_manager)
        , tgInfo_(NULL)
{
}
MiningManager::MiningManager(const std::string& collectionDataPath, const std::string& queryDataPath,
                             const boost::shared_ptr<LAManager>& laManager,
                             const boost::shared_ptr<DocumentManager>& documentManager,
                             const boost::shared_ptr<IndexManager>& index_manager,
                             const std::string& collectionName,
                             const schema_type& schema,
                             const MiningConfig& miningConfig,
                             const MiningSchema& miningSchema,
                             const boost::shared_ptr<IDManager>idManager)
        :collectionDataPath_(collectionDataPath), queryDataPath_(queryDataPath)
        ,collectionName_(collectionName), schema_(schema), miningConfig_(miningConfig), mining_schema_(miningSchema)
        , laManager_(laManager), document_manager_(documentManager), index_manager_(index_manager)
        , tgInfo_(NULL)
        , idManager_(idManager)
{
}
// void MiningManager::setConfigClient(const boost::shared_ptr<ConfigurationManagerClient>& configClient)
// {
// 	configClient_ = configClient;
// }

MiningManager::~MiningManager()
{
    //close();
}

void MiningManager::close()
{

//     doIDManagerRelease_();
}

bool MiningManager::open()
{
    std::cout<<"DO_TG : "<<(int)mining_schema_.tg_enable<<std::endl;
    std::cout<<"DO_DUPD : "<<(int)mining_schema_.dupd_enable<<std::endl;
    std::cout<<"DO_SIM : "<<(int)mining_schema_.sim_enable<<std::endl;
    std::cout<<"DO_FACETED : "<<(int)mining_schema_.faceted_enable<<std::endl;
    std::cout<<"DO_IISE : "<<(int)mining_schema_.ise_enable<<std::endl;

    /** Global variables **/
    try
    {


        bfs::path cookiePath(bfs::path(basicPath_) / "cookie");
        directory::DirectoryCookie cookie(cookiePath);
        if ( cookie.read() )
        {
            if ( !cookie.valid() )
            {
                sflog->info(SFL_MINE, "Mining data is broken.");
                boost::filesystem::remove_all(basicPath_);
                sflog->info(SFL_MINE, "Mining data deleted.");
            }
        }

        std::string prefix_path  = collectionDataPath_;
        FSUtil::createDir(prefix_path);
//       status_.reset(new MiningStatus(prefix_path+"/mining_status") );
        std::string kpe_res_path = system_resource_path_+"/kpe";
        /** analyzer */

        std::string kma_path;
        LAPool::getInstance()->get_kma_path(kma_path );
        analyzer_ = new idmlib::util::IDMAnalyzer(kma_path);
        if ( !analyzer_->LoadT2SMapFile(kpe_res_path+"/cs_ct") )
        {
            return false;
        }
        kpe_analyzer_ = new idmlib::util::IDMAnalyzer(kma_path);
        if ( !kpe_analyzer_->LoadT2SMapFile(kpe_res_path+"/cs_ct") )
        {
            return false;
        }
        kpe_analyzer_->ExtractSymbols();
//       id_path_ = prefix_path + "/id";
//       FSUtil::createDir(id_path_);
//       doIDManagerInit_();

        /** TG */
        if ( mining_schema_.tg_enable )
        {
            tg_path_ = prefix_path + "/tg";
            tg_label_path_ = tg_path_ + "/label";
            tg_label_sim_path_ = tg_path_+"/sim";
            tg_label_sim_table_path_ = tg_path_+"/sim_table";
            FSUtil::createDir(tg_path_);
            FSUtil::createDir(tg_label_path_);
            FSUtil::createDir(tg_label_sim_path_);

            labelManager_.reset(new LabelManager(tg_path_ + "/label"));
            labelManager_->open();

            tgManager_.reset(new TaxonomyGenerationSubManager(miningConfig_.taxonomy_param,labelManager_, analyzer_));

            label_sim_table_.reset(new LabelSimilarity::SimTableType(tg_label_sim_table_path_));
            if (!label_sim_table_->Open())
            {
                std::cerr<<"open label sim table failed"<<std::endl;
            }
        }

        /** Recommend */
        qr_path_ = queryDataPath_;
        FSUtil::createDir(qr_path_);
        ///TODO Yingfeng
        uint32_t logdays = 7;//SF1Config::get()->getQuerySupportConfig().log_days;
        rmDb_.reset(new RecommendManager(qr_path_, collectionName_,miningConfig_.recommend_param,
                                         mining_schema_, document_manager_, labelManager_, analyzer_, logdays));

        /** log manager */
        MiningQueryLogHandler* handler = MiningQueryLogHandler::getInstance();
        handler->addCollection(collectionName_, rmDb_);

        qrManager_.reset(new QueryRecommendSubmanager(rmDb_));

        /** DUPD */
        if ( mining_schema_.dupd_enable )
        {
            dupd_path_ = prefix_path + "/dupd/";
            FSUtil::createDir(dupd_path_);
            dupManager_.reset(new DupDType(dupd_path_, document_manager_, mining_schema_.dupd_properties, analyzer_));
            if (!dupManager_->Open())
            {
                std::cerr<<"open DD failed"<<std::endl;
                return false;
            }
//             dupManager_.reset(new DupDType((dupd_path_).c_str()));
        }

        /** SIM */
        if ( mining_schema_.sim_enable )
        {
            sim_path_ = prefix_path + "/sim";
            FSUtil::createDir(sim_path_);
            std::string hdb_path = sim_path_ + "/hdb";
            boost::filesystem::create_directories(hdb_path);
            similarityIndex_.reset(new SimilarityIndex(sim_path_, hdb_path));
        }

        /** faceted */
        if ( mining_schema_.faceted_enable )
        {
            faceted_path_ = prefix_path + "/faceted";
            FSUtil::createDir(faceted_path_);
            std::string reader_dir = tg_label_path_+"/labeldistribute";
            LabelManager::LabelDistributeSSFType::ReaderType* reader = new LabelManager::LabelDistributeSSFType::ReaderType(reader_dir);
            faceted_.reset(new faceted::OntologyManager(faceted_path_,
                           tg_label_path_,
                           collectionName_,
                           schema_,
                           mining_schema_,
                           document_manager_,
                           analyzer_,
                           index_manager_,
                           labelManager_,
                           idManager_,
                           reader,
                           laManager_));
            //faceted_.reset(new faceted::OntologyManager(faceted_path_, document_manager_, mining_schema_.faceted_properties, analyzer_));
            faceted_->Open();

        }


        //do mining continue;
        try
        {
            std::string continue_file = collectionDataPath_+"/continue";
            if ( boost::filesystem::exists(continue_file) )
            {
                boost::filesystem::remove_all(continue_file);
                DoMiningCollection();
            }
        }
        catch (std::exception ex)
        {
            std::cerr<<ex.what()<<std::endl;
        }

    }
    catch (NotEnoughMemoryException& ex)
    {
        sflog->crit(SFL_MINE, ex.toString());
        return false;
    }
    catch (MiningConfigException& ex)
    {
        sflog->crit(SFL_MINE, ex.toString());
        return false;
    }
    catch (FileOperationException& ex)
    {
        sflog->crit(SFL_MINE, ex.toString());
        return false;
    }
    catch (ResourceNotFoundException& ex)
    {
        sflog->crit(SFL_MINE, ex.toString());
        return false;
    }
    catch (FileCorruptedException& ex)
    {
        sflog->crit(SFL_MINE, ex.toString());
        return false;
    }
    catch (boost::filesystem::filesystem_error& e)
    {
        sflog->crit(SFL_MINE,  e.what());
        return false;
    }
    catch (izenelib::ir::indexmanager::IndexManagerException& ex)
    {
        sflog->crit(SFL_MINE, ex.what());
        return false;
    }
    return true;
}

bool MiningManager::DoMiningCollection()
{
    //do TG
    if ( mining_schema_.tg_enable )
    {
        MEMLOG("[Mining] TG starting..");
        if (doTgInfoInit_())
        {
            izenelib::am::rde_hash<std::string, bool> tg_properties;
            for (uint32_t i=0;i<mining_schema_.tg_properties.size();i++)
            {
                tg_properties.insert( mining_schema_.tg_properties[i], 0);
            }
            uint32_t max_docid = tgInfo_->GetMaxDocId();
            uint32_t dm_maxid = document_manager_->getMaxDocId();
            uint32_t process_count = 0;

            std::string rig_path = system_resource_path_+"/sim/rig";
            std::cout<<"rig path : "<<rig_path<<std::endl;
            LabelSimilarity label_sim(tg_label_sim_path_, rig_path, label_sim_table_);
            if (!label_sim.Open(dm_maxid))
            {
                std::cerr<<"open label sim failed : "<<dm_maxid<<std::endl;
                return false;
            }

            LabelManager::TaskFunctionType label_sim_task = boost::bind( &LabelSimilarity::Append, &label_sim, _1, _2, _3, _4, _5);
            labelManager_->AddTask(label_sim_task);

            Document doc;
            std::cout<<"Will processing from "<<max_docid+1<<" to "<<dm_maxid<<std::endl;
            for ( uint32_t docid = max_docid+1; docid<=dm_maxid; docid++)
            {
                bool b = document_manager_->getDocument(docid, doc);
                if (!b) continue;
                Document::property_iterator property_it = doc.propertyBegin();
                while (property_it != doc.propertyEnd() )
                {
                    if ( tg_properties.find( property_it->first)!= NULL)
                    {
                        const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
                        tgInfo_->addDocument(docid, content);
                    }
                    property_it++;
                }
                process_count++;
                if ( process_count %1000 == 0 )
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
            printSimilarLabelResult_(1);
            printSimilarLabelResult_(2);
        }
        labelManager_->ClearAllTask();
        MEMLOG("[Mining] TG finished.");
        MiningQueryLogHandler::getInstance()->runEvents();
    }
    //do DUPD
    if ( mining_schema_.dupd_enable)
    {
        MEMLOG("[Mining] DUPD starting..");
        dupManager_->ProcessCollection();
        MEMLOG("[Mining] DUPD finished.");
    }
    if ( mining_schema_.faceted_enable )
    {
        faceted_->ProcessCollection(false);
    }

    //do Similarity
    if ( mining_schema_.sim_enable )
    {
        MEMLOG("[Mining] SIM starting..");
        computeSimilarity_(index_manager_->getIndexReader(), mining_schema_.sim_properties);
        MEMLOG("[Mining] SIM finished.");
    }
    return true;
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
        START_PROFILER( qr);
        if (true)
        {
            izenelib::util::ClockTimer clocker;
            addQrResult_(miaInput);
            std::cout<<"QR ALL COST: "<<clocker.elapsed()<<" seconds."<<std::endl;
        }
        STOP_PROFILER(qr);

        START_PROFILER( tg);
        if (mining_schema_.tg_enable)
        {
            izenelib::util::ClockTimer clocker;
            addTgResult_(miaInput);
            std::cout<<"TG ALL COST: "<<clocker.elapsed()<<" seconds."<<std::endl;
        }
        STOP_PROFILER(tg);

        START_PROFILER( dupd);
        //get dupd result
        if (mining_schema_.dupd_enable)
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
        sflog->error(SFL_MINE, ex.toString());
        bResult = false;
    }
    catch (MiningConfigException& ex)
    {
        sflog->error(SFL_MINE, ex.toString());
        bResult = false;
    }
    catch (FileOperationException& ex)
    {
        sflog->error(SFL_MINE, ex.toString());
        bResult = false;
    }
    catch (ResourceNotFoundException& ex)
    {
        sflog->error(SFL_MINE, ex.toString());
        bResult = false;
    }
    catch (FileCorruptedException& ex)
    {
        sflog->error(SFL_MINE, ex.toString());
        bResult = false;
    }
    catch (boost::filesystem::filesystem_error& e)
    {
        sflog->error(SFL_MINE, e.what());
        bResult = false;
    }
    catch (izenelib::ir::indexmanager::IndexManagerException& ex)
    {
        sflog->error(SFL_MINE, ex.what());
        bResult = false;
    }
    catch (std::exception& ex)
    {
        sflog->error(SFL_MINE, ex.what());
        bResult = false;
    }
    //get query recommend result
    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.MiningManager" )
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
            sflog->error(SFL_MINE, ex.toString());
            bResult = false;
        }
        catch (MiningConfigException& ex)
        {
            sflog->error(SFL_MINE, ex.toString());
            bResult = false;
        }
        catch (FileOperationException& ex)
        {
            sflog->error(SFL_MINE, ex.toString());
            bResult = false;
        }
        catch (ResourceNotFoundException& ex)
        {
            sflog->error(SFL_MINE, ex.toString());
            bResult = false;
        }
        catch (FileCorruptedException& ex)
        {
            sflog->error(SFL_MINE, ex.toString());
            bResult = false;
        }
        catch (boost::filesystem::filesystem_error& e)
        {
            sflog->error(SFL_MINE, e.what());
            bResult = false;
        }
        catch (izenelib::ir::indexmanager::IndexManagerException& ex)
        {
            sflog->error(SFL_MINE, ex.what());
            bResult = false;
        }
        catch (std::exception& ex)
        {
            sflog->error(SFL_MINE, ex.what());
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
//     if( status_ )
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
//     // 	struct timezone tz;
//     gettimeofday(&tv_start, NULL);
//     boost::timer timer;
//     bool ret = tgManager_->getQuerySpecificTaxonomyInfo(docIdList,
//                queryStr, totalCount, numberFromSia, taxonomyRep, neList);
//     double cpu_time = timer.elapsed();
//     gettimeofday(&tv_end, NULL);
//     double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
//             + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
//     // 	time_t second = end - start;
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
    if ( qrManager_ )
    {
        bool r = qrManager_->getReminderQuery(popularQueries,realtimeQueries);
        return r;
    }
    return false;
}

bool MiningManager::getUniqueDocIdList(const std::vector<uint32_t>& docIdList,
                                       std::vector<uint32_t>& cleanDocs)
{
    if ( !mining_schema_.dupd_enable || !dupManager_)
    {
        return false;
    }
    return dupManager_->getUniqueDocIdList(docIdList, cleanDocs);

}

bool MiningManager::getDuplicateDocIdList(uint32_t docId, std::vector<
        uint32_t>& docIdList)
{
    if ( dupManager_ )
    {
        return dupManager_->getDuplicatedDocIdList(docId, docIdList);
    }
    return false;
}




bool MiningManager::computeSimilarity_(izenelib::ir::indexmanager::IndexReader* pIndexReader, const std::vector<std::string>& property_names)
{
//   if(pIndexReader->maxDoc()> similarityIndex_->GetMaxDocId() )
    if (true)
    {
        std::cout << "Start to compute similarity index, please wait..."<< std::endl;
        // we should be able get information from index manager by property name only
        std::vector<uint32_t> property_ids(property_names.size() );
        PropertyConfigBase property_config;
        for (uint32_t i=0;i<property_ids.size();i++)
        {
            PropertyConfigBase byName;
            byName.propertyName_ = property_names[i];
            schema_type::const_iterator it(schema_.find(byName));
            if (it != schema_.end())
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

bool MiningManager::getSimilarDocIdList(uint32_t documentId, uint32_t maxNum,
                                        std::vector<std::pair<uint32_t, float> >& result)
{
    if (!similarityIndex_)
    {
        // TODO display warning message
        cerr << "[MiningManager] Warning - similarity index is not executed"
             << endl;
        return false;
    }

    result.clear();
    similarityIndex_->getSimilarDocIdScoreList(documentId, maxNum,
            std::back_inserter(result));


    return true;
}
bool MiningManager::getSimilarLabelList(uint32_t label_id, std::vector<uint32_t>& sim_list)
{
    if (!label_sim_table_) return false;
    return label_sim_table_->Get(label_id, sim_list);

}

bool MiningManager::getSimilarLabelStringList(uint32_t label_id, std::vector<izenelib::util::UString>& sim_list)
{
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

void MiningManager::replicatingLabel_()
{
    RsyncInfo rinfo;
    struct passwd *pwd = getpwuid(getuid());
    rinfo.setUser(pwd->pw_name);

    rinfo.collectionName =collectionName_;
    rinfo.srcFilePath = boost::filesystem::system_complete(tg_label_path_ + "/" + "label.stream").file_string();
    rinfo.srcFileSize = boost::filesystem::file_size( rinfo.srcFilePath );
    //TODO to rsync the label.
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


    if ( !ret  ) return false;
    return true;
}

bool MiningManager::addTgResult_(KeywordSearchResult& miaInput)
{
    std::cout << "[MiningManager::getTGResult]" << std::endl;
    if ( miaInput.topKDocs_.size()==0 ) return true;
    uint32_t top_docs_count = miningConfig_.taxonomy_param.top_docnum;
    if (top_docs_count > miaInput.topKDocs_.size())
    {
        top_docs_count = miaInput.topKDocs_.size();
    }
    std::vector<docid_t> topDocIdList;
    topDocIdList.assign(miaInput.topKDocs_.begin(), miaInput.topKDocs_.begin()+top_docs_count);
    std::cout << "Use top " << top_docs_count <<" docs for TG, getting "<<miaInput.topKDocs_.size()<<std::endl;

    TaxonomyRep taxonomyRep;
    bool ret = tgManager_->getQuerySpecificTaxonomyInfo(topDocIdList,
               izenelib::util::UString(miaInput.rawQueryString_, miaInput.encodingType_), miaInput.totalCount_, miaInput.topKDocs_.size(), taxonomyRep, miaInput.neList_);
    if (!ret)
    {
        std::string msg = "error at getting taxonomy";
        if (taxonomyRep.error_.length() > 0)
        {
            msg = taxonomyRep.error_;
        }
        miaInput.error_ += "[TG: "+msg+"]";
        sflog->error(SFL_MINE, msg.c_str());
    }
    else
    {
        taxonomyRep.fill(miaInput);
//       {
//         //debug output
//         for(uint32_t i=0;i<taxonomyRep.result_.size();i++)
//         {
//             std::string str;
//             taxonomyRep.result_[i]->name_.convertString(str, izenelib::util::UString::UTF_8);
//             std::cout<<"Top Cluster: "<<str<<std::endl;
//         }
//       }

    }

    return ret;

}

bool MiningManager::addDupResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(dup_mtx_);
    if ( !dupManager_ ) return true;
    miaInput.numberOfDuplicatedDocs_.resize(miaInput.topKDocs_.size());
    std::vector<count_t>::iterator result =
        miaInput.numberOfDuplicatedDocs_.begin();

    std::vector<docid_t> tmpDupDocs;
    typedef std::vector<docid_t>::const_iterator iterator;
    for (iterator i = miaInput.topKDocs_.begin(); i != miaInput.topKDocs_.end(); ++i)
    {
        tmpDupDocs.clear(); // use resize to reserve the memory
        dupManager_->getDuplicatedDocIdList(*i, tmpDupDocs);
        *result++ = tmpDupDocs.size();
    }

    return true;

}

bool MiningManager::addSimilarityResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(sim_mtx_);
    if ( !similarityIndex_ ) return true;
    miaInput.numberOfSimilarDocs_.resize(miaInput.topKDocs_.size());
    std::vector<count_t>::iterator result =
        miaInput.numberOfSimilarDocs_.begin();
    typedef std::vector<docid_t>::const_iterator iterator;

    for (iterator i = miaInput.topKDocs_.begin(); i != miaInput.topKDocs_.end(); ++i)
    {
        //		std::vector<std::pair<uint32_t, float> > simResult;
        //		simResult.resize(0);
        //		getSimilarDocIdList(*i, 30, simResult);
        *result++ = similarityIndex_->getSimilarDocNum(*i);
        //		*result++ =simResult.size();
    }

    return true;
}

bool MiningManager::addFacetedResult_(KeywordSearchResult& miaInput)
{
//    boost::mutex::scoped_lock lock(sim_mtx_);
    if ( !faceted_ ) return true;
    faceted_->GetSearcher()->GetRepresentation(miaInput.topKDocs_, miaInput.onto_rep_);

    return true;
}

// bool MiningManager::addDcResult_(KeywordSearchResult& miaInput)
// {
//     if( !dcSubManager_ ) return true;
//     std::cout<<"Start to add DC result"<<std::endl;
//     miaInput.docCategories_.resize(miaInput.topKDocs_.size());
//     std::vector< std::vector<izenelib::util::UString> >::iterator result =
//             miaInput.docCategories_.begin();
//     typedef std::vector<docid_t>::const_iterator iterator;
//
//     std::vector<izenelib::util::UString> catList;
//     for (iterator i = miaInput.topKDocs_.begin(); i != miaInput.topKDocs_.end(); ++i)
//     {
//         dcSubManager_->getCategoryResult(*i, catList);
// //         for(unsigned int j=0;j<catList.size();j++)
// //         {
// //             catList[j].displayStringValue(izenelib::util::UString::UTF_8);
// //         }
// //         std::cout<<::std::endl;
//         *result++ = catList;
//         catList.clear();
//     }
//
//     return true;
// }


// void MiningManager::doIDManagerInit_()
// {
//     if(idManager_ == NULL)
//     {
//         std::string termid_path = id_path_+"/termid";
//         boost::filesystem::create_directories(termid_path);
//         idManager_ = new MiningIDManager(termid_path, laManager_);
//
//     }
// }
//
// void MiningManager::doIDManagerRelease_()
// {
//     if(idManager_ != NULL)
//     {
//         delete idManager_;
//         idManager_ = NULL;
//     }
// }



bool MiningManager::doTgInfoInit_()
{
    if ( tgInfo_ == NULL)
    {
        tgInfo_ = new TaxonomyInfo(tg_path_,miningConfig_.taxonomy_param, labelManager_, kpe_analyzer_, system_resource_path_);
        tgInfo_->setKPECacheSize(1000000);
        if ( !tgInfo_->Open())
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
    if ( tgInfo_ != NULL)
    {
        delete tgInfo_;
        tgInfo_ = NULL;
    }
}

