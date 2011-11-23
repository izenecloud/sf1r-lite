#ifndef SF1R_PRODUCTMANAGER_PRODUCTBACKUPDEF_H
#define SF1R_PRODUCTMANAGER_PRODUCTBACKUPDEF_H

#include <common/type_defs.h>


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include <boost/variant.hpp>
#include <boost/serialization/variant.hpp>
#include <am/sequence_file/ssfr.h>

namespace sf1r
{

    struct PriceComparisonItem
    {
        izenelib::util::UString uuid;
        std::vector<izenelib::util::UString> docid_list;
        int type; // 1 means append, 2 means remove

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & uuid & docid_list & type;
        }
    };

    struct ProductUpdateItem
    {
        PMDocumentType doc;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & doc;
        }
    };

    typedef boost::variant<PriceComparisonItem, ProductUpdateItem> ProductBackupItemType;


    struct ProductBackupDataValueType
    {
        std::vector<izenelib::util::UString> docid_list;
        PMDocumentType doc_info;
    };

    typedef boost::unordered_map<std::string, ProductBackupDataValueType> ProductBackupDataType;
//
//     typedef izenelib::am::ssf::Writer<uint32_t, izenelib::util::izene_serialization_boost_wtext > WriterType;
//     typedef izenelib::am::ssf::Reader<uint32_t, izenelib::util::izene_deserialization_boost_wtext> ReaderType;

}

#endif
