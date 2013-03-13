#ifndef SF1R_COMMON_SCDWRITER_H_
#define SF1R_COMMON_SCDWRITER_H_

#include "ScdParser.h"
#include <document-manager/Document.h>
#include <boost/function.hpp>
#include <boost/variant.hpp>
namespace sf1r
{

class DocumentOutputVisitor : public boost::static_visitor<void>
{
public:
    DocumentOutputVisitor():os_(NULL) {}
    DocumentOutputVisitor(std::ostream* os):os_(os) {}
    void SetOutStream(std::ostream* os) {os_ = os; }

    template <typename T>
    void operator()(const T& value) const
    {
        *os_ << value;
    }

    void operator()(const izenelib::util::UString& value) const
    {
        std::string str;
        value.convertString(str, izenelib::util::UString::UTF_8);
        *os_ << str;
    }
    void operator()(const std::vector<izenelib::util::UString>& value) const
    {
        for(uint32_t i=0;i<value.size();i++)
        {
            std::string str;
            value[i].convertString(str, izenelib::util::UString::UTF_8);
            if(i>0) *os_ <<",";
            *os_ << str;
        }
    }
    void operator()(const std::vector<uint32_t>& value) const
    {
        for(uint32_t i=0;i<value.size();i++)
        {
            if(i>0) *os_ <<",";
            *os_ << value[i];
        }
    }
private:
    std::ostream* os_;
};

///@brief only write UString properties in Document to SCD file.
class ScdWriter
{
    typedef boost::function<bool
        (const std::string&) > PropertyNameFilterType;

public:
    ScdWriter(const std::string& dir, SCD_TYPE scd_type);

    ~ScdWriter();

    void SetFileName(const std::string& fn)
    {
        filename_ = fn;
    }

    static std::string GenSCDFileName(SCD_TYPE scd_type);

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
    SCD_TYPE scd_type_;
    std::ofstream ofs_;

    DocumentOutputVisitor output_visitor_;

    PropertyNameFilterType pname_filter_;

};


}

#endif
