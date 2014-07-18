/*
 *@file:GeoFilter.h
 *@brief: implement nearby searching filter
 *@author:wangbaobao@b5m.com
 *@date:2014-07-09.
 */

#ifndef GEO_FILTER_H_
#define GEO_FILTER_H_

#include "RTypeStringPropTableBuilder.h"
#include "GeoHashEncoder.h"
#include <common/RTypeStringPropTable.h>
#include <stdint.h>
#include <vector>
#include <assert.h>

namespace sf1r
{
class GeoFilter
{
 public:
	 typedef uint32_t			       doc_id_t;
	 typedef std::vector<doc_id_t>     DocIdVector;
	 typedef std::pair<double, double> CoordinatePair;

 public:
	 GeoFilter(double                       scope,
			   const  CoordinatePair&       coordinate,
			   RTypeStringPropTableBuilder *rtype_builder)
	  :scope_(scope),
	   rtype_builder_(rtype_builder),
	   coordinate_(coordinate)
	 {
	 }

	 ~GeoFilter()
	 {
	 }

	 void Filter(DocIdVector &doc_ids)
	 {
		 GeoHashEncoder encoder;
		 GeoHashNeighbors neighbor_grids = encoder.GetNeighborsGridsByScope(coordinate_.first,
																		    coordinate_.second,
																			scope_);

		//get rtype property "GeoHash" instance
		assert(rtype_builder_);
		boost::shared_ptr<RTypeStringPropTable> &rtype_table = 
			rtype_builder_->createPropertyTable("GeoHash");
		
		DocIdVector::iterator doc_id_iter = doc_ids.begin();
		for(; doc_id_iter != doc_ids.end(); ++doc_id_iter)
		{
			bool is_matched = false;
			std::string rtype_value;

			rtype_table->getRTypeString((unsigned int)(*doc_id_iter), rtype_value);
			if(rtype_value.empty()) {
				//need remove ???
				continue;
			}

			for(size_t i = 0; i < 9; i++)
			{
				if(neighbor_grids.grids[i].empty()) continue;
				//match neighbor grids
				//rtype_value stores 12bytes geohash string
				//so we check whether rtype_value start with grids[j]
				size_t pos = rtype_value.find(neighbor_grids.grids[i]);
				if(pos == 0) {
					is_matched = true;
					break;
				}
			}

			if(is_matched == false){
				doc_ids.erase(doc_id_iter);
			}
		}
	 }

 private:
	 double                       scope_;
	 RTypeStringPropTableBuilder *rtype_builder_;
	 const CoordinatePair        &coordinate_;
};
}
#endif
