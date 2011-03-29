/*
 * CandidateScoreItem.cpp
 *
 *  Created on: 2009-7-13
 *      Author: jinglei
 */

#include "CandidateScoreItem.h"


///For sorting the scoreItem in ascending order of score.
bool CandidateScoreItem::operator<(const CandidateScoreItem& rightItem) const
{
	return(this->score_>rightItem.score_);
}
