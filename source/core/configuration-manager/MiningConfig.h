#ifndef SF1V5_MINING_CONFIG_H_
#define SF1V5_MINING_CONFIG_H_

#include <util/ustring/UString.h>

#include <stdint.h>
#include <string>
#include <map>

namespace sf1r
{
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

/**
 * @brief for configuration <MiningTaskPara>
 */
class MiningTaskPara
{
public:
    std::size_t threadNum;

    MiningTaskPara() : threadNum(1) {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & threadNum;
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
        ar & dcmin_param;
        ar & product_ranking_param;
        ar & mining_task_param;
    }

public:

    DocumentMiningPara dcmin_param;
    ProductRankingPara product_ranking_param;
    MiningTaskPara mining_task_param;
};

} // namespace

#endif //_MINING_CONFIG_H_
