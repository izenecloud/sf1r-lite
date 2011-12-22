#ifndef SF1R_DOCID_DISPATCHER_H
#define SF1R_DOCID_DISPATCHER_H

#include <3rdparty/am/drum/null_dispatcher.hpp>

template <class key_t,
          class value_t,
          class aux_t>
class DocidDispatcher
    : public izenelib::drum::NullDispatcher<key_t, value_t, aux_t>
{
public:
    virtual void UniqueKeyCheck(key_t const&, aux_t const&) const;
    virtual void DuplicateKeyCheck(key_t const&, value_t const&, aux_t const&) const;
    virtual void UniqueKeyUpdate(key_t const&, value_t const&, aux_t const&) const;
    virtual void DuplicateKeyUpdate(key_t const&, value_t const&, aux_t const&) const;
    virtual void Update(key_t const&, value_t const&, aux_t const&) const;
};

template <class key_t,
          class value_t,
          class aux_t>
void DocidDispatcher<key_t, value_t, aux_t>::
UniqueKeyCheck(key_t const& key, aux_t const& aux) const
{
}

template <class key_t,
          class value_t,
          class aux_t>
void DocidDispatcher<key_t, value_t, aux_t>::
DuplicateKeyCheck(key_t const& key, value_t const& value, aux_t const& aux) const
{
}

template <class key_t,
          class value_t,
          class aux_t>
void DocidDispatcher<key_t, value_t, aux_t>::
UniqueKeyUpdate(key_t const& key, value_t const& value, aux_t const& aux) const
{
}

template <class key_t,
          class value_t,
          class aux_t>
void DocidDispatcher<key_t, value_t, aux_t>::
DuplicateKeyUpdate(key_t const& key, value_t const& value, aux_t const& aux) const
{
}

template <class key_t,
          class value_t,
          class aux_t>
void DocidDispatcher<key_t, value_t, aux_t>::
Update(key_t const& key, value_t const& value, aux_t const& aux) const
{
}

#endif  //SF1R_DOCID_DISPATCHER_H
