#include "XmlSchema.h"

#include <iostream>

namespace sf1r
{

void addSchemaValidityError(void *ctx, const char *msg, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, msg);
    int len = vsnprintf(buf, sizeof(buf)/sizeof(buf[0]), msg, args);
    va_end(args);

    XmlSchema *parser = (XmlSchema *) ctx;
    if (len == 0)
        parser->addError("Can't create schema validity error!");
    else
        parser->addError(buf);
}

void addSchemaValidityWarning(void *ctx, const char *msg, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, msg);
    int len = vsnprintf(buf, sizeof(buf)/sizeof(buf[0]), msg, args);
    va_end(args);

    XmlSchema *parser = (XmlSchema *) ctx;
    if (len == 0)
        parser->addWarning("Can't create schema validity warning!");
    else
        parser->addWarning(buf);
}

XmlSchema::XmlSchema(const std::string& schemaFilePath)
        :schema_parser_ptr_(NULL)
        ,schema_ptr_(NULL)
{
    schema_parser_ptr_ = xmlSchemaNewParserCtxt (schemaFilePath.c_str());
    schema_ptr_ = xmlSchemaParse (schema_parser_ptr_);
}

XmlSchema::~XmlSchema()
{
    if (schema_ptr_ != NULL)
    {
        xmlSchemaFree (schema_ptr_);
    }

    if (schema_parser_ptr_ != NULL)
    {
        xmlSchemaFreeParserCtxt (schema_parser_ptr_);
    }
}

bool XmlSchema::validate(const std::string& configFilePath)
{
    // The valid schema pointer, used to validate the document
    xmlSchemaValidCtxtPtr validity_context_ptr = xmlSchemaNewValidCtxt (schema_ptr_);
    bool result = false;
    xmlSchemaSetValidErrors(validity_context_ptr,
                            addSchemaValidityError,
                            addSchemaValidityWarning,
                            this);

    result = xmlSchemaValidateFile (validity_context_ptr, configFilePath.c_str(), 0) == 0;
    xmlSchemaFreeValidCtxt (validity_context_ptr);
    return result;
}

}
