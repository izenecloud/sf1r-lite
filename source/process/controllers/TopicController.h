#ifndef PROCESS_CONTROLLERS_TOPIC_CONTROLLER_H
#define PROCESS_CONTROLLERS_TOPIC_CONTROLLER_H
/**
 * @file process/controllers/TopicController.h
 * @author Jia Guo
 * @date Created <2011-03-25>
 */
#include "Sf1Controller.h"

#include <util/ustring/UString.h>

#include <string>

namespace sf1r
{
class MiningSearchService;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b faceted
 *
 * faceted related
 */
class TopicController : public Sf1Controller
{
public:
    TopicController();

    /// @brief alias for set_ontology
    void index()
    {
        get_similar();
    }

    void get_similar();

    void get_in_date_range();

    void get_temporal_similar();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    bool requireTID_();
    bool requireDateRange_();
    bool requireTopicText_();

private:
    uint32_t tid_;
    izenelib::util::UString start_date_;
    izenelib::util::UString end_date_;
    boost::gregorian::date start_;
    boost::gregorian::date end_;
    izenelib::util::UString topic_text_;
    MiningSearchService* miningSearchService_;
};

/// @}

} // namespace sf1r

#endif
