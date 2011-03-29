/*!
 * \file      bitset.hpp
 * \brief     Provides Boost.Serialization support for std::bitset
 * \author    Brian Ravnsgaard Riis
 * \date      16.09.2004
 * \copyright 2004 Brian Ravnsgaard Riis
 * \license   Boost Software License 1.0
 */
#ifndef BOOST_SERIALIZATION_BITSET_HPP
#define BOOST_SERIALIZATION_BITSET_HPP

#include <boost/config.hpp>
#include <boost/serialization/split_free.hpp>

#include <bitset>

#if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
#define STD _STLP_STD
#else
#define STD std
#endif

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
namespace boost {
  namespace serialization {
#else
namespace STD {
#endif

    template <class Archive>
    inline void save(Archive & ar,
                      STD::bitset<4294967296> const & t,
        const unsigned int version)
    {
        for(uint64_t i = 0; i < 4294967296; ++i)
        {
            bool b = t[i];
            ar << b;
        }
    }

 template <class Archive>
    inline void load(Archive & ar,
                      STD::bitset<4294967296> & t,
        const unsigned int version)
    {
      bool temp;
      for(uint64_t i = 0; i < 4294967296; ++i)
      {
        ar >> temp;
        t.set(i, temp);
      }
    }

 template <class Archive>
    inline void serialize(Archive & ar,
                           STD::bitset<4294967296> & t,
        const unsigned int version)
    {
      boost::serialization::split_free(ar, t, version);
    }
    


#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace STD 
#else
  } // namespace serialization
} // namespace boost
#endif

#endif // BOOST_SERIALIZATION_BITSET_HPP

