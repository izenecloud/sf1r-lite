#ifndef UTIL_SDB_CURSOR_ITERATOR_H
#define UTIL_SDB_CURSOR_ITERATOR_H
/**
 * @file util/SDBCursorIterator.h
 * @author Ian Yang
 * @date Created <2009-08-25 11:08:15>
 * @date Updated <2009-09-02 12:56:37>
 * @brief iterator wrapper of SDBCursor
 * @todo should move to izenelib util
 */
#include <boost/iterator/iterator_facade.hpp>

namespace izenelib
{
namespace util
{

template<typename AM>
class SDBCursorIterator;

namespace detail
{
template<typename AM>
struct SDBCursorIteratorBase
{
    typedef typename AM::data_type data_type;

    typedef boost::iterator_facade<SDBCursorIterator<AM> , data_type,
    boost::forward_traversal_tag, const data_type&> type;
};

} // namespace detail

/**
 * @brief forward iterator wrapper of SDBCursor used in AccessMethod
 * iteration.
 */
template<typename AM>
class SDBCursorIterator: public detail::SDBCursorIteratorBase<AM>::type
{
    friend class boost::iterator_core_access;
    typedef typename detail::SDBCursorIteratorBase<AM>::type super;
    typedef typename AM::data_type data_type;

public:
    /**
     * @brief constructs a end sentry
     * @return iterator as end
     */
    SDBCursorIterator() :
            am_(0), cur_(), data_()
    {
    }

    /**
     * @brief begin iterator
     * @param am AccessMethod object supporting SDBCursor
     * @return iterator as begin
     */
    explicit SDBCursorIterator(AM& am) :
            am_(&am), cur_(am.get_first_locn()), data_()
    {
        if (!(am.seq(cur_, data_)))
        {
            am_ = 0;
        }
    }

private:
    typename super::reference dereference() const
    {
        return data_;
    }

    void increment()
    {
        if (am_ && !am_->seq(cur_, data_))
        {
            am_ = 0;
        }
    }

    bool equal(const SDBCursorIterator<AM>& rhs) const
    {
        return (this == &rhs) || (am_ == 0 && rhs.am_ == 0) || (am_ == rhs.am_
                && cur_ == rhs.cur_);
    }

    AM* am_;
    typename AM::SDBCursor cur_;
    data_type data_;
};

}
} // namespace izenelib::util

#endif // UTIL_SDB_CURSOR_ITERATOR_H
