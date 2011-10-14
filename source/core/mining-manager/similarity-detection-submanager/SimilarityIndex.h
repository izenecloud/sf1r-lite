/**
 * @file sf1r/mining-manager/similarity-detection-submanager/SimilarityIndex.h
 * @author Jinglei&&Ian
 * @date Created <2009-07-17 09:07:53>
 * @date Updated <2009-10-20 10:10:33>
 */
#ifndef SF1R_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_SIMILARITY_INDEX_H
#define SF1R_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_SIMILARITY_INDEX_H

#include <common/type_defs.h>
#include <am/tc/raw/Hash.h>
#include <hdb/HugeDB.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <am/sdb_hash/sdb_fixedhash.h>
#include <am/external_sort/izene_sort.hpp>
#include <ir/mr_word_id/bigram_freq.hpp>
#include <boost/unordered_map.hpp>
namespace sf1r {
namespace sim {

//namespace detail {
//struct SimilarityIndexHugeDB
//{
//    typedef std::pair<docid_t, docid_t> key_type;
//    typedef float value_type;
//    typedef std::pair<char, float> tag_type;
//    typedef izenelib::util::NullLock lock_type;
//    typedef izenelib::am::sdb_btree<key_type, tag_type, lock_type, true>
//    container_type;
//
//    typedef izenelib::hdb::HugeDB<
//        key_type,
//        value_type,
//        lock_type,
//        container_type,
//        true
//    > type;
//};
//} // namespace detail

class SimilarityIndex
{
    typedef izenelib::am::tc::raw::Hash Hash;
    typedef izenelib::am::raw::Buffer Buffer;
//    typedef detail::SimilarityIndexHugeDB::type HugeDB;
    typedef izenelib::am::sdb_fixedhash<uint32_t, uint8_t> INT_FIXED_HASH;

public:
    typedef std::pair<uint32_t, float> value_type;

    explicit SimilarityIndex(
        const std::string& filePrefix,
        const std::string& hdbPath,
        unsigned cacheSize = 0
    )
    :filePrefix_(filePrefix)
//    , hdbPath_(hdbPath)
//    , weightTable_(hdbPath + "/weights.hdb")
    , cacheSize_(cacheSize)
    , useFirst_(true)
    {
        useFirstSdb_ = new izenelib::am::sdb_fixedhash<bool, bool>(filePrefix_+"/useFirst.sdb");
        useFirstSdb_->open();
        bool ret = useFirstSdb_->get(0, useFirst_);
        if(!ret)
        {
            useFirst_ = true;
            useFirstSdb_->update(0, useFirst_);
            useFirstSdb_->flush();
        }
        std::cout<<"use first: "<<(int)useFirst_<<std::endl;
        std::string currentFileName=filePrefix_ + (useFirst_ ? "/similarity.idx.1" : "/similarity.idx.2");
        db_.reset(new Hash(currentFileName));
        db_->open();

        currentFileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.1" : "/similarity_num.idx.2");
        simCount_.reset(new INT_FIXED_HASH(currentFileName.c_str()));
        simCount_->open();
        simCount_->fillCache();

//        weightTable_.setCachedRecordsNumber(8000000);
//        weightTable_.setMergeFactor(4);
//        weightTable_.open();

//        pairTable_=new izenelib::ir::BigramFrequency<>((filePrefix_+"/pair.table").c_str());
//        pairTable_->load();

        sorter_=new izenelib::am::IzeneSort<uint32_t, uint8_t, true>((filePrefix_+"/pair.sort").c_str(), 100000000);

    }

    ~SimilarityIndex()
    {
    	if(useFirstSdb_)
    	{
    		delete useFirstSdb_;
    		useFirstSdb_=0;
    	}
//    	if(pairTable_)
//    	{
//    	    delete pairTable_;
//    	    pairTable_=0;
//    	}
    	if(sorter_)
    	{
    	    delete sorter_;
    	    sorter_=0;
    	}
    }

    void setCache(const boost::shared_ptr<Hash>& db, unsigned cacheSize)
    {
        BOOST_ASSERT(!db->is_open());
        db->setcache(cacheSize);
    }

    template<typename ReaderT>
    void startIndexing(const std::vector<boost::shared_ptr<ReaderT> >& readers, std::size_t numDocs, std::size_t docsLimit)
    {
        boost::lock_guard<boost::mutex> lg(workerMutex_);
        stopIndexing();
        worker_ = boost::thread(
            boost::bind(&SimilarityIndex::indexWorker<ReaderT>,
                        this,
                        readers,
                        numDocs,
                        docsLimit)
                        );
    }

    void joinIndexing()
    {
        worker_.join();
    }

    template<typename ReaderT>
    void index(const std::vector<boost::shared_ptr<ReaderT> >& readers, std::size_t numDocs, std::size_t docsLimit = 30 )
    {
        startIndexing(readers, numDocs, docsLimit);
        joinIndexing();
    }

    void stopIndexing()
    {
        worker_.interrupt();
        worker_.join();
    }

    template<typename ReaderT>
    void indexWorker(const std::vector<boost::shared_ptr<ReaderT> >& readers, std::size_t numDocs, std::size_t docsLimit);

    template<typename OutputIterator>
    OutputIterator getSimilarDocIdScoreList(uint32_t documentId,
                                            unsigned maxNum,
                                            OutputIterator result) const;

    uint32_t getSimilarDocNum(uint32_t documentId) const;

private:
    bool addDocument(
        docid_t docid,
        std::vector<value_type>& list,
        std::size_t docsLimit,
        const boost::shared_ptr<Hash>& db,
        const boost::shared_ptr<INT_FIXED_HASH>& simCount
    );
//    void buildIndexFromWeightTable_(std::size_t docsLimit);
//    void buildIndexFromPairTable_(uint32_t docNums, std::size_t docsLimit);
//
//    void constructPairTable(uint32_t numDocs);
//    void constructWeightTable(uint32_t numDocs);

    void constructSimIndex(uint32_t docsLimit);

    void convert(
            char* data,
            uint32_t doc1,
            uint32_t doc2,
            float weight,
            uint8_t& len);
    void convert(char* data, uint32_t& doc1, uint32_t& doc2,
            float& weight);

    std::string filePrefix_;
//    std::string hdbPath_;

    mutable boost::shared_ptr<Hash> db_;
    boost::shared_ptr<INT_FIXED_HASH> simCount_;
//    mutable HugeDB weightTable_;
//    izenelib::ir::BigramFrequency<>* pairTable_;
    izenelib::am::IzeneSort<uint32_t, uint8_t, true>* sorter_;



    unsigned cacheSize_;

    bool useFirst_;
    izenelib::am::sdb_fixedhash<bool, bool>* useFirstSdb_;
    boost::thread worker_;
    boost::mutex workerMutex_;

    mutable boost::shared_mutex mutex_;
}; // class SimilarityIndex

template<typename ReaderT>
void SimilarityIndex::indexWorker(const std::vector<boost::shared_ptr<ReaderT> >& readers, std::size_t numDocs, std::size_t docsLimit)
{
    typedef typename ReaderT::doc_weight_list_type doc_weight_list_type;
    typedef typename ReaderT::doc_weight_pair_type doc_weight_pair_type;
//
//    weightTable_.clear();
//    string tempFileName(filePrefix_+"/pair.seq");
    uint32_t dataLen=sizeof(uint32_t)*2+sizeof(float);
    char* data=(char*)malloc(dataLen);
    std::cout<<"start score accumulation: "<<std::endl;
    // use hdb to reduce document pair-wise similarity
    for(size_t i=0;i<readers.size();i++)
    {
    	std::cout<<"Processing property: "<<i+1<<std::endl;
    	boost::shared_ptr<ReaderT> reader=readers[i];
    	int count=0;
    	while (reader->next())
        {
        	if(count%1001==0)
        	{
        		std::cout << "\r";
     	  		std::cout<<"processed terms: "<<(float)count/reader->size()*100<<"%"<<std::flush;
        	}
        	count++;
            if (reader->getDocFreq() > 0)
            {
                doc_weight_list_type docWeightList;
                reader->getDocWeightList(docWeightList);

                if (docWeightList.size() > 1)
                {
                    typedef typename doc_weight_list_type::const_iterator iterator;
                    for (iterator outer = docWeightList.begin(),
                               outerEnd = boost::prior(docWeightList.end());
                         outer != outerEnd; ++outer)
                    {
                        for (iterator inner = boost::next(outer),
                                   innerEnd = docWeightList.end();
                             inner != innerEnd; ++inner)
                        {
                            uint32_t doc1=outer->first;
                            uint32_t doc2=inner->first;
                            float weight=outer->second*inner->second;
                            uint8_t len=0;
//                            std::cout<<"weight: "<<weight<<std::endl;
                            convert(data, doc1, doc2, weight, len);
                            sorter_->add_data(len, data);
                            convert(data, doc2, doc1, weight, len);
                            sorter_->add_data(len, data);
                        }
                    }
                }
            }
        }

    }

    std::cout<<"\n start to sort doc pairs."<<std::endl;
    sorter_->sort();
    std::cout<<"end to sort doc pairs."<<std::endl;

//    pairTable_->reset();
//    constructPairTable(numDocs);
//    constructWeightTable(numDocs);
//    sorter_->clear_files();

    std::cout<<"Build the index..."<<std::endl;
    constructSimIndex(docsLimit);
//    buildIndexFromPairTable_(numDocs);
//    buildIndexFromWeightTable_(docsLimit);

    free(data);

}

template<typename OutputIterator>
OutputIterator SimilarityIndex::getSimilarDocIdScoreList(
    uint32_t documentId,
    unsigned maxNum,
    OutputIterator result
) const
{
	boost::lock_guard<boost::shared_mutex> lg(mutex_);
    if (!db_->is_open() && !db_->open())
    {
        std::cerr << "similarity index db is not opened" << std::endl;
        return result;
    }

    Buffer key(reinterpret_cast<char*>(&documentId), sizeof(uint32_t));
    Buffer value;
    if (db_->get(key, value))
    {
        value_type* first = reinterpret_cast<value_type*>(value.data());
        std::size_t size = value.size() / sizeof(value_type);
        value_type* last = first + size;
        if (maxNum < size)
        {
            last = first + maxNum;
        }

//        std::cout << "Found " << size << " similar documents for document " << documentId << std::endl;
        result = std::copy(first, last, result);
    }
    else
    {
        std::cout << "No similarity documents for document "
                  << documentId << std::endl;
    }

    return result;
};


}} // namespace sf1r::sim

namespace sf1r {
using sf1r::sim::SimilarityIndex;
} // namespace sf2v5

#endif // SF1R_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_SIMILARITY_INDEX_H
