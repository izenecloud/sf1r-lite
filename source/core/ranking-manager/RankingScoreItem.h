/**
* Name        : RankingScoreItem.h
* Author      : Jinglei Zhao & Dohyun Yun
* Version     : v1.0
* Copyright   : Wisenut & iZENESoft
* Description : A document's ranking score information.
*/

#ifndef RANKINGSCOREITEM_H_
#define RANKINGSCOREITEM_H_

#include <string>
#include <utility>

#include <util/izene_serialization.h>

using namespace std;

namespace sf1r{

///This class represents a document's ranking score information.
class RankingScoreItem {
public:
    RankingScoreItem();
    //# SeungEun added a new constructor.
    RankingScoreItem(unsigned int documentID, float score) : docId_(documentID), score_(score)
    {
    }

    RankingScoreItem(const std::pair<unsigned, float>& idScore)
    : docId_(idScore.first), score_(idScore.second)
    {
    }
    RankingScoreItem& operator=(const std::pair<unsigned, float>& idScore)
    {
        docId_ = idScore.first;
        score_ = idScore.second;

        return *this;
    }

    virtual ~RankingScoreItem();
    ///the document's unique ID.
    unsigned int docId_;
    ///the document's score.
    float score_;
    ///For sorting the scoreItem in ascending order of score.
    bool operator<(const RankingScoreItem& rightItem) const;

private:
    // For serialization
    friend class boost::serialization::access;

    template<class Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & docId_;
        ar & score_;
    } // serialize()

    DATA_IO_LOAD_SAVE(RankingScoreItem, &docId_&score_)

};
}
#endif /* RANKINGSCOREITEM_H_ */
