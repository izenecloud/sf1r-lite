/**
 * \file controllers/ServiceController.h
 * \brief 
 * \date Jun 1, 2011
 * \author Xin Liu
 */

#ifndef BA_PROCESS_CONTROLLERS_SERVICE_CONTROLLER_H_
#define BA_PROCESS_CONTROLLERS_SERVICE_CONTROLLER_H_


#include "Sf1Controller.h"
#include <license-manager/LicenseManager.h>

#include <string>

namespace sf1r {

/**
 * @brief Controller \b service
 */
class ServiceController : public ::izenelib::driver::Controller
{
public:
    void process_overdue();
    void renew();
};

/// @}
} // namespace sf1r

#endif /* BA_PROCESS_CONTROLLERS_SERVICE_CONTROLLER_H_ */
