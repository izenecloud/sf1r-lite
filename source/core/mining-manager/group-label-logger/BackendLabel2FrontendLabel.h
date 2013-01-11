#ifndef SF1R_BACKEND_LABEL_2_FRONTEND_LABEL_H
#define SF1R_BACKEND_LABEL_2_FRONTEND_LABEL_H

#include <util/ustring/UString.h>
#include <util/singleton.h>
//#include <am/succinct/ux-trie/uxMap.hpp>
#include <am/succinct/marisa/map.h>

#include <boost/thread/once.hpp>

#include <string>

namespace sf1r
{
using izenelib::util::UString;
class BackendLabelToFrontendLabel
{
public:
    BackendLabelToFrontendLabel();

    ~BackendLabelToFrontendLabel();

    static BackendLabelToFrontendLabel* Get()
    {
        return ::izenelib::util::Singleton<BackendLabelToFrontendLabel>::get();
    }
    void Init(const std::string& path);
    bool Map(const UString&backend, UString&frontend);  
    bool PrefixMap(const UString&backend, UString&frontend);
private:
    void InitOnce_(const std::string& path);
    static marisa::Map<UString> back2front_;		
};

}

#endif

