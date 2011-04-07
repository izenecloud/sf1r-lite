///
/// @file prop_value_table.h
/// @brief a table contains below things for a specific property:
///        1. mapping between property UString value and id
///        2. mapping from doc id to property value id
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-24
///

#ifndef SF1R_PROP_VALUE_TABLE_H_
#define SF1R_PROP_VALUE_TABLE_H_

#include <common/inttypes.h>
#include <util/ustring/UString.h>
#include "faceted_types.h"

#include <vector>
#include <string>
#include <map>

NS_FACETED_BEGIN

class PropValueTable
{
public:
    /**
     * property value id type.
     * as 0 is reserved as invalid id, meaning no property value is availabe,
     * the valid id range is [1, 2^16) (65535 ids)
     */
    typedef uint16_t pvid_t;

    PropValueTable(
        const std::string& dirPath,
        const std::string& propName
    );

    bool open();

    bool flush();

    const char* propName() const {
        return propName_.c_str();
    }

    std::vector<pvid_t>& propIdTable() {
        return propIdVec_;
    }

    const std::vector<pvid_t>& propIdTable() const {
        return propIdVec_;
    }

    const izenelib::util::UString& propValueStr(pvid_t pvId) const {
        return propStrVec_[pvId];
    }

    pvid_t propValueId(const izenelib::util::UString& value);

    pvid_t propValueId(const izenelib::util::UString& value) const {
        pvid_t pvId = 0;

        std::map<izenelib::util::UString, pvid_t>::const_iterator it = propStrMap_.find(value);
        if (it != propStrMap_.end())
        {
            pvId = it->second; 
        }

        return pvId;
    }

private:
    /** directory path */
    std::string dirPath_;

    /** property name */
    std::string propName_;

    /** mapping from property value string to value id */
    std::map<izenelib::util::UString, pvid_t> propStrMap_;

    /** mapping from property value id to value string */
    std::vector<izenelib::util::UString> propStrVec_;

    /** the number of value strings saved in file */
    unsigned int savePropStrNum_;

    /** mapping from doc id to property value id */
    std::vector<pvid_t> propIdVec_;

    /** the number of value ids saved in file */
    unsigned int savePropIdNum_;
};

NS_FACETED_END

#endif 
