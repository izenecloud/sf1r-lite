/**
 * @file ItemIterator.h
 * @author Jun Jiang
 * @date 2011-05-03
 */

#ifndef ITEM_ITERATOR_H
#define ITEM_ITERATOR_H

#include <idmlib/resys/ItemIterator.h>

namespace sf1r
{

class ItemIterator : public idmlib::recommender::ItemIterator
{
public:
    ItemIterator(uint32_t min, uint32_t max)
        : min_(min)
        , max_(max)
        , now_(min)
    {
    }

    bool hasNext()
    {
        return now_ <= max_;
    }

    uint32_t next()
    {
        return now_++;
    }

    void reset()
    {
        now_ = min_;
    }

private:
    const uint32_t min_;
    const uint32_t max_;
    uint32_t now_;
};

} // namespace sf1r

#endif // ITEM_ITERATOR_H
