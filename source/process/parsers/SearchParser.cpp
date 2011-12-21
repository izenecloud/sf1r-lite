/**
 * @file process/parsers/SearchParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 17:12:27>
 */
#include "SearchParser.h"

#include <common/BundleSchemaHelpers.h>
#include <common/Keys.h>

#include <boost/algorithm/string/case_conv.hpp>

namespace sf1r {

using driver::Keys;

/**
 * @class SearchParser
 *
 * The @b search field is an object. It specifies a full text search in
 * documents. The search object has following fields:
 *
 * - @b keywords* (@c String): Keywords used in the search.
 * - @b USERID (@c String): USERID is used to get the user info, which assists
 *   to get personalized search result. This item is not required to set. if it
 *   is not set, the result is the normal ranking search result.
 * - @b in (@c Array): Which properties the search is performed in. Every item
 *   can be an Object or an String. If it is an Object, the \b property field is
 *   used as the property name, otherwise, the String itself is used as property
 *   name. If this is not specified, all indexed properties are used. Valid
 *   properties can be check though schema/get (see SchemaController::get() ).
 *   All index names (the @b name field in every index) can be used.
 * - @b taxonomy_label (@c String): Only get documents in the specified
 *   label. It cannot be used with @b name_entity_type and @b name_entity_item
 *   together.
 * - @b name_entity_type (@c String): Only get documents in the specified name
 *   entity item. Must be used with @b name_entity_item together and cannot be
 *   used with @b taxonomy_label.
 * - @b name_entity_item (@c Object): Only get documents in the specified name
 *   entity item. Must be used with @b name_entity_type together and cannot be
 *   used with @b taxonomy_label.
 * - @b group_label (@c Array): Only get documents in the specified group labels.@n
 *   Each label is an object consisting of a property name and value.@n
 *   You could specify multiple labels, for example, there are 4 labels named as "A1", "A2", "B1", "B2".@n
 *   If the property name of label "A1" and "A2" is "A", while the property name of label "B1" and "B2" is "B",@n
 *   then the documents returned would belong to the labels with the condition of ("A1" or "A2") and ("B1" or "B2").
 *   - @b property* (@c String): the property name.
 *   - @b value* (@c Array): the label value.@n
 *   For the property type of string, it's an array of the path from root to leaf node.
 *   Each is a @c String of node value. Only those documents belonging to the leaf node
 *   would be returned.@n
 *   For the property type of int or float, you could either specify a property value
 *   or a range in @b value[0]. If it is a range, you could not specify the same @b property
 *   in request["group"].@n
 *   Regarding the range format, you could specify it in the form of "101-200",
 *   meaning all values between 101 and 200, including both boundary values,
 *   or in the form of "-200", meaning all values not larger than 200,
 *   or in the form of "101-", meaning all values not less than 101.
 * - @b attr_label (@c Array): Only get documents in the specified attribute groups.
 *   Multiple labels could be specified. It could also be used with @b group_label
 *   together, so that the documents returned must be contained in each label in
 *   @b attr_label and @b group_label.
 *   - @b attr_name* (@c String): the attribute name
 *   - @b attr_value* (@c String): the attribute value, only those documents with this
 *   attribute value would be returned.
 * - @b ranking_model (@c String): How to rank the search result. Result with
 *   high ranking score is considered has high relevance. Now SF1 supports
 *   following ranking models.
 *   - @b plm
 *   - @b bm25
 *   - @b kl
 *   If this is omitted, A default ranking model specified in configuration file
 *   is used (In Collection node attribute).
 * - @b log_keywords (@c Bool = @c true): Whether the keywords should be
 *   logged.
 * - @b analyzer (@c Object): Keywords analyzer options
 *   - @b apply_la (@c Bool = @c true): TODO
 *   - @b use_synonym_extension (@c Bool = @c false): TODO
 *   - @b use_original_keyword (@c Bool = @c false): TODO
 *
 * Example
 * @code
 * {
 *   "keywords": "test",
 *   "USERID": "56",
 *   "in": [
 *     {"property": "title"},
 *     {"property": "content"}
 *   ],
 *   "ranking_model": "plm",
 *   "log_keywords": true,
 *   "analyzer": {
 *     "use_synonym_extension": true,
 *     "apply_la": true,
 *     "use_original_keyword": true
 *   }
 * }
 * @endcode
 */

bool SearchParser::parse(const Value& search)
{
    clearMessages();

    // keywords
    keywords_ = asString(search[Keys::keywords]);
    if (keywords_.empty())
    {
        error() = "Require keywords in search.";
        return false;
    }

    userID_ = asString(search[Keys::USERID]);

    int labelCount = 0;
    if (search.hasKey(Keys::taxonomy_label))
        ++labelCount;
    if (search.hasKey(Keys::name_entity_item) ||
         search.hasKey(Keys::name_entity_type))
        ++labelCount;
    if (labelCount > 1)
    {
        error() = "At most one label type (taxonomy label, name entity label) can be specided.";
        return false;
    }

    taxonomyLabel_ = asString(search[Keys::taxonomy_label]);
    nameEntityItem_ = asString(search[Keys::name_entity_item]);
    nameEntityType_ = asString(search[Keys::name_entity_type]);

    if ((nameEntityType_.empty() && !nameEntityItem_.empty()) ||
        (!nameEntityType_.empty() && nameEntityItem_.empty()))
    {
        error() = "Name entity type and name entity item must be used together";
        return false;
    }

    if (!parseGroupLabel_(search) || !parseAttrLabel_(search))
        return false;

    logKeywords_ = asBoolOr(search[Keys::log_keywords], true);

    // properties
    const Value& propertiesNode = search[Keys::in];
    if (nullValue(propertiesNode))
    {
        getDefaultSearchPropertyNames(indexSchema_, properties_);
    }
    else if (propertiesNode.type() == Value::kArrayType)
    {
        properties_.resize(propertiesNode.size());
        for (std::size_t i = 0; i < properties_.size(); ++i)
        {
            const Value& currentProperty = propertiesNode(i);
            if (currentProperty.type() == Value::kObjectType)
            {
                properties_[i] = asString(currentProperty[Keys::property]);
            }
            else
            {
                properties_[i] = asString(currentProperty);
            }

            if (properties_[i].empty())
            {
                error() = "Failed to parse properties in search.";
                return false;
            }

            // validation
            PropertyConfig propertyConfig;
            propertyConfig.setName(properties_[i]);
            if (!getPropertyConfig(indexSchema_,propertyConfig))
            {
                error() = "Unknown property in search/in: " +
                          propertyConfig.getName();
                return false;
            }

            if (!propertyConfig.bIndex_)
            {
                warning() = "Property is not indexed, ignore it: " +
                            propertyConfig.getName();
            }
        }
    }

    if (properties_.empty())
    {
        error() = "Require list of properties in which search is performed.";
        return false;
    }

    // La
    const Value& analyzer = search[Keys::analyzer];
    analyzerInfo_.applyLA_ = asBoolOr(analyzer[Keys::apply_la], true);
    analyzerInfo_.useOriginalKeyword_ = asBool(analyzer[Keys::use_original_keyword]);
    analyzerInfo_.synonymExtension_ = asBool(analyzer[Keys::use_synonym_extension]);

    // ranker
    rankingModel_ = RankingType::DefaultTextRanker;
    if (search.hasKey(Keys::ranking_model))
    {
        Value::StringType ranker = asString(search[Keys::ranking_model]);
        boost::to_lower(ranker);

        if (ranker == "plm")
        {
            rankingModel_ = RankingType::PLM;
        }
        else if (ranker == "bm25")
        {
            rankingModel_ = RankingType::BM25;
        }
        else if (ranker == "kl")
        {
            rankingModel_ = RankingType::KL;
        }
        else
        {
            warning() = "Unknown rankingModel. Default one is used.";
        }
    }

    return true;
}

bool SearchParser::parseGroupLabel_(const Value& search)
{
    const Value& groupNode = search[Keys::group_label];

    if (nullValue(groupNode))
        return true;

    if (groupNode.type() != Value::kArrayType)
    {
        error() = "Require an array for group labels.";
        return false;
    }

    for (std::size_t i = 0; i < groupNode.size(); ++i)
    {
        const Value& groupPair = groupNode(i);
        std::string propName = asString(groupPair[Keys::property]);

        const Value& pathNode = groupPair[Keys::value];
        faceted::GroupParam::GroupPath groupPath;
        if (pathNode.type() == Value::kArrayType)
        {
            for (std::size_t j = 0; j < pathNode.size(); ++j)
            {
                std::string nodeValue = asString(pathNode(j));
                if (nodeValue.empty())
                {
                    error() = "request[search][group_label][value] is empty for property " + propName;
                    return false;
                }
                groupPath.push_back(nodeValue);
            }
        }

        if (propName.empty() || groupPath.empty())
        {
            error() = "Must specify both property name and array of value path for group label";
            return false;
        }

        groupLabels_[propName].push_back(groupPath);
    }

    return true;
}

bool SearchParser::parseAttrLabel_(const Value& search)
{
    const Value& attrNode = search[Keys::attr_label];

    if (nullValue(attrNode))
        return true;

    if (attrNode.type() != Value::kArrayType)
    {
        error() = "Require an array for attr labels.";
        return false;
    }

    attrLabels_.resize(attrNode.size());
    for (std::size_t i = 0; i < attrNode.size(); ++i)
    {
        const Value& attrPair = attrNode(i);
        faceted::GroupParam::AttrLabel& attrLabel = attrLabels_[i];
        attrLabel.first = asString(attrPair[Keys::attr_name]);
        attrLabel.second = asString(attrPair[Keys::attr_value]);

        if (attrLabel.first.empty() || attrLabel.second.empty())
        {
            error() = "Must specify both attribute name and value for attr label";
            return false;
        }
    }

    return true;
}

} // namespace sf1r
