#include "log_server_handler.h"
#include "log_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <log-manager/LogServerRequest.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

LogServerHandler::LogServerHandler(const LogServerConnectionConfig& config, bool reindex)
:logserver_config_(config), reindex_(reindex), open_(false)
{
}

bool LogServerHandler::Open()
{
    if(open_) return true;
    if(!LogServerClient::Init(logserver_config_))
    {
        std::cout<<"log server init failed"<<std::endl;
        return false;
    }
    if(!LogServerClient::Test())
    {
        std::cout<<"log server test connection failed"<<std::endl;
        return false;
    }
    open_ = true;
    return true;
}

void LogServerHandler::Process(const BuueItem& item)
{
    if(item.type==BUUE_APPEND)
    {
        if(item.pid.length()>0)
        {
            if(reindex_)
            {
                LogServerClient::Update(item.pid, item.docid_list);
            }
            else
            {
                LogServerClient::AppendUpdate(item.pid, item.docid_list);
            }

        }
    }
    else if(item.type==BUUE_REMOVE)
    {
        if(!reindex_)
        {
            //if(uuid=="*")
            //{
                //if(!LogServerClient::GetUuid(item.docid_list[0], uuid)) return;
            //}
            LogServerClient::RemoveUpdate(item.pid, item.docid_list);
        }

    }
}

void LogServerHandler::Finish()
{
    LogServerClient::Flush();
}

