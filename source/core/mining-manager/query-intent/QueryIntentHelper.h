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
#include <configuration-manager/QueryIntentConfig.h>

namespace sf1r
{
void rewriteRequest(izenelib::driver::Request& request,
             std::map<QueryIntentCategory, std::list<std::string> >& intents);
}

#endif // QueryIntentHelper.h

