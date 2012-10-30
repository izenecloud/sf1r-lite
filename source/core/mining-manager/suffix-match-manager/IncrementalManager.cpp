#include "IncrementalManager.hpp"

#include <common/JobScheduler.h>
#include <common/Utilities.h>

#include <la-manager/LAManager.h>//
#include <document-manager/Document.h>

#include <document-manager/DocumentManager.h>
#include <ir/index_manager/index/LAInput.h>

#include <fstream>


namespace sf1r
{

	IndexBarral::IndexBarral(const std::string& path,
		boost::shared_ptr<IDManager>& idManager,
		boost::shared_ptr<LAManager>& laManager,
		IndexBundleSchema& indexSchema)
	:idManager_(idManager)
	, laManager_(laManager)
	, indexSchema_(indexSchema)
	{
		pForwardIndex_  = NULL;
		pIncrementIndex_ = NULL;
		DocNumber_ = 0;
		doc_file_path_ = path;
	}

	bool IndexBarral::buildIndex_(docid_t docId, std::string& text)
	{
 		izenelib::util::UString utext(text, izenelib::util::UString::UTF_8);
 
 		PropertyConfig tempPropertyConfig;
		tempPropertyConfig.propertyName_ = "Title";
		IndexBundleSchema::iterator iter = indexSchema_.find(tempPropertyConfig);

		AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_sia";
        
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

 		pForwardIndex_->addDocForwardIndex_(docId, termidList, posList);

 		pIncrementIndex_->addIndex_(docId, termidList , posList);
 		
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
	{
		BerralNum_ = 0;
		last_docid_ = 1;
		IndexedDocNum_ = 0;
		pMainBerral_ =  NULL;
		pTmpBerral_ = NULL;
		property_ = property;
		isInitIndex_ = true;
		isMergeToFm_ = false;
		tmpdata = 1000000;
		isAddingIndex_ = false;
		index_path_ = path;
	}

 	bool IncrementalManager::search_(const std::string& query, std::vector<docid_t>& resultList, std::vector<float> &ResultListSimilarity)
	{
		if (BerralNum_ == 0)
		{
			std::cout<<"[NOTICE]:THERE IS NOT Berral"<<endl;
		}
		else
		{
			{
				izenelib::util::ClockTimer timer;
				
				izenelib::util::UString utext(query, izenelib::util::UString::UTF_8);
 				AnalysisInfo analysisInfo;
 				analysisInfo.analyzerId_ = "la_sia";
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

 				std::vector<uint32_t>::const_iterator ittmp = termidList.begin();
 	
 				cout<<"[][]search query:"<<endl;
 				for (; ittmp != termidList.end(); ++ittmp)
 				{
 					cout<<*ittmp<<" ";
 				}
 				cout<<endl;
 				if (pMainBerral_ != NULL && isInitIndex_ == false)
 				{
					pMainBerral_->getResult_(termidList, resultList, ResultListSimilarity);

 				}
 				if (pTmpBerral_ != NULL && isAddingIndex_ == false)
 				{ 
 					{
 						ScopedReadLock lock(mutex_);
 						pTmpBerral_->getResult_(termidList, resultList, ResultListSimilarity);
 					}
 				}
 				cout<<"search ResulList number:"<<resultList.size()<<endl;
 				//ResultListSimilarity.clear();
 				/*for (std::vector<uint32_t>::iterator i = resultList.begin(); i != resultList.end(); ++i)
 				{
 					PropertyValue result;
 					document_manager_->getPropertyValue(*i, "Title", result);
        			std::string oldvalue_str;
        			izenelib::util::UString& propertyValueU = result.get<izenelib::util::UString>();
        			propertyValueU.convertString(oldvalue_str, izenelib::util::UString::UTF_8);

        			uint32_t dis = edit_distance(query, oldvalue_str);
        			uint32_t max = query.length() > oldvalue_str.length() ? query.length():oldvalue_str.length();
        			float score = 1.0 - ((float)max/(2*(float)max - (float)dis));
        			ResultListSimilarity.push_back(score);
 				}*/
				LOG(INFO)<<"timer total elapsed:"<<timer.elapsed()<<" seconds"<<endl;
			}
		}
		return true;
	}

	void IncrementalManager::doCreateIndex_()
	{
		if (BerralNum_ == 0)
		{
			init_();
		}

		uint32_t i = 0;
		if (isInitIndex_ == true)
		{
			isAddingIndex_ = false;
		}
		else
			isAddingIndex_ = true;

		for ( i = last_docid_; i <= document_manager_->getMaxDocId(); ++i)
    	{
        	if (i % 100000 == 0)
        	{
            	LOG(INFO) << "inserted docs: " << i;
            	if (i == tmpdata)
            	{
            		break;
            	}
        	}
        	Document doc;
        	document_manager_->getDocument(i, doc);
        	Document::property_const_iterator it = doc.findProperty(property_);

        	const izenelib::util::UString& text = it->second.get<UString>();
        	std::string textStr;
        	text.convertString(textStr, izenelib::util::UString::UTF_8);

        	index_(i, textStr);

        	last_docid_++;
    	}
    	izenelib::util::ClockTimer timer;
    	LOG(INFO) <<"begin prepare_index_....."<<endl;
    	prepare_index_();
    	isInitIndex_ = false;// first time init over;
    	isAddingIndex_= false;
    	LOG(INFO) <<"prepare_index_ total elapsed:"<<timer.elapsed()<<" seconds"<<endl;
    	pMainBerral_->print();
    	tmpdata += 500000;	
	}

	void IncrementalManager::createIndex_()
	{
		string name = "createIndex_";
		//cout<<"getMaxDocId.............."<<document_manager_->getMaxDocId()<<endl;
		task_type task = boost::bind(&IncrementalManager::doCreateIndex_, this);
    	JobScheduler::get()->addTask(task, name);
	}

	/*void void IncrementalManager::mergeIndex() //merge tmpBerral to MainBerral...
	{

	}*/
}