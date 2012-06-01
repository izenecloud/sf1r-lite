///
/// @file GroupManagerTestFixture.h
/// @brief fixture class to test groupby functions.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-11-14
///

#ifndef SF1R_GROUP_MANAGER_TEST_FIXTURE_H
#define SF1R_GROUP_MANAGER_TEST_FIXTURE_H

#include <mining-manager/group-manager/GroupParam.h>
#include <mining-manager/group-manager/GroupRep.h>
#include <configuration-manager/PropertyConfig.h>

#include <string>
#include <vector>
#include <map>

namespace sf1r
{

class NumericPropertyTableBuilderStub;
class DocumentManager;
class GroupConfig;

namespace faceted
{
class GroupManager;
}

class GroupManagerTestFixture
{
public:
    struct DocInput
    {
        unsigned int docId_;
        std::string title_;
        std::string groupStr_;
        int groupInt_;
        float groupFloat_;

        DocInput();
        DocInput(
            unsigned int docId,
            const std::string& groupStr,
            int groupInt,
            float groupFloat
        );
    };

    GroupManagerTestFixture();

    ~GroupManagerTestFixture();

    void resetGroupManager();

    void createDocument(int num);

    void checkGetGroupRep();

    void checkGroupRepMerge();

private:
    void initConfig_();

    void checkCollection_();

    void createAndCheckGroupRep_(const faceted::GroupParam::GroupLabelMap& labels);

    void createGroupRep_(
        const faceted::GroupParam::GroupLabelMap& labels,
        faceted::GroupRep& groupRep
    );

    typedef std::vector<unsigned int> DocIdList;
    typedef std::map<std::string, DocIdList> DocIdMap; // key: property value
    typedef std::map<std::string, DocIdMap> PropertyMap; // key: property name

    /**
    * @param labels the label list, it is assumed that each path contains only one element.
    */
    void createPropertyMap_(
        const faceted::GroupParam::GroupLabelMap& labels,
        PropertyMap& propertyMap
    ) const;

    void checkGroupRep_(
        const faceted::GroupRep& groupRep,
        PropertyMap& propertyMap
    ) const;

protected:
    set<PropertyConfig, PropertyComp> schema_;
    std::vector<std::string> propNames_;
    std::vector<GroupConfig> groupConfigs_;

    DocumentManager* documentManager_;
    std::vector<DocInput> docInputVec_;
    std::vector<unsigned int> docIdList_;

    std::string groupPath_;
    faceted::GroupManager* groupManager_;
    NumericPropertyTableBuilderStub* numericTableBuilder_;
};

}

#endif 
