/**
 * \file QueryNormalizeController.h
 * \brief Controller QeryNormalize dictionary;
 * \date 2012-08-10
 * \author Hongliang.zhao
 */

#ifndef PROCESS_CONTROLLERS_QUERYNORMALIZE_CONTROLLER_H_
#define PROCESS_CONTROLLERS_QUERYNORMALIZE_CONTROLLER_H_

#include <util/driver/Controller.h>
#include <common/Keys.h>
#include <util/driver/value/types.h>

#include <string>

namespace sf1r
{
using driver::Keys;
using namespace ::izenelib::driver;
using namespace std;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b synonym
 *
 * Add, update, delete synonym list in Synonym dictionary
 */
#define MAX_QUERY_LENGTH 100
#define MAX_QUERYWORD_LENGTH 4048
class QueryNormalizeController : public ::izenelib::driver::Controller
{
public:
    void add_Query();

    void delete_Query();

private:
    bool load(string nameFile);
    bool add_query(const string &query);
    bool delete_query(const string &query);
    void save();
    /**
    @brief the path of dictionary for normalize;
    */
    string filePath_;
    /**
    @brief the number of words can be normalize
    */
    uint32_t count_;
    /**
    @brief keyStr_ store the brand and valueStr_ store the type
    */
    string* keyStr_;
    string* valueStr_;
};

/// @}
} // namespace sf1r

#endif // PROCESS_CONTROLLERS_QUERYNORMALIZE_CONTROLLER_H
