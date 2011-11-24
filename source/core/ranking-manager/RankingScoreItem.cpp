/**
* Name        : RankingScoreItem.cpp
* Author      : Jinglei Zhao & Dohyun Yun
* Version     : v1.0
* Copyright   : Wisenut & iZENESoft
* Description : TA document's ranking score information.
*/

#include "RankingScoreItem.h"

using namespace std;
using namespace sf1r;


RankingScoreItem::RankingScoreItem() {
    /// TODO Auto-generated constructor stub

}

RankingScoreItem::~RankingScoreItem() {
    /// TODO Auto-generated destructor stub
}

bool RankingScoreItem::operator<(const RankingScoreItem& rightItem)   const
{
    return score_>rightItem.score_;
}
