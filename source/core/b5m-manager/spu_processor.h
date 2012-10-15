#ifndef SF1R_B5MMANAGER_SPUPROCESSOR_H_
#define SF1R_B5MMANAGER_SPUPROCESSOR_H_

#include <string>
#include <vector>
#include <common/ScdMerger.h>

namespace sf1r {
    class SpuProcessor {
    public:
        SpuProcessor(const std::string& dir);

        bool Upgrade();

    private:
        void Merge_(ScdMerger::ValueType& value, const ScdMerger::ValueType& another_value);
        void Output_(Document& doc, int& type);

    private:
        std::string dir_;
    };

}

#endif

