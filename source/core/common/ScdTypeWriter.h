#ifndef SF1R_COMMON_SCDTYPEWRITER_H_
#define SF1R_COMMON_SCDTYPEWRITER_H_

#include "ScdWriter.h"
#include <document-manager/Document.h>
#include <boost/function.hpp>
#include <boost/variant.hpp>
namespace sf1r
{

///@brief only write UString properties in Document to SCD file.
class ScdTypeWriter
{
    typedef boost::function<bool
        (const std::string&) > PropertyNameFilterType;

public:
    ScdTypeWriter(const std::string& dir)
    : dir_(dir), iwriter_(NULL), uwriter_(NULL), dwriter_(NULL)
    {
    }

    ~ScdTypeWriter()
    {
        if(iwriter_!=NULL) delete iwriter_;
        if(uwriter_!=NULL) delete uwriter_;
        if(dwriter_!=NULL) delete dwriter_;
    }

    bool Append(const Document& doc, SCD_TYPE scd_type)
    {
        ScdWriter* writer = GetWriter_(scd_type);
        if(writer==NULL) return false;
        return writer->Append(doc);
    }

    void Close()
    {
        if(iwriter_!=NULL) iwriter_->Close();
        if(uwriter_!=NULL) uwriter_->Close();
        if(dwriter_!=NULL) dwriter_->Close();
    }

private:

    ScdWriter* GetWriter_(SCD_TYPE scd_type)
    {
        ScdWriter* writer = NULL;
        switch (scd_type)
        {
        case INSERT_SCD:
            if (iwriter_ == NULL)
            {
                iwriter_ = new ScdWriter(dir_, scd_type);
            }
            writer = iwriter_;
            break;

        case UPDATE_SCD:
            if (uwriter_ == NULL)
            {
                uwriter_ = new ScdWriter(dir_, scd_type);
            }
            writer = uwriter_;
            break;

        case DELETE_SCD:
            if (dwriter_ == NULL)
            {
                dwriter_ = new ScdWriter(dir_, scd_type);
            }
            writer = dwriter_;
            break;

        default:
            break;
        }

        return writer;
    }


private:
    std::string dir_;
    ScdWriter* iwriter_;
    ScdWriter* uwriter_;
    ScdWriter* dwriter_;
};


}

#endif

