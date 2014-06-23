#include "GeoLocationRanker.h"
#include "NumericPropertyTableBuilder.h"

#include <cmath>

namespace sf1r
{

namespace detail
{

static const double EARTH_RADIUS = 6378.137;
static const double HALF_EQUATOR = EARTH_RADIUS * M_PI;

inline double rad(double d)
{
    return d * M_PI / 180.0;
}

inline double geodist(double long1, double lat1, double long2, double lat2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(long1) - rad(long2);
    double s = 2 * asin(sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    return s * EARTH_RADIUS * 1000.0;
}

}

GeoLocationRanker::GeoLocationRanker(
        const std::pair<double, double>& reference,
        const boost::shared_ptr<NumericPropertyTableBase>& propertyTable)
    : reference_(reference)
    , propertyTable_(propertyTable)
{
}

GeoLocationRanker::~GeoLocationRanker()
{
}

double GeoLocationRanker::evaluate(docid_t& docid)
{
    std::pair<double, double> coordinate;
    if (!propertyTable_->getDoublePairValue(docid, coordinate, false)
            || abs(coordinate.first) > 180.0 || abs(coordinate.second) > 180.0)
        return detail::HALF_EQUATOR * 1000.0;

    return detail::geodist(reference_.first, reference_.second, coordinate.first, coordinate.second);
}

}
