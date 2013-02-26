#include "IncrementalFuzzyManager.hpp"

#include <common/JobScheduler.h>
#include <common/Utilities.h>

#include <la-manager/LAManager.h>//
#include <document-manager/Document.h>

#include <document-manager/DocumentManager.h>
#include <ir/index_manager/index/LAInput.h>
#include <icma/icma.h>
#include <fstream>

using namespace cma;
namespace sf1r
{
    IncrementalFuzzyManager::IncrementalFuzzyManager(const std::string& path,
                                        const std::string& tokenize_path,
                                        const std::string& property,
                                        boost::shared_ptr<DocumentManager>& document_manager,
                                        boost::shared_ptr<IDManager>& idManager,
                                        boost::shared_ptr<LAManager>& laManager,
                                        IndexBundleSchema& indexSchema,
                                        faceted::GroupManager* groupmanager,
                                        faceted::AttrManager* attrmanager,
                                        NumericPropertyTableBuilder* numeric_tablebuilder)
                                    : document_manager_(document_manager)
                                    , idManager_(idManager)
                                    , laManager_(laManager)
                                    , indexSchema_(indexSchema)
                                    , analyzer_(NULL)
                                    , knowledge_(NULL)
    {
        BarrelNum_ = 0;
        last_docid_ = 0;
        IndexedDocNum_ = 0;
        pMainFuzzyIndexBarrel_ =  NULL;
        property_ = property;
        isInitIndex_ = false;
        isMergingIndex_ = false;
        isAddingIndex_ = false;
        index_path_ = path;
        tokenize_path_ = tokenize_path;
        buildTokenizeDic();

        filter_manager_.reset(new FilterManager(groupmanager, index_path_,
            attrmanager, numeric_tablebuilder));
    }

    void IncrementalFuzzyManager::buildTokenizeDic()
    {
        std::string cma_path;
        LAPool::getInstance()->get_cma_path(cma_path);
        boost::filesystem::path cma_fmindex_dic(cma_path);
        cma_fmindex_dic /= boost::filesystem::path(tokenize_path_);

        knowledge_ = CMA_Factory::instance()->createKnowledge();
        knowledge_->loadModel("utf8", cma_fmindex_dic.c_str(), false);
        analyzer_ = CMA_Factory::instance()->createAnalyzer();
        analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
        // using the maxprefix analyzer
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
        analyzer_->setKnowledge(knowledge_);
    }

    IncrementalFuzzyManager::~IncrementalFuzzyManager()
    {
        if (pMainFuzzyIndexBarrel_ != NULL)
        {
            delete pMainFuzzyIndexBarrel_;
            pMainFuzzyIndexBarrel_ = NULL;
        }
        if (analyzer_)
        {
            //delete analyzer_;
            analyzer_ = NULL;
        }
        if (knowledge_)
        {
            //delete knowledge_;
            analyzer_ = NULL;
        }
    }

    void IncrementalFuzzyManager::Init()
    {
        string path = index_path_ + "/Main";
        pMainFuzzyIndexBarrel_ = new IncrementalFuzzyIndex(
                                                path,
                                                idManager_,
                                                laManager_,
                                                indexSchema_,
                                                MAX_INCREMENT_DOC,
                                                analyzer_
                                                );
        BarrelNum_++;
        pMainFuzzyIndexBarrel_->init(index_path_);
        
        std::string pathMainInv = index_path_ + "/Main.inv.idx";
        std::string pathMainFd = index_path_ + "/Main.fd.idx";
        std::string pathLastDocid = index_path_ + "/last.docid";

        if (bfs::exists(pathMainInv) && bfs::exists(pathMainFd))   //main ...
        {
            if (!pMainFuzzyIndexBarrel_->load_())
            {
                LOG(INFO) << "local index is not right, and clear ..."<<endl;
                delete_AllIndexFile();
            }
            else
                loadLastDocid(pathLastDocid);
        }
        else
            delete_AllIndexFile();
    }


    void IncrementalFuzzyManager::createIndex()
    {
        LOG(INFO) << "document_manager_->getMaxDocId()" << document_manager_->getMaxDocId();
        string name = "createIndex";
        task_type task = boost::bind(&IncrementalFuzzyManager::doCreateIndex, this);
        JobScheduler::get()->addTask(task, name);
    }

    //filter-group

    //filter-numric

    void IncrementalFuzzyManager::doCreateIndex()
    {
        // ismergeing...
        {
            ScopedWriteLock lock(mutex_);
            isInitIndex_ = true;
            LOG(INFO) << "Adding new documnents to index......";
            for (uint32_t i = last_docid_ + 1; i <= document_manager_->getMaxDocId(); i++)
            {
                if (i % 100000 == 0)
                {
                    LOG(INFO) << "inserted docs: " << i;
                }
                Document doc;
                document_manager_->getDocument(i, doc);
                Document::property_const_iterator it = doc.findProperty(property_);


                if (it == doc.propertyEnd())
                {
                    last_docid_++;
                    continue;
                }
                const izenelib::util::UString& text = it->second.get<UString>();//text.length();
                std::string textStr;
                text.convertString(textStr, izenelib::util::UString::UTF_8);
                if (!indexForDoc(i, textStr))
                {
                    LOG(INFO) << "Add index error";
                    return;
                }
                last_docid_++;

                // filter....
                // add filter...
            }
            saveLastDocid();
            save();
            izenelib::util::ClockTimer timer;
            LOG(INFO) << "Begin prepare_index_.....";
            prepare_index();
            isInitIndex_ = false;
            LOG(INFO) << "Prepare_index_ total elapsed:" << timer.elapsed() << " seconds";
        }
        pMainFuzzyIndexBarrel_->print();
    }
    
    bool IncrementalFuzzyManager::indexForDoc(uint32_t& docId, std::string propertyString)
    {
        if (IndexedDocNum_ >= MAX_INCREMENT_DOC)
            return false;
        {
            {
                if (pMainFuzzyIndexBarrel_ != NULL)
                {
                    pMainFuzzyIndexBarrel_->setStatus();
                    if (!pMainFuzzyIndexBarrel_->buildIndex_(docId, propertyString))
                        return false;
                }
                IndexedDocNum_++;
            }
        }
        return true;
    }

    void IncrementalFuzzyManager::prepare_index()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->prepare_index_();
        }
    }

    bool IncrementalFuzzyManager::fuzzySearch(
                        const std::string& query
                        , std::vector<uint32_t>& resultList
                        , std::vector<double> &ResultListSimilarity
                        //, const SearchingMode::SuffixMatchFilterMode& filter_mode
                        //, const std::vector<QueryFiltering::FilteringType>& filter_param
                        , const faceted::GroupParam& group_param)
    {
        cout<<"begin fuzzySearch..."<<endl;
        izenelib::util::ClockTimer timer;
        if (BarrelNum_ == 0 || isMergingIndex_ || isInitIndex_)
        {
            return true;
        }
        else
        {
            izenelib::util::UString utext(query, izenelib::util::UString::UTF_8);
            AnalysisInfo analysisInfo;
            analysisInfo.analyzerId_ = "la_unigram";
            LAInput laInput;
            laInput.setDocId(0);
            if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
                return false;
            std::set<termid_t> setDocId;
            std::vector<termid_t> termidList;

            for (LAInput::const_iterator it = laInput.begin(); it != laInput.end(); ++it)
            {
                setDocId.insert(it->termid_);
            }
            for (std::set<termid_t>::iterator i = setDocId.begin(); i != setDocId.end(); ++i)
            {
                termidList.push_back(*i);
            }

            std::vector<FilterManager::FilterIdRange> filter_range_list;
            std::vector<size_t> prop_id_list;
            getAllFilterRangeFromGroupLable(group_param, prop_id_list, filter_range_list);
            std::vector<uint32_t> docidlist;
            getAllFilterDocid(prop_id_list, filter_range_list, docidlist);

            if (pMainFuzzyIndexBarrel_ != NULL && isInitIndex_ == false)
            {
                {
                    ScopedReadLock lock(mutex_);
                    uint32_t hitdoc;
                    pMainFuzzyIndexBarrel_->getFuzzyResult_(termidList, resultList, ResultListSimilarity, hitdoc);
                    pMainFuzzyIndexBarrel_->score(query, resultList, ResultListSimilarity);
                }
            }

            //merge ,,,,...
            LOG(INFO) << "INC Search ResulList Number:" << resultList.size();
            LOG(INFO) << "INC Search Time Cost: " << timer.elapsed() << " seconds";
            return true;
        }
    }


    //bool IncrementalFuzzyManager::cron-job...
    /*
    {
        main.index .reset....
        filter..reset...

        wait.. .empty.
    }
    */

    bool IncrementalFuzzyManager::exactSearch(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
    {
        if (BarrelNum_ == 0)
        {
            LOG(INFO) << "[NOTICE]:THERE IS NOT Berral";
        }
        else
        {
            if (isMergingIndex_)
            {
                LOG(INFO) << "Merging Index...";
                return true;
            }
            izenelib::util::UString utext(query, izenelib::util::UString::UTF_8);
            AnalysisInfo analysisInfo;
            analysisInfo.analyzerId_ = "la_unigram";
            LAInput laInput;
            laInput.setDocId(0);
            if (laManager_)
            {
                if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
                    return false;
            }
            else
                return false;

            std::set<termid_t> setDocId;
            std::vector<termid_t> termidList;

            for (LAInput::const_iterator it = laInput.begin(); it != laInput.end(); ++it)
            {
                setDocId.insert(it->termid_);
            }
            for (std::set<termid_t>::iterator i = setDocId.begin(); i != setDocId.end(); ++i)
            {
                termidList.push_back(*i);
            }


            if (pMainFuzzyIndexBarrel_ != NULL && isInitIndex_ == false)
            {
                {
                    ScopedReadLock lock(mutex_);
                    pMainFuzzyIndexBarrel_->getExactResult_(termidList, resultList, ResultListSimilarity);
                }
            }
            LOG(INFO) << "search ResulList number: " << resultList.size();
        }
        return true;
    }

    void IncrementalFuzzyManager::setLastDocid(uint32_t last_docid)
    {
        last_docid_ = last_docid;
        saveLastDocid();
    }

    void IncrementalFuzzyManager::getLastDocid(uint32_t& last_docid)
    {
        last_docid = last_docid_;
    }

    void IncrementalFuzzyManager::getDocNum(uint32_t& docNum)
    {
        docNum = IndexedDocNum_;
    }

    void IncrementalFuzzyManager::getMaxNum(uint32_t& maxNum)
    {
        maxNum = MAX_INCREMENT_DOC;
    }

    void IncrementalFuzzyManager::reset()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->reset();
            delete pMainFuzzyIndexBarrel_;
            pMainFuzzyIndexBarrel_ = NULL;
            BarrelNum_ = 0;
            IndexedDocNum_ = 0;
        }
    }

    void IncrementalFuzzyManager::save()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->save_();
        }
    }

    bool IncrementalFuzzyManager::saveLastDocid(std::string path)
    {
        string docid_path = index_path_ + "/last.docid" + path;
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file"<<endl;
            return false;
        }
        fwrite(&last_docid_, sizeof(last_docid_), 1, file);
        fwrite(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file);
        fclose(file);
        return true;
    }

    bool IncrementalFuzzyManager::loadLastDocid(std::string path)
    {
        string docid_path = index_path_ + "/last.docid" + path;
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) << "Cannot open input file"<<endl;
            return false;
        }
        if (1 != fread(&last_docid_, sizeof(last_docid_), 1, file) ) return false;
        if (1 != fread(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file) ) return false;
        fclose(file);
        return true;
    }

    void IncrementalFuzzyManager::delete_AllIndexFile()
    {
        bfs::path pathMainInc = index_path_ + "/Main.inv.idx";
        bfs::path pathMainFd = index_path_ + "/Main.fd.idx";

        ///xxx need try...
        bfs::remove(pathMainInc);
        bfs::remove(pathMainFd);
    }

    bool IncrementalFuzzyManager::getAllFilterRangeFromGroupLable(
                                    const faceted::GroupParam& group_param,
                                    std::vector<size_t>& prop_id_list,
                                    std::vector<FilterManager::FilterIdRange>& filter_range_list) const
    {
         if (!filter_manager_)
        {
            LOG(INFO) << "no filter support.";
            return false;
        }
        FilterManager::FilterIdRange filterid_range;
        
        for (GroupParam::GroupLabelMap::const_iterator cit = group_param.groupLabels_.begin();
            cit != group_param.groupLabels_.end(); ++cit)
        {
            bool is_numeric = filter_manager_->isNumericProp(cit->first);
            bool is_date = filter_manager_->isDateProp(cit->first);

            const GroupParam::GroupPathVec& group_values = cit->second;
            size_t prop_id = filter_manager_->getPropertyId(cit->first);

            if (prop_id == (size_t)-1) continue;

            for (size_t i = 0; i < group_values.size(); ++i)
            {
                if (is_numeric)
                {

                }
                else if (is_date)
                {
                   
                }
                else
                {
                    const UString& group_filterstr = filter_manager_->formatGroupPath(group_values[i]);
                    string group_filterstrs;
                    group_filterstr.convertString(group_filterstrs, UString::UTF_8);
                    cout << group_filterstrs << endl;
                    filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, group_filterstr);
                }
            }

            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "one of group label filter id range not found.";
                continue;
            }

            prop_id_list.push_back(prop_id);
            filter_range_list.push_back(filterid_range);
        }
        return true;
    }

    void IncrementalFuzzyManager::getAllFilterDocid(
                            std::vector<size_t>& prop_id_list,
                            std::vector<FilterManager::FilterIdRange>& filter_range_list,
                            std::vector<uint32_t>& filterDocidList)
    {

        std::vector<std::vector<FilterManager::FilterDocListT> >& filter_list = filter_manager_->getFilterList();
        int k = 0;
        for (std::vector<FilterManager::FilterIdRange>::iterator i = filter_range_list.begin(); i != filter_range_list.end(); ++i, ++k)
        {
            for (unsigned int j = filter_range_list[k].start; j < filter_range_list[k].end; ++j)
            {
                for(unsigned int x = 0; x < filter_list[k][j].size(); ++x)
                    cout << filter_list[k][j][x] << ",";
                cout<<endl;
            }
            cout << "===" << endl;
        }
    }
}
