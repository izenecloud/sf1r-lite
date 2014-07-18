/////////////////////////////////////////////////////////////////
/**
 *@brief: Geohash encode API 
 * client user can use Encoder(...)
 * and GetNeighborsGrids(...) interface to encode the given longitude 
 * and latitude.
 *@author:wangbaobao@b5m.com
 */

#include "GeoHashEncoder.h"

namespace sf1r{

//max geohash string length
//notice:when length equal 9, accuracy is about 2m
const size_t GeoHashEncoder::kMaxGeoHashLength = 12;

//base32 characters
const char GeoHashEncoder::kBase32EncoderTable[33] = 
{
	"0123456789bcdefghjkmnpqrstuvwxyz"
};

//geohash neighbors list
const char *GeoHashEncoder::kNeighborsTable[8] = 
{
	"0145hjnp02367kmqr289destwx8bcfguvyzb",		/* NORTH EVEN */
	"028b0139c146df457eg5hksuhjmtvjnqwynprxzp", /* NORTH ODD  */
	"prxzpnqwynjmtvjhksuh57eg546df4139c1028b0", /* EAST EVEN  */
	"bcfguvyzb89destwx82367kmqr20145hjnp0",		/* EAST ODD   */
	"zxrpzywqnyvtmjvuskhuge75gfd64fc931cb820b", /* WEST EVEN  */
	"zyvugfcbzxwtsed98xrqmk7632rpnjh5410p",		/* WEST ODD   */
	"pnjh5410prqmk7632rxwtsed98xzyvugfcbz",		/* SOUTH EVEN */
	"b820bc931cfd64fge75guskhuvtmjvywqnyzxrpz"  /* SOUTH ODD  */
};

//geohash border list 
const char *GeoHashEncoder::kBordersTable[8] = 
{
	"prxz",     /* NORTH EVEN */
	"bcfguvyz", /* NORTH ODD */
	"bcfguvyz", /* EAST  EVEN */
	"prxz",     /* EAST  ODD */
	"0145hjnp", /* WEST  EVEN */
	"028b",     /* WEST  ODD */
	"028b",     /* SOUTH EVEN */
	"0145hjnp"  /* SOUTH ODD */
};

//geohash scope list
//min precision is about 2m
const double GeoHashEncoder::kScope[9] = 
{
	2500.0,
	630.0,
	78.0,
	20.0,
	2.4,
	0.61,
	0.076,
	0.019,
	0.002
};

GeoHashEncoder::GeoHashEncoder()
{
}

GeoHashEncoder::~GeoHashEncoder()
{
}

void GeoHashEncoder::SetBit(unsigned char &bits,
						    double value, 
						    size_t offset,
						    GeoHashRange *range)
{
	double mid = (range->max + range->min) / 2.0;
	if (value >= mid) {
		range->min = mid;
		bits |= (0x1 << offset);
	} else {
		range->max = mid;
		bits |= (0x0 << offset);
	}
}

std::string GeoHashEncoder::Encoder(double longitude,
									double latitude,
									size_t length)
{
	std::string geohash("");
	if(latitude < -90.0 || latitude > 90.0 ||
		longitude < -180.0 || longitude > 180.0 || 
			length > kMaxGeoHashLength || length <= 0)
	{
		return geohash;
	}

	unsigned char bits = 0;
	double val1 = longitude;
	double val2 = latitude;
	GeoHashRange lat_range = {90.0, -90.0};
	GeoHashRange lon_range = {180.0, -180.0}; 
	GeoHashRange *range1 = &lon_range;
	GeoHashRange *range2 = &lat_range;

	for (size_t i = 0; i < length; i++) 
	{
		bits = 0;
		SetBit(bits, val1, 4, range1);
		SetBit(bits, val2, 3, range2);
		SetBit(bits, val1, 2, range1);
		SetBit(bits, val2, 1, range2);
		SetBit(bits, val1, 0, range1);
		
		assert(bits <= 32);
		
		geohash.push_back(kBase32EncoderTable[bits]);
		std::swap(val1,val2);
		std::swap(range1,range2);
	}
	return geohash;
}

GeoHashNeighbors GeoHashEncoder::GetNeighborsGrids(double longitude,
												   double latutude,
												   size_t length)
{
	GeoHashNeighbors neighbors;
	std::string hash = Encoder(longitude,latutude,length);
	if(hash.empty()){
		return neighbors;
	}
	neighbors.grids[0] = hash;

	neighbors.grids[1] = GetAdjacent(hash, GEOHASH_NORTH);
	neighbors.grids[2]  = GetAdjacent(hash, GEOHASH_EAST);
	neighbors.grids[3]  = GetAdjacent(hash, GEOHASH_WEST);
	neighbors.grids[4] = GetAdjacent(hash, GEOHASH_SOUTH);

	neighbors.grids[5] = GetAdjacent(neighbors.grids[1], GEOHASH_EAST);
	neighbors.grids[6] = GetAdjacent(neighbors.grids[1], GEOHASH_WEST);
	neighbors.grids[7] = GetAdjacent(neighbors.grids[4], GEOHASH_EAST);
	neighbors.grids[8] = GetAdjacent(neighbors.grids[4], GEOHASH_WEST);
	return neighbors;
}

std::string GeoHashEncoder::GetAdjacent(const std::string&hash, GeoHashDirection dir)
{
	if(hash.empty()) {	
		return "";
	}
	std::string base;
	std::string refined_base;
	size_t len  = hash.length();
	char   last = tolower(hash[len - 1]);
	size_t idx  = dir * 2 + (len % 2);
	const char *border_table = kBordersTable[idx];

	assert(len >= 1);
	base = hash.substr(0,len - 1);
	if (strchr(border_table, last) != NULL) 
	{
		//Notice:recursive call the GetAdjacent()
		//get the previous geohash code
		refined_base = GetAdjacent(base, dir);
		if(refined_base.empty())
		{
			return "";
		}
		base = refined_base;
	}

	const char *neighbor_table = kNeighborsTable[idx];
	char *ptr = (char *)strchr(neighbor_table, last);
	assert(ptr != NULL);
	//get next position
	base.push_back(*++ptr);
	return base;
}

GeoHashNeighbors GeoHashEncoder::GetNeighborsGridsByScope(double longitude, 
														  double latitude, 
														  double scope)
{
	GeoHashNeighbors neighbor_grids;
	size_t length = 0;
	if(scope > kScope[0] || scope < kScope[8]) return neighbor_grids;

	for(int i = 0; i < 8; i++)
	{
		//find corresponding geohash length
		if(scope <= kScope[i] && scope > kScope[i + 1]){
			length = i + 1;
			break;
		}
	}
	if(scope == kScope[8]){
		assert(length == 0);
		length = 9;
	}
	assert(length >= 1 && length <= 9);
	//calculate neighbor grids by the given latitude and longitude
	neighbor_grids = GetNeighborsGrids(longitude, latitude, length);
}
}
