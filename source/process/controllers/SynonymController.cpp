/**
 * \file SynonymController.cpp
 * \brief 
 * \date Dec 12, 2011
 * \author Xin Liu
 */

#include "SynonymController.h"
#include <common/XmlConfigParser.h>

#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

namespace sf1r
{

/**
 * @brief Action @b add_synonym. Add a synonym list to synonym dictionary.
 *
 * @section request
 *
 * - @b synonym_list* (@c Array): Synonym tokens list that need to be added in synonym dictionary.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "synonym_list": [
 *     "计算机",
 *     "电脑",
 *     "笔记本",
 *     "台式机"
 *   ]
 * }
 * @endcode
 *
 */
void SynonymController::add_synonym()
{
    std::string synonym_list("");
    if (!nullValue(request()[Keys::synonym_list]))
    {
        const Value::ArrayType* array = request()[Keys::synonym_list].getPtr<Value::ArrayType>();
        if(!array || array->size() <= 1)
        {
            response().addError("the synonym list size must be bigger than 1.");
            return;
        }
        synonym_list = str_join( array, "," );
    }
    else
    {
        response().addError("Require a synonym list in request.");
        return;
    }

    std::string dictionaryFile = SF1Config::get()->laDictionaryPath_;
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/synonym.txt";
    }
    else
    {
        dictionaryFile += "synonym.txt";
    }

    ifstream infile(dictionaryFile.c_str());
    std::string line;
    std::vector<std::string> synonym_dictionary;
    while(getline(infile, line))
    {
        if(!line.empty())
        {
            synonym_dictionary.push_back(line);
        }
    }
    infile.close();

    std::vector<std::string>::iterator it;
    // iterator to vector element:
    it = find (synonym_dictionary.begin(), synonym_dictionary.end(), synonym_list);
    if(it != synonym_dictionary.end())
        return;

    ofstream outfile(dictionaryFile.c_str(), ios::app);
    outfile<<synonym_list<<std::endl;
    outfile.close();
}

/**
 * @brief Action @b update_synonym. Update a synonym list from synonym dictionary.
 *
 * @section request
 *
 * - @b new_synonym_list* (@c Array): New synonym tokens list that need to be added in synonym dictionary.
 * - @b old_synonym_list* (@c Array): Old synonym tokens list that need to be deleted from synonym dictionary.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "new_synonym_list": [
 *     "电话",
 *     "手机",
 *     "移动电话"
 *   ],
 *
 *   "old_synonym_list": [
 *     "座机",
 *     "固定电话"
 *   ]
 * }
 *
 * @endcode
 *
 */
void SynonymController::update_synonym()
{
    std::string old_synonym_list("");
    std::string new_synonym_list("");

    if (!nullValue(request()[Keys::old_synonym_list]))
    {
        const Value::ArrayType* array = request()[Keys::old_synonym_list].getPtr<Value::ArrayType>();
        if(!array || array->size() <= 1)
        {
            response().addError("the input old synonym list size must be bigger than 1.");
            return;
        }
        old_synonym_list = str_join( array, "," );
    }
    else
    {
        response().addError("Require a old synonym list in request.");
        return;
    }

    if (!nullValue(request()[Keys::new_synonym_list]))
    {
        const Value::ArrayType* array = request()[Keys::new_synonym_list].getPtr<Value::ArrayType>();
        if(!array || array->size() <= 1)
        {
            response().addError("the input new synonym list size must be bigger than 1.");
            return;
        }
        new_synonym_list = str_join( array, "," );
    }
    else
    {
        response().addError("Require a new synonym list in request.");
        return;
    }

    std::string dictionaryFile = SF1Config::get()->laDictionaryPath_;
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/synonym.txt";
    }
    else
    {
        dictionaryFile += "synonym.txt";
    }

    ifstream infile(dictionaryFile.c_str());
    std::string line;
    std::vector<std::string> synonym_dictionary;
    while(getline(infile, line))
    {
        if(!line.empty())
        {
            synonym_dictionary.push_back(line);
        }
    }
    infile.close();

    std::vector<std::string>::iterator it;
    // iterator to vector element:
    it = find (synonym_dictionary.begin(), synonym_dictionary.end(), old_synonym_list);
    if(it == synonym_dictionary.end())
        return;

    *it = new_synonym_list;

    //output new synonym dictionary
    ofstream outfile(dictionaryFile.c_str());
    for(size_t i = 0; i < synonym_dictionary.size(); i++)
    {
        outfile<<synonym_dictionary[i]<<endl;
    }
    outfile.close();
}

/**
 * @brief Action @b delete_synonym. Delete a synonym list from synonym dictionary.
 *
 * @section request
 *
 * - @b synonym_list* (@c Array): Old synonym tokens list that need to be deleted in synonym dictionary.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "synonym_list": [
 *     "座机",
 *     "固定电话"
 *   ]
 * }
 *
 * @endcode
 *
 */
void SynonymController::delete_synonym()
{
    std::string synonym_list("");

    if (!nullValue(request()[Keys::synonym_list]))
    {
        const Value::ArrayType* array = request()[Keys::synonym_list].getPtr<Value::ArrayType>();
        if(!array || array->size() <= 1)
        {
            response().addError("the synonym list size must be bigger than 1.");
            return;
        }
        synonym_list = str_join( array, "," );
    }
    else
    {
        response().addError("Require a old synonym list in request.");
        return;
    }

    std::string dictionaryFile = SF1Config::get()->laDictionaryPath_;
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/synonym.txt";
    }
    else
    {
        dictionaryFile += "synonym.txt";
    }

    std::ifstream infile(dictionaryFile.c_str());
    std::string line;
    std::vector<std::string> synonym_dictionary;
    while(getline(infile, line))
    {
        if(!line.empty())
        {
            synonym_dictionary.push_back(line);
        }
    }
    infile.close();

    std::vector<std::string>::iterator it;
    // iterator to vector element:
    it = find (synonym_dictionary.begin(), synonym_dictionary.end(), synonym_list);
    if(it == synonym_dictionary.end())
        return;

    synonym_dictionary.erase(it);

    //output new synonym dictionary
    ofstream outfile(dictionaryFile.c_str());
    for(size_t i = 0; i < synonym_dictionary.size(); i++)
    {
        outfile<<synonym_dictionary[i]<<endl;
    }
    outfile.close();
}

std::string SynonymController::str_join( const Value::ArrayType* array, const std::string & separator )
{
    std::stringstream ret;
    for( size_t i = 0; i < array->size()-1; i++)
        ret<<asString((*array)[i])<<separator;
    ret<<asString((*array)[array->size()-1]);
    return ret.str();
}

}// namespace  sf1r
