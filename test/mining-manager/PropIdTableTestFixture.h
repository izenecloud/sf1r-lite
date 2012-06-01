///
/// @file PropIdTableTestFixture.h
/// @brief fixture class to test PropIdTable functions.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-31
///

#ifndef SF1R_PROP_ID_TABLE_TEST_FIXTURE_H
#define SF1R_PROP_ID_TABLE_TEST_FIXTURE_H

#include <mining-manager/group-manager/PropIdTable.h>

namespace sf1r
{

class PropIdTableTestFixture
{
public:
    PropIdTableTestFixture();

    void appendIdList(const std::string& idListStr);

    void checkIdList();

private:
    typedef uint16_t valueid_t;
    typedef uint32_t index_t;
    typedef faceted::PropIdTable<valueid_t, index_t> PropIdTable;
    typedef std::vector<valueid_t> InputIdList;

    void checkIdList_(
        docid_t docId,
        const PropIdTable::PropIdList& propIdList,
        const InputIdList& inputIdList
    );

private:
    PropIdTable propIdTable_;

    std::vector<InputIdList> inputIdTable_;
};

} // namespace sf1r

#endif // SF1R_PROP_ID_TABLE_TEST_FIXTURE_H
