#include "IncrementalManager.hpp"

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

	IndexBarral::IndexBarral(const std::string& path,
		boost::shared_ptr<IDManager>& idManager,
		boost::shared_ptr<LAManager>& laManager,
		IndexBundleSchema& indexSchema,
		unsigned int Max_Doc_Num,
		cma::Analyzer* &analyzer)
	: idManager_(idManager)
	, laManager_(laManager)
	, indexSchema_(indexSchema)
	, Max_Doc_Num_(Max_Doc_Num)
	, analyzer_(analyzer)
	{
		pForwardIndex_  = NULL;
		pIncrementIndex_ = NULL;
		DocNumber_ = 0;
		doc_file_path_ = path;
		isAddedIndex_ = false;
	}

	bool IndexBarral::buildIndex_(docid_t docId, std::string& text)
	{
 		izenelib::util::UString utext(text, izenelib::util::UString::UTF_8);

		AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_unigram";    
 		LAInput laInput;
 		laInput.setDocId(docId);

 		if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
 			return false;	
 		LAInput::const_iterator it;
 		std::vector<uint32_t> termidList;
 		std::vector<unsigned short> posList;
 		for ( it = laInput.begin(); it != laInput.end(); ++it)
 		{
 			termidList.push_back((*it).termid_);
 			posList.push_back((*it).wordOffset_);
 		}
 		if (pForwardIndex_)
 			pForwardIndex_->addDocForwardIndex_(docId, termidList, posList);
 		else
 			LOG(INFO) <<" E:ForwardIndex is empty"<<endl;

 		if (pIncrementIndex_)
        	pIncrementIndex_->addIndex_(docId, termidList , posList);
        else
        	LOG(INFO) <<" E:IncrementIndex is empty"<<endl;
 		return true;
 	}

 	bool IndexBarral::score(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
    {
        Sentence querySentence(query.c_str());
        izenelib::util::ClockTimer timer;
        analyzer_->runWithSentence(querySentence);
        std::vector<std::vector<uint32_t> > termIN;
        std::vector<std::vector<uint32_t> > termNotIN;

        AnalysisInfo analysisInfo;
 		analysisInfo.analyzerId_ = "la_unigram";
 	    
        for (int i = 0; i < querySentence.getCount(0); ++i)
        {
            //printf("%s, ", querySentence.getLexicon(0, i));
            string str(querySentence.getLexicon(0, i));
            LAInput laInput;
 		    laInput.setDocId(0);
            izenelib::util::UString utext(str, izenelib::util::UString::UTF_8);
 		    if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
 			   return false;
 			LAInput::const_iterator it = laInput.begin();
 			std::vector<uint32_t> termidList;
 			for (; it != laInput.end(); ++it)
 			{
 				termidList.push_back((*it).termid_);
 			}
 			termIN.push_back(termidList);
        }

        for (int i = 0; i < querySentence.getCount(1); i++)
        {
            //printf("%s, ", querySentence.getLexicon(1, i));
            string str(querySentence.getLexicon(1, i));
            izenelib::util::UString utext(str, izenelib::util::UString::UTF_8);
            LAInput laInput;
 		    laInput.setDocId(0);
 		    if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
 			   return false;
 			LAInput::const_iterator it = laInput.begin();
 			std::vector<uint32_t> termidList;
 			for (; it != laInput.end(); ++it)
 			{
 				termidList.push_back((*it).termid_);
 			}
 			termNotIN.push_back(termidList);
        }

        double rank = 0;
        for (uint32_t i = 0; i < resultList.size(); ++i)
        {
        	rank = 0;
        	std::vector<std::vector<uint32_t> >::iterator it = termIN.begin();
        	for (; it != termIN.end(); ++it)
        	{
        		rank += getScore(*it, resultList[i])*double(2.0);
        	}
        	std::vector<std::vector<uint32_t> >::iterator iter = termNotIN.begin();
        	for (; iter != termNotIN.end(); ++iter)
        	{
        		rank += getScore(*iter, resultList[i])*double(1.0);
        	}
        	ResultListSimilarity.push_back(rank);
        }
		return true;
    }

	IncrementalManager::IncrementalManager(const std::string& path,
		const std::string& property,
		boost::shared_ptr<DocumentManager>& document_manager,
		boost::shared_ptr<IDManager>& idManager,
		boost::shared_ptr<LAManager>& laManager,
		IndexBundleSchema& indexSchema
		):document_manager_(document_manager)
	, idManager_(idManager)
	, laManager_(laManager)
	, indexSchema_(indexSchema)
	, analyzer_(NULL)
	, knowledge_(NULL)
	{
		BerralNum_ = 0;
		last_docid_ = 0;
		IndexedDocNum_ = 0;
		pMainBerral_ =  NULL;
		pTmpBerral_ = NULL;
		property_ = property;
		isInitIndex_ = false;
		isMergingIndex_ = false;
		isStartFromLocal_ = false;
		isAddingIndex_ = false;
		index_path_ = path;
		buildTokenizeDic();
	}

	void IncrementalManager::buildTokenizeDic()
	{
        std::string cma_path;
        LAPool::getInstance()->get_cma_path(cma_path);
        boost::filesystem::path cma_fmindex_dic(cma_path);
        cma_fmindex_dic /= boost::filesystem::path("fmindex_dic");

        knowledge_ = CMA_Factory::instance()->createKnowledge();
        knowledge_->loadModel( "utf8", cma_fmindex_dic.c_str(), false);
        analyzer_ = CMA_Factory::instance()->createAnalyzer();
        analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
        // using the maxprefix analyzer
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
        analyzer_->setKnowledge(knowledge_);
	}


	bool IncrementalManager::fuzzySearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
	{
		izenelib::util::ClockTimer timer;
		if (BerralNum_ == 0)
		{
			std::cout<<"[NOTICE]:THERE IS NOT Berral"<<endl;
		}
		else
		{
			if (isMergingIndex_)
			{
				LOG(INFO)<< " Merging Index..."<<endl;
				return true;
			}
			izenelib::util::UString utext(query, izenelib::util::UString::UTF_8);
 			AnalysisInfo analysisInfo;
 			analysisInfo.analyzerId_ = "la_unigram";
 			LAInput laInput;
 			laInput.setDocId(0);
 			if (!laManager_->getTermIdList(idManager_.get(), utext, analysisInfo, laInput))
 				return false;
 			LAInput::const_iterator it = laInput.begin();
 			std::set<termid_t> setDocId;
 			std::vector<termid_t> termidList;

 			for (; it != laInput.end(); ++it)
 			{
 				setDocId.insert((*it).termid_);
 			}
 			for (std::set<termid_t>::iterator i = setDocId.begin(); i != setDocId.end(); ++i)
 			{
 				termidList.push_back(*i);
 			}

 			if (pMainBerral_ != NULL && isInitIndex_ == false)
 			{
 				{
 					ScopedReadLock lock(mutex_);
 					uint32_t hitdoc;
					pMainBerral_->getFuzzyResult_(termidList, resultList, ResultListSimilarity, hitdoc);
					pMainBerral_->score(query, resultList, ResultListSimilarity);
				}
 			}
 			if (pTmpBerral_ != NULL && isAddingIndex_ == false)
 			{ 
 				{
 					cout<<"get temp"<<endl;
 					ScopedReadLock lock(mutex_);
 					//pTmpBerral_->getFuzzyResult_(termidList, resultList, ResultListSimilarity);
 				}
 			}
 			LOG(INFO)<<"INC Search ResulList Number:"<<resultList.size()<<endl;
 			LOG(INFO)<<"INC Search Time Cost: "<<timer.elapsed()<<" seconds"<<endl;
		}
		return true;
	}

 	bool IncrementalManager::exactSearch_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<double> &ResultListSimilarity)
 	{
 		if (BerralNum_ == 0)
		{
			std::cout<<"[NOTICE]:THERE IS NOT Berral"<<endl;
		}
		else
		{
			if (isMergingIndex_)
			{
				LOG(INFO)<< " Merging Index..."<<endl;
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
 			
 			LAInput::const_iterator it = laInput.begin();
 			std::set<termid_t> setDocId;
 			std::vector<termid_t> termidList;

 			for (; it != laInput.end(); ++it)
 			{
 				setDocId.insert((*it).termid_);
 			}
 			for (std::set<termid_t>::iterator i = setDocId.begin(); i != setDocId.end(); ++i)
 			{
 				termidList.push_back(*i);
 			}

 			std::vector<uint32_t>::const_iterator ittmp = termidList.begin();
 	
 			if (pMainBerral_ != NULL && isInitIndex_ == false)
 			{
 				{
 					ScopedReadLock lock(mutex_);
					pMainBerral_->getExactResult_(termidList, resultList, ResultListSimilarity);
				}
 			}
 			if (pTmpBerral_ != NULL && isAddingIndex_ == false)
 			{ 
 				{
 					cout<<"get temp"<<endl;
 					ScopedReadLock lock(mutex_);
 					pTmpBerral_->getExactResult_(termidList, resultList, ResultListSimilarity);
 				}
 			}
 			cout<<"search ResulList number:"<<resultList.size()<<endl;
		}
		return true;
 	}

 	void IncrementalManager::setLastDocid(uint32_t last_docid)
 	{
 		last_docid_ = last_docid;
 		saveLastDocid_();
 	}

 	void IncrementalManager::InitManager_()
 	{
 		startIncrementalManager();
 		init_();
 		LOG(INFO)<<"Init IncrementalManager...."<<endl;
 	}

	void IncrementalManager::doCreateIndex_()
	{
		isInitIndex_ = true;
		if (BerralNum_ == 0)
		{
			//isInitIndex_ = true;
			startIncrementalManager();
			init_();
		}

		/*if (isInitIndex_ == true)// this is before lock(mutex), used to prevent new request;
		{
			isAddingIndex_ = false;// this means add to tempBarrel; 
		}
		else
			isAddingIndex_ = true;*/ //this part is discard means: all the index just at to mainbarrel. There is only one Barrel now

		{
			ScopedWriteLock lock(mutex_);

			uint32_t i = 0;
			LOG(INFO) << "Adding new documnent to index......"<<endl;
			for(i = last_docid_ + 1; i <= document_manager_->getMaxDocId(); i++)
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
	        	if(!index_(i, textStr))
	        	{
	        		LOG(INFO)<< " Add index error"<<endl;
	        		return;
	        	}    	
	        	last_docid_++;
			}
			saveLastDocid_();
			save_();
	    	izenelib::util::ClockTimer timer;
	    	LOG(INFO) <<"Begin prepare_index_....."<<endl;
	    	prepare_index_();
	    	isInitIndex_ = false;
	    	//isAddingIndex_= false; there is only one Barrel now. 
	    	LOG(INFO) <<"Prepare_index_ total elapsed:"<<timer.elapsed()<<" seconds"<<endl;
    	}
    	pMainBerral_->print();
    	if(pTmpBerral_) pTmpBerral_->print(); 
	}

	void IncrementalManager::createIndex_()
	{
		cout<<"document_manager_->getMaxDocId()"<<document_manager_->getMaxDocId()<<endl;
		string name = "createIndex_";
		task_type task = boost::bind(&IncrementalManager::doCreateIndex_, this);
    	JobScheduler::get()->addTask(task, name);
	}

	void IncrementalManager::mergeIndex()
	{
		isMergingIndex_ = true;
		{
			ScopedWriteLock lock(mutex_);
			if (pMainBerral_ != NULL && pTmpBerral_ != NULL)
			{
				std::vector<IndexItem>::const_iterator i = (*(pTmpBerral_->getForwardIndex()->getIndexItem_())).begin();
				for ( ; i != (*(pTmpBerral_->getForwardIndex()->getIndexItem_())).end(); ++i)
				{
					pMainBerral_->getForwardIndex()->addIndexItem_(*i);
				}

				for (std::set<std::pair<uint32_t, uint32_t>, pairLess>::const_iterator it = (*(pTmpBerral_->getIncrementIndex()->gettermidList_())).begin()
						; it != (*(pTmpBerral_->getIncrementIndex()->gettermidList_())).end()
						; ++it)
				{
					pMainBerral_->getIncrementIndex()->addTerm_(*it, (*(pTmpBerral_->getIncrementIndex()->getdocidLists_()))[(*it).second]);////xxx
				}
				prepare_index_();
				delete_AllIndexFile();
				string file = ".tmp";
				pMainBerral_->save_(file);
				delete pTmpBerral_;
				pTmpBerral_ = NULL;
				saveLastDocid_();
				string tmpName = pMainBerral_->getForwardIndex()->getPath() + file;
				string fileName = pMainBerral_->getForwardIndex()->getPath();

				boost::filesystem::path path(tmpName);
				boost::filesystem::path path1(fileName);
				boost::filesystem::rename(path, path1);

				string tmpName1 = pMainBerral_->getIncrementIndex()->getPath() + file;
				string fileName2 = pMainBerral_->getIncrementIndex()->getPath();

				boost::filesystem::path path2(tmpName1);
				boost::filesystem::path path3(fileName2);
				boost::filesystem::rename(path2, path3);
				pMainBerral_->resetStatus();
				pMainBerral_->print();
			}
		}
		isMergingIndex_ = false;
	}
}