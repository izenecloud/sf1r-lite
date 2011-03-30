
/**
 * @file CandidateScoreItem.h
 * @brief  Defination of CandidateScoreItem,it is used to store one candidate and its score.
 * @author  Jinglei Zhao
 * @date July 13,2009
 * @version v1.0
 */
#ifndef __CANDIDATESCOREITEM_H_
#define __CANDIDATESCOREITEM_H_

#include<string>
#include <util/ustring/UString.h>
#include <boost/serialization/serialization.hpp>

class CandidateScoreItem
{
public:
    CandidateScoreItem() : path_(), score_(-1.0)
    {
    }
    ///  Added a new constructor.
    CandidateScoreItem(const izenelib::util::UString& path, float score) : path_(path), score_(score)
    {
    }

    ///the candidate
    izenelib::util::UString path_;
    ///the document's score.
    double score_;

    ///For sorting the scoreItem in ascending order of score.
    bool operator<(const CandidateScoreItem& rightItem) const;

private:
    /// For serialization
    friend class boost::serialization::access;
    ///boost serialization
    template<class Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & path_;
        ar & score_;
    } // serialize()

};

#endif /* CANDIDATESCOREITEM_H_ */
