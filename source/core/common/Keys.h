#ifndef SF1R_DRIVER_KEYS_H
#define SF1R_DRIVER_KEYS_H
/**
 * @file sf1r/driver/Keys.h
 * @author Ian Yang
 * @date Created <2010-06-10 15:28:15>
 * @brief Key names used as convention.
 */
#include <boost/preprocessor.hpp>
#include <util/driver/Keys.h>
#include <string>

#include "Keys.inl"

namespace sf1r {
namespace driver {

#define SF1_DRIVER_KEYS_DECL(z,i,l) \
    static const std::string BOOST_PP_SEQ_ELEM(i, l);

struct Keys : public ::izenelib::driver::Keys
{
    BOOST_PP_REPEAT(
        BOOST_PP_SEQ_SIZE(SF1_DRIVER_KEYS),
        SF1_DRIVER_KEYS_DECL,
        SF1_DRIVER_KEYS
    )
};

#undef SF1_DRIVER_KEYS_DECL
}} // namespace sf1r::driver

#endif // SF1R_DRIVER_KEYS_H
