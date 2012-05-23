#ifndef SF1R_B5MMANAGER_CATEGORYSCDSPLITER_H_
#define SF1R_B5MMANAGER_CATEGORYSCDSPLITER_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>

namespace sf1r {
    class CategoryScdSpliter {

        struct ValueType
        {
            boost::regex regex;
            ScdWriter* writer;
        };

    public:
        CategoryScdSpliter();
        ~CategoryScdSpliter();

        bool Load(const std::string& dir, const std::string& name);

        bool Split(const std::string& scd_file);

    private:
        bool Load_(const std::string& dir, const std::string& name);

    private:
        std::vector<ValueType> values_;
    };

}

#endif

