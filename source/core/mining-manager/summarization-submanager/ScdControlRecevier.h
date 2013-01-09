#ifndef SF1R_MINING_MANAGER_SCDCONTROL_RECEVIER
#define SF1R_MINING_MANAGER_SCDCONTROL_RECEVIER

#include <sstream>
#include <common/type_defs.h>

#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <product-manager/pm_def.h>
#include <product-manager/pm_types.h>

#include <node-manager/synchro/SynchroFactory.h>
namespace sf1r
{
class ScdControlRecevier
{
public:
    ScdControlRecevier(const std::string& syncID, const std::string& collectionName, const std::string& controlFilePath);

    bool Run(const std::string& scd_source_dir);

private:
    std::string syncID_;
    std::string collectionName_;
    std::string controlFilePath_;
    SynchroConsumerPtr syncConsumer_;
};
}
#endif
