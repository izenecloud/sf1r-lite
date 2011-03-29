#ifndef SF1R_PARSER_SELECT_PARSER_H
#define SF1R_PARSER_SELECT_PARSER_H
/**
 * @file core/common/parsers/SelectParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 16:30:14>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>
#include <util/ustring/UString.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <vector>
#include <string>

namespace sf1r {

class DisplayProperty;
/// @addtogroup parsers
/// @{

/**
 * @brief Parse \b select field.
 */
using namespace izenelib::driver;
class SelectParser : public ::izenelib::driver::Parser
{
    typedef std::vector<izenelib::util::UString> query_list;
public:
    explicit SelectParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {}

    /// @brief Default suffix of the summary property
    static const std::string kDefaultSummaryPropertySuffix;

    enum {
        /// @brief Default count of sentences in summary.
        kDefaultSummarySentenceCount = 3
    };


    bool parse(const Value& select);

    std::vector<DisplayProperty>& mutableProperties()
    {
        return properties_;
    }
    const std::vector<DisplayProperty>& properties() const
    {
        return properties_;
    }

private:
    const IndexBundleSchema& indexSchema_;

    std::vector<DisplayProperty> properties_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSER_SELECT_PARSER_H
