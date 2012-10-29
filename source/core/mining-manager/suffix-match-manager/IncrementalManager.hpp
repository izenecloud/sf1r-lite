
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <ir/id_manager/IDManager.h>
#include <configuration-manager/PropertyConfig.h>
#include <util/ClockTimer.h>
/**
@brief The search result from IncrementIndex just need two parts: vector<uint32_t> ResultList and vectort<similarity> ResultList;
*/

namespace sf1r
{
#define MAX_INCREMENT_DOC 500000
#define MAX_POS_LEN 24
#define MAX_HIT_NUMBER 3
class DocumentManager;
class LAManager;
using izenelib::ir::idmanager::IDManager;

/**
@brife use this 24 bit data to mark if is this position is hit or not;
postion 0-23;
*/
struct BitMap
{
	unsigned int data_;//init is zero

	bool getBitMap(unsigned short i) const
	{
		if (i > MAX_POS_LEN)//front first;
		{
			return false;
		}
		unsigned int data = 0;
		data = data_>>i;
		unsigned int j = 1;
		return (data&j) == 1;

	}
	bool setBitMap(unsigned short i)
	{
		if (i > MAX_POS_LEN)
		{
			return false;
		}
		unsigned int data = 1;
		data<<=i;
		data_ |= data;
		return true;
	}

	void reset()
	{
		data_ = 0;
	}
};
/**
@brief IndexItem for forwardIndex;
*/
struct  IndexItem
{
	uint32_t Docid_;

	uint32_t Termid_;

	unsigned short Pos_;

	void setIndexItem_(uint32_t docid, uint32_t termid, unsigned short pos)
	{
		Docid_ = docid;
		Termid_ = termid;
		Pos_ = pos;
	}

	IndexItem()
	{
		Docid_ = 0;
		Termid_ = 0;
		Pos_ = 0;
	}

	IndexItem(const IndexItem& indexItem)
	{
		*this = indexItem;
	}
};
/**
@brief ForwardIndex is for restore query 
*/
struct BitMapLess
{
  	bool operator() (const pair<uint32_t, BitMap>&s1, const pair<uint32_t, BitMap>&s2) const
   	{
		return s1.first < s2.first;
   	}
};

class ForwardIndex
{
public:
	ForwardIndex(std::string index_path)
	{
		index_path_ = index_path;
		IndexCount_ = 1;
		IndexItems_.clear();
	}

	~ForwardIndex()
	{

	}
	/**
	@brief add one doc's termid and pos to ForwardIndex
	*/
	void addDocForwardIndex_(uint32_t docId, std::vector<uint32_t>& termidList, std::vector<unsigned short>& posList)
	{
		unsigned int count = termidList.size();	
		for (unsigned int i = 0; i < count; ++i)
		{
			addOneIndexIterms_(docId, termidList[i], posList[i]);
			IndexCount_++;
		}
		BitMap b;
		b.reset();
		pair<uint32_t, BitMap> pa(docId, b);
		if(BitMapMatrix_.find(pa) == BitMapMatrix_.end())//
			BitMapMatrix_.insert(pa);
	}
    
	void addOneIndexIterms_(uint32_t docId, uint32_t termid, unsigned short pos)
	{
		IndexItem indexItem;
		indexItem.setIndexItem_(docId, termid, pos);
		IndexItems_.push_back(indexItem);
	}

	void setPosition_(std::vector<pair<uint32_t, unsigned short> >*& v)
	{
		std::vector<pair<uint32_t, unsigned short> >::const_iterator it = (*v).begin();
		BitMap b;
		b.reset();
		for (; it != (*v).end(); ++it)
		{
			pair<uint32_t, BitMap> pa((*it).first, b);
			std::set<pair<uint32_t, BitMap>, BitMapLess>::iterator its = BitMapMatrix_.find(pa);
			const_cast<pair<uint32_t, BitMap> &>(*its).second.setBitMap((*it).second);
		}
	}

	bool getTermIDBy(const uint32_t& docid, const std::vector<unsigned short>& pos, std::vector<uint32_t>& termidList)
	{
		std::set<uint32_t> termidSet; 
		std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
		std::vector<IndexItem>::const_iterator its = IndexItems_.begin();
		for (; it != IndexItems_.end(); ++its,++it)
		{
			if ((*it).Docid_ == docid)
				break;
		}
		for (std::vector<unsigned short>::const_iterator i = pos.begin(); i != pos.end(); ++i)
		{
			uint32_t k = 0;
			it = its;
			while(k < MAX_POS_LEN)
			{
				if ((*it).Docid_ == docid && (*it).Pos_ == *i)
				{
					//cout<<"the choosen termid:"<<(*it).Termid_<<endl;
					termidSet.insert((*it).Termid_);
					break;
				}
				k++;
				it++;
			}
		}
		for (std::set<uint32_t>::iterator i = termidSet.begin(); i != termidSet.end(); ++i)
		{
			termidList.push_back(*i);
		}
		return true;
	}

	void resetBitMapMatrix()
	{
		std::set<pair<uint32_t, BitMap>, BitMapLess>::iterator it = BitMapMatrix_.begin();
		for (; it != BitMapMatrix_.end(); ++it)
		{
			const_cast<pair<uint32_t, BitMap> &>(*it).second.reset();
		}
	}

	void getLongestSubString_(std::vector<uint32_t>& termidList, const std::vector<uint32_t>& resultList)
	{
		std::vector<uint32_t>::const_iterator it;
		BitMap b;
		b.reset();
		vector<unsigned short> temp;
		vector<unsigned short> longest;
		uint32_t resultDocid;
		for (it = resultList.begin(); it != resultList.end(); ++it)
		{
			temp.clear();
			pair<uint32_t, BitMap> pa(*it, b);

			std::set<pair<uint32_t, BitMap>, BitMapLess>::iterator its = BitMapMatrix_.find(pa);
			bool isNext = true;
			for (unsigned short i = 0; i < MAX_POS_LEN; ++i)//keep at most 32 positions
			{
				if((*its).second.getBitMap(i))
				{
					temp.push_back(i);
					isNext = true;
				}
				else 
				{
					if(isNext == false)
					{
						if(temp.size() > longest.size())
						{
							longest = temp;
							resultDocid = *it;
						}
						temp.clear();
					}
					else
					{
						isNext = false;
					}
				}
			}
			if(temp.size() > longest.size())
			{
				longest = temp;
				resultDocid = *it;
				temp.clear();
			}
		}
		LOG(INFO)<<"[INFO] Resulst Docid:"<<resultDocid<<" And hit pos:"<<endl;
		for (unsigned int i = 0; i < longest.size(); ++i)
		{	
			cout<<longest[i]<<" ";
		}
		cout<<endl;
		/**
		If longest.size() is less than 3, means this is almost no doc similar with this doc;
		In order to prevent get those not releated files;
		*/
		if (longest.size() < 3)
		{
			return;
		}
		getTermIDBy(resultDocid, longest, termidList);
	}

	void print()
	{
		cout<<"IndexCount_:"<<IndexCount_<<endl;
	}

private:

	void save_();

	void load_();

private:
 	std::string index_path_;

 	std::vector<IndexItem> IndexItems_;

 	unsigned int IndexCount_;

 	std::set<pair<uint32_t, BitMap>, BitMapLess> BitMapMatrix_;
};

/**
@brief: since this is an temporary index, the doc will less than 10000; so the time complexity is less important;
*/
struct pairLess
{
   bool operator() (const pair<unsigned int, unsigned short>&s1, const pair<unsigned int, unsigned short>&s2) const
   {
    	return s1.first < s2.first;
   }
};


class IncrementIndex
{
public:
	IncrementIndex(std::string file_path)
	{
		Increment_index_path_ = file_path;
		termidNum_ = 0;
	}


	~IncrementIndex()
	{

	}


	void addIndex_(uint32_t docid, std::vector<uint32_t> termidList, std::vector<unsigned short>& posList)// std::vector<unsigned short> posList)
	{
		unsigned int count = termidList.size();
		for (unsigned int j = 0; j < count; ++j)
		{
			pair<uint32_t, uint32_t> temp(termidList[j], 0);
			std::set<std::pair<uint32_t, uint32_t>, pairLess >::iterator i = termidList_.find(temp);
			if (i != termidList_.end())
			{
				uint32_t offset = (*i).second;
				pair<uint32_t, unsigned short> term(docid, posList[j]);
				docidLists_[offset].push_back(term);
			}
			else
			{
				std::vector<pair<uint32_t, unsigned short> > tmpDocList;
				pair<uint32_t, unsigned short> term(docid, posList[j]);
				tmpDocList.push_back(term);
				pair<uint32_t, uint32_t> termid(termidList[j], docidLists_.size());
				termidList_.insert(termid);
				docidLists_.push_back(tmpDocList);
				termidNum_++;
			}
		}
	}

	void getResultAND_(const std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList, std::vector<float>& ResultListSimilarity)
	{
		std::vector<std::vector<pair<uint32_t, unsigned short> >* > docidLists;
		std::vector<uint32_t> sizeLists;
		uint32_t size;
		std::vector<pair<uint32_t, unsigned short> >* vresult = NULL;
		for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
		{
			if(!getTermIdResult_(*i, vresult, size))
				return;
			docidLists.push_back(vresult);
			sizeLists.push_back(size);
		}
		
		mergeAnd_(docidLists, resultList, ResultListSimilarity);
	}

	bool getTermIdResult_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >* &v, uint32_t& size)//xxx
	{
		pair<uint32_t, uint32_t> temp(termid, 0);

		std::vector<std::vector<pair<uint32_t, unsigned short> > >::const_iterator its = docidLists_.begin();
		std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
		if (i != termidList_.end())
		{
			v = &(docidLists_[(*i).second]);
			size = docidLists_[(*i).second].size();
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void getTermIdPos_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >*& v)
	{
		pair<uint32_t, uint32_t> temp(termid, 0);
		std::vector<std::vector<pair<uint32_t, unsigned short> > >::const_iterator its = docidLists_.begin();
		std::set<std::pair<uint32_t, uint32_t> >::iterator i = termidList_.find(temp);
		if (i != termidList_.end())
		{
			v = &(docidLists_[(*i).second]);
		}
	}

	void getResultOR_(const std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList)
	{
		std::set<uint32_t> resultSet;
		std::vector<pair<uint32_t, unsigned short> >* v = NULL;
		uint32_t size;
		for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
		{
			getTermIdResult_(*i, v, size);
			for (std::vector<pair<uint32_t, unsigned short> >::iterator iter = (*v).begin(); iter != (*v).end(); ++iter)
			{
				resultSet.insert((*iter).first);
			}
		}
		for (std::set<uint32_t>::const_iterator i = resultSet.begin(); i != resultSet.end(); ++i)
		{
			resultList.push_back(*i);
		}
	}

	void mergeAnd_(const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& docidLists, std::vector<uint32_t>& resultList, std::vector<float>& ResultListSimilarity)// get add result; all the vector<docid> is sorted;
	{
		izenelib::util::ClockTimer timer;
		unsigned int size = docidLists.size();
		if (size == 0)
		{
			return;
		}
		unsigned int min = 0;
		unsigned int label = 0;
		for (unsigned int i = 0; i < size; ++i)
		{
			if (min == 0)
			{
				min = (*(docidLists[i])).size();
				label = i;
				continue;
			}
			if (min > (*(docidLists[i])).size())
			{
				min = (*(docidLists[i])).size();
				label = i;
			}
		}

		std::vector<pair<uint32_t, unsigned short> >::const_iterator iter = (*(docidLists[label])).begin();
		vector<int> iteratorList;
		for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin(); i != docidLists.end(); ++i)
		{
			iteratorList.push_back(0);
		}

 		for ( ; iter != (*(docidLists[label])).end(); ++iter)
 		{
 			std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator i = docidLists.begin();
 			uint32_t count = 0;
 			bool flag = true;
 			while(count < size)
 			{
 				if (count == label)
 				{
 					count++;
 					i++;
 					continue;
 				}
 				iteratorList[count] = BinSearch_(**i, iteratorList[count], (**i).size(), (*iter).first) ;
 				if( iteratorList[count] == -1)//optim
 				{
 					flag = false;
 					break;
 				}
 				i++;
 				count++;
 			}

 			if(flag)
 			{
 				resultList.push_back((*iter).first);
 				ResultListSimilarity.push_back(1.0);//
 			}
 		}
 		LOG(INFO)<<"mergeAnd and cost:"<<timer.elapsed()<<" seconds"<<endl;
	}

	void print()
	{
		cout<<"docidNumbers_:";
		unsigned int k = 0;
		for (uint32_t i = 0; i < termidList_.size(); ++i)
		{
			k += docidLists_[i].size();
		}
		cout<<k<<endl<<"termidList_.....:"<<termidList_.size()<<endl;
	}

	bool save_();

	bool load_();

	void prepare_index_()
	{
		std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator iter = docidLists_.begin();
		for (; iter != docidLists_.end(); ++iter)
		{
			sort((*iter).begin(), (*iter).end());
		}
	}
private:
	int BinSearch_(std::vector<pair<unsigned int, unsigned short> >&A,  int min,  int max, unsigned int key)
	{
		while(min <= max)
		{
			int mid = (min + max)/2;
			if(A[mid].first == key)
				return mid;
			else if(A[mid].first > key)
				max = mid - 1;
			else
				min = mid + 1;
		}
		return -1;
	}
	
	std::string Increment_index_path_;

	std::vector<std::vector<pair<uint32_t, unsigned short> > > docidLists_;

	std::vector<unsigned int> docidNumbers_;

	std::set<std::pair<uint32_t, uint32_t>, pairLess> termidList_;

	unsigned int termidNum_;
};

class IndexBarral
{
public:
	IndexBarral(
		const std::string& path,
		boost::shared_ptr<IDManager>& idManager,
		boost::shared_ptr<LAManager>& laManager,
		IndexBundleSchema& indexSchema);


	~IndexBarral()
	{
		if (pForwardIndex_ != NULL)
		{
			delete pForwardIndex_;
		}

		if (pIncrementIndex_ != NULL)
		{
			delete pIncrementIndex_;
		}
	}

	void init(std::string& path)
	{
		doc_file_path_ = path;
		pForwardIndex_ = new ForwardIndex(path);
		pIncrementIndex_ = new IncrementIndex(path);
	}

	void load_();

	void save_();

 	bool buildIndex_(uint32_t docId, std::string& text);

	void getResult_(std::vector<uint32_t>& termidList, std::vector<uint32_t>& resultList,  std::vector<float>& ResultListSimilarity)//
	{
		pIncrementIndex_->getResultAND_(termidList, resultList, ResultListSimilarity);
		LOG(INFO)<<"getResultAND_ ::resultList.size()"<<resultList.size()<<endl;
		if (resultList.size() < MAX_HIT_NUMBER)
		{
			std::vector<pair<uint32_t, unsigned short> >* v = NULL;
			pForwardIndex_->resetBitMapMatrix();
	izenelib::util::ClockTimer timer1;
			for (std::vector<uint32_t>::iterator i = termidList.begin(); i != termidList.end(); ++i)
			{
				pIncrementIndex_->getTermIdPos_(*i, v);
				if (v == NULL)
				{
					return;
				}
				pForwardIndex_->setPosition_(v);
			}
	cout<<"setPosition_"<<timer1.elapsed()<<" second"<<endl;

	izenelib::util::ClockTimer timer2;
			std::vector<uint32_t> ORResultList;
			pIncrementIndex_->getResultOR_(termidList, ORResultList);
	cout<<"getResultOR_"<<timer2.elapsed()<<" second"<<endl;
	
	izenelib::util::ClockTimer timer3;
			std::vector<uint32_t> newTermidList;
			pForwardIndex_->getLongestSubString_(newTermidList, ORResultList);
	cout<<"getLongestSubString_"<<timer3.elapsed()<<" second"<<endl;
			pIncrementIndex_->getResultAND_(newTermidList, resultList, ResultListSimilarity);//xxx
		}
	}

	void print()
	{
		pForwardIndex_->print();
		pIncrementIndex_->print();
	}

	void prepare_index_()
	{
		pIncrementIndex_->prepare_index_();
	}

private:
	ForwardIndex* pForwardIndex_;

	IncrementIndex* pIncrementIndex_;

	boost::shared_ptr<IDManager> idManager_;

	boost::shared_ptr<LAManager> laManager_;

	IndexBundleSchema& indexSchema_;

	unsigned int DocNumber_;

	std::string doc_file_path_;
};

/**
@brief 	IncrementManager is the Manger for the whole IncrementIndex;
		It has and not just has the following task:
		Build a new Berral when the old Berral is full and is adding to FM;
		Set New Berral as the main one;//
		Build index and search result;

*/
class IncrementalManager
{
public:
	IncrementalManager(const std::string& path,
		const std::string& property,
		boost::shared_ptr<DocumentManager>& document_manager,
		boost::shared_ptr<IDManager>& idManager,
		boost::shared_ptr<LAManager>& laManager,
		IndexBundleSchema& indexSchema
		);

	~IncrementalManager()
	{
		if (pMainBerral_ != NULL)
		{
			delete pMainBerral_;
		}
		if (pTmpBerral_!= NULL)
		{
			delete pTmpBerral_;
		}
	}


	bool init_()
	{
		out.open("./outtime");
		BerralNum_++;
		//add robust
		pMainBerral_ = new IndexBarral(
			index_path_,
			idManager_,
			laManager_,
			indexSchema_
			);

		if (pMainBerral_ == NULL)
		{
			return false;
		}
		pMainBerral_->init(index_path_);
		return true;
	}

	bool init_tmpBerral()
	{
		BerralNum_++;
		//add robust
		pTmpBerral_ = new IndexBarral(
			index_path_,
			idManager_,
			laManager_,
			indexSchema_
			);

		if (pTmpBerral_ == NULL)
		{
			return false;
		}
		pTmpBerral_->init(index_path_);
		return true;
	}
	/**
	@brief when the mainBerral is added to fm, and documentManager delete these docs and should rebuid these all really add to TmpBerral_;
	If there is new doc add ;BerralNum = 2; change the pointer and is no new doc, berral is emtpy ;back to init status;
	*/
	void resetMainBerral_()
	{
		//delete mainBerral 
		//set tmpBerral as mainBerral
		delete pMainBerral_;
		pMainBerral_ = pTmpBerral_;
		pTmpBerral_ = NULL;
		isMergeToFm_ = false;

	}

	unsigned int edit_distance(const string& s1, const string& s2)
	{
        const size_t len1 = s1.size(), len2 = s2.size();
        vector<vector<unsigned int> > d(len1 + 1, vector<unsigned int>(len2 + 1));
 
        d[0][0] = 0;
        for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
        for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;
 
        for(unsigned int i = 1; i <= len1; ++i)
                for(unsigned int j = 1; j <= len2; ++j)
 
                      d[i][j] = std::min( std::min(d[i - 1][j] + 1,d[i][j - 1] + 1),
                                          d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) );
        return d[len1][len2];
	}

	/**
	@brief that is add file to fm's documnetManger, and notice  the higher manager to let fm build collection... che 
	*/

	void createIndex_();

	void index_(uint32_t& docId, std::string propertyString) 
	{
		//build Document
		if ( IndexedDocNum_ >= MAX_INCREMENT_DOC )// || is isMergeToFm_
		{
			/**
			@ this will allow parrel processing.....
			*/
			/*
			task_type task = boost::bind(&IndexWorker::buildCollection, this, numdoc);
    		JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
    		result = true;
    		void resetMainBerral_()
    		*/
    		/*init_tmpBerral();
    		IndexedDocNum_ = 0;
			IndexedDocNum_++;*/
		}
		else
		{
			{
				ScopedWriteLock lock(mutex_);
				if (pMainBerral_ != NULL)
			    {
			    	pMainBerral_->buildIndex_(docId, propertyString);//need propertyManager and config Manager
				}
				IndexedDocNum_++;
			}
		}
	}

	void prepare_index_()
	{
		pMainBerral_->prepare_index_();
	}

	bool search_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<float> &ResultListSimilarity);

private:
	ofstream out;
			
	uint32_t last_docid_;

	std::string index_path_;

	std::string property_;

	unsigned int BerralNum_;	///at most two, if more than two.....DANGER!
	
	IndexBarral* pMainBerral_;
	
	IndexBarral* pTmpBerral_;

	unsigned int IndexedDocNum_;

	bool isMergeToFm_;

	boost::shared_ptr<DocumentManager> document_manager_;

	boost::shared_ptr<IDManager> idManager_;

	boost::shared_ptr<LAManager> laManager_;

	IndexBundleSchema indexSchema_;

	typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;// lock when search is adding index;

    mutable MutexType mutex_;
};

}

