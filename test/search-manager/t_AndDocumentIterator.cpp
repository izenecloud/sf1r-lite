/**
 * @author Yingfeng Zhang
 * @date 2010-08-13
 * @update Hongliang Zhao
 * @date 2012-12-27
 */

#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <search-manager/ANDDocumentIterator.h>
#include <search-manager/VirtualTermDocumentIterator.h>
#include <search-manager/TermDocumentIterator.h>
#include <search-manager/ORDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( ANDDocumentIterator_Suite )

///DocumentIterator for one term in one property
class MockTermDocumentIterator: public DocumentIterator
{
public:
    MockTermDocumentIterator(
            unsigned int termid,
            unsigned int colID,
            izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader,
            const std::string& property,
            unsigned int propertyId
    )
        : termId_(termid)
        , colID_(colID)
        , property_(property)
        , propertyId_(propertyId)
        , pIndexReader_(pIndexReader)
        , pTermReader_(0)
        , pTermDocReader_(0)
    {
        accept();
    }

    ~MockTermDocumentIterator()
    {
        if(pTermReader_)
            delete pTermReader_;
        if(pTermDocReader_)
            delete pTermDocReader_;
    }

public:
    void add(DocumentIterator* pDocIterator){}

    void accept()
    {
        Term term(property_.c_str(), termId_);

        pTermReader_ = (MockTermReader*)pIndexReader_->getTermReader(colID_);
        bool find = pTermReader_->seek(&term);
        if (find)
        {
            pTermDocReader_ = (MockTermDocFreqs*)pTermReader_->termPositions();
        }
        else
            cout<<"No term..."<<endl;
    }

    bool next()
    {
        if (pTermDocReader_)
            return pTermDocReader_->next();
        return false;
    }

    unsigned int doc()
    {
        BOOST_ASSERT(pTermDocReader_);
        return pTermDocReader_->doc();
    }

    unsigned int skipTo(unsigned int target)
    {
        return pTermDocReader_->skipTo(target);
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0){}

    void df_cmtf(DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {}

    izenelib::ir::indexmanager::count_t tf() 
    {
        BOOST_ASSERT(pTermDocReader_);
        return pTermDocReader_->freq();
    }

private:
    unsigned int termId_;

    unsigned int colID_;

    std::string property_;

    unsigned int propertyId_;

    izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader_;

    izenelib::ir::indexmanager::MockTermReader* pTermReader_;

    izenelib::ir::indexmanager::MockTermDocFreqs* pTermDocReader_;

    unsigned int currDoc_;

};

class MockVirtualTermDocumentIterator: public DocumentIterator
{//(1, 1, &indexer, subProperties, propertyIdList, dataTypeList, 1);
public:
    MockVirtualTermDocumentIterator(
        unsigned int termid,
        unsigned int colID,
        izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader,
        const std::vector<std::string>& virtualPropreties,
        std::vector<unsigned int>& propertyIdList,
        std::vector<sf1r::PropertyDataType>& dataTypeList,
        unsigned int termIndex
    ):termId_(termid)
    , colID_(colID)
    , pIndexReader_(pIndexReader)
    , pTermReader_(0)
    , subProperties_(virtualPropreties)
    , propertyIdList_(propertyIdList)
    , dataTypeList_(dataTypeList)
    , termIndex_(termIndex)
    {

    }

    ~MockVirtualTermDocumentIterator()
    {
        if (pTermReader_)
        {
            delete pTermReader_;
        }
        for (uint32_t i = 0; i < pTermDocReaderList_.size(); ++i)
        {
            if (pTermDocReaderList_[i])
            {
                delete pTermDocReaderList_[i];
            }
        }
    }

    void add(VirtualPropertyTermDocumentIterator* pDocIterator) {};

    void setPropertiesAndIDs(std::vector<std::string> virtualPropreties, std::vector<unsigned int> propertyIdList)
    {
        pIndexReader_->setPropertiesAndIDs(virtualPropreties, propertyIdList);
    }

    unsigned int tf()
    {
        unsigned int freq = 0;
        for (uint32_t i = 0; i < pTermDocReaderList_.size(); ++i)
        {
            if ( pTermDocReaderList_[i])
            {
                freq += pTermDocReaderList_[i]->freq();
            }
        }
        return freq;
    }

    bool next()
    {
        return OrDocIterator_->next();
    }

    unsigned int doc()
    {
        return OrDocIterator_->doc();
    }

    bool isCurrent()
    {
        return OrDocIterator_->isCurrent();
    }

    void add(DocumentIterator* pDocIterator) 
    {
        OrDocIterator_ = pDocIterator;
    }

    void accept()
    {
        for (uint32_t i = 0; i < subProperties_.size(); ++i)
        {
            pTermDocReaderList_.push_back(0);
        }
        for (uint32_t i = 0; i < subProperties_.size(); ++i)
        {
            Term term(subProperties_[i].c_str(), termId_);
            if(!pTermReader_)
                pTermReader_ = (MockTermReader*)pIndexReader_->getTermReader(colID_);
            if (!pTermReader_)
                return;
            bool find = pTermReader_->seek(&term);
            if (find)
            {
                df_ += pTermReader_->docFreq(&term);
                if(pTermDocReaderList_[i]) 
                {
                    delete pTermDocReaderList_[i];
                    pTermDocReaderList_[i] = 0;
                }

                pTermDocReaderList_[i] = (MockTermDocFreqs*)pTermReader_->termDocFreqs();
                if(!pTermDocReaderList_[i]) find = false;
            }
            else
            {
                delete pTermReader_;
                pTermReader_ = 0;
            }
        }
    }

    void doc_item(
        RankDocumentProperty& rankDocumentProperty,
        unsigned propIndex = 0)
    {
        unsigned int docLength = 0;
        unsigned int docid = doc();
        for (unsigned int i = 0; i < subProperties_.size(); ++i)
        {
            docLength += pIndexReader_->docLength(docid, propertyIdList_[i]);
        }
        rankDocumentProperty.setDocLength(docLength);

        unsigned int freq = 0;
        for (unsigned int i = 0; i < reinterpret_cast<ORDocumentIterator*>(OrDocIterator_)->getdocIteratorList_().size(); ++i)
        {
            sf1r::DocumentIterator* pEntry = reinterpret_cast<ORDocumentIterator*>(OrDocIterator_)->getdocIteratorList_()[i];
            if (pEntry->doc() == docid)
            {
                cout<<(reinterpret_cast<MockTermDocumentIterator*>(pEntry))->tf()<<endl;
                freq += (reinterpret_cast<MockTermDocumentIterator*>(pEntry))->tf();
            }
        }
        rankDocumentProperty.setTermFreq(termIndex_, freq);
    }

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap)
    {
        for (unsigned int i = 0; i < pTermDocReaderList_.size(); ++i)
        {
            if (pTermDocReaderList_[i])
            {
                ID_FREQ_MAP_T& df = dfmap[subProperties_[i]];
                df[termId_] = df[termId_] + (float)pTermDocReaderList_[i]->docFreq();

                ID_FREQ_MAP_T& ctf = ctfmap[subProperties_[i]];
                ctf[termId_] = ctf[termId_] + (float)pTermDocReaderList_[i]->getCTF();

                ID_FREQ_MAP_T& maxtf = maxtfmap[subProperties_[i]];
                maxtf[termId_] = maxtf[termId_] + (float)pTermDocReaderList_[i]->getMaxTF();
            }
        }
    }
private:
    unsigned int termId_;

    unsigned int colID_;

    izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader_;

    izenelib::ir::indexmanager::MockTermReader* pTermReader_;

    std::vector<izenelib::ir::indexmanager::MockTermDocFreqs*> pTermDocReaderList_;

    const std::vector<std::string> subProperties_;
 
    std::vector<unsigned int> propertyIdList_;

    std::vector<sf1r::PropertyDataType> dataTypeList_;

    unsigned int termIndex_;

    unsigned int currDoc_;

    unsigned int df_;

    DocumentIterator* OrDocIterator_;
};

BOOST_AUTO_TEST_CASE(and_test)
{
    MockIndexReaderWriter indexer;
    //izenelib::ir::indexmanager::IndexWriter indexer1;
    indexer.insertDoc(0, "title", "1 2 3 4 5 6");
    indexer.insertDoc(1, "title", "1 2 3 4 5 8 9 10");
    indexer.insertDoc(2, "title", "1 3 5 6 7 8 9 11 11");
    indexer.insertDoc(3, "title", "3 9 10 10 10 13 14 15 16 10");

    indexer.insertDoc(4, "title", "1 3 4 6");
    indexer.insertDoc(5, "title", "1 3 5 6 11 24");
    indexer.insertDoc(6, "title", "10 18 19");
    indexer.insertDoc(7, "title", "1 3 5 8 11");
    indexer.insertDoc(8, "title", "1 5 15 19");
    indexer.insertDoc(9, "title", "1 9 15 11 18");
    indexer.insertDoc(10, "title", "3 6 8 15");
    indexer.insertDoc(11, "title", "1 5 9 11 18");
    indexer.insertDoc(12, "title", "1 7 8 9 10 12");
    indexer.insertDoc(13, "title", "3 6 7 8 9 10 12");
    indexer.insertDoc(14, "title", "1 2 3 5 15 19");
    indexer.insertDoc(15, "title", "5 15 19");

    indexer.insertDoc(16, "title", "1 3 4 6");
    indexer.insertDoc(17, "title", "1 3 6 24");
    indexer.insertDoc(18, "title", "10 18 19");
    indexer.insertDoc(19, "title", "1 3 8 11");
    indexer.insertDoc(20, "title", "5 15 19");
    indexer.insertDoc(21, "title", "1 9 15 18");
    indexer.insertDoc(22, "title", "1 3 6 8 15");
    indexer.insertDoc(23, "title", "1 5 9 18");
    indexer.insertDoc(24, "title", "7 8 9 10 12");
    indexer.insertDoc(25, "title", "1 3 6 7 8 9 10 12");
    indexer.insertDoc(26, "title", "1 2 3 5 15 19");
    indexer.insertDoc(27, "title", "5 15 19");
    indexer.insertDoc(28, "title", "5 15 19");
    indexer.insertDoc(29, "title", "5 15 19");

    indexer.insertDoc(3, "Source", "5 9"); // 5 - 11
    indexer.insertDoc(5, "Source", "5 6 8 11");
    indexer.insertDoc(7, "Source", "7 10");
    indexer.insertDoc(9, "Source", "6 10 11");
    indexer.insertDoc(11, "Source", "10 11");
    indexer.insertDoc(13, "Source", "5 6 7 8 9 10");

    indexer.insertDoc(3, "Cat", "8 15");// 8- 15
    indexer.insertDoc(6, "Cat", "12 13");
    indexer.insertDoc(7, "Cat", "14");
    indexer.insertDoc(9, "Cat", "11 12");
    indexer.insertDoc(10, "Cat", "9 11");
    indexer.insertDoc(11, "Cat", "11 13");
    indexer.insertDoc(13, "Cat", "12 14");
    indexer.insertDoc(15, "Cat", "11 12 13 14 15");

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(2, 1, &indexer,"title",1);
        iter.add(pTermDocIterator2);
        //BOOST_CHECK_EQUAL(iter.next(), true );
        //BOOST_CHECK_EQUAL(iter.doc(), 1U);
        //BOOST_CHECK_EQUAL(iter.next(), true );
        //BOOST_CHECK_EQUAL(iter.doc(), 1U);
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter.add(pTermDocIterator2);
        MockTermDocumentIterator* pTermDocIterator3 = new MockTermDocumentIterator(5, 1, &indexer,"title",1);
        iter.add(pTermDocIterator3);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(16, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 8U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 9U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 11U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 12U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 21U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 23U);
        BOOST_CHECK_EQUAL(iter.next(), false );

        ANDDocumentIterator iter1;
        MockTermDocumentIterator* pTermDocIterator3 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter1.add(pTermDocIterator3);
        MockTermDocumentIterator* pTermDocIterator4 = new MockTermDocumentIterator(5, 1, &indexer,"title",1);
        pTermDocIterator4->setNot(true);
        iter1.add(pTermDocIterator4);

        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 3U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 4U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 10U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 13U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 16U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 17U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 19U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 22U);
        BOOST_CHECK_EQUAL(iter1.next(), true );
        BOOST_CHECK_EQUAL(iter1.doc(), 25U);
        BOOST_CHECK_EQUAL(iter1.next(), false );
    }

    {
        std::vector<std::string> subProperties;
        subProperties.push_back("title");
        subProperties.push_back("Source");
        subProperties.push_back("Cat");

        std::vector<unsigned int> propertyIdList;
        propertyIdList.push_back(1);
        propertyIdList.push_back(2);
        propertyIdList.push_back(3);

        std::vector<sf1r::PropertyDataType> dataTypeList;
        dataTypeList.push_back(STRING_PROPERTY_TYPE);
        dataTypeList.push_back(STRING_PROPERTY_TYPE);
        dataTypeList.push_back(STRING_PROPERTY_TYPE);

        MockVirtualTermDocumentIterator * pVtermIterator1 = new MockVirtualTermDocumentIterator(11, 1, &indexer, subProperties, propertyIdList, dataTypeList, 0);
        pVtermIterator1->setPropertiesAndIDs(subProperties, propertyIdList);
        MockVirtualTermDocumentIterator * pVtermIterator2 = new MockVirtualTermDocumentIterator(11, 1, &indexer, subProperties, propertyIdList, dataTypeList, 0);
        pVtermIterator2->setPropertiesAndIDs(subProperties, propertyIdList);

        MockTermDocumentIterator* pTermDocIterator11 = new MockTermDocumentIterator(11, 1, &indexer, "title", 1);
        pTermDocIterator11->accept();
        MockTermDocumentIterator* pTermDocIterator12 = new MockTermDocumentIterator(11, 1, &indexer, "title", 1);
        pTermDocIterator12->accept();

        MockTermDocumentIterator* pTermDocIterator21 = new MockTermDocumentIterator(11, 1, &indexer, "Source", 2);
        pTermDocIterator21->accept();
        MockTermDocumentIterator* pTermDocIterator22 = new MockTermDocumentIterator(11, 1, &indexer, "Source", 2);
        pTermDocIterator22->accept();

        MockTermDocumentIterator* pTermDocIterator31 = new MockTermDocumentIterator(11, 1, &indexer, "Cat", 3);
        pTermDocIterator31->accept();
        MockTermDocumentIterator* pTermDocIterator32 = new MockTermDocumentIterator(11, 1, &indexer, "Cat", 3);
        pTermDocIterator32->accept();

        ORDocumentIterator* pOrDocumentIterator1 = new ORDocumentIterator();
        pOrDocumentIterator1->add(pTermDocIterator11);
        pOrDocumentIterator1->add(pTermDocIterator21);
        pOrDocumentIterator1->add(pTermDocIterator31);
        pVtermIterator1->add(pOrDocumentIterator1);
        pVtermIterator1->accept();

        /**
        @Brief  check VirtualTermDocumentIterator
        */
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 2U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 5U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 7U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 9U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 10U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 11U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 15U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), true );
        BOOST_CHECK_EQUAL(pVtermIterator1->doc(), 19U);
        BOOST_CHECK_EQUAL(pVtermIterator1->next(), false );

        /**
        @Brief  check doc_item, rankDocumentPropety
        */
        ORDocumentIterator* pOrDocumentIterator2 = new ORDocumentIterator();
        pOrDocumentIterator2->add(pTermDocIterator12);
        pOrDocumentIterator2->add(pTermDocIterator22);
        pOrDocumentIterator2->add(pTermDocIterator32);
        pVtermIterator2->add(pOrDocumentIterator2);
        pVtermIterator2->accept();

        RankDocumentProperty rankDocumentProperty;
        rankDocumentProperty.resize_and_initdata(1);
        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//2
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 2U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 9U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//5
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 2U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 10U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//7
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 1U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 8U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//9
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 3U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 10U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//10
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 1U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 6U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//11
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 3U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 9U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//15
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 1U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 8U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), true );//19
        pVtermIterator2->doc_item(rankDocumentProperty);
        BOOST_CHECK_EQUAL(rankDocumentProperty.termFreqAt(0), 1U);
        BOOST_CHECK_EQUAL(rankDocumentProperty.docLength(), 4U);

        BOOST_CHECK_EQUAL(pVtermIterator2->next(), false );//false

        /**
        @Brief  Test rankQuery and integration test; 
        */
        MockTermDocumentIterator* pTermDocIterator_11 = new MockTermDocumentIterator(3, 1, &indexer, "title", 1);
        pTermDocIterator_11->accept();
        MockTermDocumentIterator* pTermDocIterator_12 = new MockTermDocumentIterator(8, 1, &indexer, "title", 1);
        pTermDocIterator_12->accept();
        MockTermDocumentIterator* pTermDocIterator_13 = new MockTermDocumentIterator(11, 1, &indexer, "title", 1);
        pTermDocIterator_13->accept();

        MockTermDocumentIterator* pTermDocIterator_21 = new MockTermDocumentIterator(3, 1, &indexer, "Source", 2);
        pTermDocIterator_21->accept();
        MockTermDocumentIterator* pTermDocIterator_22 = new MockTermDocumentIterator(8, 1, &indexer, "Source", 2);
        pTermDocIterator_22->accept();
        MockTermDocumentIterator* pTermDocIterator_23 = new MockTermDocumentIterator(11, 1, &indexer, "Source", 2);
        pTermDocIterator_23->accept();

        MockTermDocumentIterator* pTermDocIterator_31 = new MockTermDocumentIterator(3, 1, &indexer, "Cat", 3);
        pTermDocIterator_31->accept();
        MockTermDocumentIterator* pTermDocIterator_32 = new MockTermDocumentIterator(8, 1, &indexer, "Cat", 3);
        pTermDocIterator_32->accept();
        MockTermDocumentIterator* pTermDocIterator_33 = new MockTermDocumentIterator(11, 1, &indexer, "Cat", 3);
        pTermDocIterator_33->accept();

        MockVirtualTermDocumentIterator * pVtermIterator3 = new MockVirtualTermDocumentIterator(3, 1, &indexer, subProperties, propertyIdList, dataTypeList, 0);
        pVtermIterator3->setPropertiesAndIDs(subProperties, propertyIdList);
        MockVirtualTermDocumentIterator * pVtermIterator8 = new MockVirtualTermDocumentIterator(8, 1, &indexer, subProperties, propertyIdList, dataTypeList, 0);
        pVtermIterator8->setPropertiesAndIDs(subProperties, propertyIdList);
        MockVirtualTermDocumentIterator * pVtermIterator11 = new MockVirtualTermDocumentIterator(11, 1, &indexer, subProperties, propertyIdList, dataTypeList, 0);
        pVtermIterator11->setPropertiesAndIDs(subProperties, propertyIdList);

        ORDocumentIterator* pOrDocumentIterator3 = new ORDocumentIterator();
        ORDocumentIterator* pOrDocumentIterator8 = new ORDocumentIterator();
        ORDocumentIterator* pOrDocumentIterator11 = new ORDocumentIterator();

        pOrDocumentIterator3->add(pTermDocIterator_11);
        pOrDocumentIterator3->add(pTermDocIterator_21);
        pOrDocumentIterator3->add(pTermDocIterator_31);

        pOrDocumentIterator8->add(pTermDocIterator_12);
        pOrDocumentIterator8->add(pTermDocIterator_22);
        pOrDocumentIterator8->add(pTermDocIterator_32);

        pOrDocumentIterator11->add(pTermDocIterator_13);
        pOrDocumentIterator11->add(pTermDocIterator_23);
        pOrDocumentIterator11->add(pTermDocIterator_33);

        pVtermIterator3->add(pOrDocumentIterator3);
        pVtermIterator8->add(pOrDocumentIterator8);
        pVtermIterator11->add(pOrDocumentIterator11);

        ANDDocumentIterator ANDVirtualTerm;
        ANDVirtualTerm.add(pVtermIterator3);
        ANDVirtualTerm.add(pVtermIterator8);
        ANDVirtualTerm.add(pVtermIterator11);

        DocumentFrequencyInProperties dfmap;
        CollectionTermFrequencyInProperties ctfmap;
        MaxTermFrequencyInProperties maxtfmap;
        
        ANDVirtualTerm.df_cmtf(dfmap, ctfmap, maxtfmap);

        
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), true );//2
        BOOST_CHECK_EQUAL(ANDVirtualTerm.doc(), 2U);
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), true );//5
        BOOST_CHECK_EQUAL(ANDVirtualTerm.doc(), 5U);
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), true );//7
        BOOST_CHECK_EQUAL(ANDVirtualTerm.doc(), 7U);
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), true );//10
        BOOST_CHECK_EQUAL(ANDVirtualTerm.doc(), 10U);
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), true );//19
        BOOST_CHECK_EQUAL(ANDVirtualTerm.doc(), 19U);
        BOOST_CHECK_EQUAL(ANDVirtualTerm.next(), false );//false
    }
}

BOOST_AUTO_TEST_SUITE_END()
