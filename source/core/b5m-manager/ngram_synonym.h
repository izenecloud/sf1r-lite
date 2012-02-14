#ifndef SF1R_B5MMANAGER_NGRAMSYNONYM_H
#define SF1R_B5MMANAGER_NGRAMSYNONYM_H 
#include <boost/regex.hpp>
#include <util/ustring/UString.h>

namespace sf1r {

    class NgramSynonym{
    public:
        NgramSynonym();
        bool Get(const izenelib::util::UString& ngram, std::vector<izenelib::util::UString>& synonym);

    private:

    private:
        boost::regex gb_regex_;
    };
}

#endif

