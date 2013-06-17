/**
 * @file QueryIntentHelper.h
 * @brief help functions for query intent.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */
#ifndef SF1R_QUERY_INTENT_HELPER_H
#define SF1R_QUERY_INTENT_HELPER_H

#include <map>
#include <list>

#include <util/driver/Request.h>
#include <util/driver/Response.h>
#include <configuration-manager/QueryIntentConfig.h>

namespace sf1r
{
void refineRequest(izenelib::driver::Request& request,
             izenelib::driver::Response& response,
             std::map<QueryIntentCategory, std::list<std::string> >& intents);
}

#endif // QueryIntentHelper.h

