#ifndef SF1R_B5MMANAGER_ATTRIBUTEINDEXER_H_
#define SF1R_B5MMANAGER_ATTRIBUTEINDEXER_H_
#include "ngram_synonym.h"
//#include "synonym_handler.h"
#include "b5m_types.h"
#include "attribute_id_manager.h"
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <ir/id_manager/IDManager.h>
#include <am/sequence_file/ssfr.h>
#include <am/leveldb/Table.h>
#include <idmlib/util/idm_analyzer.h>

namespace sf1r {

    class AttributeIndexer {


    public:
        //typedef boost::unordered_map<std::string, uint32_t> AidMap;
        //typedef boost::unordered_map<std::string, uint32_t> AnidMap;

        //typedef izenelib::ir::idmanager::DocIdManager<AttribRep, AttribId> AttribIDManager;
        //typedef izenelib::ir::idmanager::DocIdManager<AttribRep, AttribNameId> AttribNameIDManager;

        typedef AttributeIdManager AttribIDManager;
        typedef AttributeIdManager AttribNameIDManager;

        //typedef izenelib::ir::idmanager::_IDManager<AttribRep, AttribId,
                           //izenelib::util::NullLock,
                           //izenelib::ir::idmanager::EmptyWildcardQueryHandler<AttribRep, AttribId>,
                           //izenelib::ir::idmanager::UniqueIDGenerator<AttribRep, AttribId>,
                           //izenelib::ir::idmanager::HDBIDStorage<AttribRep, AttribId> >  AttribIDManager;
        //typedef izenelib::ir::idmanager::_IDManager<AttribRep, AttribNameId,
                           //izenelib::util::NullLock,
                           //izenelib::ir::idmanager::EmptyWildcardQueryHandler<AttribRep, AttribNameId>,
                           //izenelib::ir::idmanager::UniqueIDGenerator<AttribRep, AttribNameId>,
                           //izenelib::ir::idmanager::HDBIDStorage<AttribRep, AttribNameId> >  AttribNameIDManager;
        typedef izenelib::am::leveldb::Table<izenelib::util::UString, std::vector<AttribId> > AttribIndex;
        typedef izenelib::am::leveldb::Table<AttribId, AttribNameId > AttribNameIndex;
        AttributeIndexer(const std::string& knowledge_dir);
        ~AttributeIndexer();
        bool LoadSynonym(const std::string& file);
        bool Index();
        bool Open();
        void GetAttribIdList(const izenelib::util::UString& value, std::vector<AttribId>& id_list);
        void GetAttribIdList(const izenelib::util::UString& category, const izenelib::util::UString& value, std::vector<AttribId>& id_list);
        bool GetAttribRep(const AttribId& aid, AttribRep& rep);
        bool GetAttribNameId(const AttribId& aid, AttribNameId& name_id);
        void GenClassiferInstance();
        void ProductMatchingSVM();
        void ProductMatchingLR(const std::string& scd_file);

        bool TrainSVM();

        bool SplitScd(const std::string& scd_file);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        void ClearKnowledge_();
        inline AttribRep GetAttribRep(const izenelib::util::UString& category, const izenelib::util::UString& attrib_name, const izenelib::util::UString& attrib_value)
        {
            AttribRep result;
            result.append(category);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_name);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_value);
            return result;
        }

        inline void SplitAttribRep(const AttribRep& rep, izenelib::util::UString& category, izenelib::util::UString& attrib_name, izenelib::util::UString& attrib_value)
        {
            std::string srep;
            rep.convertString(srep, izenelib::util::UString::UTF_8);
            std::vector<std::string> words;
            boost::algorithm::split( words, srep, boost::algorithm::is_any_of("|") );
            category = izenelib::util::UString( words[0], izenelib::util::UString::UTF_8);
            attrib_name = izenelib::util::UString( words[1], izenelib::util::UString::UTF_8);
            attrib_value = izenelib::util::UString( words[2], izenelib::util::UString::UTF_8);
        }

        inline AttribRep GetAttribRep(const izenelib::util::UString& category, const izenelib::util::UString& attrib_name)
        {
            AttribRep result;
            result.append(category);
            result.append(izenelib::util::UString("|", izenelib::util::UString::UTF_8));
            result.append(attrib_name);
            return result;
        }

        inline void SplitAttribRep(const AttribRep& rep, izenelib::util::UString& category, izenelib::util::UString& attrib_name)
        {
            std::string srep;
            rep.convertString(srep, izenelib::util::UString::UTF_8);
            std::vector<std::string> words;
            boost::algorithm::split( words, srep, boost::algorithm::is_any_of("|") );
            category = izenelib::util::UString( words[0], izenelib::util::UString::UTF_8);
            attrib_name = izenelib::util::UString( words[1], izenelib::util::UString::UTF_8);
        }
        AttribValueId GetAttribValueId_(const izenelib::util::UString& av)
        {
            return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(av);
        }

        void BuildProductDocuments_();

        void NormalizeText_(const izenelib::util::UString& text, izenelib::util::UString& ntext);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        //av means 'attribute value'
        void NormalizeAV_(const izenelib::util::UString& av, izenelib::util::UString& nav);
        void GetNgramAttribIdList_(const izenelib::util::UString& ngram, std::vector<AttribId>& aid_list);

        bool IsBoolAttribValue_(const izenelib::util::UString& av, bool& b);
        void GetAttribNameMap_(const std::vector<AttribId>& aid_list, boost::unordered_map<AttribNameId, std::vector<AttribId> >& map);
        void GetFeatureVector_(const std::vector<AttribId>& o, const std::vector<AttribId>& p, FeatureType& feature_vec, FeatureStatus& fs);
        void GenNegativeIdMap_(std::map<std::size_t, std::vector<std::size_t> >& map);
        void BuildCategoryMap_(boost::unordered_map<std::string, std::vector<std::size_t> >& map);
        void WriteIdInfo_();
        bool ValidAid_(const AttribId& aid);

        bool ValidAnid_(const AttribNameId& anid);

        void RemoveNegative_(FeatureType& feature_vec)
        {
            FeatureType ft;
            for(uint32_t i=0;i<feature_vec.size();i++)
            {
                if(feature_vec[i].second>=0.0)
                {
                    ft.push_back(feature_vec[i]);
                }
            }
            feature_vec.swap(ft);
        }

    private:
        std::string knowledge_dir_;
        bool is_open_;
        std::string cma_path_;
        std::vector<std::pair<boost::regex, std::string> > normalize_pattern_;
        NgramSynonym ngram_synonym_;
        std::ofstream logger_;
        AttribIndex* index_;
        AttribIDManager* id_manager_;
        AttribNameIndex* name_index_;
        AttribNameIDManager* name_id_manager_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* char_analyzer_;
        std::vector<ProductDocument> product_list_;
        boost::unordered_set<AttribNameId> filter_anid_;
        boost::unordered_set<std::string> filter_attrib_name_;
    };
}

#endif
