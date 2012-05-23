#ifndef SF1R_B5MMANAGER_B5MPPROCESSOR_H_
#define SF1R_B5MMANAGER_B5MPPROCESSOR_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "product_db.h"
#include "offer_db.h"
#include <common/ScdMerger.h>
#include <am/sequence_file/ssfr.h>

namespace sf1r {

    ///B5mpProcessor is responsibility to generate b5mp scds and also b5mo_mirror scd
    class B5mpProcessor{
        typedef izenelib::am::ssf::Writer<> PoMapWriter;
    public:
        B5mpProcessor(const std::string& mdb_instance, const std::string& last_mdb_instance);

        bool Generate();

    private:

        void ProductMerge_(ScdMerger::ValueType& value, const ScdMerger::ValueType& another_value);

        void ProductOutput_(Document& doc, int& type);

    private:
        std::string mdb_instance_;
        std::string last_mdb_instance_;
        PoMapWriter* po_map_writer_;
    };

}

#endif

