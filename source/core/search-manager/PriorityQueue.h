/**
* @file        PriorityQueue.h
* @author     Yingfeng Zhang
* @brief An implementation of priority queue
*/
#ifndef SEARCHMANAGER_PRIORITYQUEUE_H
#define SEARCHMANAGER_PRIORITYQUEUE_H

#include <string>

namespace sf1r{

/**
* This priorityqueue is used temporarily because it does not need to allocate
* memory on heap.
 */
template <class Type>
class PriorityQueue
{

private:
    Type*   heap_;
    size_t  size_;
    size_t  maxSize_;

    void upHeap()
    {
        size_t i = size_;
        Type node = heap_[i];  // save bottom node (WAS object)
        int32_t j = ((uint32_t)i) >> 1;
        while (j > 0 && lessThan(node,heap_[j]))
        {
            heap_[i] = heap_[j];  // shift parents down
            i = j;
            j = ((uint32_t)j) >> 1;
        }
        heap_[i] = node;  // install saved node
    }
    void downHeap()
    {
        size_t i = 1;
        Type node = heap_[i];  // save top node
        size_t j = i << 1;  // find smaller child
        size_t k = j + 1;
        if (k <= size_ && lessThan(heap_[k], heap_[j]))
        {
            j = k;
        }
        while (j <= size_ && lessThan(heap_[j],node))
        {
            heap_[i] = heap_[j];  // shift up child
            i = j;
            j = i << 1;
            k = j + 1;
            if (k <= size_ && lessThan(heap_[k], heap_[j]))
            {
                j = k;
            }
        }
        heap_[i] = node;  // install saved node
    }

protected:
    PriorityQueue()
    {
        size_ = 0;
        heap_ = NULL;
        maxSize_ = 0;
    }

    /**
     * Determines the ordering of objects in this priority queue.
     * Subclasses must define this one method.
     */
    virtual bool lessThan(Type a, Type b)=0;

    /**
     * Subclass constructors must call this.
     */
    void initialize(const size_t maxSize)
    {
        size_ = 0;
        maxSize_ = maxSize;
        size_t heapSize = maxSize_ + 1;
        heap_ = new Type[heapSize];
    }

public:
    virtual ~PriorityQueue()
    {
        delete[] heap_;
    }

    /**
     * Adds an Object to a PriorityQueue in log(size) time.
     * If one tries to add more objects than maxSize_ from initialize
     * a RuntimeException (ArrayIndexOutOfBound) is thrown.
     */
    void put(Type element)
    {
        size_++;
        heap_[size_] = element;
        upHeap();
    }

    /**
     * Adds element to the PriorityQueue in log(size) time if either
     * the PriorityQueue is not full, or not lessThan(element, top()).
     * @param element
     * @return true if element is added, false otherwise.
     */
    bool insert(Type element)
    {
        if (size_ < maxSize_)
        {
            put(element);
            return true;
        }
        else if (size_ > 0 && !lessThan(element, top()))
        {
            heap_[1] = element;
            adjustTop();
            return true;
        }
        else
            return false;
    }

    /**
     * Returns the least element of the PriorityQueue in constant time.
     */
    Type top()
    {
        if (size_ > 0)
            return heap_[1];
        else
            return Type();
    }

    /**
     * Removes and returns the least element of the PriorityQueue in log(size) time.
     */
    Type pop()
    {
        if (size_ > 0)
        {
            Type result = heap_[1];  // save first value
            heap_[1] = heap_[size_];  // move last to first

            heap_[size_] = (Type)0;  // permit GC of objects
            size_--;
            downHeap();  // adjust heap_
            return result;
        }
        else
            return Type();
    }

    /**
     * Should be called when the object at top changes values.  Still log(n)
     * worst case, but it's at least twice as fast to <pre>
     * { pq.top().change(); pq.adjustTop(); }
     * </pre> instead of <pre>
     * { o = pq.pop(); o.change(); pq.push(o); }
     * </pre>
     */
    void adjustTop()
    {
        downHeap();
    }


    /**
     * Returns the number of elements currently stored in the PriorityQueue.
     */
    size_t size()
    {
        return size_;
    }

    /** return element by position */
    Type operator [](size_t _pos)
    {
        return heap_[_pos+1];
    }
    Type getAt(size_t _pos)
    {
        return heap_[_pos+1];
    }

};

}
#endif
