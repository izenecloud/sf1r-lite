/**
 * @file XmlSchema.h
 * @brief Validate config.xml according to schema
 * @author Yingfeng Zhang
 * @date 2010-11-09
 */

#ifndef XML_SCHEMA_H
#define XML_SCHEMA_H

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>

#include <string>
#include <list>

namespace sf1r
{
class XmlSchema
{
public:
    XmlSchema(const std::string& schemaFilePath);

    ~XmlSchema();

public:
    /// add schema validity error message
    void addError(const char *msg)
    {
        errors_.push_back(std::string(msg));
    }

    /// get schema validity error messages
    std::list<std::string> getSchemaValidityErrors() const
    {
        return errors_;
    }

    /// add schema validity warning message
    void addWarning(const char *msg)
    {
        warnings_.push_back(std::string(msg));
    }

    /// get schema validity warning messages
    std::list<std::string> getSchemaValidityWarnings() const
    {
        return warnings_;
    }

    /// validate the correctness of config.xml
    bool validate(const std::string& configFilePath);
private:
    xmlSchemaParserCtxtPtr schema_parser_ptr_;
    xmlSchemaPtr schema_ptr_;
    std::list<std::string> errors_;
    std::list<std::string> warnings_;
};


}

#endif

