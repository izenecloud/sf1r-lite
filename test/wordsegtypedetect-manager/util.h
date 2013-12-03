#ifndef LATTICELM_UTIL_H__
#define LATTICELM_UTIL_H__

#include <vector>

#define LATTICELM_SAFE

#define THROW_ERROR(msg) do {                   \
    std::ostringstream oss;                     \
    oss << msg;                                 \
    throw std::runtime_error(oss.str()); }       \
  while (0);


namespace latticelm {

// Perform safe access to a vector
template < class T >
inline const T & SafeAccess(const std::vector<T> & vec, int idx) {
#ifdef LATTICELM_SAFE
    if(idx < 0 || idx >= (int)vec.size())
        THROW_ERROR("Out of bound access size="<<vec.size()<<", idx="<<idx);
#endif
    return vec[idx];
}

// Perform safe access to a vector
template < class T >
inline T & SafeAccess(std::vector<T> & vec, int idx) {
#ifdef LATTICELM_SAFE
    if(idx < 0 || idx >= (int)vec.size())
        THROW_ERROR("Out of bound access size="<<vec.size()<<", idx="<<idx);
#endif
    return vec[idx];
}

}

#endif
