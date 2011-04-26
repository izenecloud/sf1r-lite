/**
 * @file RecommendSchema.h
 * @author Jun Jiang
 * @date 2011-04-22
 */

#ifndef RECOMMEND_SCHEMA_H
#define RECOMMEND_SCHEMA_H

#include <configuration-manager/PropertyConfig.h>

#include <vector>
#include <string>

namespace sf1r
{

/**
 * Confiugruration of recommend property.
 */
struct RecommendProperty : public PropertyConfigBase
{
    RecommendProperty()
        :isOption_(false)
    {}

    /**
     * if true, this property is optional, so that the record without this property value is valid,
     * if false, the record without this property value could not be added.
     */
    bool isOption_;
};

struct RecommendSchema
{
    std::vector<RecommendProperty> userSchema_;

    std::vector<RecommendProperty> itemSchema_;

    /**
     * Find user property by @p name.
     * @p name the property name in user schema
     * @p property the object store the result
     * @return true for found, false for not found
     */
    bool getUserProperty(const std::string& name, RecommendProperty& property) const;

    /**
     * Find item property by @p name.
     * @p name the property name in item schema
     * @p property the object store the result
     * @return true for found, false for not found
     */
    bool getItemProperty(const std::string& name, RecommendProperty& property) const;
};

} // namespace sf1r

#endif // RECOMMEND_SCHEMA_H
