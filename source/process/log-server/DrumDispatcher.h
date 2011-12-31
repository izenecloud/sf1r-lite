#ifndef SF1R_DRUM_DISPATCHER_H
#define SF1R_DRUM_DISPATCHER_H

#include <vector>

#include <3rdparty/am/drum/null_dispatcher.hpp>

#include <boost/function.hpp>

namespace sf1r
{

template <class key_t,
          class value_t,
          class aux_t>
class DrumDispatcher
    : public izenelib::drum::NullDispatcher<key_t, value_t, aux_t>
{
public:
    enum OpType
    {
        UNIQUE_KEY_CHECK,
        DUPLICATE_KEY_CHECK,
        UNIQUE_KEY_UPDATE,
        DUPLICATE_KEY_UPDATE,
        UPDATE
    };

    typedef boost::function3<
            void,
            key_t const&,
            value_t const&,
            aux_t const&
    > func_t;

    struct Op
    {
        Op(OpType type, func_t& func)
            : type_(type)
            , func_(func)
        {}

        OpType type_;
        func_t func_;
    };

public:
    DrumDispatcher()
        : defaultValue_()
    {
    }

    virtual void UniqueKeyCheck(key_t const&, aux_t const&) const;
    virtual void DuplicateKeyCheck(key_t const&, value_t const&, aux_t const&) const;
    virtual void UniqueKeyUpdate(key_t const&, value_t const&, aux_t const&) const;
    virtual void DuplicateKeyUpdate(key_t const&, value_t const&, aux_t const&) const;
    virtual void Update(key_t const&, value_t const&, aux_t const&) const;

    void registerOp(OpType type, func_t func)
    {
        Op op(type, func);
        opList_.push_back(op);
    }

protected:
    void dispatch(OpType type, key_t const& key, value_t const& value, aux_t const& aux) const
    {
        for (std::size_t i = 0; i < opList_.size(); i++)
        {
            if (opList_[i].type_ == type)
            {
                opList_[i].func_(key, value, aux);
            }
        }
    }

protected:
    std::vector<Op> opList_;

    value_t defaultValue_;
};

template <class key_t,
          class value_t,
          class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
UniqueKeyCheck(key_t const& key, aux_t const& aux) const
{
    //std::cout << "UniqueKeyCheck" << std::endl; //xxx
    dispatch(UNIQUE_KEY_CHECK, key, defaultValue_, aux);
}

template <class key_t,
          class value_t,
          class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
DuplicateKeyCheck(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "DuplicateKeyCheck" << std::endl;
    dispatch(DUPLICATE_KEY_CHECK, key, value, aux);
}

template <class key_t,
          class value_t,
          class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
UniqueKeyUpdate(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "UniqueKeyUpdate" << std::endl;
    dispatch(UNIQUE_KEY_UPDATE, key, value, aux);
}

template <class key_t,
          class value_t,
          class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
DuplicateKeyUpdate(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "DuplicateKeyUpdate" << std::endl;
    dispatch(DUPLICATE_KEY_UPDATE, key, value, aux);
}

template <class key_t,
          class value_t,
          class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
Update(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "Update" << std::endl;
    dispatch(UPDATE, key, value, aux);
}

}

#endif // SF1R_DRUM_DISPATCHER_H
