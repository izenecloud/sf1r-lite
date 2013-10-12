/*
 *  DNFParser.h
 */
#ifndef SF1_AD_INDEX_DNFPARSER_H_
#define SF1_AD_INDEX_DNFPARSER_H_

#include "AdMiningTask.h"

namespace sf1r
{
using izenelib::ir::be_index::DNF;
class DNFParser
{
public:
    static bool parseDNF(const std::string& str, DNF& dnf);
};
}//namespace sf1r

#endif
