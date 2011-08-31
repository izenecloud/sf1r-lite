#include "NumericPropertyManager.h"

namespace sf1r{

NumericPropertyManager* NumericPropertyManager::instance_;

NumericPropertyManager* NumericPropertyManager::instance()
{
    if (instance_ == NULL)
    {
        instance_ = new NumericPropertyManager;
    }

    return instance_;
}

void NumericPropertyManager::setPropertyCache(SortPropertyCache *propertyCache)
{
    propertyCache_ = propertyCache;
}

NumericPropertyTable* NumericPropertyManager::getPropertyTable(const string& property, PropertyDataType type)
{
    void *data;

    if (propertyCache_->getSortPropertyData(property, type, data))
        return new NumericPropertyTable(type, data);
    else return NULL;
}

}
