#include "DriverLogServerController.h"
#include "LogDispatchHandler.h"

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace sf1r
{
using namespace izenelib::driver;

bool DriverLogServerController::preprocess()
{
    log_dispatch_handler_->setRequestContext(request(), response());
    return true;
}

/**
 * @brief Action \b update_cclog
 *
 * - @b record (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_cclog",
 *   },
 *   "host" : "172.16.0.161",
 *   "port" : "18181",
 *   "filename" : "cclog20120113.txt",
 *   "record" : {
 *       # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    log_dispatch_handler_->DispatchLog();
}

/**
 * @brief Action \b convert_raw_cclog
 * Convert value of docids from uuid to raw docid, for backup.
 *
 * - @b record (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "backup_raw_cclog",
 *   },
 *   "filename" : "cclog20120113.txt",
 *   "record" : {
 *       # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::backup_raw_cclog()
{
    log_dispatch_handler_->BackupRawCclog();
}

/**
 * @brief Action \b convert_raw_cclog
 * Convert value of docids from raw docid to latest uuid, for updating.
 *
 * - @b record (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "convert_raw_cclog",
 *   },
 *   "filename" : "cclog20120113.txt",
 *   "record" : {
 *       # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::convert_raw_cclog()
{
    log_dispatch_handler_->ConvertRawCclog();
}

/**
 * @brief Action \b update_scd
 *
 * - @b record (@c String): a doc record in SCD
 * - @b DOCID (@c String): the docid of the doc record
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_scd",
 *   },
 *   "filename" : "B-00-201112011437-38743-I-C.SCD",
 *   "DOCID" : "#{uuid_docid}"
 *   "record" : "#{the text of a record in SCD file}"
 *   }
 * }
 *
 */
void DriverLogServerController::update_scd()
{
    LOG(INFO)  << request().controller() << "/" << request().action();
    log_dispatch_handler_->UpdateSCD();
}

/**
 * @brief Action \b update_documents
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_documents",
 *   },
 *   "host" : "172.16.0.163",
 *   "port" : "18181"
 *   "collection" : "example"
 *   }
 * }
 *
 */
void DriverLogServerController::update_documents()
{
    log_dispatch_handler_->UpdateDocuments();
}

/**
 * @brief Action \b flush
 *
 * Client should send this request after all update requests for a file has been finished,
 * so that Server can perform flush properly.
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "flush",
 *   },
 *   "filename" : "cclog20120113.txt"
 *   "action" : "update_cclog" # update_cclog, update_cclog_history, backup_raw_cclog, convert_raw_cclog
 *   }
 * }
 *
 */
void DriverLogServerController::flush()
{
    LOG(INFO) << request().controller() << "/" << request().action();
    log_dispatch_handler_->Flush();
}

}
