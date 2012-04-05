#ifndef SF1V5_RANKING_MANAGER_RANK_QUERY_PROPERTY_H
#define SF1V5_RANKING_MANAGER_RANK_QUERY_PROPERTY_H
/**
 * @file ranking-manager/RankQueryProperty.h
 * @author Ian Yang
 * @date Created <2010-03-19 14:16:10>
 * @brief required data related to a query property in ranking
 */
#include "TermFreqsOrPositionsVector.h"

#include <common/inttypes.h>

#include <boost/assert.hpp>

#include <iostream>
#include <sstream>

namespace sf1r {

/**
 * @brief Manages all data required in ranking in a query property
 *
 * Term related data are stored as parellal array.
 */
class RankQueryProperty
{
    ///@brief parallel array element
    struct data_t
    {
        data_t(termid_t t)
        : term(t), totalTermFreq(0), documentFreq(0)
        {}

        termid_t term;
        float totalTermFreq;
        float documentFreq;
        float maxTermFreq;
    };

public:
    typedef std::vector<data_t>::size_type size_type;
    typedef TermFreqsOrPositionsVector::const_position_iterator const_position_iterator;

    RankQueryProperty()
    : numDocs_(0), totalPropertyLength_(0), length_(0)
    {}

    ///@{
    ///@brief global data
    void setNumDocs(unsigned n)
    {
        numDocs_ = n;
    }
    unsigned getNumDocs() const
    {
        return numDocs_;
    }
    void setTotalPropertyLength(unsigned l)
    {
        totalPropertyLength_ = l;
    }
    unsigned getTotalPropertyLength() const
    {
        return totalPropertyLength_;
    }
    float getAveragePropertyLength() const
    {
        if (numDocs_ != 0)
        {
            return static_cast<float>(totalPropertyLength_)
                / numDocs_;
        }

        return 0.0F;
    }

    void setQueryLength(unsigned l)
    {
        length_ = l;
    }
    unsigned getQueryLength() const
    {
        return length_;
    }
    ///@}

    ///@{
    ///@brief term based data. addTerm() adds one term, and all following
    ///functions operates on the last added term. 
    /**
     * @brief add a term
     */
    void addTerm(termid_t term)
    {
        attributes_.push_back(term);
        freqsOrPositionsVector_.addTerm();
    }

    void setTotalTermFreq(float freq)
    {
        BOOST_ASSERT(!attributes_.empty());
        attributes_.back().totalTermFreq = freq;
    }
    void setDocumentFreq(float freq)
    {
        BOOST_ASSERT(!attributes_.empty());
        attributes_.back().documentFreq = freq;
    }
    void setMaxTermFreq(float freq)
    {
        BOOST_ASSERT(!attributes_.empty());
        attributes_.back().maxTermFreq = freq;
    }

    void setTermFreq(unsigned freq)
    {
        BOOST_ASSERT(!freqsOrPositionsVector_.empty());
        freqsOrPositionsVector_.setFreq(freqsOrPositionsVector_.size() - 1, freq);
    }
    void pushPosition(loc_t position)
    {
        BOOST_ASSERT(!freqsOrPositionsVector_.empty());
        freqsOrPositionsVector_.activate(freqsOrPositionsVector_.size() - 1);
        freqsOrPositionsVector_.push(position);
    }
    ///@}

    ///@brief number of terms
    size_type size() const
    {
        return attributes_.size();
    }

    ///@brief contains any term?
    bool empty() const
    {
        return attributes_.empty();
    }

    termid_t termAt(size_type i) const
    {
        BOOST_ASSERT(i < size());
        return attributes_[i].term;
    }

    float totalTermFreqAt(size_type i) const
    {
        BOOST_ASSERT(i < size());
        return attributes_[i].totalTermFreq;
    }

    float documentFreqAt(size_type i) const
    {
        BOOST_ASSERT(i < size());
        return attributes_[i].documentFreq;
    }

    float maxTermFreqAt(size_type i) const
    {
        BOOST_ASSERT(i < size());
        return attributes_[i].maxTermFreq;
    }

    size_type termFreqAt(size_type i) const
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

    ///@brief for testing
    void print(std::ostream& out = std::cout)
    {
        using namespace std;
        stringstream ss;
        ss << "[RankQueryProperty]" << endl
           << "documents in collection: " << numDocs_ << endl
           << "terms in property: " << totalPropertyLength_ << endl
           << "length of query: " << length_ << endl

           << "terms info of collection (id, tf, df): " << endl;
        std::vector<data_t>::iterator iter;
        for (iter = attributes_.begin(); iter != attributes_.end(); iter++)
        {
            ss << iter->term << ", " << iter->documentFreq << ", " << iter->totalTermFreq << endl;
        }

        ss << "terms info of query (id, tf)" << endl;
        for (size_t i = 0; i < attributes_.size(); i++)
        {
            ss << attributes_[i].term << freqsOrPositionsVector_.freqAt(i) << endl;
        }

        out << ss.str();
    }
private:
    std::vector<data_t> attributes_;

    ///@brief term freqs or positions in query
    TermFreqsOrPositionsVector freqsOrPositionsVector_;

    unsigned numDocs_;             ///< total # of documents in collection
    unsigned totalPropertyLength_; ///< total # of terms in property
    unsigned length_;              ///< length of query
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_RANK_QUERY_PROPERTY_H
