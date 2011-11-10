/**
 * @file ItemBundle.h
 * @brief recommend result of "top item bundle" 
 * @author Jun Jiang
 * @date 2011-10-16
 */

#ifndef ITEM_BUNDLE_H
#define ITEM_BUNDLE_H

#include <document-manager/Document.h>

#include <vector>

namespace sf1r
{

struct ItemBundle
{
    ItemBundle() : freq(0) {}

    int freq;
    std::vector<Document> items;
};

} // namespace sf1r

#endif // ITEM_BUNDLE_H
