/**
 * \file CollectionController.h
 * \brief 
 * \date Dec 20, 2011
 * \author Xin Liu
 */

#ifndef PROCESS_CONTROLLERS_COLLECTION_CONTROLLER_H_
#define PROCESS_CONTROLLERS_COLLECTION_CONTROLLER_H_

#include <util/driver/Controller.h>
#include <common/Keys.h>
#include <process/common/XmlConfigParser.h>
#include <util/driver/value/types.h>
#include <string>

namespace sf1r
{
using driver::Keys;
using namespace ::izenelib::driver;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b collection
 *
 * Start or stop a collection
 */
class CollectionController : public ::izenelib::driver::Controller
{
public:
    void start_collection();

    void stop_collection();

    void check_collection();
    
    void rebuild_collection();

    void rebuild_from_scd();

    void create_collection();

    void delete_collection();

    void set_kv();
    void get_kv();

    void load_license();

    bool preprocess();
    bool callDistribute();
    void postprocess();

private:
    static std::string getConfigPath_()
    {
    	std::string configFilePath = SF1Config::get()->getHomeDirectory();
    	return configFilePath;
    }
};

/// @}

} // namespace sf1r

#endif /* PROCESS_CONTROLLERS_COLLECTION_CONTROLLER_H_ */
