///
/// @file TaxonomyGenerationSubManager.h
/// @brief Provide taxonomy generation algorithm.
/// @author Jinglei Zhao <zhao.jinglei@gmail.com>
/// @date Created 2009-05-18
/// @date Updated 2009-11-21 00:59:16  Jia Guo <guojia@gmail.com>
///

#ifndef TAXONOMYGENERATIONSUBMANAGER_H_
#define TAXONOMYGENERATIONSUBMANAGER_H_


#include "TgTypes.h"
#include "TaxonomyRep.h"
#include <mining-manager/MiningManagerDef.h>
#include <common/PropertyTermInfo.h>
#include <configuration-manager/MiningConfig.h>
#include <boost/shared_ptr.hpp>

#include <common/type_defs.h>

#include <am/3rdparty/rde_hash.h>
#include <idmlib/concept-clustering/concept_clustering.h>

#include <string>
#include <vector>

namespace idmlib
{
namespace util
{
class IDMAnalyzer;
}
}

namespace sf1r
{

class LabelManager;
class TaxonomyGenerationSubManager
{

    struct ScoreItem
    {
        ScoreItem(){}
        ScoreItem(uint32_t doc_count)
        :doc_invert(doc_count)
        {
        }
        uint32_t concept_id;
        boost::dynamic_bitset<uint32_t> doc_invert;
        double score;
        double score2rank;

        bool operator<(const ScoreItem& another) const
        {
            return score2rank > another.score2rank;
        }
    };


public:
    TaxonomyGenerationSubManager(
        const TaxonomyPara& taxonomy_param,
        const boost::shared_ptr<LabelManager>& labelManager,
        idmlib::util::IDMAnalyzer* analyzer);
    ~TaxonomyGenerationSubManager();

public:

    bool GetConceptsByDocidList(const std::vector<docid_t>& docIdList, const izenelib::util::UString& queryStr, uint32_t totalCount, idmlib::cc::CCInput32& input);

    bool GetResult(const idmlib::cc::CCInput64& input, TaxonomyRep& taxonomyRep ,NEResultList& neList);

    void AggregateInput(const std::vector<wdocid_t>& top_wdoclist, const std::vector<std::pair<uint32_t, idmlib::cc::CCInput32> >& input_list, idmlib::cc::CCInput64& result);


    /// @brief Given a query, get the query specific taxonomy information to display.
    ///
    /// @param docIdList The top doc id list to be taxonomy.
    /// @param queryTermIdList The query term id list, used for label ranking.
    /// @param docQueryPosition Query term position in doc id list.
    /// @param idManager The idManager, to convert id to ustring.
    /// @param taxonomyRep The output parameter.
    ///
    /// @return If succ.
//     bool getQuerySpecificTaxonomyInfo(
//         const std::vector<docid_t>& docIdList,
//         const izenelib::util::UString& queryStr,
//         uint32_t totalCount,
//         uint32_t numberFromSia,
//         TaxonomyRep& taxonomyRep
//         ,NEResultList& neList);


private:
    DISALLOW_COPY_AND_ASSIGN(TaxonomyGenerationSubManager);

    void CombineConceptItem_(const idmlib::cc::ConceptItem& from, idmlib::cc::ConceptItem& to);

    void GetNEResult_(const idmlib::cc::CCInput64& input, const std::vector<wdocid_t>& doc_list, std::vector<NEItem>& item_list);

//     void getNEList_(std::vector<std::pair<labelid_t, docid_t> >& inputPairList
//                     , const std::vector<uint32_t>& docIdList, uint32_t totalDocCount,uint32_t max, std::vector<ne_item_type >& neList);

private:
    TaxonomyPara taxonomy_param_;
    std::string path_;
    boost::shared_ptr<LabelManager> labelManager_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::cc::ConceptClustering<wdocid_t> algorithm_;
//     idmlib::cc::Algorithm<LabelManager> algorithm_;
    idmlib::cc::Parameters tgParams_;
//     idmlib::cc::Parameters peopParams_;
//     idmlib::cc::Parameters locParams_;
//     idmlib::cc::Parameters orgParams_;

    static const unsigned int default_levels_=3;
    static const unsigned int default_perLevelNum_=8;
    static const unsigned int default_candLabelNum_=250;


};
}

#endif /* TAXONOMYGENERATIONSUBMANAGER_H_ */
