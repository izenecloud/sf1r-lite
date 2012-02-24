#ifndef SF1R_B5MMANAGER_SCDGENERATOR_H_
#define SF1R_B5MMANAGER_SCDGENERATOR_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <configuration-manager/LogServerConnectionConfig.h>

namespace sf1r {
    class ScdGenerator {
    public:
        ScdGenerator();

        bool Load(const std::string& category_dir);

        void SetExclude(bool exclude = true)
        {
            exclude_ = exclude;
        }

        void SetUseUuid(const LogServerConnectionConfig& config)
        {
            buuid_ = true;
            logserver_config_ = config;
        }

        bool Generate(const std::string& scd_file, const std::string& output_dir, const std::string& working_dir = "");

    private:
        bool Load_(const std::string& dir);

    private:
        bool exclude_;
        bool buuid_;
        LogServerConnectionConfig logserver_config_;
        std::vector<boost::regex> category_regex_;
        boost::unordered_map<std::string, std::string> o2p_map_;
    };

}

#endif

