/**
 * \file ba-process/controllers/ServiceController.cpp
 * \date Jun 1, 2011
 * \author Xin Liu
 */

#include "ServiceController.h"
#include <common/XmlConfigParser.h>
#include <boost/filesystem.hpp>

#include <fstream>
#include <map>

namespace sf1r {

/**
 * @brief Action @b process_overdue. This API is invoked when the service
 * fee is overdued.
 */
void ServiceController::process_overdue()
{
}

/**
 * @brief Action @b process_overdue. When the overdue fees is paid, this API is invoked
 * to renew service.
 */
void ServiceController::renew()
{
}

}
