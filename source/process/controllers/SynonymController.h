/**
 * \file SynonymController.h
 * \brief 
 * \date Dec 12, 2011
 * \author Xin Liu
 */

#ifndef PROCESS_CONTROLLERS_SYNONYM_CONTROLLER_H_
#define PROCESS_CONTROLLERS_SYNONYM_CONTROLLER_H_

#include <util/driver/Controller.h>
#include <common/Keys.h>
#include <util/driver/value/types.h>

#include <string>

namespace sf1r
{
using driver::Keys;
using namespace ::izenelib::driver;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b synonym
 *
 * Add, update, delete synonym list in Synonym dictionary
 */
class SynonymController : public ::izenelib::driver::Controller
{
public:
    void add_synonym();

    void update_synonym();

    void delete_synonym();

private:
    std::string str_join( const Value::ArrayType* array, const std::string & separator );
};

/// @}
} // namespace sf1r

#endif // PROCESS_CONTROLLERS_SYNONYM_CONTROLLER_H
