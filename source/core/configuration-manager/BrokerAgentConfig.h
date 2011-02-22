#ifndef _BROKER_AGENT_CONFIG_H_
#define _BROKER_AGENT_CONFIG_H_

#include "ManagerConfigBase.h"

namespace sf1r {

    class BrokerAgentConfig
    {
        public:
            BrokerAgentConfig()
            : useCache_(false),
              threadNum_(2),
              enableTest_(true)
            {}

            ~BrokerAgentConfig() {}

            void setUseCache(bool enable)
            {
                useCache_ = enable;
            }
            bool useCache() const
            {
                return useCache_;
            }

            void setThreadNums(const size_t& threadNum)
            {
                threadNum_ = threadNum;
            }
            size_t threadNums() const
            {
                return threadNum_;
            }

            void setEnableTest(bool enable)
            {
                enableTest_ = enable;
            }
            bool enableTest() const
            {
                return enableTest_;
            }

        private:
            bool useCache_;
            size_t threadNum_;
            bool enableTest_;

    }; // end - BrokerAgentConfig

} // end - namespace sf1r

#endif //  _BROKER_AGENT_CONFIG_H_
