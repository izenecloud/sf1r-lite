#ifndef SF1R_B5MMANAGER_TOURPROCESSOR_H
#define SF1R_B5MMANAGER_TOURPROCESSOR_H 
#include <util/ustring/UString.h>
#include <product-manager/product_price.h>
#include "b5m_helper.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <document-manager/ScdDocument.h>

namespace sf1r {

    class TourProcessor{
        typedef izenelib::util::UString UString;

        typedef std::pair<std::string, std::string> BufferKey;
        struct BufferValueItem
        {
            std::string from;
            std::string to;
            uint32_t days;
            double price;
            bool operator<(const BufferValueItem& another) const
            {
                return days<another.days;
            }
        };
        typedef std::vector<BufferValueItem> BufferValue;
        typedef boost::unordered_map<BufferKey, BufferValue> Buffer;


    public:
        TourProcessor();
        bool Generate(const std::string& scd_path, const std::string& mdb_instance);

    private:

        void Insert_(ScdDocument& doc);
        uint32_t ParseDays_(const std::string& sdays) const;
        void Finish_();


    private:
        Buffer buffer_;
        boost::shared_ptr<ScdWriter> pwriter_;
    };

}

#endif


