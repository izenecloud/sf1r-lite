#ifndef PROCESS_CONTROLLERS_SF1CONTROLLER_H
#define PROCESS_CONTROLLERS_SF1CONTROLLER_H
/**
 * @file process/controllers/Sf1Controller.h
 * @author Ian Yang
 * @date Created <2011-01-25 17:30:12>
 */
#include <util/driver/Controller.h>
#include <common/CollectionManager.h>
#include <boost/thread/shared_mutex.hpp>
#include <string>

namespace sf1r
{

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
class CollectionHandler; 
class Sf1Controller : public ::izenelib::driver::Controller
{
public:
    /**
     * @param requireCollectionName whether collection name is required in request parameter.
     */
    Sf1Controller(bool requireCollectionName = true);

    virtual ~Sf1Controller();

public:
    bool preprocess();

protected:
    /**
     * @brief sub-class could override this function to check any service it
     * required, such as IndexSearchService, etc.
     * @pre @c collectionName_ and @c collectionHandler_ are not empty.
     */
    virtual bool checkCollectionService(std::string& error)
    {
        return true;
    }

    /**
     * @brief when @c collectionName_ is empty, output error message to @c response(),
     * and return false, otherwise, true is returned.
     */
    bool checkCollectionName();

private:
    /**
     * @brief do collection related check, such as ACL, handler, etc.
     * @return true for collection check success, false for failure
     */
    bool doCollectionCheck(std::string& error);

    /**
     * @brief parse collection parameter in request
     */
    bool parseCollectionName(std::string& error);

    /**
     * @brief check whether collection is configured
     */
    bool checkCollectionExist(std::string& error);

    /**
     * @brief check collection level ACL
     */
    bool checkCollectionAcl(std::string& error);

    /**
     * @brief check collection handler
     */
    bool checkCollectionHandler(std::string& error);

protected:
    std::string collectionName_;
    CollectionHandler* collectionHandler_;

private:
    CollectionManager::MutexType* mutex_;
    const bool requireCollectionName_;
};

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_SF1CONTROLLER_H
