#ifndef SF1R_B5MMANAGER_UUEGENERATOR_H_
#define SF1R_B5MMANAGER_UUEGENERATOR_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "offer_db.h"

namespace sf1r {

    class UueGenerator {
    public:
        UueGenerator(OfferDb* odb);

        bool Generate(const std::string& b5mo_scd, const std::string& result);

    private:
        OfferDb* odb_;
    };

}

#endif

