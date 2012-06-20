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
        UNIQUE_KEY_CHECK = 0,
        DUPLICATE_KEY_CHECK,
        UPDATE,
        UNIQUE_KEY_UPDATE,
        DUPLICATE_KEY_UPDATE,
        DELETE,
        UNIQUE_KEY_DELETE,
        DUPLICATE_KEY_DELETE,
        UNIQUE_KEY_APPEND,
        DUPLICATE_KEY_APPEND,
        EOC
    };

    typedef boost::function3<
    void,
    key_t const&,
    value_t const&,
    aux_t const&
    > func_t;

public:
    DrumDispatcher()
        : opList_(EOC)
        , defaultValue_()
    {
    }

    virtual void UniqueKeyCheck(key_t const&, aux_t const&) const;
    virtual void DuplicateKeyCheck(key_t const&, value_t const&, aux_t const&) const;

    virtual void Update(key_t const&, value_t const&, aux_t const&) const;
    virtual void UniqueKeyUpdate(key_t const&, value_t const&, aux_t const&) const;
    virtual void DuplicateKeyUpdate(key_t const&, value_t const&, aux_t const&) const;

    virtual void Delete(key_t const&, aux_t const&) const;
    virtual void UniqueKeyDelete(key_t const&, aux_t const&) const;
    virtual void DuplicateKeyDelete(key_t const&, value_t const&, aux_t const&) const;

    virtual void UniqueKeyAppend(key_t const&, value_t const&, aux_t const&) const;
    virtual void DuplicateKeyAppend(key_t const&, value_t const&, aux_t const&) const;

    void registerOp(OpType type, func_t func)
    {
        opList_[type].push_back(func);
    }

protected:
    void dispatch(OpType type, key_t const& key, value_t const& value, aux_t const& aux) const
    {
        const std::vector<func_t>& funcList = opList_[type];
        for (std::size_t i = 0; i < funcList.size(); i++)
        {
            funcList[i](key, value, aux);
        }
    }

protected:
    std::vector<std::vector<func_t> > opList_;

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
Update(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "Update" << std::endl;
    dispatch(UPDATE, key, value, aux);
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
Delete(key_t const& key, aux_t const& aux) const
{
    //std::cout << "Delete" << std::endl;
    dispatch(DELETE, key, defaultValue_, aux);
}

template <class key_t,
         class value_t,
         class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
UniqueKeyDelete(key_t const& key, aux_t const& aux) const
{
    //std::cout << "UniqueKeyDelete" << std::endl;
    dispatch(UNIQUE_KEY_DELETE, key, defaultValue_, aux);
}

template <class key_t,
         class value_t,
         class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
DuplicateKeyDelete(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "DuplicateKeyDelete" << std::endl;
    dispatch(DUPLICATE_KEY_DELETE, key, value, aux);
}

template <class key_t,
         class value_t,
         class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
UniqueKeyAppend(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "UniqueKeyUpdate" << std::endl;
    dispatch(UNIQUE_KEY_APPEND, key, value, aux);
}

template <class key_t,
         class value_t,
         class aux_t>
void DrumDispatcher<key_t, value_t, aux_t>::
DuplicateKeyAppend(key_t const& key, value_t const& value, aux_t const& aux) const
{
    //std::cout << "DuplicateKeyUpdate" << std::endl;
    dispatch(DUPLICATE_KEY_APPEND, key, value, aux);
}

}

#endif // SF1R_DRUM_DISPATCHER_H
