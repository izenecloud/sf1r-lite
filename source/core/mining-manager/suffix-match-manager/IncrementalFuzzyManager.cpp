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
                                        const vector<std::string> properties,
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
        start_docid_ = 0;
        IndexedDocNum_ = 0;
        pMainFuzzyIndexBarrel_ =  NULL;
        properties_ = properties;
        isInitIndex_ = false;
        isMergingIndex_ = false;
        isAddingIndex_ = false;
        index_path_ = path + "/inc";
        tokenize_path_ = tokenize_path;
        buildTokenizeDic();
        if (!boost::filesystem::is_directory(index_path_))
        {
            boost::filesystem::create_directories(index_path_);
        }
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
            knowledge_ = NULL;
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
        std::string pathLastDocid = index_path_ + "/inc.docid";

        if (bfs::exists(pathMainInv) && bfs::exists(pathMainFd))
        {
            if (!pMainFuzzyIndexBarrel_->load_())
            {
                LOG(INFO) << "local index is not right, and clear ..."<<endl;
                delete_AllIndexFile();
            }
            else
                loadDocid(pathLastDocid);
        }
        else
            delete_AllIndexFile();

        if (IndexedDocNum_ == 0)
        {
            start_docid_ = last_docid_ = document_manager_->getMaxDocId();
        }

        LOG (INFO) << "Load incremental fuzzy index ... ";
        pMainFuzzyIndexBarrel_->print();
    }

    void IncrementalFuzzyManager::createIndex()//cancle...
    {
        LOG(INFO) << "document_manager_->getMaxDocId()" << document_manager_->getMaxDocId();
        string name = "createIndex";
        task_type task = boost::bind(&IncrementalFuzzyManager::doCreateIndex, this);
        JobScheduler::get()->addTask(task, name);
    }

    void IncrementalFuzzyManager::doCreateIndex()
    {
        {
            ScopedWriteLock lock(mutex_);
            isInitIndex_ = true;
            bool isIncre = true;
            //build filter just for all new documents...

            filter_manager_->buildFilters(start_docid_, document_manager_->getMaxDocId(), isIncre); // 
            LOG(INFO) << "last_docid_" << last_docid_ << endl;
            for (uint32_t i = last_docid_ + 1; i <= document_manager_->getMaxDocId(); i++)
            {
                if (i % 100000 == 0)
                {
                    LOG(INFO) << "inserted docs: " << i;
                }
                Document doc;
                document_manager_->getDocument(i, doc);
                Document::property_const_iterator it = doc.findProperty(properties_[0]);

                if (it == doc.propertyEnd())
                {
                    last_docid_++;
                    continue;
                }

                filter_manager_->buildStringFiltersForDoc(i, doc);

                const izenelib::util::UString& text = it->second.get<UString>();
                std::string textStr;
                text.convertString(textStr, izenelib::util::UString::UTF_8);
                if (!indexForDoc(i, textStr))
                {
                    LOG(INFO) << "Add index error";
                    return;
                }
                last_docid_++;
            }
            filter_manager_->saveFilterId();
            filter_manager_->saveFilterList();
            
            saveDocid();
            save();
            izenelib::util::ClockTimer timer;
            prepare_index();
            isInitIndex_ = false;
            LOG(INFO) << "Prepare_index_ total elapsed:" << timer.elapsed() << " seconds";
        }
        pMainFuzzyIndexBarrel_->print();
        printFilter();
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
                        , std::vector<double>& ResultListSimilarity
                        //, const SearchingMode::SuffixMatchFilterMode& filter_mode
                        , const std::vector<QueryFiltering::FilteringType>& filter_param
                        , const faceted::GroupParam& group_param)
    {
        std::vector<uint32_t> resultList_;
        std::vector<double> ResultListSimilarity_;
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
            
            {// lock...
                if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
                return false;
            }

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

            std::vector<std::vector<size_t> > prop_id_lists;
            std::vector<std::vector<FilterManager::FilterIdRange> > filter_range_lists;
            std::vector<size_t> prop_id_list_1;
            std::vector<FilterManager::FilterIdRange> filter_range_list_1;
            if (!group_param.isGroupEmpty())
            {
                if (!getAllFilterRangeFromGroupLable(group_param, prop_id_list_1, filter_range_list_1))
                    return false;
                LOG(INFO)<<"finish getAllFilterRangeFromGroupLable" << filter_range_list_1.size()<<endl;
            }
            if (filter_range_list_1.size() > 0)
            {
                prop_id_lists.push_back(prop_id_list_1);
                filter_range_lists.push_back(filter_range_list_1);
            }
            

            std::vector<size_t> prop_id_list_2;
            std::vector<FilterManager::FilterIdRange> filter_range_list_2;
            if (!group_param.isAttrEmpty())
            {
                if (!getAllFilterRangeFromAttrLable(group_param, prop_id_list_2, filter_range_list_2))
                    return false;
                LOG(INFO)<<"finish getAllFilterRangeFromAttrLable" << prop_id_list_2.size() <<endl;
            }
            if (filter_range_list_2.size() > 0)
            {
                prop_id_lists.push_back(prop_id_list_2);
                filter_range_lists.push_back(filter_range_list_2);
            }

            std::vector<size_t> prop_id_list_3;
            std::vector<FilterManager::FilterIdRange> filter_range_list_3;
            if (!filter_param.empty())
            {
                if (!getAllFilterRangeFromFilterParam(filter_param, prop_id_list_3, filter_range_list_3))
                    return false;
                LOG(INFO)<<"finish getAllFilterRangeFromFilterParam" << prop_id_list_3.size() <<endl;
            }
            if (filter_range_list_3.size() > 0)
            {
                prop_id_lists.push_back(prop_id_list_3);
                filter_range_lists.push_back(filter_range_list_3);
            }

            bool needFilter = false;
            std::vector<uint32_t> docidlist;

            //get filter docidList ...
            LOG (INFO) << "get filter docidList ...";
            if (filter_range_lists.size() > 0)
            {
                needFilter = true; 
                getAllFilterDocid(prop_id_lists, filter_range_lists, docidlist);
            }

            //get raw result 
            LOG (INFO) << "get raw result ...";
            if (pMainFuzzyIndexBarrel_ != NULL && isInitIndex_ == false)
            {
                {
                    ScopedReadLock lock(mutex_);
                    uint32_t hitdoc;
                    pMainFuzzyIndexBarrel_->getFuzzyResult_(termidList, resultList_, ResultListSimilarity_, hitdoc);
                    pMainFuzzyIndexBarrel_->score(query, resultList_, ResultListSimilarity_);
                }
            }
            cout << "begin filter ..., raw result size is " << resultList_.size() << endl;

            //get final result by Filter;
            LOG (INFO) << "get final result by Filter" << endl;
            if (needFilter)
            {
                getResultByFilterDocid(
                docidlist
                , resultList_
                , ResultListSimilarity_
                , resultList
                , ResultListSimilarity);
            }
            else
            {
                resultList = resultList_;
                ResultListSimilarity = ResultListSimilarity_;
            }
            
            //LOG(INFO) << "INC Search ResulList Number:" << resultList.size();
            //LOG(INFO) << "INC Search Time Cost: " << timer.elapsed() << " seconds";
            return true;
        }
    }

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
        saveDocid();
    }

    void IncrementalFuzzyManager::getLastDocid(uint32_t& last_docid)
    {
        last_docid = last_docid_;
    }

    void IncrementalFuzzyManager::setStartDocid(uint32_t start_docid)
    {
        if (start_docid_ == 0)
        {
            start_docid_ = start_docid;
            saveDocid();
        }
    }

    void IncrementalFuzzyManager::getStartDocid(uint32_t& last_docid)
    {
        last_docid = start_docid_;
    }

    void IncrementalFuzzyManager::getDocNum(uint32_t& docNum)
    {
        docNum = IndexedDocNum_;
    }

    void IncrementalFuzzyManager::getMaxNum(uint32_t& maxNum)
    {
        maxNum = MAX_INCREMENT_DOC;
    }

    void IncrementalFuzzyManager::reset()   // use lock..... // this is not reset, but this not reset, but like delete;
    {
        if (pMainFuzzyIndexBarrel_)
        {
            //delete;
            pMainFuzzyIndexBarrel_->reset();
            delete pMainFuzzyIndexBarrel_;
            pMainFuzzyIndexBarrel_ = NULL;
            BarrelNum_ = 0;
            IndexedDocNum_ = 0;
            last_docid_ = 0;
            start_docid_ = 0;

            resetFilterManager();
            delete_AllIndexFile();
            //restart 
            Init();
        }
    }

    bool IncrementalFuzzyManager::resetFilterManager()
    {
        filter_manager_->clearFilterList();
        filter_manager_->clearFilterId();
        
        filter_manager_->generatePropertyId();
        return true;
    }

    void IncrementalFuzzyManager::save()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->save_();
        }
    }

    bool IncrementalFuzzyManager::saveDocid(std::string path)
    {
        string docid_path = index_path_ + "/inc.docid";
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "wb")) == NULL)
        {
            LOG(INFO) << "Cannot open output file"<<endl;
            return false;
        }
        fwrite(&last_docid_, sizeof(last_docid_), 1, file);
        fwrite(&start_docid_, sizeof(start_docid_), 1, file);
        fwrite(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file);
        fclose(file);
        return true;
    }

    bool IncrementalFuzzyManager::loadDocid(std::string path)
    {
        string docid_path = index_path_ + "/inc.docid";
        FILE* file;
        if ((file = fopen(docid_path.c_str(), "rb")) == NULL)
        {
            LOG(INFO) << "Cannot open input file"<<endl;
            return false;
        }
        if (1 != fread(&last_docid_, sizeof(last_docid_), 1, file) ) return false;
        if (1 != fread(&start_docid_, sizeof(start_docid_), 1, file) ) return false;
        if (1 != fread(&IndexedDocNum_, sizeof(IndexedDocNum_), 1, file) ) return false;
        fclose(file);
        return true;
    }

    void IncrementalFuzzyManager::delete_AllIndexFile()
    {
        bfs::path pathMainInc = index_path_ + "/Main.inv.idx";
        bfs::path pathMainFd = index_path_ + "/Main.fd.idx";
        bfs::path docid_path = index_path_ + "/inc.docid";

        ///xxx need try...
        bfs::remove(pathMainInc);
        bfs::remove(pathMainFd);
        bfs::remove(docid_path);
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
                    bool is_range = false;
                    if (!checkLabelParam_inc(*cit, is_range)) // only for range ...
                    {
                        continue;
                    }
                    if (is_range)
                    {
                        NumericRange range;
                        if (!convertRangeLabel_inc(group_values[i][0], range))
                            continue;
                        FilterManager::FilterIdRange tmp_range;
                        tmp_range = filter_manager_->getNumFilterIdRangeLess(prop_id, std::max(range.first, range.second), true);
                        filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, std::min(range.first, range.second), true);
                        filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                        filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                    }
                    else
                    {
                        float value = 0;
                        if (!convertNumericLabel_inc(group_values[i][0], value))
                            continue;
                        filterid_range = filter_manager_->getNumFilterIdRangeExact(prop_id, value);
                    }
                }
                else if (is_date)
                {
                    const std::string& propValue = group_values[i][0];
                    faceted::DateStrFormat dsf;
                    faceted::DateStrFormat::DateVec date_vec;
                    if (!dsf.apiDateStrToDateVec(propValue, date_vec))
                    {
                        LOG(WARNING) << "get date from datestr error. " << propValue;
                        continue;
                    }
                    double value = (double)dsf.createDate(date_vec);
                    LOG(INFO) << "date value is : " << value;
                    int date_index = date_vec.size() - 1;

                    if (date_index < faceted::DateStrFormat::DATE_INDEX_DAY)
                    {
                        faceted::DateStrFormat::DateVec end_date_vec = date_vec;
                        // convert to range.
                        if (date_index == faceted::DateStrFormat::DATE_INDEX_MONTH)
                        {
                            end_date_vec.push_back(31);
                        }
                        else if (date_index == faceted::DateStrFormat::DATE_INDEX_YEAR)
                        {
                            end_date_vec.push_back(12);
                            end_date_vec.push_back(31);
                        }
                        else
                        {
                            continue;
                        }
                        double end_value = (double)dsf.createDate(end_date_vec);
                        LOG(INFO) << "end date value is : " << end_value;
                        FilterManager::FilterIdRange tmp_range;
                        tmp_range = filter_manager_->getNumFilterIdRangeLess(prop_id, end_value, true);
                        filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, value, true);
                        filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                        filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                    }
                    else
                    {
                        filterid_range = filter_manager_->getNumFilterIdRangeExact(prop_id, value);
                    }
                }
                else
                {
                    const UString& group_filterstr = filter_manager_->formatGroupPath(group_values[i]);
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

    bool IncrementalFuzzyManager::getAllFilterRangeFromFilterParam(
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const
    {
        if(!filter_manager_)
        {
            LOG(INFO) << "no filter support.";
            return false;
        }

        FilterManager::FilterIdRange filterid_range;
        for(size_t i = 0; i < filter_param.size(); i++)
        {
            const QueryFiltering::FilteringType& filtertype = filter_param[i];

            bool is_numeric = filter_manager_->isNumericProp(filtertype.property_)
                || filter_manager_->isDateProp(filtertype.property_);
            size_t prop_id = filter_manager_->getPropertyId(filtertype.property_);
            if(prop_id == (size_t)-1) continue;
            for(size_t j = 0; j < filtertype.values_.size(); j++)
            {
                try
                {
                    if(is_numeric)
                    {
                        double filter_num = filtertype.values_[j].getNumber();
                        switch (filtertype.operation_)
                        {
                        case QueryFiltering::LESS_THAN_EQUAL:
                            filterid_range = filter_manager_->getNumFilterIdRangeLess(prop_id, filter_num, true);
                            break;
                        case QueryFiltering::LESS_THAN:
                            filterid_range = filter_manager_->getNumFilterIdRangeLess(prop_id, filter_num, false);
                            break;
                        case QueryFiltering::GREATER_THAN_EQUAL:
                            filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, filter_num, true);
                            break;
                        case QueryFiltering::GREATER_THAN:
                            filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, filter_num, false);;
                            break;
                        case QueryFiltering::RANGE:
                            {
                                assert(filtertype.valses_size() == 2);
                                if(j>=1)continue;
                                double filter_num_2 = filtertype.values_[1].getNumber();
                                FilterManager::FilterIdRange tmp_range;
                                tmp_range = filter_manager_ -> getNumFilterIdRangeLess(prop_id, std::max(filter_num, filter_num_2), true);
                                filterid_range = filter_manager_ -> getNumFilterIdRangeGreater(prop_id, std::min(filter_num, filter_num_2), true);
                                filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                                filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                                j++;
                            }
                            break;
                        case QueryFiltering::EQUAL:
                            filterid_range = filter_manager_ -> getNumFilterIdRangeExact(prop_id, filter_num);
                            break;
                        default:
                            LOG(WARNING) << "not support filter operation for numeric property in incremental fuzzy searching.";
                            continue;
                        }
                    }
                    else
                    {
                        const std::string& filter_str = filtertype.values_[j].get<std::string>();
                        LOG(INFO) << "filter range by : " << filter_str;
                        switch (filtertype.operation_)
                        {
                        case QueryFiltering::EQUAL:
                            filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, UString(filter_str, UString::UTF_8));
                            break;
                        case QueryFiltering::GREATER_THAN:
                            filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), false);
                            break;
                        case QueryFiltering::GREATER_THAN_EQUAL:
                            filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), true);
                            break;
                        case QueryFiltering::LESS_THAN:
                            filterid_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), false);
                            break;
                        case QueryFiltering::LESS_THAN_EQUAL:
                            filterid_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), true);
                            break;
                        case QueryFiltering::RANGE:
                            {
                                assert(filtertype.values_.size() == 2);
                                if (j >= 1) continue;
                                const std::string& filter_str1 = filtertype.values_[1].get<std::string>();
                                FilterManager::FilterIdRange tmp_range;
                                if (filter_str < filter_str1)
                                {
                                    tmp_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str1, UString::UTF_8), true);
                                    filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), true);
                                }
                                else
                                {
                                    tmp_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), true);
                                    filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str1, UString::UTF_8), true);
                                }
                                filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                                filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                            }
                            break;

                        case QueryFiltering::PREFIX:
                            filterid_range = filter_manager_->getStrFilterIdRangePrefix(prop_id, UString(filter_str, UString::UTF_8));
                            break;

                        default:
                            LOG(WARNING) << "not support filter operation for string property in fuzzy searching.";
                            continue;
                        }
                    }
                }
                catch (const boost::bad_get &)
                {
                    LOG(INFO) << "get filter string failed. boost::bad_get.";
                    continue;
                }

                if(filterid_range.start >= filterid_range.end)
                {
                    LOG(WARNING) << "filter id range not found. ";
                    continue;
                }
                prop_id_list.push_back(prop_id);
                filter_range_list.push_back(filterid_range);
            }
        }
        return true;
    }

    bool IncrementalFuzzyManager::getAllFilterRangeFromAttrLable(
        const GroupParam& group_param,
        std::vector<size_t>& prop_id_list,
        std::vector<FilterManager::FilterIdRange>& filter_range_list) const
    {
        if (!filter_manager_)
        {
            LOG(INFO) << "no filter support.";
            return false;
        }

        size_t prop_id = filter_manager_->getAttrPropertyId();
        if (prop_id == (size_t)-1) return true;

        FilterManager::FilterIdRange filterid_range;
        for (GroupParam::AttrLabelMap::const_iterator cit = group_param.attrLabels_.begin();
                cit != group_param.attrLabels_.end(); ++cit)
        {
            const std::vector<std::string>& attr_values = cit->second;
            for (size_t i = 0; i < attr_values.size(); ++i)
            {
                const UString& attr_filterstr = filter_manager_->formatAttrPath(UString(cit->first, UString::UTF_8),
                        UString(attr_values[i], UString::UTF_8));
                filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, attr_filterstr);
                if (filterid_range.start >= filterid_range.end)
                {
                    LOG(WARNING) << "attribute filter id range not found. " << attr_filterstr;
                    continue;
                }
            }
        }
/*
        if (!temp_range_list.empty())
        {
            prop_id_list.push_back(prop_id);
            filter_range_list.push_back(temp_range_list);
        }
*/
        return true;
    }

    void IncrementalFuzzyManager::printFilter()
    {
        std::vector<std::vector<FilterManager::FilterDocListT> >& filter_list = filter_manager_->getFilterList();

        int n = 0;
        for (std::vector<std::vector<FilterManager::FilterDocListT> >::iterator FilterDocListTs = filter_list.begin()
            ; FilterDocListTs != filter_list.end(); ++FilterDocListTs)
        {
            cout<<"============= For each property =============="<<endl;
            int k = 0;
            for (std::vector<FilterManager::FilterDocListT>::iterator i = FilterDocListTs->begin(); i != FilterDocListTs->end(); ++i, ++k)
            {
                cout<<"For each filter key in this property ... "<<endl;
                for(unsigned int x = 0; x < (*i).size(); ++x)
                {
                    //cout << (*i)[x] << ",";
                }
                cout<<endl;
            }
            n++;
        }
    }

    void IncrementalFuzzyManager::getAllFilterDocid(
                            vector<std::vector<size_t> >& prop_id_lists,
                            std::vector<std::vector<FilterManager::FilterIdRange> >& filter_range_lists,
                            std::vector<uint32_t>& filterDocidList)
    {
        std::vector<std::vector<uint32_t> > filter_doc_lists;
        filter_doc_lists.resize(prop_id_lists.size());

        std::vector<std::vector<FilterManager::FilterDocListT> >& filter_list = filter_manager_->getFilterList();

        int n = 0;
        for (std::vector<std::vector<FilterManager::FilterIdRange> >::iterator filter_range_list = filter_range_lists.begin()
            ; filter_range_list != filter_range_lists.end(); ++filter_range_list)
        {
            //cout<<"======"<<endl;
            int k = 0;
            std::vector<size_t>& prop_id_list = prop_id_lists[n];
            for (std::vector<FilterManager::FilterIdRange>::iterator i = filter_range_list->begin(); i != filter_range_list->end(); ++i, ++k)
            {   
                //cout<<"(*filter_range_list)[k].end" << (*filter_range_list)[k].end <<":" << (*filter_range_list)[k].start << endl;
                for (unsigned int j = (*filter_range_list)[k].start; j < (*filter_range_list)[k].end; ++j)
                {
                    for(unsigned int x = 0; x < filter_list[prop_id_list[k]][j].size(); ++x)
                    {
                        filter_doc_lists[n].push_back(filter_list[prop_id_list[k]][j][x]);
                        //cout<<filter_list[prop_id_lists[n][k]][j][x]<<",";
                    }
                }
            }
            //cout<<endl<<"-----"<<endl;
            sort(filter_doc_lists[n].begin(), filter_doc_lists[n].end());
            n++;
        }

        getFinallyFilterDocid(filter_doc_lists, filterDocidList);
    }

    void IncrementalFuzzyManager::getFinallyFilterDocid(std::vector<std::vector<uint32_t> >& filter_doc_lists
                            , std::vector<uint32_t>& filterDocidList )
    {
        if (filter_doc_lists.size() == 0)
        {
            return;
        }
        if (filter_doc_lists.size() == 1)
        {
            for (std::vector<uint32_t>::iterator i = filter_doc_lists[0].begin(); i != filter_doc_lists[0].end(); ++i)
            {
                filterDocidList.push_back(*i);
            }
            return ;
        }
        unsigned int min = filter_doc_lists[0].size();
        unsigned int label = 0;
        for (unsigned int i = 1; i < filter_doc_lists.size(); ++i)
        {
            //LOG (INFO) <<"Filter "<<i<<" size " << filter_doc_lists[i].size() << endl;
            if (min > filter_doc_lists[i].size())
            {
                min = filter_doc_lists[i].size();
                label = i;
            }
        }
        std::vector<int> num;
        num.resize(filter_doc_lists.size());
        for (std::vector<int>::iterator i = num.begin(); i != num.end(); ++i)
        {
            *i = 0;
        }
        for (unsigned int i = 0; i < filter_doc_lists[label].size(); ++i)
        {
            bool flag = true;
            uint32_t value = filter_doc_lists[label][i];
            
            for ( unsigned int j = 0; j < filter_doc_lists.size(); j++)
            {
                if (j == label)
                {
                    break;
                }
                unsigned int count = num[j];
                while (filter_doc_lists[j][count] < value)
                {
                    count++;
                    if (count == filter_doc_lists[j].size())
                    {
                        flag = false;
                        break;
                    }
                }
                num[j] = count;
                if (filter_doc_lists[j][count] != value)
                {
                    flag = false;
                    break;
                }
            }
            if (flag)
            {
                filterDocidList.push_back(value);
            }
        }

        LOG (INFO) <<"filter docid number:" << filterDocidList.size() << endl;
    }

    void IncrementalFuzzyManager::getResultByFilterDocid(
                        std::vector<uint32_t>& filterDocidList
                        , std::vector<uint32_t>& resultListRaw
                        , std::vector<double>& ResultListSimilarityRaw
                        , std::vector<uint32_t>& resultList
                        , std::vector<double>& ResultListSimilarity)
    {
        if (filterDocidList.size() == 0)
        {
            cout << "filterDocidList is empty..." << endl;
            return;
        }
        uint32_t j = 0;
        uint32_t docid = filterDocidList[j];
        //cout<<"docid"<<docid<<endl;
        std::vector<double>::iterator k = ResultListSimilarityRaw.begin();
        for (std::vector<uint32_t>::iterator i = resultListRaw.begin(); i != resultListRaw.end(); ++i, ++k)
        {
            while (*i > docid)
            {
                j++;
                if (j >= filterDocidList.size())
                {
                    return;
                }
                docid = filterDocidList[j];
            }

            if (*i == docid)
            {
                resultList.push_back(*i);
                ResultListSimilarity.push_back(*k);
            }
        }
        LOG (INFO) << "getResultByFilterDocid finish ... " << endl;

    }
}
