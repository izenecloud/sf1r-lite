#ifndef __SCD__WRITER__H__
#define __SCD__WRITER__H__

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

    virtual ~ScdWriter();

    static std::string GenSCDFileName( int op);
    
    void Append(const Document& doc);
    
    void Close();
    
    void SetPropertyNameFilter(const PropertyNameFilterType& filter)
    {
        pname_filter_ = filter;
    }

    
private:

    std::ofstream ofs_;
    
    UStringVisitor ustring_visitor_;
    
    PropertyNameFilterType pname_filter_;
};


}

#endif

