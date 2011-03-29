/**
 * @file mining-manager/similarity-detection-submanager/SimilarityIndex.cpp
 * @author Jinglei&&Ian
 * @date Created <2009-09-11 23:30:19>
 * @date Updated <2009-10-20 10:33:56>
 * @brief
 */
#include "SimilarityIndex.h"

#include <util/functional.h>

namespace sf1r {
namespace sim {

bool SimilarityIndex::addDocument(
    docid_t docid,
    std::vector<value_type>& list,
    std::size_t docsLimit,
    const boost::shared_ptr<Hash>& db,
    const boost::shared_ptr<INT_FIXED_HASH>& simCount
)
{
    // std::cout << "Document " << docid << ", length " << list.size() << std::endl;

    typedef izenelib::util::second_greater<value_type> greater_than;
    std::sort(list.begin(), list.end(),
              greater_than());
    if (list.size() > docsLimit)
    {
        list.resize(docsLimit);
    }
    boost::shared_lock<boost::shared_mutex> lg(mutex_);
    if (!simCount_->is_open() && !simCount_->open())
    {
        return false;
    }

    uint8_t listSize=(uint8_t)list.size();
    simCount->update(docid, listSize);

    if (!db->is_open() && !db->open())
    {
        return false;
    }

    izenelib::am::raw::Buffer keyBuffer(
        (char*)&docid,
        sizeof(docid_t)
    );
    izenelib::am::raw::Buffer valueBuffer(
        (char*)list.data(),
        list.size() * sizeof(value_type)
    );


    bool ret=db->update(keyBuffer, valueBuffer);

    return ret;

}


uint32_t SimilarityIndex::getSimilarDocNum(uint32_t documentId) const
{
    boost::shared_lock<boost::shared_mutex> lg(mutex_);
    uint8_t result;
	simCount_->get(documentId, result);
	return (uint32_t)result;
}

void SimilarityIndex::constructSimIndex(uint32_t docsLimit)
{
    std::cout<<"Start to construct similarity index."<<std::endl;
    const uint32_t pruningThreshold=280;

    std::string fileName =filePrefix_ + (useFirst_ ? "/similarity.idx.2" : "/similarity.idx.1");

    boost::shared_ptr<Hash> db(new Hash(fileName));
    if (cacheSize_ != 0)
    {
          setCache(db, cacheSize_);
    }
    db->open();

    fileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.2" : "/similarity_num.idx.1");
    boost::shared_ptr<INT_FIXED_HASH> simCount(new INT_FIXED_HASH(fileName));
    simCount->open();

    if(!sorter_->begin())
    {
        std::cout<<"No data in sorter!!!"<<std::endl;
        return;
    }

    char* data=NULL;
    float frequency=0;
    uint8_t len;
    uint32_t lastDoc1, lastDoc2;
    lastDoc1=0;
    lastDoc2=0;
    std::vector<value_type> docData;
    while (sorter_->next_data(len, &data))
    {
        uint32_t doc1, doc2;
        float weight;
        convert(data, doc1, doc2, weight);
//        std::cout<<"<"<<doc1<<" , "<<doc2<<">: "<<weight<<std::endl;
        if(doc1!=lastDoc1)
        {
            if(doc1!=0)
                addDocument(lastDoc1, docData, docsLimit, db, simCount);
            docData.clear();
            lastDoc1=doc1;
        }
        else
        {
            if(doc2!=lastDoc2)
            {
                if(doc2!=0&&frequency>pruningThreshold)
                {
                     docData.push_back(make_pair(lastDoc2,frequency));
                }
                frequency=0;
                lastDoc2=doc2;
            }
            frequency+=weight;
        }

        free (data);
    }
    //clear the file of sorter;
    sorter_->clear_files();
    //release the memory of sorter;
    delete sorter_;
    sorter_=new izenelib::am::IzeneSort<uint32_t, uint8_t, true>((filePrefix_+"/pair.sort").c_str(), 100000000);
    
    //flush new data.
    db->flush();
    simCount->commit();
    {
        boost::lock_guard<boost::shared_mutex> lg(mutex_);
        db_. reset();
        std::string currentFileName=filePrefix_ + (useFirst_ ? "/similarity.idx.1" : "/similarity.idx.2");
        std::remove(currentFileName.c_str());
        db_=db;
        
        simCount_.reset();
        currentFileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.1" : "/similarity_num.idx.2");
        std::remove(currentFileName.c_str());
        simCount_=simCount;
    }
    useFirst_=!useFirst_;
    useFirstSdb_->update(0, useFirst_);
    useFirstSdb_->flush();

    std::cout<<"End to construct similarity index."<<std::endl;

}

void SimilarityIndex::convert(
        char* data,
        uint32_t doc1,
        uint32_t doc2,
        float weight,
        uint8_t& len)
{
    len = sizeof(doc1) + sizeof(doc2)+ sizeof(weight);
    char* pData = data;
    memcpy(pData, &(doc1), sizeof(doc1));
    pData += sizeof(doc1);
    memcpy(pData, &(doc2), sizeof(doc2));
    pData += sizeof(doc2);
    memcpy(pData, &(weight), sizeof(weight));
    pData += sizeof(weight);
}

void SimilarityIndex::convert(
        char* data,
        uint32_t& doc1,
        uint32_t& doc2,
        float& weight)
{
    char* pData = data;
    memcpy(&doc1, pData, sizeof(doc1));
    pData += sizeof(doc1);
    memcpy(&doc2, pData, sizeof(doc2));
    pData += sizeof(doc2);
    memcpy(&weight, pData, sizeof(weight));
    pData += sizeof(weight);
}


//void SimilarityIndex::buildIndexFromWeightTable_(std::size_t docsLimit)
//{
//    std::pair<docid_t, docid_t> docPair;
//    float weight = 0.0;
//
//    docid_t lastId = 0;
//    std::vector<value_type> indexedList;
//
//    std::string fileName =filePrefix_ + (useFirst_ ? "/similarity.idx.2" : "/similarity.idx.1");
//
//    boost::shared_ptr<Hash> db(new Hash(fileName));
//    if (cacheSize_ != 0)
//    {
//          setCache(db, cacheSize_);
//    }
//    db->open();
//
//    fileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.2" : "/similarity_num.idx.1");
//    boost::shared_ptr<INT_FIXED_HASH> simCount(new INT_FIXED_HASH(fileName));
//    simCount->open();
//
//
//    HugeDB::HDBCursor cursor = weightTable_.get_first_locn();
//    if (weightTable_.get(cursor, docPair, weight))
//    {
//        lastId = docPair.first;
//        indexedList.push_back(std::make_pair(docPair.second, weight));
//
//        while (weightTable_.seq(cursor, docPair, weight))
//        {
//            if (docPair.first != lastId)
//            {
//                addDocument(lastId, indexedList, docsLimit,db, simCount);
//                indexedList.clear();
//                lastId = docPair.first;
//            }
//
//            indexedList.push_back(std::make_pair(docPair.second, weight));
//        }
//
//        addDocument(lastId, indexedList, docsLimit, db, simCount);
//
//    }
//
//    mutex_.lock_shared();
//    db_. reset();
//    std::string currentFileName=filePrefix_ + (useFirst_ ? "/similarity.idx.1" : "/similarity.idx.2");
//    std::remove(currentFileName.c_str());
//    db_=db;
//    db_->flush();
//
//    simCount_.reset();
//    currentFileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.1" : "/similarity_num.idx.2");
//    std::remove(currentFileName.c_str());
//    simCount_=simCount;
//    simCount_->commit();
//    mutex_.unlock_shared();
//
//    useFirst_=!useFirst_;
//    useFirstSdb_->update(0, useFirst_);
//    useFirstSdb_->flush();
//}
//
//void SimilarityIndex::buildIndexFromPairTable_(uint32_t docNums,std::size_t docsLimit)
//{
//    std::string fileName =filePrefix_ + (useFirst_ ? "/similarity.idx.2" : "/similarity.idx.1");
//
//    boost::shared_ptr<Hash> db(new Hash(fileName));
//    if (cacheSize_ != 0)
//    {
//          setCache(db, cacheSize_);
//    }
//    db->open();
//
//    fileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.2" : "/similarity_num.idx.1");
//    boost::shared_ptr<INT_FIXED_HASH> simCount(new INT_FIXED_HASH(fileName));
//    simCount->open();
//
//
//    std::cout<<"start to construct from pair table"<<std::endl;
//    uint32_t start=0;
//    uint32_t interval = 10000;
//    while(start<=docNums)
//    {
//        pairTable_->load(start, start+interval);
//        uint32_t lastDoc=(start+interval)<docNums ? (start+interval):docNums;
//        for(unsigned int id=start;id<=lastDoc;id++)
//        {
//            std::vector<value_type> indexedList;
//           if(pairTable_->find(id, indexedList))
//               addDocument(id, indexedList, docsLimit, db, simCount);
//        }
//        start+=interval+1;
//
//    }
//
//    std::cout<<"end to construct from pair table"<<std::endl;
//
//    mutex_.lock_shared();
//    db_. reset();
//    std::string currentFileName=filePrefix_ + (useFirst_ ? "/similarity.idx.1" : "/similarity.idx.2");
//    std::remove(currentFileName.c_str());
//    db_=db;
//    db_->flush();
//
//    simCount_.reset();
//    currentFileName=filePrefix_ + (useFirst_ ? "/similarity_num.idx.1" : "/similarity_num.idx.2");
//    std::remove(currentFileName.c_str());
//    simCount_=simCount;
//    simCount_->commit();
//    mutex_.unlock_shared();
//
//    useFirst_=!useFirst_;
//    useFirstSdb_->update(0, useFirst_);
//    useFirstSdb_->flush();
//
//    boost::filesystem::remove_all(filePrefix_+"/pair.table.over");
//    boost::filesystem::remove_all(filePrefix_+"/pair.table.tbl");
//}


//void SimilarityIndex::constructPairTable(uint32_t numDocs)
//{
//    const uint32_t pruningThreshold=180;
//    std::cout<<"start to construct pair table."<<std::endl;
//    uint32_t start = 0;
//    uint32_t interval = 10000;
//    uint32_t count=1;
//    while(start<numDocs)
//    {
//        std::cout<<"\r";
//        std::cout<<"Interval: "<<count<<std::flush;
//        pairTable_->set_start(start);
//        pairTable_->set_end(start+interval);
//        start+= interval+1;
//        if(!sorter_->begin())
//        {
//            std::cout<<"No data in sorter!!!"<<std::endl;
//            return;
//        }
//
//        char* data=NULL;
//        float frequency=0;
//        uint8_t len;
//        uint32_t lastDoc1, lastDoc2;
//        lastDoc1=0;
//        lastDoc2=0;
//        while (sorter_->next_data(len, &data))
//        {
//            uint32_t doc1, doc2;
//            float weight;
//            convert(data, doc1, doc2, weight);
////            std::cout<<"!!!!("<<doc1<<" : "<<doc2<<") " << (uint32_t)frequency<< std::endl;
//            if(doc1!=lastDoc1||doc2!=lastDoc2)
//            {
//                  if(doc1!=0&&doc2!=0&&frequency>pruningThreshold)
//                 {
//                     pairTable_->add(lastDoc1, lastDoc2,(uint32_t)frequency);
//                     pairTable_->add(lastDoc2, lastDoc1, (uint32_t)frequency);
////                     std::cout<<"("<<lastDoc1<<" : "<<lastDoc2<<") " << (uint32_t)frequency<< std::endl;
//                 }
//                 frequency=0;
//                 lastDoc1=doc1;
//                 lastDoc2=doc2;
//            }
//            frequency+=weight;
//            free (data);
//        }
//        count++;
//        std::cout<<"start to flush"<<std::endl;
//        pairTable_->flush();
//    }
//    std::cout<<std::endl;
//
////    pairTable_->optimize(pruningThreshold);
////    std::cout<<"Number items in pair table: "<<pairTable_->num_items();
//
//    //clear the file of sorter;
//    sorter_->clear_files();
//    //release the memory of sorter;
//    delete sorter_;
//    sorter_=new izenelib::am::IzeneSort<uint32_t, uint8_t, true>((filePrefix_+"/pair.sort").c_str(), 100000000);
//
//    std::cout<<"end to construct pair table."<<std::endl;
//
//
//}
//
//void SimilarityIndex::constructWeightTable(uint32_t numDocs)
//{
//    std::cout<<"start to construct weight table."<<std::endl;
//    const uint32_t pruningThreshold=280;
//
//    if(!sorter_->begin())
//    {
//        std::cout<<"No data in sorter!!!"<<std::endl;
//        return;
//    }
//
//    char* data=NULL;
//    float frequency=0;
//    uint8_t len;
//    uint32_t lastDoc1, lastDoc2;
//    lastDoc1=0;
//    lastDoc2=0;
//    while (sorter_->next_data(len, &data))
//    {
//        uint32_t doc1, doc2;
//        float weight;
//        convert(data, doc1, doc2, weight);
////        std::cout<<"!!!!("<<doc1<<" : "<<doc2<<") " << (uint32_t)frequency<< std::endl;
//        if(doc1!=lastDoc1||doc2!=lastDoc2)
//        {
//              if(doc1!=0&&doc2!=0&&frequency>pruningThreshold)
//             {
//                 weightTable_.delta(make_pair(lastDoc1, lastDoc2),frequency);
//                 weightTable_.delta(make_pair(lastDoc2, lastDoc1),frequency);
////                 std::cout<<"("<<doc1<<" : "<<doc2<<") " << frequency<< std::endl;
//             }
//             frequency=0;
//             lastDoc1=doc1;
//             lastDoc2=doc2;
//        }
//        frequency+=weight;
//        free (data);
//    }
//    //clear the file of sorter;
//    sorter_->clear_files();
//    //release the memory of sorter;
//    delete sorter_;
//    sorter_=new izenelib::am::IzeneSort<uint32_t, uint8_t, true>((filePrefix_+"/pair.sort").c_str(), 100000000);
//    std::cout<<"end to construct weight table."<<std::endl;
//
//}


}} // namespace sf1r::sim
