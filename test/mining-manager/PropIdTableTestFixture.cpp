#include "PropIdTableTestFixture.h"
#include "../recommend-manager/test_util.h"
#include <boost/test/unit_test.hpp>

namespace sf1r
{

PropIdTableTestFixture::PropIdTableTestFixture()
    : inputIdTable_(1)
{
}

void PropIdTableTestFixture::appendIdList(const std::string& idListStr)
{
    InputIdList inputIdList;
    split_str_to_items(idListStr, inputIdList);

    propIdTable_.appendIdList(inputIdList);
    inputIdTable_.push_back(inputIdList);
}

void PropIdTableTestFixture::checkIdList()
{
    PropIdTable::PropIdList propIdList;

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

void PropIdTableTestFixture::checkIdList_(
    docid_t docId,
    const PropIdTable::PropIdList& propIdList,
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

} // namespace sf1r
