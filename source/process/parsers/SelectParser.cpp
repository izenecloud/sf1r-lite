/**
 * @file process/parsers/SelectParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 16:33:08>
 */
#include "SelectParser.h"

#include <common/BundleSchemaHelpers.h>

#include <common/Keys.h>

#include <query-manager/ActionItem.h>

#include <util/swap.h>

namespace sf1r {

using driver::Keys;

const std::string SelectParser::kDefaultSummaryPropertySuffix = ".summary";

/**
 * @class SelectParser
 *
 * The @b select field is an array. It specifies which properties should be
 * included in the response and how the property value is formatted.
 *
 * Every item is an object of following fields:
 * - @b property* (@c String): Name of the property that should be in the
 *   response.
 * - @b highlight (@c Bool = @c false): Whether highlight the search keywords in
 *   value. It is only valid if search keywords is specified in request message.
 * - @b snippet (@c Bool = @c false): Whether snippet is returned instead of
 *   full text.
 * - @b summary (@c Bool = @c false): Whether an extra field is returned
 *   containing the summary of this property.
 * - @b summary_setence_count (@c Uint = @ref kDefaultSummarySentenceCount): Max
 *   count of sentences in summary.
 * - @b summary_property_alias (@c String): Alias of the extra summary
 *   property. By default, the name is original property name plus suffix
 *   @ref kDefaultSummaryPropertySuffix.
 * - @b split_property_value (@c Bool = @c false): Whether split the property
 *   value into multiple values.
 *
 * Specially, if the item is of type String, it is considered as an Object with
 * only one key @b property with the specified value. I.e., \c "title" is
 * identical with \c {"property":"title"}
 *
 * If @b select is an empty array, only internal properties such as _id are
 * returned in response.
 *
 * If @b select is not specified, all properties specified in document schema in
 * configuration file are used. The valid properties can be check by schema/get
 * (SchemaController::get() ).
 *
 * If ACL_ALLOW and ACL_DENY exist in document schema, the two fields are added
 * so that they are always returned in response.
 *
 * If @b split_property_value is specified as @c yes,@n
 * - if the @b property is configured in @c <MiningBundle><Schema><Group>,@n
 * given the original property value such as:
 * @code
 * "数码>手机通讯>手机,苹果商城>iPhone"
 * @endcode
 * the splitted property value would be:
 * @code
 * [
 *   ["数码", "手机通讯", "手机"],
 *   ["苹果商城", "iPhone"],
 * ]
 * @endcode
 * - if the @b property is configured in @c <MiningBundle><Schema><Attr>,@n
 * given the original property value such as:
 * @code
 * "领子:圆领,尺码:S|M|L|XL"
 * @endcode
 * the splitted property value would be:
 * @code
 * [
 *   {"attr_name": "领子", "attr_values": ["圆领"]},
 *   {"attr_name": "尺码", "attr_values": ["S", "M", "L", "XL"]}
 * ]
 * @endcode
 * - in other cases, the property value would not be splitted.
 */
bool SelectParser::parse(const Value& select)
{
    clearMessages();

    if (nullValue(select))
    {
        // use default properties
        std::vector<std::string> defaultSelectProperties;
        getDefaultSelectPropertyNames(indexSchema_, defaultSelectProperties);

        properties_.resize(defaultSelectProperties.size());
        for (std::size_t i = 0; i < defaultSelectProperties.size(); ++i)
        {
            properties_[i].propertyString_ = defaultSelectProperties[i];
        }

        return true;
    }

    if (select.type() != Value::kArrayType)
    {
        error() = "Select must be an array";
        return false;
    }

    std::size_t propertyCount = select.size();
    // reserve 2 spaces for ACL fields
    properties_.reserve(propertyCount + 2);
    properties_.resize(propertyCount);

    PropertyConfig config;

    // flags to remember whether ACL flags should be added
    bool shouldAddAclAllow = getPropertyConfig(indexSchema_, "ACL_ALLOW", config)
                             && config.isIndex()
                             && config.getIsFilter()
                             && config.getIsMultiValue();
    bool shouldAddAclDeny = getPropertyConfig(indexSchema_, "ACL_DENY", config)
                            && config.isIndex()
                            && config.getIsFilter()
                            && config.getIsMultiValue();

    for (std::size_t i = 0; i < propertyCount; ++i)
    {
        // TODO: Check the name? Or let SIA do it.
        const Value& currentProperty = select(i);

        // default values
        properties_[i].isHighlightOn_ = false;
        properties_[i].isSummaryOn_ = false;
        properties_[i].summarySentenceNum_ = kDefaultSummarySentenceCount;
        properties_[i].summaryPropertyAlias_.resize(0);
        properties_[i].isSnippetOn_ = false;

        if (currentProperty.type() != Value::kObjectType)
        {
            properties_[i].propertyString_ = asString(currentProperty);
        }
        else
        {
            properties_[i].propertyString_ = asString(currentProperty[Keys::property]);
            properties_[i].isHighlightOn_ = asBool(currentProperty[Keys::highlight]);
            properties_[i].isSplitPropertyValue_ =
                asBool(currentProperty[Keys::split_property_value]);
            properties_[i].isSummaryOn_ = asBool(currentProperty[Keys::summary]);
            if (properties_[i].isSummaryOn_)
            {
                properties_[i].summarySentenceNum_ = asUintOr(
                    currentProperty[Keys::summary_sentence_count],
                    kDefaultSummarySentenceCount
                );
                properties_[i].summaryPropertyAlias_ = asString(
                    currentProperty[Keys::summary_property_alias]
                );
            }
            properties_[i].isSnippetOn_ = asBool(currentProperty[Keys::snippet]);
        }

        if (properties_[i].propertyString_.empty())
        {
            error() = "Require property name in select.";
            return false;
        }
        else if (properties_[i].propertyString_[0] == '_')
        {
            error() =
                "Property starting with _ is reserved as internal property. "
                "They cannot be specified in select.";
            return false;
        }

        // if user has specified, do not append ACL fields
        if (shouldAddAclAllow && properties_[i].propertyString_ == "ACL_ALLOW")
        {
            shouldAddAclAllow = false;
        }
        else if (shouldAddAclDeny && properties_[i].propertyString_ == "ACL_DENY")
        {
            shouldAddAclDeny = false;
        }

        // validate property
        PropertyConfig propertyConfig;
        propertyConfig.setName(properties_[i].propertyString_);
        if (!getPropertyConfig(indexSchema_, propertyConfig))
        {
            error() = "Unknown property in select: " + propertyConfig.getName();
            return false;
        }

        if (!propertyConfig.getIsSummary() && properties_[i].isSummaryOn_)
        {
            warning() = "Property summary is not enabled: " +
                        propertyConfig.getName();
            properties_[i].isSummaryOn_ = false;
        }

        if (!propertyConfig.getIsSnippet() && properties_[i].isSnippetOn_)
        {
            warning() = "Property snippet is not enabled: " +
                        propertyConfig.getName();
            properties_[i].isSnippetOn_ = false;
        }

        if (properties_[i].summaryPropertyAlias_.empty())
        {
            properties_[i].summaryPropertyAlias_ =
                properties_[i].propertyString_ + kDefaultSummaryPropertySuffix;
        }
    }

    // append ACL fields
    if (shouldAddAclAllow)
    {
        DisplayProperty aclProperty;
        aclProperty.propertyString_ = "ACL_ALLOW";
        aclProperty.isHighlightOn_ = false;
        aclProperty.isSummaryOn_ = false;
        aclProperty.summarySentenceNum_ = kDefaultSummarySentenceCount;
        aclProperty.summaryPropertyAlias_.resize(0);
        aclProperty.isSnippetOn_ = false;

        izenelib::util::swapBack(properties_, aclProperty);
    }
    if (shouldAddAclDeny)
    {
        DisplayProperty aclProperty;
        aclProperty.propertyString_ = "ACL_DENY";
        aclProperty.isHighlightOn_ = false;
        aclProperty.isSummaryOn_ = false;
        aclProperty.summarySentenceNum_ = kDefaultSummarySentenceCount;
        aclProperty.summaryPropertyAlias_.resize(0);
        aclProperty.isSnippetOn_ = false;

        izenelib::util::swapBack(properties_, aclProperty);
    }

    return true;
}

} // namespace sf1r
