/**
 * @file process/controllers/CollectionHandler.cpp
 * @author Ian Yang
 * @date Created <2011-01-25 18:40:12>
 */
#include "CollectionHandler.h"
#include "DocumentsGetHandler.h"
#include "DocumentsSearchHandler.h"


namespace sf1r
{

CollectionHandler::CollectionHandler(const string& collection)
        : collection_(collection)
        , indexSearchService_(0)
        , miningSearchService_(0)
{
}


CollectionHandler::~CollectionHandler()
{
}

void CollectionHandler::search(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsSearchHandler handler(request,response,*this);
    handler.search();
}

void CollectionHandler::get(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.get();
}

void CollectionHandler::similar_to(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.similar_to();
}

void CollectionHandler::duplicate_with(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.duplicate_with();
}

void CollectionHandler::similar_to_image(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.similar_to_image();
}

void CollectionHandler::create(const ::izenelib::driver::Value& document)
{
}

void CollectionHandler::update(const ::izenelib::driver::Value& document)
{
}

void CollectionHandler::destroy(const ::izenelib::driver::Value& document)
{
}

} // namespace sf1r
