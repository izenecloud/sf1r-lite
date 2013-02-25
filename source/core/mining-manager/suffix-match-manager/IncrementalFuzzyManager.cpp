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

    void IncrementalFuzzyManager::prepare_index_()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->prepare_index_();
        }
        if (pTmpFuzzyIndexBarrel_)
        {
            pTmpFuzzyIndexBarrel_->prepare_index_();
        }
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

    void IncrementalFuzzyManager::save_()
    {
        if (pMainFuzzyIndexBarrel_)
        {
            pMainFuzzyIndexBarrel_->save_();
        }
    }

    bool IncrementalFuzzyManager::saveLastDocid_(std::string path)
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

    bool IncrementalFuzzyManager::loadLastDocid_(std::string path)
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

    void IncrementalFuzzyManager::startIncrementalManager()
    {
        bool flag = true;
        std::string pathMainInc = index_path_ + "/Main.inv.idx";
        std::string pathMainFd = index_path_ + "/Main.fd.idx";
        std::string pathTmpInc = index_path_ + "/Tmp.inv.idx";
        std::string pathTmpFd = index_path_ + "/Tmp.fd.idx";
        std::string pathLastDocid = index_path_ + "/last.docid";

        if (bfs::exists(pathMainInc) && bfs::exists(pathMainFd))//main
        {
            initMainFuzzyIndexBarrel_();
            if (!pMainFuzzyIndexBarrel_->load_())
            {
                LOG(INFO) << "Index Wrong!!"<<endl;
                delete_AllIndexFile();
                flag = false;
            }
            else
            {
                if (bfs::exists(pathTmpInc) && bfs::exists(pathTmpFd))//tmp
                {
                    init_tmpBerral();
                    if (!pTmpFuzzyIndexBarrel_->load_())
                    {
                        LOG(INFO) << "Index Wrong!!"<<endl;
                        delete_AllIndexFile();
                        flag = false;
                    }
                }
            }
        }
        else
        {
            flag = false;
            loadLastDocid_();
        }

        if (flag)
        {
            if (loadLastDocid_())
            {
                isStartFromLocal_ = true;
            }
        }
        else
        {
            if (!isStartFromLocal_)
            {
                if (pMainFuzzyIndexBarrel_ != NULL)
                {
                    delete pMainFuzzyIndexBarrel_;
                    pMainFuzzyIndexBarrel_ = NULL;
                }
                if (pTmpFuzzyIndexBarrel_!= NULL)
                {
                    delete pTmpFuzzyIndexBarrel_;
                    pTmpFuzzyIndexBarrel_ = NULL;
                }
                delete_AllIndexFile();
            }
        }
    }

    bool IncrementalFuzzyManager::initMainFuzzyIndexBarrel_()
    {
        if (pMainFuzzyIndexBarrel_ == NULL)
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
            if (pMainFuzzyIndexBarrel_ == NULL)
            {
                BarrelNum_--;
                return false;
            }
            pMainFuzzyIndexBarrel_->init(index_path_);
        }
        return true;
    }
    bool IncrementalFuzzyManager::index_(uint32_t& docId, std::string propertyString)
    {
        ///     xxxxxxxxxxxxxxxx  ／／这里一个一个的建立也是一个问题....
        ///     还有一个问题：在tmp构建时是否提供搜索
        
        if (IndexedDocNum_ >= MAX_INCREMENT_DOC)
            return false;
        if ( isInitIndex_ == false)
        {
            init_tmpBerral();
            {
                if (pTmpFuzzyIndexBarrel_ != NULL)
                {
                    pTmpFuzzyIndexBarrel_->setStatus();
                    if (!pTmpFuzzyIndexBarrel_->buildIndex_(docId, propertyString))
                        return false;
                }
                IndexedDocNum_++;
            }
        }
        else
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

    void IncrementalFuzzyManager::delete_AllIndexFile()
    {
        bfs::path pathMainInc = index_path_ + "/Main.inv.idx";
        bfs::path pathMainFd = index_path_ + "/Main.fd.idx";
        bfs::path pathTmpInc = index_path_ + "/Tmp.inv.idx";
        bfs::path pathTmpFd = index_path_ + "/Tmp.fd.idx";

        bfs::remove(pathMainInc);
        bfs::remove(pathMainFd);
        bfs::remove(pathTmpInc);
        bfs::remove(pathTmpFd);
    }

    bool IncrementalFuzzyManager::init_tmpBerral()
    {
        if (pTmpFuzzyIndexBarrel_ == NULL)
        {
            BarrelNum_++;
            string path = index_path_ + "/Tmp";
            pTmpFuzzyIndexBarrel_ = new IncrementalFuzzyIndex(
                path,
                idManager_,
                laManager_,
                indexSchema_,
                MAX_TMP_DOC,
                analyzer_
            );
            if (pTmpFuzzyIndexBarrel_ == NULL)
            {
                BarrelNum_--;
                return false;
            }
            pTmpFuzzyIndexBarrel_->init(index_path_);
        }
        return true;
    }

    IncrementalFuzzyManager::~IncrementalFuzzyManager()
    {
        if (pMainFuzzyIndexBarrel_ != NULL)
        {
            delete pMainFuzzyIndexBarrel_;
            pMainFuzzyIndexBarrel_ = NULL;
        }
        if (pTmpFuzzyIndexBarrel_!= NULL)
        {
            delete pTmpFuzzyIndexBarrel_;
            pTmpFuzzyIndexBarrel_ = NULL;
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

    IncrementalFuzzyManager::IncrementalFuzzyManager(const std::string& path,
                                       const std::string& tokenize_path,
                                       const std::string& property,
                                       boost::shared_ptr<DocumentManager>& document_manager,
                                       boost::shared_ptr<IDManager>& idManager,
                                       boost::shared_ptr<LAManager>& laManager,
                                       IndexBundleSchema& indexSchema)
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
        pTmpFuzzyIndexBarrel_ = NULL;
        property_ = property;
        isInitIndex_ = false;
        isMergingIndex_ = false;
        isStartFromLocal_ = false;
        isAddingIndex_ = false;
        index_path_ = path;
        tokenize_path_ = tokenize_path;
        buildTokenizeDic();
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


    bool IncrementalFuzzyManager::fuzzySearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
    {
        izenelib::util::ClockTimer timer;
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

            if (pMainFuzzyIndexBarrel_ != NULL && isInitIndex_ == false)
            {
                {
                    ScopedReadLock lock(mutex_);
                    uint32_t hitdoc;
                    pMainFuzzyIndexBarrel_->getFuzzyResult_(termidList, resultList, ResultListSimilarity, hitdoc);
                    pMainFuzzyIndexBarrel_->score(query, resultList, ResultListSimilarity);
                }
            }
            if (pTmpFuzzyIndexBarrel_ != NULL && isAddingIndex_ == false)
            {
                {
                    LOG(INFO) << "get temp";
                    ScopedReadLock lock(mutex_);
                    //pTmpFuzzyIndexBarrel_->getFuzzyResult_(termidList, resultList, ResultListSimilarity);
                }
            }
            LOG(INFO) << "INC Search ResulList Number:" << resultList.size();
            LOG(INFO) << "INC Search Time Cost: " << timer.elapsed() << " seconds";
        }
        return true;
    }

    bool IncrementalFuzzyManager::exactSearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
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
            if (pTmpFuzzyIndexBarrel_ != NULL && isAddingIndex_ == false)
            {
                {
                    LOG(INFO) << "get temp";
                    ScopedReadLock lock(mutex_);
                    pTmpFuzzyIndexBarrel_->getExactResult_(termidList, resultList, ResultListSimilarity);
                }
            }
            LOG(INFO) << "search ResulList number: " << resultList.size();
        }
        return true;
    }

    void IncrementalFuzzyManager::setLastDocid(uint32_t last_docid)
    {
        last_docid_ = last_docid;
        saveLastDocid_();
    }

    void IncrementalFuzzyManager::InitManager_()
    {
        startIncrementalManager();
        initMainFuzzyIndexBarrel_();
        LOG(INFO) << "Init IncrementalManager....";
    }

    void IncrementalFuzzyManager::doCreateIndex_()
    {
        isInitIndex_ = true;
        if (BarrelNum_ == 0)
        {
            //isInitIndex_ = true;
            startIncrementalManager();
            initMainFuzzyIndexBarrel_();
        }

        /*if (isInitIndex_ == true)// this is before lock(mutex), used to prevent new request;
        {
        	isAddingIndex_ = false;// this means add to tempBarrel;
        }
        else
        	isAddingIndex_ = true;*/ //this part is discard means: all the index just at to mainbarrel. There is only one Barrel now

        {
            ScopedWriteLock lock(mutex_);

            LOG(INFO) << "Adding new documnent to index......";
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
                if (!index_(i, textStr))
                {
                    LOG(INFO) << "Add index error";
                    return;
                }
                last_docid_++;
            }
            saveLastDocid_();
            save_();
            izenelib::util::ClockTimer timer;
            LOG(INFO) << "Begin prepare_index_.....";
            prepare_index_();
            isInitIndex_ = false;
            //isAddingIndex_= false; there is only one Barrel now.
            LOG(INFO) << "Prepare_index_ total elapsed:" << timer.elapsed() << " seconds";
        }
        pMainFuzzyIndexBarrel_->print();
        if (pTmpFuzzyIndexBarrel_) pTmpFuzzyIndexBarrel_->print();
    }

    void IncrementalFuzzyManager::createIndex_()
    {
        LOG(INFO) << "document_manager_->getMaxDocId()" << document_manager_->getMaxDocId();
        string name = "createIndex_";
        task_type task = boost::bind(&IncrementalFuzzyManager::doCreateIndex_, this);
        JobScheduler::get()->addTask(task, name);
    }

    void IncrementalFuzzyManager::mergeIndex()
    {
        isMergingIndex_ = true;
        {
            ScopedWriteLock lock(mutex_);
            if (pMainFuzzyIndexBarrel_ != NULL && pTmpFuzzyIndexBarrel_ != NULL)
            {
                for (std::vector<IndexItem>::const_iterator it = pTmpFuzzyIndexBarrel_->getForwardIndex()->getIndexItem_().begin();
                        it != pTmpFuzzyIndexBarrel_->getForwardIndex()->getIndexItem_().end(); ++it)
                {
                    pMainFuzzyIndexBarrel_->getForwardIndex()->addIndexItem_(*it);
                }

                for (std::set<std::pair<uint32_t, uint32_t>, pairLess>::const_iterator it = pTmpFuzzyIndexBarrel_->getIncrementIndex()->gettermidList_().begin();
                        it != pTmpFuzzyIndexBarrel_->getIncrementIndex()->gettermidList_().end(); ++it)
                {
                    pMainFuzzyIndexBarrel_->getIncrementIndex()->addTerm_(*it, pTmpFuzzyIndexBarrel_->getIncrementIndex()->getdocidLists_()[it->second]);////xxx
                }
                prepare_index_();
                delete_AllIndexFile();
                string file = ".tmp";
                pMainFuzzyIndexBarrel_->save_(file);
                delete pTmpFuzzyIndexBarrel_;
                pTmpFuzzyIndexBarrel_ = NULL;
                saveLastDocid_();
                string tmpName = pMainFuzzyIndexBarrel_->getForwardIndex()->getPath() + file;
                string fileName = pMainFuzzyIndexBarrel_->getForwardIndex()->getPath();

                boost::filesystem::path path(tmpName);
                boost::filesystem::path path1(fileName);
                boost::filesystem::rename(path, path1);

                string tmpName1 = pMainFuzzyIndexBarrel_->getIncrementIndex()->getPath() + file;
                string fileName2 = pMainFuzzyIndexBarrel_->getIncrementIndex()->getPath();

                boost::filesystem::path path2(tmpName1);
                boost::filesystem::path path3(fileName2);
                boost::filesystem::rename(path2, path3);
                pMainFuzzyIndexBarrel_->resetStatus();
                pMainFuzzyIndexBarrel_->print();
            }
        }
        isMergingIndex_ = false;
    }
}
