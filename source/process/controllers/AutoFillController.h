#ifndef PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
#define PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
/**
 * @file process/controllers/AutoFillController.h
 * @author Ian Yang
 * @date Created <2010-05-31 14:11:35>
 */
#include "Sf1Controller.h"

namespace sf1r
{
class RecommendManager;
/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b auto_fill
 *
 * Gets list of popular keywords starting with specified prefix.
 */
class AutoFillController : public Sf1Controller
{
public:
    enum
    {
        kDefaultCount = 8,      /**< Default count of result */
        kMaxCount = 100         /**< Max count of result */
    };
    void index();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    boost::shared_ptr<RecommendManager> recommend_manager_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
