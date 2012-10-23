#ifndef SF1R_PARSER_SEARCH_PARSER_H
#define SF1R_PARSER_SEARCH_PARSER_H
/**
 * @file core/common/parsers/SearchParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 17:00:38>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <query-manager/ConditionInfo.h>
#include <ranking-manager/RankingEnumerator.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <query-manager/SearchingEnumerator.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <string>
#include <vector>
#include <utility>

namespace sf1r {
/// @addtogroup parsers
/// @{

/**
 * @brief Parse \b search field.
 */
using namespace izenelib::driver;
class SearchParser : public ::izenelib::driver::Parser
{
public:
    bool parse(const Value& search);

    SearchParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {}

    std::string& mutableKeywords()
    {
        return keywords_;
    }
    const std::string& keywords() const
    {
        return keywords_;
    }

    std::string& mutableUserID()
    {
        return userID_;
    }

    const std::string& userID() const
    {
        return userID_;
    }

    std::string& mutableSessionID()
    {
        return sessionID_;
    }

    const std::string& sessionID() const
    {
        return sessionID_;
    }

    std::string& mutableTaxonomyLabel()
    {
        return taxonomyLabel_;
    }
    const std::string& taxonomyLabel() const
    {
        return taxonomyLabel_;
    }

    std::string& mutableNameEntityItem()
    {
        return nameEntityItem_;
    }
    const std::string& nameEntityItem() const
    {
        return nameEntityItem_;
    }

    std::string& mutableNameEntityType()
    {
        return nameEntityType_;
    }
    const std::string& nameEntityType() const
    {
        return nameEntityType_;
    }

    faceted::GroupParam::GroupLabelMap& mutableGroupLabels()
    {
        return groupLabels_;
    }
    const faceted::GroupParam::GroupLabelMap& groupLabels() const
    {
        return groupLabels_;
    }

    faceted::GroupParam::AutoSelectLimitMap& mutableGroupLabelAutoSelectLimits()
    {
        return groupLabelAutoSelectLimits_;
    }
    const faceted::GroupParam::AutoSelectLimitMap& groupLabelAutoSelectLimits() const
    {
        return groupLabelAutoSelectLimits_;
    }

    faceted::GroupParam::AttrLabelMap& mutableAttrLabels()
    {
        return attrLabels_;
    }
    const faceted::GroupParam::AttrLabelMap& attrLabels() const
    {
        return attrLabels_;
    }

    bool logKeywords() const
    {
        return logKeywords_;
    }
    std::vector<std::string>& mutableProperties()
    {
        return properties_;
    }
    const std::vector<std::string>& properties() const
    {
        return properties_;
    }
    std::vector<std::string>& mutableCounterList()
    {
        return countList_;
    }
    const std::vector<std::string>& counterList() const
    {
        return countList_;
    }

    RankingType::TextRankingType rankingModel() const
    {
        return rankingModel_;
    }

    LanguageAnalyzerInfo analyzerInfo() const
    {
        return analyzerInfo_;
    }

    SearchingModeInfo searchingModeInfo() const
    {
        return searchingModeInfo_;
    }

private:
    bool parseGroupLabel_(const Value& search);
    bool parseAttrLabel_(const Value& search);

private:
    const IndexBundleSchema& indexSchema_;

    std::string keywords_;
    std::string userID_;
    std::string sessionID_;
    std::string taxonomyLabel_;
    std::string nameEntityType_;
    std::string nameEntityItem_;

    faceted::GroupParam::GroupLabelMap groupLabels_;
    faceted::GroupParam::AutoSelectLimitMap groupLabelAutoSelectLimits_;
    faceted::GroupParam::AttrLabelMap attrLabels_;

    bool logKeywords_;
    std::vector<std::string> properties_;
    std::vector<std::string> countList_;
    RankingType::TextRankingType rankingModel_;
    SearchingModeInfo searchingModeInfo_;
    LanguageAnalyzerInfo analyzerInfo_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSER_SEARCH_PARSER_H
