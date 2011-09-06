/**
 * @brief   The code is used to write logs of events that happen in a process.
 * @author  MyungHyun Lee (Kent)
 * @details
 *  This is brought in from SF-1 v4.5. The code is modified to meet our needs.
 *  - Log
 *      - 2010.03.30 Dohyun Yun, remove nArgs parameter in all interfaces.
 *      - 2010.07.14 Wei Cao, merge SFLogger and LogManager, this header file is kept just for consistence
 */


#ifndef _SF1V5_SF_LOGGER__
#define _SF1V5_SF_LOGGER__

#include <log-manager/LogManager.h>

#include <glog/logging.h>

#define sflog (&sf1r::LogManager::instance())

#endif // _SF1V5_SF_LOGGER__
