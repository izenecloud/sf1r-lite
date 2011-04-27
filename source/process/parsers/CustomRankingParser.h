/**
 * @file CustomRankingParser.h
 * @author Zhongxia Li
 * @date Apr 18, 2011
 */
#ifndef BA_PROCESS_PARSERS_CUSTOM_RANKING_PARSER_H
#define BA_PROCESS_PARSERS_CUSTOM_RANKING_PARSER_H

#include <boost/shared_ptr.hpp>

#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <common/PropertyValue.h>
#include <search-manager/CustomRanker.h>

#include <bundles/index/IndexBundleConfiguration.h>

namespace sf1r {

using namespace izenelib::driver;

/// @addtogroup parsers
/// @{

/**
 * @brief Parse \b custom ranking field.
 */
class CustomRankingParser : public ::izenelib::driver::Parser
{
public:
    explicit CustomRankingParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {
        customRanker_.reset(new CustomRanker());
    }

    bool parse(const Value& custom_rank);

    boost::shared_ptr<CustomRanker>& getCustomRanker()
    {
        return customRanker_;
    }

private:
    bool parse_param(const Value& param);

private:
    const IndexBundleSchema& indexSchema_;

    boost::shared_ptr<CustomRanker> customRanker_;
};


/// @}

} // namespace sf1r

#endif /* BA_PROCESS_PARSERS_CUSTOM_RANKING_PARSER_H */
