#ifndef SF1R_B5MMANAGER_ATTRIBUTEPROCESSOR_H_
#define SF1R_B5MMANAGER_ATTRIBUTEPROCESSOR_H_
#include "ngram_synonym.h"
//#include "synonym_handler.h"
#include "b5m_types.h"
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <ir/id_manager/IDManager.h>
#include <am/sequence_file/ssfr.h>
#include <am/leveldb/Table.h>
#include <idmlib/util/idm_analyzer.h>

namespace sf1r {

    class AttributeProcessor {


    public:

        typedef boost::unordered_map<std::string, uint32_t> AidMap;
        typedef boost::unordered_map<std::string, uint32_t> AnidMap;
        AttributeProcessor();
        ~AttributeProcessor();
        bool LoadSynonym(const std::string& file);
        bool Process(const std::string& knowledge_dir);



    private:
        uint32_t GetNameId_(const UString& category, const UString& attrib_name);
        uint32_t GetAid_(const UString& category, const UString& attrib_name, const UString& attrib_value);
        inline std::string GetAttribRep_(const izenelib::util::UString& category, const izenelib::util::UString& attrib_name, const izenelib::util::UString& attrib_value)
        {
            UString result;
            result.append(category);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_name);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_value);
            std::string str;
            result.covertString(str, UString::UTF_8);
            return str;
        }


        inline std::string GetAttribRep_(const izenelib::util::UString& category, const izenelib::util::UString& attrib_name)
        {
            UString result;
            result.append(category);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_name);
            std::string str;
            result.covertString(str, UString::UTF_8);
            return str;
        }



    private:
        std::string knowledge_dir_;
        uint32_t aid_;
        AidMap aid_map_;
        uint32_t anid_;
        AnidMap anid_map_;
    };
}

#endif

