#ifndef SF1V5_MINING_CONFIG_H_
#define SF1V5_MINING_CONFIG_H_

#include <util/ustring/UString.h>

#include <stdint.h>
#include <string>
#include <map>

namespace sf1r
{

/**
  * @brief   stores TG parameters
  */
class TaxonomyPara
{
public:
    TaxonomyPara() {}
private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & top_docnum;
        ar & levels;
        ar & per_levelnum;
        ar & candidate_labelnum;
        ar & enable_nec;
        ar & max_peopnum;
        ar & max_locnum;
        ar & max_orgnum;
    }

public:
    /**
      * @brief the number of top docs used to generate the taxonomy.
      */
    uint32_t top_docnum;

    /// @brief  the levels in the taxonomy.
    uint32_t levels;

    /**
      * @brief  the number of nodes in each level
      */
    uint32_t per_levelnum;

    /**
      * @brief  the maximum number of labels inputed to the taxonomy generation.
      */
    uint32_t candidate_labelnum;

    bool enable_nec;

    uint32_t max_peopnum;

    uint32_t max_locnum;

    uint32_t max_orgnum;

};

/**
  * @brief   the parameters for document mining
  */
class DocumentMiningPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & docnum_limit & cron ;
    }
public:
    /**
      * @brief  the limit number of documents to do mining
      */
    uint32_t docnum_limit;

    std::string cron;
};

/**
  * @brief   the parameters for autofill
  */
class AutofillPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & cron ;
    }
public:
    std::string cron;
};

/**
  * @brief   the parameters for autofill
  */
class FuzzyIndexMergePara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & cron ;
    }
public:
    std::string cron;
};

/**
  * @brief   the parameters for query recommend
  */
class RecommendPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & recommend_num & cron ;
    }
public:
    /**
      * @brief  the number of recommend items to display
      */
    uint32_t recommend_num;

    std::string cron;


};

/**
  * @brief   Stores "Store strategy" configuration of IndexManager
  */
class SimilarityPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & docnum_limit;
        ar & termnum_limit;
        ar & enable_esa;
    }
public:
    /// @brief  the number of docs in idf accumulators.
    uint32_t docnum_limit;
    /// @brief the number of terms in tf accumulators.
    uint32_t termnum_limit;
    /// @brief whether enable Explicit Semantic Analysis(ESA) for similarity
    bool enable_esa;
};

/**
  * @brief   the parameters for document classification
  */
class DocumentClassificationPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & customize_training & encoding_type;
    }
public:
    /**
      * @brief  the number of recommend items to display
      */
    bool customize_training;
    izenelib::util::UString::EncodingType encoding_type;

};

/**
  * @brief   the parameters for image search.
  */
class IsePara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & build_img_index & store_locally & max_img_num& related_img_num;
    }
public:
    /**
      * @brief  the number of recommend items to display
      */
    bool build_img_index ;
    bool store_locally;
    uint32_t max_img_num;
    uint32_t related_img_num;

};

/**
  * @brief   the parameters for query correction.
  */
class QueryCorrectionPara
{

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & enableEK & enableCN;
    }
public:
    bool enableEK;
    bool enableCN;

};

/**
 * @brief for configuration <ProductRankingPara>
 */
class ProductRankingPara
{
public:
    std::string cron;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & cron;
    }
};

class MiningConfig
{

public:
    //----------------------------  CONSTRUCTORS  ----------------------------

    MiningConfig() {}

    ~MiningConfig() {}

private:

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & taxonomy_param;
        ar & autofill_param;
        ar & fuzzyIndexMerge_param;
        ar & recommend_param;
        ar & similarity_param;
        ar & dc_param;
        ar & ise_param;
        ar & query_correction_param;
        ar & product_ranking_param;
    }

public:

    TaxonomyPara taxonomy_param;
    DocumentMiningPara dcmin_param;
    AutofillPara autofill_param;
    FuzzyIndexMergePara fuzzyIndexMerge_param;
    RecommendPara recommend_param;
    SimilarityPara similarity_param;

    DocumentClassificationPara dc_param;

    IsePara ise_param;

    QueryCorrectionPara query_correction_param;

    ProductRankingPara product_ranking_param;
};

} // namespace

#endif //_MINING_CONFIG_H_
