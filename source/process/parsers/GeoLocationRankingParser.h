#ifndef SF1R_PARSERS_GEOLOCATION_RANKING_PARSER_H
#define SF1R_PARSERS_GEOLOCATION_RANKING_PARSER_H

#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <string>

namespace sf1r {

using namespace izenelib::driver;
class GeoLocationRankingParser : public ::izenelib::driver::Parser
{
public:
    explicit GeoLocationRankingParser(const IndexBundleSchema& indexSchema)
        : indexSchema_(indexSchema)
    {}

    bool parse(const Value& geoLocationValue);

    const std::pair<double, double>& getReference() const
    {
        return reference_;
    }

    const std::string& getPropertyName() const
    {
        return property_;
    }

private:
    const IndexBundleSchema& indexSchema_;

    std::string property_;
    std::pair<double, double> reference_;
};

} // namespace sf1r

#endif // SF1R_PARSERS_RANGE_PARSER_H
