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
 *   in each groups. It cannot be used with @b taxonomy_label, @b name_entity_type
 *   and @b name_entity_item together.
 *   - @b property* (@c String): the property name, this name must be also specified
 *   in request["group"].
 *   - @b value* (@c String): the property value, only those documents with this
 *   property value would be returned.
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

    int labelCount = 0;
    if (search.hasKey(Keys::taxonomy_label))
        ++labelCount;
    if (search.hasKey(Keys::name_entity_item) ||
         search.hasKey(Keys::name_entity_type))
        ++labelCount;
    if (search.hasKey(Keys::group_label))
        ++labelCount;
    if (labelCount > 1)
    {
        error() = "At most one label type (taxonomy label, name entity item and group label) can be specided.";
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
                groupLabels_[i].second = asString(groupPair[Keys::value]);

                if (groupLabels_[i].first.empty() || groupLabels_[i].second.empty())
                {
                    error() = "Must specify both property name and value for group label";
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

} // namespace sf1r
