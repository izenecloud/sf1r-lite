/**
 * @file process/parsers/SearchParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 17:12:27>
 */
#include "SearchParser.h"

#include <common/IndexBundleSchemaHelpers.h>
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
 * - @b group_label (@c Array): Only get documents in the specified groups. Multiple
 *   groups could be specified, so that the documents returned must be contained
 *   in each groups.
 *   - @b property* (@c String): the property name, this name must be also specified
 *   in request["group"].
 *   - @b value* (@c Array): the label value. It's an array of the path from root to leaf node.
 *   Each is a @c String of node value.
 *   Only those documents belonging to the leaf node would be returned.
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
 * - @b log_group_labels (@c Bool = @c true): Whether the @b group_label should be logged.@n
 *   If @c true, those @b group_label of string property type would be logged.
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

    // group label
    const Value& groupNode = search[Keys::group_label];
    if (! nullValue(groupNode))
    {
        if (groupNode.type() == Value::kArrayType)
        {
            groupLabels_.resize(groupNode.size());

            for (std::size_t i = 0; i < groupNode.size(); ++i)
            {
                const Value& groupPair = groupNode(i);
                groupLabels_[i].first = asString(groupPair[Keys::property]);
                faceted::GroupParam::GroupPath& groupPath = groupLabels_[i].second;

                const Value& pathNode = groupPair[Keys::value];
                if (pathNode.type() == Value::kArrayType)
                {
                    groupPath.resize(pathNode.size());
                    for (std::size_t j = 0; j < pathNode.size(); ++j)
                    {
                        groupPath[j] = asString(pathNode(j));
                    }
                }

                if (groupLabels_[i].first.empty() || groupLabels_[i].second.empty())
                {
                    error() = "Must specify both property name and array of value path for group label";
                    return false;
                }
            }
        }
        else
        {
            error() = "Require an array for group labels.";
            return false;
        }
    }

    // attr label
    const Value& attrNode = search[Keys::attr_label];
    if (! nullValue(attrNode))
    {
        if (attrNode.type() == Value::kArrayType)
        {
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
        }
        else
        {
            error() = "Require an array for attr labels.";
            return false;
        }
    }

    logKeywords_ = asBoolOr(search[Keys::log_keywords], true);
    logGroupLabels_ = asBoolOr(search[Keys::log_group_labels], true);

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

} // namespace sf1r
