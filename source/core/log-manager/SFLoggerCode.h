#ifndef __SF_LOGGER_CODE__
#define __SF_LOGGER_CODE__

enum SFLoggerApp
{
    SL_APP_DUMMY    = 0, // dummy
    SL_APP_BA       = 1, // indexer
    SL_APP_SIA      = 2, // isc
    SL_APP_MIA      = 3, // broker

    SL_APP_EOA      = 10, // end of application
};

enum SFLoggerStatus
{
    SFL_SYS, SFL_SCD, SFL_LA, SFL_IDX, SFL_MINE, SFL_INIT, SFL_SRCH,
    SFL_STAT_EOS    // end of status
};

enum LOG_TYPE { LOG_INFO, LOG_WARN, LOG_ERR, LOG_CRIT, LOG_EOS };

enum FIND_TYPE { FIND_FIRST, FIND_ALL };

#endif // __SF_LOGGER_CODE__
