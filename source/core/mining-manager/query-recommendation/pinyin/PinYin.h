#ifndef PINYIN_H
#define PINYIN_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <vector>

#include <idmlib/query-correction/cn_query_correction.h>
#include <util/DynamicBloomFilter.h>


namespace sf1r
{
namespace Recommend
{
typedef boost::function<bool(const std::string&, std::vector<std::string>&)> PinYinConverter;

namespace PinYin
{
class QueryCorrection
{
public:
    QueryCorrection(const std::string dir);
    ~QueryCorrection();
    
    static QueryCorrection& getInstance();

public:
    PinYinConverter* getPinYinConverter()
    {
        static PinYinConverter converter = boost::bind(&QueryCorrection::getPinyin, this, _1, _2);
        return &converter;
    }

    PinYinConverter* getPinYinApproximator()
    {
        static PinYinConverter converter = boost::bind(&QueryCorrection::getApproximatePinyin, this, _1, _2);
        return &converter;
    }
    
    bool getPinyin(const std::string& str, std::vector<std::string>& results) const;
    bool getApproximatePinyin(const std::string& str, std::vector<std::string>& results) const;

private:
    bool approximate(const std::string& pinyin, std::vector<std::string>& results) const;

private:
    idmlib::qc::CnQueryCorrection* cmgr_;
};

}

}
}

#endif
