/**
 * @file ItemContainer.h
 * @brief interface class to get each item from container
 * @author Jun Jiang
 * @date 2012-04-20
 */

#ifndef ITEM_CONTAINER_H
#define ITEM_CONTAINER_H

#include "../common/RecommendItem.h"
#include <document-manager/Document.h>

#include <vector>

namespace sf1r
{

class ItemContainer
{
public:
    virtual ~ItemContainer() {}

    virtual std::size_t getItemNum() const = 0;

    virtual Document& getItem(std::size_t i) = 0;
};

class SingleItemContainer : public ItemContainer
{
public:
    SingleItemContainer(Document& doc) : item_(doc) {}

    virtual std::size_t getItemNum() const { return 1; }

    virtual Document& getItem(std::size_t i) { return item_; }

private:
    Document& item_;
};

class MultiItemContainer : public ItemContainer
{
public:
    MultiItemContainer(std::vector<Document>& items) : items_(items) {}

    virtual std::size_t getItemNum() const { return items_.size(); }

    virtual Document& getItem(std::size_t i) { return items_[i]; }

private:
    std::vector<Document>& items_;
};

class RecommendItemContainer : public ItemContainer
{
public:
    RecommendItemContainer(std::vector<RecommendItem>& recItems) : recItems_(recItems) {}

    virtual std::size_t getItemNum() const { return recItems_.size(); }

    virtual Document& getItem(std::size_t i) { return recItems_[i].item_; }

private:
    std::vector<RecommendItem>& recItems_;
};

} // namespace sf1r

#endif // ITEM_CONTAINER_H
