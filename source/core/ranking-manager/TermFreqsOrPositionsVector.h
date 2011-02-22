#ifndef CORE_RANKING_MANAGER_TERM_FREQS_OR_POSITIONS_VECTOR_H
#define CORE_RANKING_MANAGER_TERM_FREQS_OR_POSITIONS_VECTOR_H
/**
 * @file core/ranking-manager/TermFreqsOrPositionsVector.h
 * @author Ian Yang
 * @date Created <2010-03-18 16:41:44>
 * @brief Vector of term positions array
 */
#include <common/inttypes.h>

#include <boost/assert.hpp>

#include <limits>
#include <utility>
#include <vector>

namespace sf1r {

/**
 * @brief vector of term freqs positions tuned for performance
 *
 * The class has very limit interface. It is tuned for best performance. All
 * positions are stored in one vector. The positions must be added term by term
 * without interleaving.
 *
 * A typical scenario using this class is fetching positions for several terms
 * one by one, and then positions are read only. The object can be reused by
 * calling \c reset().
 *
 * Term frequency also can be set without storing positions. \c setFreq() is
 * used to set term freq for specified term. Once it is called, the original
 * added positions for that term are abandoned. The freq set though by \c
 * setFreq() is also invalidated by using \c activate().
 */
class TermFreqsOrPositionsVector
{
public:
    typedef std::vector<loc_t>::size_type size_type;
    typedef std::vector<loc_t>::iterator position_iterator;
    typedef std::vector<loc_t>::const_iterator const_position_iterator;

    enum {
        kNotPosition = static_cast<size_type>(-1)
    };

    TermFreqsOrPositionsVector()
    : active_(kNotPosition)
    {}

    ///@brief Initializes the vector with specified size.
    explicit TermFreqsOrPositionsVector(size_type size)
    : active_(kNotPosition)
    , offsets_(size, std::pair<size_type, size_type>(kNotPosition, 0))
    {}

    ///@brief Number of terms
    size_type size() const
    {
        return offsets_.size();
    }

    ///@brief Number of terms
    bool empty() const
    {
        return offsets_.empty();
    }

    void resize(size_type size)
    {
        if (active_ >= size)
        {
            active_ = kNotPosition;
        }
        offsets_.resize(size, std::pair<size_type, size_type>(kNotPosition, 0));
    }

    ///@brief Clears data but reserve memory chunk
    void reset()
    {
        active_ = kNotPosition;
        offsets_.resize(0);
        positions_.resize(0);
    }

    /**
     * @brief Tests whether \a i -th term has ever been activated.
     *
     * Use this to avoid abandoning existing position block for term \a i.
     */
    bool activated(size_type i) const
    {
        return i < offsets_.size() && offsets_[i].first != kNotPosition;
    }

    /**
     * @brief Sets \a i -th term as active term.
     *
     * Allocate a new block for positions of this term at the end of the
     * all-in-one positions victor. Following `push' will add position in this
     * block. If the term has been set active before, old position for the term
     * is abandoned and the memory will not be reused unless `reset' is invoked.
     *
     * If \c i is current active term, this function has no effect.
     */
    void activate(size_type i)
    {
        BOOST_ASSERT(i < offsets_.size());

        if (active_ != i)
        {
            // points to the end of positions
            offsets_[i].first = positions_.size();
            offsets_[i].second = 0;

            active_ = i;
        }
    }

    /**
     * @brief add one slot for a new term and activate it
     */
    void addTerm()
    {
        offsets_.push_back(std::pair<size_type, size_type>(positions_.size(), 0));
        active_ = offsets_.size() - 1;
    }

    /**
     * @brief Tells the index of current active term.
     * @return Index of the active term of \c kNotPosition if no term is active.
     */
    size_type getActive() const
    {
        return active_;
    }

    /**
     * @brief only stores freq without storing positions
     */
    void setFreq(size_type i, size_type freq)
    {
        BOOST_ASSERT(i < offsets_.size());
        offsets_[i].first = kNotPosition;
        offsets_[i].second = freq;

        if (active_ == i)
        {
            active_ = kNotPosition;
        }
    }

    /**
     * @brief pushes a position for current activated term.
     */
    void push(loc_t position)
    {
        BOOST_ASSERT(active_ != kNotPosition);

        positions_.push_back(position);
        ++(offsets_[active_].second);
    }

    ///@brief begin iterator of \a i -th term
    position_iterator beginAt(size_type i)
    {
        BOOST_ASSERT(i < offsets_.size());
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first;
    }

    ///@brief end iterator of \a i -th term
    position_iterator endAt(size_type i)
    {
        BOOST_ASSERT(i < offsets_.size());
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first + offsets_[i].second;
    }

    ///@brief begin iterator of \a i -th term
    const_position_iterator beginAt(size_type i) const
    {
        BOOST_ASSERT(i < offsets_.size());
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first;
    }

    ///@brief end iterator of \a i -th term
    const_position_iterator endAt(size_type i) const
    {
        BOOST_ASSERT(i < offsets_.size());
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first + offsets_[i].second;
    }

    ///@brief number of positions of \a i -th term, i.e., tf of the term.
    size_type freqAt(size_type i) const
    {
        BOOST_ASSERT(i < offsets_.size());
        return offsets_[i].second;
    }

private:
    ///@brief active index for `push'
    size_type active_;

    ///@brief positions of all terms stored in one vector
    std::vector<loc_t> positions_;

    ///@brief start and end position of positions in \c positions_
    std::vector<std::pair<size_type, size_type> > offsets_;
};

} // namespace sf1r

#endif // CORE_RANKING_MANAGER_TERM_FREQS_OR_POSITIONS_VECTOR_H
