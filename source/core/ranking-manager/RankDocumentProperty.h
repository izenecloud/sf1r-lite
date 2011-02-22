#ifndef SF1V5_RANKING_MANAGER_RANK_DOCUMENT_PROPERTY_H
#define SF1V5_RANKING_MANAGER_RANK_DOCUMENT_PROPERTY_H
/**
 * @file ranking-manager/RankDocumentProperty.h
 * @author Ian Yang
 * @date Created <2010-03-24 14:18:53>
 * @brief required data in hit document in ranking
 */
#include "TermFreqsOrPositionsVector.h"

#include <common/inttypes.h>

namespace sf1r {

/**
 * @brief Manages all data required in ranking in a hit document property
 */
class RankDocumentProperty
{
public:
    typedef TermFreqsOrPositionsVector::size_type size_type;
    typedef TermFreqsOrPositionsVector::const_position_iterator const_position_iterator;

    RankDocumentProperty()
    : length_(0)
    {}

    ///@{
    ///@brief global data
    unsigned docLength() const
    {
        return length_;
    }
    void setDocLength(unsigned length)
    {
        length_ = length;
    }
    ///@}

    ///@{
    ///@brief term based data.
    size_type size() const
    {
        return freqsOrPositionsVector_.size();
    }
    bool empty() const
    {
        return freqsOrPositionsVector_.empty();
    }
    void resize(size_type size)
    {
        freqsOrPositionsVector_.resize(size);
    }
    void reset()
    {
        freqsOrPositionsVector_.reset();
    }
    bool activated(size_type i) const
    {
        return freqsOrPositionsVector_.activated(i);
    }
    void activate(size_type i)
    {
        freqsOrPositionsVector_.activate(i);
    }
    void setTermFreq(size_type i, size_type freq)
    {
        freqsOrPositionsVector_.setFreq(i, freq);
    }
    void pushPosition(loc_t position)
    {
        freqsOrPositionsVector_.push(position);
    }

    unsigned termFreqAt(size_type i) const
    {
        return freqsOrPositionsVector_.freqAt(i);
    }

    const_position_iterator termPositionsBeginAt(size_type i) const
    {
        return freqsOrPositionsVector_.beginAt(i);
    }

    const_position_iterator termPositionsEndAt(size_type i) const
    {
        return freqsOrPositionsVector_.endAt(i);
    }

private:
    ///@brief term freqs or positions in query
    TermFreqsOrPositionsVector freqsOrPositionsVector_;

    unsigned length_;
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_RANK_DOCUMENT_PROPERTY_H
