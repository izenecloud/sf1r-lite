///
/// @file t_SequenceMerger.cpp
/// @brief test SequenceMerger in merging sorted sequences
/// @author Jun Jiang
/// @date Created 2012-04-08
///

#include <aggregator-manager/SequenceMerger.h>
#include "../recommend-manager/test_util.h"

#include <boost/test/unit_test.hpp>
#include <vector>
#include <list>
#include <iterator> // back_inserter
#include <functional> // greater

using namespace std;
using namespace sf1r;

template<typename ContainerType>
void checkCollection(
    const ContainerType& result,
    const std::string& goldStr
)
{
    typedef typename ContainerType::value_type value_type;
    vector<value_type> goldResult;
    split_str_to_items(goldStr, goldResult);

    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(),
                                  goldResult.begin(), goldResult.end());
}

BOOST_AUTO_TEST_SUITE(SequenceMergerTest)

BOOST_AUTO_TEST_CASE(merge_ascending_vector)
{
    vector<int> seq1;
    split_str_to_items("1 3 5 7", seq1);

    vector<int> seq2;
    split_str_to_items("2 4 6 8", seq2);

    SequenceMerger<vector<int>::const_iterator> merger;
    merger.addSequence(seq1.begin(), seq1.end());
    merger.addSequence(seq2.begin(), seq2.end());

    vector<int> result;
    merger.merge(back_inserter(result));

    checkCollection(result, "1 2 3 4 5 6 7 8");
}

BOOST_AUTO_TEST_CASE(merge_descending_vector)
{
    vector<int> seq1;
    split_str_to_items("7 5 3 1", seq1);

    vector<int> seq2;
    split_str_to_items("8 5 4", seq2);

    vector<int> seq3;
    split_str_to_items("9 6 5 2", seq3);

    SequenceMerger<vector<int>::const_iterator> merger;
    merger.addSequence(seq1.begin(), seq1.end());
    merger.addSequence(seq2.begin(), seq2.end());
    merger.addSequence(seq3.begin(), seq3.end());

    vector<int> result;
    merger.merge(back_inserter(result), greater<int>());

    checkCollection(result, "9 8 7 6 5 5 5 4 3 2 1");
}

BOOST_AUTO_TEST_CASE(merge_list)
{
    list<int> seq1;
    split_str_to_items("1 3 5", seq1);

    list<int> seq2;
    split_str_to_items("2 3", seq2);

    list<int> seq3;

    list<int> seq4;
    split_str_to_items("1", seq4);

    SequenceMerger<list<int>::const_iterator> merger;
    merger.addSequence(seq1.begin(), seq1.end());
    merger.addSequence(seq2.begin(), seq2.end());
    merger.addSequence(seq3.begin(), seq3.end());
    merger.addSequence(seq4.begin(), seq4.end());

    list<int> result;
    merger.merge(back_inserter(result));

    checkCollection(result, "1 1 2 3 3 5");
}

BOOST_AUTO_TEST_SUITE_END() 
