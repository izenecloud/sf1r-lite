#ifndef SF1R_COMMON_SCDWRITER_H_
#define SF1R_COMMON_SCDWRITER_H_

#include "ScdParser.h"
#include <document-manager/Document.h>
#include <boost/function.hpp>
#include <boost/variant.hpp>
namespace sf1r
{

class UStringVisitor : public boost::static_visitor<bool>
{
public:

    template <typename T>
    bool operator()(const T& value) const
    {
        return false;
    }

    bool operator()(const std::string& value) const
    {
        return false;
    }

    bool operator()(const izenelib::util::UString& value) const
    {
        return true;
    }
};

///@brief only write UString properties in Document to SCD file.
class ScdWriter
{
    typedef boost::function<bool
        (const std::string&) > PropertyNameFilterType;

public:
    ScdWriter(const std::string& dir, int op);

    ~ScdWriter();

    void SetFileName(const std::string& fn)
    {
        filename_ = fn;
    }

    static std::string GenSCDFileName( int op);

    bool Append(const Document& doc);

    bool Append(const SCDDoc& doc);

    void Close();

    void SetPropertyNameFilter(const PropertyNameFilterType& filter)
    {
        pname_filter_ = filter;
    }

private:

    void Open_();


private:
    std::string dir_;
    std::string filename_;
    int op_;
    std::ofstream ofs_;

    UStringVisitor ustring_visitor_;

    PropertyNameFilterType pname_filter_;

};


}

#endif
