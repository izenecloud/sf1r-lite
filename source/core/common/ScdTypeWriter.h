#ifndef SF1R_COMMON_SCDTYPEWRITER_H_
#define SF1R_COMMON_SCDTYPEWRITER_H_

#include "ScdWriter.h"
#include <document-manager/Document.h>
#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/unordered_map.hpp>
namespace sf1r
{

///@brief only write UString properties in Document to SCD file.
class ScdTypeWriter
{
    typedef boost::unordered_map<SCD_TYPE, ScdWriter*> WriterMap;
    typedef boost::function<bool
        (const std::string&) > PropertyNameFilterType;

public:
    ScdTypeWriter(const std::string& dir)
    : dir_(dir)
    {
    }

    ~ScdTypeWriter()
    {
        for(WriterMap::iterator it = writer_map_.begin(); it!=writer_map_.end(); ++it)
        {
            ScdWriter* writer = it->second;
            if(writer!=NULL) delete writer;
        }

        //if(iwriter_!=NULL) delete iwriter_;
        //if(uwriter_!=NULL) delete uwriter_;
        //if(rwriter_!=NULL) delete rwriter_;
        //if(dwriter_!=NULL) delete dwriter_;
    }

    bool Append(const Document& doc, SCD_TYPE scd_type)
    {
        if(scd_type==NOT_SCD) return false;
        ScdWriter* writer = GetWriter_(scd_type);
        if(writer==NULL) return false;
        return writer->Append(doc);
    }

    void Close()
    {
        for(WriterMap::iterator it = writer_map_.begin(); it!=writer_map_.end(); ++it)
        {
            ScdWriter* writer = it->second;
            if(writer!=NULL) writer->Close();
        }
        //if(iwriter_!=NULL) iwriter_->Close();
        //if(uwriter_!=NULL) uwriter_->Close();
        //if(dwriter_!=NULL) dwriter_->Close();
    }

private:

    ScdWriter* GetWriter_(SCD_TYPE scd_type)
    {
        ScdWriter* writer = NULL;
        WriterMap::iterator it = writer_map_.find(scd_type);
        if(it==writer_map_.end())
        {
            writer = new ScdWriter(dir_, scd_type);
            writer_map_.insert(std::make_pair(scd_type, writer));
        }
        else
        {
            writer = it->second;
            if(writer==NULL)
            {
                writer = new ScdWriter(dir_, scd_type);
                it->second = writer;
            }
        }

        return writer;
    }


private:
    std::string dir_;
    WriterMap writer_map_;
};


}

#endif

