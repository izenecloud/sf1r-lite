///
/// @file PropIdTableTestFixture.h
/// @brief fixture class to test PropIdTable functions.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-31
///

#ifndef SF1R_PROP_ID_TABLE_TEST_FIXTURE_H
#define SF1R_PROP_ID_TABLE_TEST_FIXTURE_H

#include <mining-manager/group-manager/PropIdTable.h>
#include <mining-manager/MiningException.hpp>
#include "../recommend-manager/test_util.h"
#include <boost/test/unit_test.hpp>

namespace sf1r
{

template <typename valueid_t, typename index_t>
class PropIdTableTestFixture
{
public:
    PropIdTableTestFixture();

    void appendIdList(const std::string& idListStr);

    void checkIdList();

    void checkOverFlow();

private:
    typedef faceted::PropIdTable<valueid_t, index_t> PropIdTable;
    typedef std::vector<valueid_t> InputIdList;

    void checkIdList_(
        docid_t docId,
        const typename PropIdTable::PropIdList& propIdList,
        const InputIdList& inputIdList
    );

    void checkOverFlow_(valueid_t valueId);

private:
    PropIdTable propIdTable_;

    std::vector<InputIdList> inputIdTable_;
};

template <typename valueid_t, typename index_t>
PropIdTableTestFixture<valueid_t, index_t>::PropIdTableTestFixture()
    : inputIdTable_(1)
{
}

template <typename valueid_t, typename index_t>
void PropIdTableTestFixture<valueid_t, index_t>::appendIdList(const std::string& idListStr)
{
    InputIdList inputIdList;
    split_str_to_items(idListStr, inputIdList);

    propIdTable_.appendIdList(inputIdList);
    inputIdTable_.push_back(inputIdList);
}

template <typename valueid_t, typename index_t>
void PropIdTableTestFixture<valueid_t, index_t>::checkIdList()
{
    typename PropIdTable::PropIdList propIdList;

    const std::size_t docNum = inputIdTable_.size();
    for (std::size_t docId = 0; docId < docNum; ++docId)
    {
        propIdTable_.getIdList(docId, propIdList);
        checkIdList_(docId, propIdList, inputIdTable_[docId]);
    }

    // when out of range, it should return an empty id list
    InputIdList inputIdList;
    propIdTable_.getIdList(docNum, propIdList);
    checkIdList_(docNum, propIdList, inputIdList);
}

template <typename valueid_t, typename index_t>
void PropIdTableTestFixture<valueid_t, index_t>::checkIdList_(
    docid_t docId,
    const typename PropIdTable::PropIdList& propIdList,
    const std::vector<valueid_t>& inputIdList
)
{
    BOOST_TEST_MESSAGE("check doc id: " << docId);
    BOOST_CHECK_EQUAL(propIdList.size(), inputIdList.size());

    std::size_t i = 0;
    for (; i < inputIdList.size(); ++i)
    {
        BOOST_TEST_MESSAGE(propIdList[i]);
        BOOST_CHECK_EQUAL(propIdList[i], inputIdList[i]);
    }

    // when out of range, it should return zero
    BOOST_CHECK_EQUAL(propIdList[i], 0U);
}

template <typename valueid_t, typename index_t>
void PropIdTableTestFixture<valueid_t, index_t>::checkOverFlow()
{
    index_t index_1 = -1; // 111...111
    index_t index_2 = (index_1 >> 1) + 1; // 100...000
    index_t index_3 = index_2 + 1; // 100...001

    InputIdList inputIdList;
    inputIdList.push_back(index_1);
    inputIdList.push_back(index_2);
    inputIdList.push_back(index_3);

    for (typename InputIdList::const_iterator it = inputIdList.begin();
        it != inputIdList.end(); ++it)
    {
        checkOverFlow_(*it);
    }

    propIdTable_.appendIdList(inputIdList);
    inputIdTable_.push_back(inputIdList);

    checkIdList();
}

template <typename valueid_t, typename index_t>
void PropIdTableTestFixture<valueid_t, index_t>::checkOverFlow_(valueid_t valueId)
{
    typedef MiningException ExpectException;

    InputIdList inputIdList;
    inputIdList.push_back(valueId);

    InputIdList goldList;

    BOOST_CHECK_THROW(propIdTable_.appendIdList(inputIdList), ExpectException);
    inputIdTable_.push_back(goldList);
}

} // namespace sf1r

#endif // SF1R_PROP_ID_TABLE_TEST_FIXTURE_H
