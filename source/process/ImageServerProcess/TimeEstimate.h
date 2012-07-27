#ifndef __TIME_ESTIMATE_H__
#define __TIME_ESTIMATE_H__
#include <time.h>
#include <sys/time.h>

#define RECORD_TIME_START(SESSION)          struct timeval __tv1_##SESSION;\
                                            gettimeofday(&__tv1_##SESSION,NULL)
#define RECORD_TIME_END(SESSION)            struct timeval __tv2_##SESSION;\
                                            gettimeofday(&__tv2_##SESSION,NULL)
#define CALC_TIME_ELAPSED(SESSION)          float __elapsed_##SESSION=((float)(__tv2_##SESSION.tv_usec-__tv1_##SESSION.tv_usec))/1000.0+((float)(__tv2_##SESSION.tv_sec-__tv1_##SESSION.tv_sec))*1000.0
#define PRINT_TIME_ELAPSED(SESSION)         CALC_TIME_ELAPSED(SESSION);\
                                            LOG(LM_INFO,"[STAT]"#SESSION"_elapsed: %.2f ms\n",__elapsed_##SESSION)

#endif
