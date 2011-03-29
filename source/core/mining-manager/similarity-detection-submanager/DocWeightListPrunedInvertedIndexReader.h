#ifndef CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_DOC_WEIGHT_LIST_PRUNED_INVERTED_INDEX_READER_H
#define CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_DOC_WEIGHT_LIST_PRUNED_INVERTED_INDEX_READER_H
/**
 * @file core/mining-manager/similarity-detection-submanager/DocWeightListPrunedInvertedIndexReader.h
 * @author Ian Yang&Jinglei
 * @date Created <2009-10-12 13:49:09>
 * @date Updated <2009-10-16 14:21:16>
 */

#include <boost/shared_ptr.hpp>

#include <util/functional.h>

namespace sf1r {
namespace sim {

template<typename ReaderT>
class DocWeightListPrunedInvertedIndexReader
{
public:
    typedef typename ReaderT::size_type size_type;
    typedef typename ReaderT::id_type id_type;
    typedef typename ReaderT::weight_type weight_type;

    typedef typename ReaderT::doc_weight_pair_type doc_weight_pair_type;
    typedef typename ReaderT::doc_weight_list_type doc_weight_list_type;

    DocWeightListPrunedInvertedIndexReader(const boost::shared_ptr<ReaderT>& reader, size_type limit)
    : base_(reader), limit_(limit)
    {}

    void rewind()
    {
        base_->rewind();
    }
    bool next()
    {
        return base_->next();
    }

    id_type getId()
    {
        return base_->getId();
    }
    size_type getDocFreq()
    {
        return base_->getDocFreq();
    }

    void getDocWeightList(doc_weight_list_type& result);

    ReaderT& base()
    {
        return *base_;
    }
    const ReaderT& base() const
    {
        return *base_;
    }

    size_t size() const
    {
    	return base_->size();
    }

private:
    boost::shared_ptr<ReaderT> base_;
    size_type limit_;
};

template<typename ReaderT>
void DocWeightListPrunedInvertedIndexReader<ReaderT>::getDocWeightList(
    doc_weight_list_type& result
)
{
    doc_weight_list_type tmp;
    base_->getDocWeightList(tmp);

    if (tmp.size() > limit_)
    {
        // After term pruning, the list is short. So use memory sort directly.
        std::sort(
            tmp.begin(),
            tmp.end(),
            ::izenelib::util::second_greater<doc_weight_pair_type>()
        );

        tmp.resize(limit_);
    }

    result.swap(tmp);
}

}} // namespace sf1r::sim
#endif // CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_DOC_WEIGHT_LIST_PRUNED_INVERTED_INDEX_READER_H
