#include "MiningUtil.h"
#include <boost/iostreams/filter/bzip2.hpp> 
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <sstream> 
#include <iostream> 
using namespace sf1r;
namespace bio = boost::iostreams;
void MiningUtil::encodeIntegers( const std::vector<uint32_t>& ints,char* data, uint32_t& len)
{
    char* source = new char[ints.size()*4];
    char* pSource = source;
    for(uint32_t i=0;i<ints.size();i++)
    {
        memcpy(pSource, &(ints[i]), 4);
        pSource+=4;
    }
    
    std::ostringstream result(std::ios::binary);
    bio::filtering_ostream fos;
    fos.push(bio::zlib_compressor(bio::zlib::best_compression));
    fos.push(result);
    fos.write(source, ints.size()*4); 
    fos.pop();
    fos.pop();
    
    len = result.str().length();
    
    delete[] source;
    
    
}
