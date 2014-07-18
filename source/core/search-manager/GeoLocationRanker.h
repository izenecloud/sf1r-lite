#ifndef GEOLOCATION_RANKER_H_
#define GEOLOCATION_RANKER_H_

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <search-manager/CustomRankTreeParser.h>

#include <common/inttypes.h>

namespace sf1r
{
class NumericPropertyTableBuilder;

class GeoLocationRanker
{
public:
    GeoLocationRanker(
			double							 scope,
            const std::pair<double, double>& reference,
            const boost::shared_ptr<NumericPropertyTableBase>& propertyTable);

    ~GeoLocationRanker();

    double evaluate(docid_t& docid);

	inline bool checkScope(double distance) const {
		if(scope_ <= 0.0) return true;
			return distance <= scope_ ? true : false;
	}

	inline double getScope() const {
		return scope_;
	}

private:
	double					  scope_;
    std::pair<double, double> reference_;
    boost::shared_ptr<NumericPropertyTableBase> propertyTable_;
};

typedef boost::shared_ptr<GeoLocationRanker> GeoLocationRankerPtr;

}

#endif
