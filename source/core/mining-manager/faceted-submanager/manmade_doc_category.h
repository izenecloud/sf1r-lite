///
/// @file ontology-rep.h
/// @brief ontology representation class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_MANMADE_DOC_CATEGORY_H_
#define SF1R_MANMADE_DOC_CATEGORY_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_rep_item.h"
#include "manmade_doc_category_item.h"
NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class ManmadeDocCategory
{
public:
    ManmadeDocCategory(const std::string& dir);
    ~ManmadeDocCategory();
    bool Open();
    bool Add(const std::vector<ManmadeDocCategoryItem>& items);

    bool Get(std::vector<ManmadeDocCategoryItem>& items);

private:
    std::string dir_;
    std::vector<ManmadeDocCategoryItem> items_;
    boost::mutex mutex_;
};
NS_FACETED_END
#endif
