/**
 * @file RecommendSchema.h
 * @author Jun Jiang
 * @date 2011-04-22
 */

#ifndef RECOMMEND_SCHEMA_H
#define RECOMMEND_SCHEMA_H

#include <configuration-manager/PropertyConfig.h>

#include <vector>
#include <set>
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

    /** the event names */
    std::set<std::string> eventSet_;
    /** the event name for not recommendation result */
    std::string notRecResultEvent_;
    /** the event name for not recommendation input */
    std::string notRecInputEvent_;

    RecommendSchema();

    /**
     * Find user property by @p name.
     * @param name the property name in user schema
     * @param property the object store the result
     * @return true for found, false for not found
     */
    bool getUserProperty(const std::string& name, RecommendProperty& property) const;

    /**
     * Find item property by @p name.
     * @param name the property name in item schema
     * @param property the object store the result
     * @return true for found, false for not found
     */
    bool getItemProperty(const std::string& name, RecommendProperty& property) const;

    /**
     * Whether @p event is one of @c eventSet_, @c notRecResultEvent_ or @c notRecInputEvent_.
     * @param event the event name
     * @return true for found, false for not found
     */
    bool hasEvent(const std::string& event) const;
};

} // namespace sf1r

#endif // RECOMMEND_SCHEMA_H
