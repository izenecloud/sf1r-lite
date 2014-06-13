#include "GeoLocationRanker.h"
#include "NumericPropertyTableBuilder.h"

#include <cmath>

namespace sf1r
{

namespace
{

inline double rad(double d)
{
    return d * M_PI / 180.0;
}

inline double dist(double lat1, double long1, double lat2, double long2)
{
    static const double EARTH_RADIUS = 6378.137;
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(long1) - rad(long2);
    double s = 2 * asin(sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    return s * EARTH_RADIUS * 1000;
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
    propertyTable_->getDoublePairValue(docid, coordinate, false);

    return dist(reference_.first, reference_.second, coordinate.first, coordinate.second);
}

}
