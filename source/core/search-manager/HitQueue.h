/**
 * @author Wei Cao
 * @date 2009/10/28
 * @brief contain various collectors for hits
 */

#ifndef _HIT_QUEUE_H_
#define _HIT_QUEUE_H_

#include <util/ustring/UString.h>

#include <common/type_defs.h>
#include "Sorter.h"
#include <util/PriorityQueue.h>


namespace sf1r{

class HitQueue {
public:
    virtual ~HitQueue() = 0;
    virtual bool insert(ScoreDoc doc) = 0;
    virtual ScoreDoc pop() = 0;
    virtual ScoreDoc top() = 0;
    virtual ScoreDoc operator[](size_t pos) = 0;
    virtual ScoreDoc getAt(size_t pos) = 0;
    virtual size_t size() = 0;
    virtual void clear() = 0;
};

class ScoreSortedHitQueue : public HitQueue
{
    class Queue_ : public izenelib::util::PriorityQueue<ScoreDoc>
    {
    public:
        Queue_(size_t size)
        {
            initialize(size);
        }
    protected:
        bool lessThan(const ScoreDoc& o1, const ScoreDoc& o2) const
        {
            return (o1.score < o2.score);
        }
    };

public:
    ScoreSortedHitQueue(size_t size) : queue_(size)
    {
    }

    ~ScoreSortedHitQueue(){}

    bool insert(ScoreDoc doc) {
        return queue_.insert(doc);
    }

    ScoreDoc pop() { return queue_.pop(); }
    ScoreDoc top() { return queue_.top(); }
    ScoreDoc operator[](size_t pos) { return queue_[pos]; }
    ScoreDoc getAt(size_t pos) { return queue_.getAt(pos); }
    size_t size() { return queue_.size(); }
    void clear() {}

private:
    Queue_ queue_;
};

class PropertySortedHitQueue : public HitQueue
{
    class Queue_ : public izenelib::util::PriorityQueue<ScoreDoc>
    {
    public:
        Queue_(boost::shared_ptr<Sorter> pSorter, size_t size)
            : pSorter_(pSorter)
        {
            if(pSorter)
                pSorter->getComparators();
            initialize(size);
        }
    protected:
        bool lessThan(const ScoreDoc& o1, const ScoreDoc& o2) const
        {
            return pSorter_->lessThan(o1,o2);
        }
    private:
        boost::shared_ptr<Sorter> pSorter_;
    };

public:
    PropertySortedHitQueue(boost::shared_ptr<Sorter> pSorter, size_t size)
        : queue_(pSorter, size)
    {
    }

    ~PropertySortedHitQueue() {}

    bool insert(ScoreDoc doc) {
         return queue_.insert(doc);
    }

    ScoreDoc pop() { return queue_.pop(); }
    ScoreDoc top() { return queue_.top(); }
    ScoreDoc operator[](size_t pos) { return queue_[pos]; }
    ScoreDoc getAt(size_t pos) { return queue_.getAt(pos); }
    size_t size() { return queue_.size(); }
    void clear() { }

private:
    Queue_ queue_;
};

}

#endif
