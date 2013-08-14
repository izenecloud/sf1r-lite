#ifndef SF1R_B5MMANAGER_CATEGORYPSM_H_
#define SF1R_B5MMANAGER_CATEGORYPSM_H_

#include <string>
#include <vector>
#include <document-manager/Document.h>
#include <types.h>
#include <boost/unordered_map.hpp>

namespace sf1r {

    class CategoryPsm
    {
    public:
        typedef boost::unordered_map<uint128_t, uint128_t> ResultMap;
        virtual ~CategoryPsm()
        {
        }
        virtual bool Open(const std::string& path) = 0;
        void SetCategory(const std::string& c)
        {
            category_ = c;
        }

        const std::string& GetCategory() const
        {
            return category_;
        }

        bool CategoryMatch(const std::string& c) const
        {
            if(c==category_) return true;
            if(boost::algorithm::starts_with(c, category_+">")) return true;
            return false;
        }

        virtual bool TryInsert(const Document& doc) = 0;
        virtual bool Flush(ResultMap& result) = 0;

    private:
        std::string category_;
    };

}


#endif

