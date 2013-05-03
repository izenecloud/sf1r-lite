#include <boost/test/unit_test.hpp>

#include <index-manager/IndexManager.h>
#include <search-manager/AllDocumentIterator.h>
#include <algorithm> // std::set_difference

using namespace sf1r;

namespace
{

static const sf1r::docid_t MAX_DOCID = 100;

static const sf1r::docid_t DELETE_DOCIDS[] = {
    1, 2, 3, 4, 5, 6,
    10, 20, 30, 33, 40, 50,
    100
};

static const std::size_t DELETE_DOCIDS_NUM =
    sizeof(DELETE_DOCIDS) / sizeof(DELETE_DOCIDS[0]);

AllDocumentIterator* createAllDocumentIterator()
{
    boost::shared_ptr<IndexManager::FilterBitmapT> pFilterIdSet(
        new IndexManager::FilterBitmapT);

    for (std::size_t i = 0; i < DELETE_DOCIDS_NUM; ++i)
    {
        pFilterIdSet->set(DELETE_DOCIDS[i]);
    }

    IndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs =
        new IndexManager::FilterTermDocFreqsT(pFilterIdSet);

    return new AllDocumentIterator(pFilterTermDocFreqs, MAX_DOCID);

}

}

BOOST_AUTO_TEST_SUITE(AllDocumentIterator_Suite)

BOOST_AUTO_TEST_CASE(testSkipTo)
{
    const sf1r::docid_t skipTestData[][2] = {
        {2, 7}, {3, 7}, {4, 7}, {5, 7}, {6, 7},
        {8, 9}, {9, 11}, {10, 11}, {20, 21},
        {30, 31}, {40, 41}, {50, 51}, {60, 61},
        {98, 99}, {99, -1}, {100, -1}
    };
    const int num = sizeof(skipTestData) / sizeof(skipTestData[0]);

    AllDocumentIterator* pIterator = createAllDocumentIterator();

    for (int i = 0; i < num; ++i)
    {
        sf1r::docid_t target = skipTestData[i][0];
        sf1r::docid_t gold = skipTestData[i][1];
        BOOST_TEST_MESSAGE("target: " << target << ", gold: " << gold);

        BOOST_CHECK_EQUAL(pIterator->skipTo(target), gold);
    }

    delete pIterator;
}

BOOST_AUTO_TEST_CASE(testNext)
{
    std::vector<sf1r::docid_t> allDocIds;
    for (sf1r::docid_t docId = 1; docId <= MAX_DOCID; ++docId)
    {
        allDocIds.push_back(docId);
    }

    std::vector<sf1r::docid_t> existDocIds(allDocIds.size() - DELETE_DOCIDS_NUM);
    std::set_difference(allDocIds.begin(), allDocIds.end(),
                        DELETE_DOCIDS, DELETE_DOCIDS + DELETE_DOCIDS_NUM,
                        existDocIds.begin());

    AllDocumentIterator* pIterator = createAllDocumentIterator();

    for (std::vector<sf1r::docid_t>::const_iterator it = existDocIds.begin();
         it != existDocIds.end(); ++it)
    {
        BOOST_CHECK(pIterator->next());
        BOOST_CHECK_EQUAL(pIterator->doc(), *it);
        BOOST_TEST_MESSAGE("next doc: " << pIterator->doc());
    }

    delete pIterator;
}

BOOST_AUTO_TEST_SUITE_END()
