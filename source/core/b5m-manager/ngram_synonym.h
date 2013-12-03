#ifndef SF1R_B5MMANAGER_NGRAMSYNONYM_H
#define SF1R_B5MMANAGER_NGRAMSYNONYM_H 
#include "b5m_types.h"
#include <boost/regex.hpp>
#include <util/ustring/UString.h>

NS_SF1R_B5M_BEGIN

class NgramSynonym{
public:
    NgramSynonym();
    bool Get(const izenelib::util::UString& ngram, std::vector<izenelib::util::UString>& synonym);

private:

private:
    boost::regex gb_regex_;
};
NS_SF1R_B5M_END

#endif

