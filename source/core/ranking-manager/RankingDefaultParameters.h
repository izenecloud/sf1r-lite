#ifndef SF1V5_RANKING_MANAGER_RANKING_DEFAULT_PARAMETERS_H
#define SF1V5_RANKING_MANAGER_RANKING_DEFAULT_PARAMETERS_H
/**
 * @file sf1r/ranking-manager/RankingDefaultParameters.h
 * @author Ian Yang
 * @date Created <2009-09-24 10:16:32>
 * @date Updated <2009-09-24 18:35:57>
 */

namespace sf1r {

///@brief k1 Parameters of the BM25 ranking formula, 1~2
const float BM25_RANKER_K1 = 1.2;
///@brief 0.75 is recommended
const float BM25_RANKER_B  = 0.75;
///@brief 0 ~ 1000
const float BM25_RANKER_K3 = 1000;

///@brief the smoothing parameter for Dirichlet smoothing
const float LANGUAGE_RANKER_SMOOTH = 2000;
///@brief the paramter in BTS-KL model.
const float LANGUAGE_RANKER_PROXIMITY = 1000;

///@brief Parameter for BTS method.
const float TERM_PROXIMITY_W = 3;

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_RANKING_DEFAULT_PARAMETERS_H
