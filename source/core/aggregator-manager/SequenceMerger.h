/**
 * @file SequenceMerger.h
 * @brief a class used to merge sorted sequences.
 * @author Jun Jiang
 * @date 2012-04-08
 */

#ifndef SF1R_SEQUENCE_MERGER_H
#define SF1R_SEQUENCE_MERGER_H

#include <list>
#include <utility> // pair
#include <algorithm> // min_element, copy

namespace sf1r
{

template <class InputIterator>
class SequenceMerger
{
public:
    /**
     * add a sequence [first, last) to merge.
     * @pre the sequence [first, last) should already be sorted in order
     */
    void addSequence(InputIterator first, InputIterator last);

    /**
     * merge all sequences and output to @p result, the elements in sequences
     * are compared using @c operator<.
     * @param result the output iterator
     * @return an iterator pointing to the past-the-end element in the result
     * sequence
     */
    template <class OutputIterator>
    OutputIterator merge(OutputIterator result);

    /**
     * merge all sequences and output to @p result.
     * @param result the output iterator
     * @param comp comparison function object, taking two values from the
     * sequences, returns @c true if the 1st argument goes before the 2nd
     * argument in the result sequence, returns @c false otherwise.
     * @return an iterator pointing to the past-the-end element in the result
     * sequence
     */
    template <class OutputIterator, class Compare>
    OutputIterator merge(OutputIterator result, Compare comp);

private:
    /**
     * @param seqPairComp comparison function object, taking two
     * @c SeqPair arguments.
     */
    template <class OutputIterator, class SeqPairCompare>
    OutputIterator mergeImpl_(OutputIterator result, SeqPairCompare seqPairComp);

private:
    // current iterator, the last iterator
    typedef std::pair<InputIterator, InputIterator> SeqPair;
    typedef std::list<SeqPair> SequenceList;

    SequenceList sequences_;

    struct DefaultLess;
    template <class Compare> struct CustomLess;
};

template <class InputIterator>
struct SequenceMerger<InputIterator>::DefaultLess
{
    bool operator()(
        const SeqPair& seqPair1,
        const SeqPair& seqPair2
    )
    {
        return *seqPair1.first < *seqPair2.first;
    }
};

template <class InputIterator>
template <class Compare>
struct SequenceMerger<InputIterator>::CustomLess
{
    Compare& compare_;

    CustomLess(Compare& compare) : compare_(compare) {}

    bool operator()(
        const SeqPair& seqPair1,
        const SeqPair& seqPair2
    )
    {
        return compare_(*seqPair1.first, *seqPair2.first);
    }
};

template <class InputIterator>
void SequenceMerger<InputIterator>::addSequence(InputIterator first, InputIterator last)
{
    if (first != last)
    {
        sequences_.push_back(SeqPair(first, last));
    }
}

template <class InputIterator>
template <class OutputIterator>
OutputIterator SequenceMerger<InputIterator>::merge(OutputIterator result)
{
    return mergeImpl_(result, DefaultLess());
}

template <class InputIterator>
template <class OutputIterator, class Compare>
OutputIterator SequenceMerger<InputIterator>::merge(OutputIterator result, Compare comp)
{
    return mergeImpl_(result, CustomLess<Compare>(comp));
}

template <class InputIterator>
template <class OutputIterator, class SeqPairCompare>
OutputIterator SequenceMerger<InputIterator>::mergeImpl_(OutputIterator result, SeqPairCompare seqPairComp)
{
    while (sequences_.size() > 1)
    {
        typename SequenceList::iterator minSeqIt =
            std::min_element(sequences_.begin(), sequences_.end(), seqPairComp);

        InputIterator& curIt = minSeqIt->first;
        InputIterator& endIt = minSeqIt->second;

        *result++ = *curIt++;

        if (curIt == endIt)
        {
            sequences_.erase(minSeqIt);
        }
    }

    if (sequences_.size() == 1)
    {
        SeqPair& seqPair = sequences_.front();
        result = std::copy(seqPair.first, seqPair.second, result);
        sequences_.clear();
    }

    return result;
}

} // namespace sf1r

#endif // SF1R_SEQUENCE_MERGER_H
