//////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *@brief: Geohash encode API 
 * client user can use Encoder(...)
 * and GetNeighborsGrids(...) interface to encode the given longitude 
 * and latitude.
 *@author:wangbaobao@b5m.com
 */

#ifndef _GEOHASHENCODER_
#define _GEOHASHENCODER_

#include<string>
#include<string.h>
#include<assert.h>

namespace sf1r
{
//latutude/longitude range 
struct GeoHashRange
{
	double max;
	double min;
};

//nearby grids
struct GeoHashNeighbors
{
	std::string grids[9];
	/*
	std::string north;
	std::string east;
	std::string west;
	std::string south;
	std::string north_east;
	std::string south_east;
	std::string north_west;
	std::string south_west;
	*/
};

//geohash direction
enum GeoHashDirection
{
	GEOHASH_NORTH = 0,
	GEOHASH_EAST,
	GEOHASH_WEST,
	GEOHASH_SOUTH
};

//geohash encode api implementation
class GeoHashEncoder
{
public:
	GeoHashEncoder();
	
	~GeoHashEncoder();

	/**
	 * @brief:get neighbor grids by scope.
	 * @param longitude[double] :between[-180,180]
	 * @param latitude[double] :between[-90,90]
	 * @param scope search scope[***km]
	 * @return nearby eight grid geohash string
	 */
	GeoHashNeighbors GetNeighborsGridsByScope(double longitude, 
											  double latitude, 
											  double scope);

	/**
	 *@brief:Encode longitude and latitude to geohash string  
	 *@param longitude[double] :between[-180,180]
	 *@param latitude[double] :between[-90,90]
	 *@param length[size_t] :geohash string length must be between[1,12]
	 *@return geohash string
	 */
	std::string Encoder(double longitude,
						double latitude,
						size_t length);
	/**
	 *@brief:Get nearby eight grids geohash string by center grid  
	 *@param longitude[double] :between[-180,180]
	 *@param latitude[double] :between[-90,90]
	 *@param length[size_t] :geohash string length must be between[1,12]
	 *@return nearby eight grid geohash string
	 */
	GeoHashNeighbors GetNeighborsGrids(double longitude,
									   double latutude,
									   size_t length);
private:
	//set base32 bits value
	inline void SetBit(unsigned char &bits,
					   double value, 
					   size_t offset,
					   GeoHashRange *range);
	//get nearby grid by direction
	std::string GetAdjacent(const std::string&hash, GeoHashDirection dir);

private:
	//define some const value
	static const char *kBordersTable[8];
	static const char *kNeighborsTable[8];
	static const char kBase32EncoderTable[33];
	static const size_t kMaxGeoHashLength; 
	static const double kScope[9];
};
}
#endif
