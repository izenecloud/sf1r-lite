#ifndef PROCESS_CONTROLLERS_SF1CONTROLLER_H
#define PROCESS_CONTROLLERS_SF1CONTROLLER_H
/**
 * @file process/controllers/Sf1Controller.h
 * @author Ian Yang
 * @date Created <2011-01-25 17:30:12>
 */
#include <util/driver/Controller.h>

#include <bundles/index/IndexSearchService.h>

#include "CollectionHandler.h"

namespace sf1r {

/**
 * @brief Base controller for all controllers
 *
 * All controllers must derived from this base class. This base class is used as
 * a hook that preprocess and postprocess can be added for all actions. See list
 * below:
 *
 * - preprocess
 *   - checkCollectionAcl : Check collection level ACL
 */
class Sf1Controller : public ::izenelib::driver::Controller
{
public:
    Sf1Controller();

    ~Sf1Controller();
public:
    bool preprocess()
    {
        if (!checkCollectionAcl())
        {
            return false;
        }

        return true;
    }

protected:
    /**
     * @brief check collection level ACL
     */
    bool checkCollectionAcl();

protected:

    CollectionHandler* collectionHandler_;
};

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_SF1CONTROLLER_H
