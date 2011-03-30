#ifndef PROCESS_CONTROLLERS_FACETED_CONTROLLER_H
#define PROCESS_CONTROLLERS_FACETED_CONTROLLER_H
/**
 * @file process/controllers/FacetedController.h
 * @author Jia Guo
 * @date Created <2010-12-27>
 */
#include "Sf1Controller.h"

#include <string>

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b faceted
 *
 * faceted related
 */
class FacetedController : public Sf1Controller
{

public:
    /// @brief alias for set_ontology
    void index()
    {
        set_ontology();
    }

    void set_ontology();
    void get_ontology();
    void get_static_rep();
    void static_click();
    void get_rep();
    void click();
    void manmade();

private:
    bool requireCID_();
    bool requireSearchResult_();
    uint32_t cid_;
    std::vector<uint32_t> search_result_;
};

/// @}

} // namespace sf1r

#endif
