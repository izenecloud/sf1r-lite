/**
 * @file RandGenerator.h
 * @brief generate random item ids
 * @author Jun Jiang
 * @date 2011-11-30
 */

#ifndef RAND_GENERATOR_H
#define RAND_GENERATOR_H

#include <util/ThreadModel.h>

#include <boost/random.hpp>

namespace sf1r
{

template<typename ValueType = int,
         typename Distribution = boost::uniform_int<ValueType>,
         typename Engine = boost::mt19937,
         typename LockType = izenelib::util::ReadWriteLock>
class RandGenerator
{
public:
    RandGenerator()
        : generator_(Engine(), Distribution())
    {}

    void seed(int value)
    {
        ScopedWriteLock lock(lock_);

        generator_.engine().seed(value);
    }

    ValueType generate(ValueType min, ValueType max)
    {
        ScopedWriteLock lock(lock_);

        Distribution& dist = generator_.distribution();
        if (dist.min() != min || dist.max() != max)
        {
            dist = Distribution(min, max);
        }

        return generator_();
    }

private:
    typedef boost::variate_generator<Engine, Distribution> Generator;
    Generator generator_;

    typedef izenelib::util::ScopedWriteLock<LockType> ScopedWriteLock;
    LockType lock_;
};

} // namespace sf1r

#endif // RAND_GENERATOR_H
