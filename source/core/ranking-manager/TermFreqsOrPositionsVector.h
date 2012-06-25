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
#include <memory.h>

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
    ,positionusedsize_(0)
    ,usedsize_(0)
    {}

    ///@brief Initializes the vector with specified size.
    explicit TermFreqsOrPositionsVector(size_type size)
    : active_(kNotPosition)
      ,positionusedsize_(0)
      , offsets_(size, std::pair<size_type, size_type>(kNotPosition, 0))
      ,usedsize_(size)
    {}

    ///@brief Number of terms
    size_type size() const
    {
        return usedsize_;
        //return offsets_.size();
    }

    ///@brief Number of terms
    bool empty() const
    {
        return usedsize_ == 0;
        //return offsets_.empty();
    }

    void resize(size_type size)
    {
        if (active_ >= size)
        {
            active_ = kNotPosition;
        }
        if(size > usedsize_)
            offsets_.resize(size, std::pair<size_type, size_type>(kNotPosition, 0));
        usedsize_ = size;
    }

    ///@brief Clears data but reserve memory chunk
    void reset()
    {
        active_ = kNotPosition;
        offsets_.resize(0);
        positions_.resize(0);
        usedsize_ = 0;
        positionusedsize_ = 0;
    }
    
    ///@brief  for efficient, only fill the data with zeros, a resize need be called after this call.
    void initdata()
    {
        active_ = kNotPosition;
        memset(&offsets_[0], 0, sizeof(offsets_[0])*offsets_.size());
        memset(&positions_[0], 0, sizeof(positions_[0])*positions_.size());
        usedsize_ = 0;
        positionusedsize_ = 0;
    }

    void resize_and_initdata(size_type size)
    {
        active_ = kNotPosition;
        positionusedsize_ = 0;
        if (active_ >= size)
        {
            active_ = kNotPosition;
        }
        if(size > usedsize_)
            offsets_.resize(size, std::pair<size_type, size_type>(kNotPosition, 0));
        usedsize_ = size;

        memset(&offsets_[0], 0, sizeof(offsets_[0])*size);
        //memset(&positions_[0], 0, sizeof(positions_[0])*size);
    }

    /**
     * @brief Tests whether \a i -th term has ever been activated.
     *
     * Use this to avoid abandoning existing position block for term \a i.
     */
    bool activated(size_type i) const
    {
        return i < usedsize_ && offsets_[i].first != kNotPosition;
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
        BOOST_ASSERT(i < usedsize_);

        if (active_ != i)
        {
            // points to the end of positions
            offsets_[i].first = positionusedsize_;
            offsets_[i].second = 0;

            active_ = i;
        }
    }

    /**
     * @brief add one slot for a new term and activate it
     */
    void addTerm()
    {
        if(usedsize_ == offsets_.size())
        {
            offsets_.push_back(std::pair<size_type, size_type>(positionusedsize_, 0));
            ++usedsize_;
        }
        else if(usedsize_ < offsets_.size())
        {
            offsets_[usedsize_++] = std::pair<size_type, size_type>(positionusedsize_, 0);
        }
        active_ = usedsize_ - 1;
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
        BOOST_ASSERT(i < usedsize_);
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

        if(positionusedsize_ == positions_.size())
        {
            positions_.push_back(position);
            ++positionusedsize_;
        }
        else
            positions_[positionusedsize_++] = position;
        ++(offsets_[active_].second);
    }

    ///@brief begin iterator of \a i -th term
    position_iterator beginAt(size_type i)
    {
        BOOST_ASSERT(i < usedsize_);
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first;
    }

    ///@brief end iterator of \a i -th term
    position_iterator endAt(size_type i)
    {
        BOOST_ASSERT(i < usedsize_);
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first + offsets_[i].second;
    }

    ///@brief begin iterator of \a i -th term
    const_position_iterator beginAt(size_type i) const
    {
        BOOST_ASSERT(i < usedsize_);
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first;
    }

    ///@brief end iterator of \a i -th term
    const_position_iterator endAt(size_type i) const
    {
        BOOST_ASSERT(i < usedsize_);
        return offsets_[i].first == kNotPosition
            ? positions_.end()
            : positions_.begin() + offsets_[i].first + offsets_[i].second;
    }

    ///@brief number of positions of \a i -th term, i.e., tf of the term.
    size_type freqAt(size_type i) const
    {
        BOOST_ASSERT(i < usedsize_);
        return offsets_[i].second;
    }

private:
    ///@brief active index for `push'
    size_type active_;

    ///@brief positions of all terms stored in one vector
    std::vector<loc_t> positions_;
    std::size_t positionusedsize_;

    ///@brief start and end position of positions in \c positions_
    std::vector<std::pair<size_type, size_type> > offsets_;
    std::size_t usedsize_;
};

} // namespace sf1r

#endif // CORE_RANKING_MANAGER_TERM_FREQS_OR_POSITIONS_VECTOR_H
