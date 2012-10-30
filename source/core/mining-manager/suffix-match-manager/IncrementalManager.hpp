
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <stdio.h>
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
#define MAX_INCREMENT_DOC 2000000 //forward index: 64M docs, about:512M memory
#define MAX_POS_LEN 24
#define MAX_HIT_NUMBER 3
#define MAX_SUB_LEN 15

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
		index_path_ = index_path + ".fdi";
		cout<<"index_path_"<<index_path_<<endl;
		IndexCount_ = 1;
		IndexItems_.clear();
		BitMapMatrix_ = new BitMap[MAX_INCREMENT_DOC];
	}

	~ForwardIndex()
	{
		delete [] BitMapMatrix_;
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
		//if(BitMapMatrix_.find(pa) == BitMapMatrix_.end())//
		BitMapMatrix_[docId].reset();
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
		uint32_t k = 0;
		for (; it != (*v).end(); ++it)
		{
			k++;
			//pair<uint32_t, BitMap> pa((*it).first, b);
			BitMapMatrix_[(*it).first].setBitMap((*it).second);
			//std::set<pair<uint32_t, BitMap>, BitMapLess>::iterator its = BitMapMatrix_.find(pa);//
			//const_cast<pair<uint32_t, BitMap> &>(*its).second.setBitMap((*it).second);
		}
		cout<<"k:"<<k<<endl;
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
		for (int i = 0; i < MAX_INCREMENT_DOC ; ++i)
		{
			BitMapMatrix_[i].reset();
		}
	}

	void getLongestSubString_(std::vector<uint32_t>& termidList, const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList)
	{
		cout<<"getLongestSubString_"<<endl;
		std::vector<pair<uint32_t, unsigned short> >::const_iterator it;
		BitMap b;
		b.reset();
		vector<unsigned short> temp;
		vector<unsigned short> longest;
		uint32_t resultDocid = 0;
		for (std::vector<std::vector<pair<uint32_t, unsigned short> >* >::const_iterator iter = resultList.begin(); iter != resultList.end(); ++iter)
		{
			for (it = (**iter).begin(); it != (**iter).end(); ++it)//xxxxx
			{
				temp.clear();
				bool isNext = true;
				for (unsigned short i = 0; i < MAX_POS_LEN; ++i)
				{
					if(BitMapMatrix_[(*it).first].getBitMap(i))
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
								resultDocid = (*it).first;
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
					resultDocid = (*it).first;
					temp.clear();
				}
				if (longest.size() > MAX_SUB_LEN)
				{
					continue;
				}
			}
		}//
		std::vector<unsigned short> hitPos;
		LOG(INFO)<<"[INFO] Resulst Docid:"<<resultDocid<<" And hit pos:"<<endl;
		for (unsigned int i = 0; (i < longest.size() && i < MAX_SUB_LEN); ++i)
		{	
			cout<<longest[i]<<" ";
			hitPos.push_back(longest[i]);
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
		getTermIDBy(resultDocid, hitPos, termidList);
	}

	void print()
	{
		cout<<"IndexCount_:"<<IndexCount_<<endl;
		/*save_();
		IndexItems_.clear();
		load_();*/
	}

private:

	void save_()
	{
		FILE *file;
		if ((file = fopen(index_path_.c_str(), "wb")) == NULL)
		{
			LOG(INFO) <<"Cannot open output file"<<endl;
			return;
		}
		unsigned int totalSize = IndexCount_ * sizeof(IndexItem);
		fwrite(&totalSize, sizeof(totalSize), 1, file);
		cout<<"set totalSize"<<totalSize<<endl;
		std::vector<IndexItem>::const_iterator it = IndexItems_.begin();
		for (; it != IndexItems_.end(); ++it)
		{
			fwrite(&(*it), sizeof(IndexItem), 1, file);	
		}
		fclose(file);
	}

	void load_()
	{
		FILE *file;
		if ((file = fopen(index_path_.c_str(), "rb")) == NULL)
		{
			LOG(INFO) <<"Cannot open output file"<<endl;
			return;
		}
		unsigned int totalSize;
		fread(&totalSize, sizeof(totalSize), 1, file);
		IndexCount_ = totalSize/(sizeof(IndexItem));
		for (unsigned int i = 0; i < IndexCount_; ++i)
		{
			IndexItem indexItem;
			fread(&indexItem, sizeof(IndexItem), 1, file);
			IndexItems_.push_back(indexItem);
		}
		fclose(file);
	}

private:
 	std::string index_path_;

 	std::vector<IndexItem> IndexItems_;

 	unsigned int IndexCount_;

 	BitMap* BitMapMatrix_;
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
		Increment_index_path_ = file_path + ".inc";
		cout<<Increment_index_path_<<endl;
		termidNum_ = 0;
		allIndexDocNum_ = 0;
	}

	~IncrementIndex()
	{

	}

	void addIndex_(uint32_t docid, std::vector<uint32_t> termidList, std::vector<unsigned short>& posList)
	{
		unsigned int count = termidList.size();
		allIndexDocNum_ += count;
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

	bool getTermIdResult_(const uint32_t& termid, std::vector<pair<uint32_t, unsigned short> >* &v, uint32_t& size)
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

	void getResultORFuzzy_(const std::vector<uint32_t>& termidList, std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList)
	{
		std::vector<pair<uint32_t, unsigned short> >* v = NULL;
		uint32_t size;
		for (std::vector<uint32_t>::const_iterator i = termidList.begin(); i != termidList.end(); ++i)
		{
			getTermIdResult_(*i, v, size);
			resultList.push_back(v);
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
 				iteratorList[count] = BinSearch_(**i, iteratorList[count], (**i).size(), (*iter).first);//XXXOPTIM
 				if( iteratorList[count] == -1)
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
 				ResultListSimilarity.push_back(1.0);
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
		/*save_();
		termidList_.clear();
		docidLists_.clear();
		cout<<"emtpy size"<<termidList_.size()<<"xxx"<<docidLists_.size()<<endl;
		load_();
		cout<<"full size"<<termidList_.size()<<"xxx"<<docidLists_.size()<<endl;*/
	}

	bool save_()
	{
		FILE *file;
		if ((file = fopen(Increment_index_path_.c_str(), "wb")) == NULL)
		{
			LOG(INFO) << "Cannot open output file" << endl;
			return false;
		}
		unsigned int count = termidList_.size();
		fwrite(&count, sizeof(count), 1, file);
		for (std::set<std::pair<uint32_t, uint32_t>, pairLess>::iterator i = termidList_.begin(); i != termidList_.end(); ++i)
		{
			fwrite(&(*i), sizeof(pair<uint32_t, uint32_t>), 1, file);
		}
		fwrite(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
		for (std::vector<std::vector<pair<uint32_t, unsigned short> > >::iterator i = docidLists_.begin(); i != docidLists_.end(); ++i)
		{
			unsigned int size = (*i).size();
			fwrite(&size, sizeof(size), 1, file);
			for (std::vector<pair<uint32_t, unsigned short> >::iterator it = (*i).begin(); it != (*i).end(); ++it)
			{
				fwrite(&(*it), sizeof(pair<uint32_t, unsigned short>), 1, file);
			}
		}
		fclose(file);
	}

	bool load_()
	{
		FILE *file;
		if ((file = fopen(Increment_index_path_.c_str(), "rb")) == NULL)
		{
			LOG(INFO) << "Cannot open output file" << endl;
			return false;
		}
		unsigned int count = 0;
		fread(&count, sizeof(unsigned int), 1, file);
		for (unsigned int i = 0; i < count; ++i)
		{
			pair<uint32_t, uint32_t> doc;
			fread(&doc, sizeof(doc), 1, file);
			termidList_.insert(doc);
		}
		fread(&allIndexDocNum_, sizeof(allIndexDocNum_), 1, file);
		std::vector<pair<uint32_t, unsigned short> > v;
		unsigned int i = 0;
		while( i < allIndexDocNum_)
		{
			unsigned int size = 0;
			std::vector<pair<uint32_t, unsigned short> > v;
			fread(&size, sizeof(count), 1, file);
			for (unsigned int j = 0; j < size; ++j)
			{
				pair<uint32_t, unsigned short> p;
				fread(&p, sizeof(pair<uint32_t, unsigned short>), 1, file);
				v.push_back(p);
				i++;
			}
			docidLists_.push_back(v);
			v.clear();
		}
		fclose(file);
	}

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

	unsigned int allIndexDocNum_;
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
		pForwardIndex_ = new ForwardIndex(doc_file_path_);
		pIncrementIndex_ = new IncrementIndex(doc_file_path_);
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
			std::vector<std::vector<pair<uint32_t, unsigned short> >* > ORResultList;
			pIncrementIndex_->getResultORFuzzy_(termidList, ORResultList);
	cout<<"getResultOR_"<<timer2.elapsed()<<" second"<<endl;
	
	izenelib::util::ClockTimer timer3;
			std::vector<uint32_t> newTermidList;
			pForwardIndex_->getLongestSubString_(newTermidList, ORResultList);
	cout<<"getLongestSubString_"<<timer3.elapsed()<<" second"<<endl;
			pIncrementIndex_->getResultAND_(newTermidList, resultList, ResultListSimilarity);
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
		//add robust
		if (pMainBerral_ == NULL)
		{
			string path = index_path_ + ".Main";
			pMainBerral_ = new IndexBarral(
			path,
			idManager_,
			laManager_,
			indexSchema_
			);
			BerralNum_++;
			if (pMainBerral_ == NULL)
			{
				BerralNum_--;
				return false;
			}
			pMainBerral_->init(index_path_);
		}
		return true;
	}

	bool init_tmpBerral()
	{
		if (pTmpBerral_ == NULL)
		{
			BerralNum_++;
			//add robust

			string path = index_path_ + ".tmp";
			pTmpBerral_ = new IndexBarral(
			index_path_,
			idManager_,
			laManager_,
			indexSchema_
			);
			if (pTmpBerral_ == NULL)
			{
				BerralNum_--;
				return false;
			}
			pTmpBerral_->init(index_path_);
		}
		return true;
	}
	/**
	@brief when the mainBerral is added to fm, and documentManager delete these docs and should rebuid these all really add to TmpBerral_;
	If there is new doc add ;BerralNum = 2; change the pointer and is no new doc, berral is emtpy ;back to init status;
	*/
	void resetMainBerral_()//for fm-index;
	{
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
		if ( isInitIndex_ == false)
		{
			init_tmpBerral();
			{
				ScopedWriteLock lock(mutex_);
				if (pTmpBerral_ != NULL)
				{
					pTmpBerral_->buildIndex_(docId, propertyString);	
				}
				IndexedDocNum_++;
			}
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

	void doCreateIndex_();
	void rebuidIndex_()
	{

	}
	void prepare_index_()
	{
		pMainBerral_->prepare_index_();
	}

	bool search_(const std::string& query, std::vector<uint32_t>& resultList, std::vector<float> &ResultListSimilarity);

private:
	uint32_t tmpdata;
	ofstream out;
			
	uint32_t last_docid_;

	std::string index_path_;

	std::string property_;

	unsigned int BerralNum_;	///at most two, if more than two.....DANGER!
	
	IndexBarral* pMainBerral_;
	
	IndexBarral* pTmpBerral_;

	unsigned int IndexedDocNum_;

	bool isMergeToFm_;//add to Fm-index;

	bool isInitIndex_;//for load and first build

	bool isAddingIndex_;//add doc to tmpBarrel....

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

