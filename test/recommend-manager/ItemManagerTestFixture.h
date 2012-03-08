///
/// @file ItemManagerTestFixture.h
/// @brief fixture class to test ItemManager
/// @author Jun Jiang
/// @date Created 2011-11-08
///

#ifndef ITEM_MANAGER_TEST_FIXTURE
#define ITEM_MANAGER_TEST_FIXTURE

#include <recommend-manager/common/RecTypes.h>
#include <configuration-manager/PropertyConfig.h>
#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <set>
#include <map>
#include <string>

namespace sf1r
{
class Document;
class ItemManager;
class DocumentManager;

class ItemManagerTestFixture
{
public:
    ItemManagerTestFixture();
    virtual ~ItemManagerTestFixture();

    virtual void setTestDir(const std::string& dir);

    virtual void resetInstance();

    void checkItemManager();

    void createItems(int num);

    void updateItems();

    void removeItems();

protected:
    void initDMSchema_();
    void createDocManager_();

    /** mapping from property name to UString value */
    typedef std::map<std::string, izenelib::util::UString> ItemInput;
    void checkItem_(const Document& doc, const ItemInput& itemInput);

    void prepareDoc_(
        itemid_t id,
        Document& doc,
        ItemInput& itemInput);

    virtual void insertItemId_(const std::string& strId, itemid_t goldId) {}

protected:
    IndexBundleSchema indexSchema_;
    std::vector<std::string> propList_;
    itemid_t maxItemId_;

    std::string testDir_;

    boost::shared_ptr<DocumentManager> documentManager_;
    boost::scoped_ptr<ItemManager> itemManager_;

    typedef std::map<itemid_t, ItemInput> ItemMap;
    ItemMap itemMap_;
};

} // namespace sf1r

#endif // ITEM_MANAGER_TEST_FIXTURE
