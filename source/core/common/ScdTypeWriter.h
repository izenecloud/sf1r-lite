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

    bool Append(const Document& doc, int op)
    {
        ScdWriter* writer = GetWriter_(op);
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

    ScdWriter* GetWriter_(int op)
    {
        ScdWriter* writer = NULL;
        if(op==INSERT_SCD)
        {
            writer = iwriter_;
        }
        else if(op==UPDATE_SCD)
        {
            writer = uwriter_;
        }
        else if(op==DELETE_SCD)
        {
            writer = dwriter_;
        }
        else return NULL;
        if(writer==NULL)
        {
            writer = new ScdWriter(dir_, op);
            if(op==INSERT_SCD)
            {
                iwriter_ = writer;
            }
            else if(op==UPDATE_SCD)
            {
                uwriter_ = writer;
            }
            else if(op==DELETE_SCD)
            {
                dwriter_ = writer;
            }
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

