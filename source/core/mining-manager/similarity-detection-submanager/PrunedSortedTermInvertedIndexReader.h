#ifndef CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_PRUNED_SORTED_TERM_INVERTED_INDEX_READER_H
#define CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_PRUNED_SORTED_TERM_INVERTED_INDEX_READER_H
/**
 * @file core/mining-manager/similarity-detection-submanager/PrunedSortedTermInvertedIndexReader.h
 * @author Ian Yang&Jinglei
 * @date Created <2009-10-12 12:03:42>
 * @date Updated <2009-10-27 15:49:14>
 */
#include <common/type_defs.h>

#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/AbsTermReader.h>
#include <ir/index_manager/index/AbsTermIterator.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assert.hpp>

#include <string>
#include <limits>

namespace sf1r {
namespace sim {
using izenelib::ir::indexmanager::IndexReader;
using izenelib::ir::indexmanager::TermReader;
using izenelib::ir::indexmanager::TermIterator;

/**
 * @brief The class to access the pruned terms from the Index.
 */
class PrunedSortedTermInvertedIndexReader : boost::noncopyable
{
public:
    typedef std::size_t size_type;
    typedef termid_t id_type;
    typedef float weight_type; // weight is tf/docLength

    typedef std::pair<docid_t, weight_type> doc_weight_pair_type;
    typedef std::vector<doc_weight_pair_type> doc_weight_list_type;

    PrunedSortedTermInvertedIndexReader(
            float termNumLimit,
            float docNumLimit,
            IndexReader* reader,
            propertyid_t fieldId,
            const std::string& field,
            size_t numDocs,
            float avgDocLen);

    void rewind()
    {
        cursor_ = 0;
    }
    bool next()
    {
        ++cursor_;
        return cursor_ <= idFreqList_.size();
    }

    id_type getId()
    {
        BOOST_ASSERT(0 < cursor_ && cursor_ <= idFreqList_.size());
        return idFreqList_[cursor_ - 1].first;
    }
    size_type getDocFreq()
    {
        BOOST_ASSERT(0 < cursor_ && cursor_ <= idFreqList_.size());
        return idFreqList_[cursor_ - 1].second;
    }


    void getDocWeightList(doc_weight_list_type& result);
    size_t size() const
    {
        return idFreqList_.size();
    }

private:
    IndexReader* indexReader_;
    boost::scoped_ptr<TermReader> termReader_;

    propertyid_t fieldId_;
    std::string field_;

    std::vector<std::pair<id_type, size_type> > idFreqList_;
    size_type cursor_;
    size_t numDocs_;
    float avgDocLen_;

    static const float default_k1_=1.2;
    static const float default_b_=0.75;
};
}} // namespace sf1r::sim

#endif // CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_PRUNED_SORTED_TERM_INVERTED_INDEX_READER_H
