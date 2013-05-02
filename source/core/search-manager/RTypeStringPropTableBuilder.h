///
/// @file RTypeStringPropTableBuilder.h
/// @brief interface to create RTypeStringPropertyTable instance
/// @author Kevin Lin <kevin.lin@b5m.com>
/// @date Created 2013-04-27
///

#ifndef SF1R_RTYPE_STRING_PROP_TABLE_BUILDER_H
#define SF1R_RTYPE_STRING_PROP_TABLE_BUILDER_H

#include <string>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class DocumentManager;
class RTypeStringPropTable;

class RTypeStringPropTableBuilder
{
public:
    RTypeStringPropTableBuilder(DocumentManager& documentManagerPtr);
    ~RTypeStringPropTableBuilder() {}
    
    boost::shared_ptr<RTypeStringPropTable>& createPropertyTable(
        const std::string& propertyName);
private:
    DocumentManager& documentManagerPtr_;
};

}

#endif
