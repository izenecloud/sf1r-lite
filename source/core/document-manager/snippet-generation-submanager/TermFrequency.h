///  @file TermFrequency.h
///  @date 6/5/2008
///  @author QuangNT
///  @brief


#if !defined(_TERMFREQUENCY_H)
#define _TERMFREQUENCY_H

/// @brief this class implements a term frequency
namespace sf1r {
class TermFrequency {
public:
	/// term identifier
    unsigned int termId;

	/// number of documents the term is in
    unsigned int termFreq;

    // overload assignment operator
    TermFrequency& operator=(const TermFrequency& termFrequency)
    {
        termId = termFrequency.termId;
        termFreq = termFrequency.termFreq;

        return *this;
    }
};
} //namespace sf1r

#endif  //_TERMFREQUENCY_H
