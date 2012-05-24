/**
 * \file CollectionController.cpp
 * \brief 
 * \date Dec 20, 2011
 * \author Xin Liu
 */

#include "CollectionController.h"
#include "process/common/XmlSchema.h"

#include <process/common/CollectionManager.h>
#include <process/common/XmlConfigParser.h>

#include <iostream>
#include <fstream>

namespace sf1r
{

/**
 * @brief Action @b start_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::start_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
        slash = "\\";
#else
        slash = "/";
#endif
    configFile += slash + collection + ".xml";
    CollectionManager::get()->startCollection(collection, configFile);
}

/**
 * @brief Action @b stop_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::stop_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    CollectionManager::get()->stopCollection(collection);
}

/**
 * @brief Action @b create_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b collection_config* (@c String): Config file string.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 *   "collection_config": "xxxxxx"
 * }
 * @endcode
 *
 */
void CollectionController::create_collection()
{
    namespace bf = boost::filesystem;
    std::string collection = asString(request()[Keys::collection]);
    std::string config = asString(request()[Keys::collection_config]);

    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
            slash = "\\";
#else
            slash = "/";
#endif
    configFile += slash + collection + ".xml";
    if(bf::exists(configFile)){
        response().addError("Collection already exists");
        return;
    }

    bf::path configPath(configFile);
    ofstream config_file(configPath.string().c_str(), ios::out);
    if(!config_file){
        response().addError("Can't create config file");
        return;
    }

    int tab = 0;
    for(string::iterator it = config.begin(); it != config.end(); ){
        if(*it == '<'){
            char c = *it;
            it++;
            if(it != config.end()){
                if(*it == '/'){
                    tab--;
                    for(int i = 0; i < tab; ++i)
                        config_file<<"    ";
                }
                else{
                    for(int i = 0; i < tab; ++i)
                        config_file<<"    ";
                    if(*it != '!' && *it != '?')
                        tab++;
                }
            }
            config_file.put(c);
        }
        else if(*it == '/'){
            config_file.put(*it);
            it++;
            if(it != config.end()){
                if(*it == '>')
                    tab--;
            }
        }
        else{
            config_file.put(*it);
            if(*it == '>')
                config_file<<'\n';
            it++;
        }
    }
    config_file.close();
    cout<<"write end"<<endl;

    bf::path config_dir = configPath.parent_path();
    bf::path schema_file = config_dir/"schema"/"collection.xsd";
    std::string schema_file_string = schema_file.string();
    if(!bf::exists(schema_file_string)){
        response().addError("[Collection] Schema File doesn't exist");
        bf::remove(configFile);
        return;
    }
    XmlSchema schema(schema_file_string);
    bool schema_valid = schema.validate(configFile);
    std::list<std::string> schema_warning = schema.getSchemaValidityWarnings();
    if(schema_warning.size()>0){
        std::list<std::string>::iterator it = schema_warning.begin();
        while (it != schema_warning.end())
        {
            response().addWarning("[Schema-warning] " + *it);
            it++;
        }
    }
    if(!schema_valid){
        std::list<std::string> schema_error = schema.getSchemaValidityErrors();
        if(schema_error.size()>0){
            std::list<std::string>::iterator it = schema_error.begin();
            while(it != schema_error.end()){
                response().addError("[Schema-Error] " + *it);
                it++;
            }
        }
        bf::remove(configFile);
        return;
    }
    cout<<"Created collection: "<<collection<<endl;

}

/**
 * @brief Action @b delete_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::delete_collection(){
    namespace bf = boost::filesystem;
    std::string collection = asString(request()[Keys::collection]);
    std::string configFile = SF1Config::get()->getHomeDirectory();

    std::string slash("");
#ifdef WIN32
            slash = "\\";
#else
            slash = "/";
#endif
    configFile += slash + collection + ".xml";
    if(!bf::exists(configFile)){
        response().addError("Collection does not exists");
        return;
    }
    bf::remove(configFile);
    
    bf::path collection_dir("collection" + slash + collection);
    if(bf::exists(collection_dir)){
        if(bf::is_empty(collection_dir))
            bf::remove(collection_dir);
        else
            bf::remove_all(collection_dir);
    }
    cout<<"deleted collection: "<<collection<<endl;
}

} //namespace sf1r

