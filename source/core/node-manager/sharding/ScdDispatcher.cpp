#include "ScdDispatcher.h"

#include <node-manager/MasterNodeManager.h>

#include <net/distribute/DataTransfer.h>

#include <boost/filesystem.hpp>
#include <boost/assert.hpp>

#include <glog/logging.h>

using namespace sf1r;
using namespace net::distribute;
namespace bfs = boost::filesystem;

ScdDispatcher::ScdDispatcher(ScdSharding* scdSharding)
: scdSharding_(scdSharding), scdEncoding_(izenelib::util::UString::UTF_8)
{
    BOOST_ASSERT(scdSharding_);
}

bool ScdDispatcher::dispatch(const std::string& dir, unsigned int docNum)
{
    LOG(INFO) << "start SCD sharding";

    std::vector<std::string> scdFileList;
    if (!getScdFileList(dir, scdFileList))
        return false;

    unsigned int docProcessed = 0;
    for (size_t i = 1; i <= scdFileList.size(); i++)
    {
        bfs::path scd_path(scdFileList[i-1]);
        curScdFileName_ = scd_path.filename();
        switchFile();

        ScdParser scdParser(scdEncoding_);
        if(!scdParser.load(scd_path.string()) )
        {
            LOG(ERROR) << "Load scd file failed: " << scd_path.string();
            return false;
        }

        for (ScdParser::iterator it = scdParser.begin(); it != scdParser.end(); it++)
        {
            SCDDocPtr pScdDoc = *it;
            shardid_t shardId = scdSharding_->sharding(*pScdDoc);

            // dispatching one doc
            dispatch_impl(shardId, *pScdDoc);

            // count
            docProcessed ++;
            if (docProcessed % 1000 == 0)
            {
                std::cout << "\rProcessed documents: "<<docProcessed<<std::flush;
            }

            if (docProcessed >= docNum && docNum > 0)
                break;
        }
        std::cout<<"\rProcessed documents: "<<docProcessed<<std::flush;
        std::cout<<std::endl;

        if (docProcessed >= docNum  && docNum > 0)
            break;
    }

    LOG(INFO) << "end SCD sharding";

    // post process of dispatching
    return finish();
}

bool ScdDispatcher::getScdFileList(const std::string& dir, std::vector<std::string>& fileList)
{
    fileList.clear();

    if ( bfs::exists(dir) )
    {
        if ( !bfs::is_directory(dir) ) {
            std::cout << "It's not a directory: " << dir << std::endl;
            return false;
        }

        bfs::directory_iterator iterEnd;
        for (bfs::directory_iterator iter(dir); iter != iterEnd; iter ++)
        {
            std::string file_name = iter->path().filename();

            if (ScdParser::checkSCDFormat(file_name) )
            {
                SCD_TYPE scd_type = ScdParser::checkSCDType(file_name);
                if( scd_type == INSERT_SCD || scd_type == UPDATE_SCD || scd_type == DELETE_SCD)
                {
                    //cout << "scd file: "<<iter->path().string() << endl;
                    fileList.push_back( iter->path().string() );
                }
            }
        }

        if (fileList.size() > 0) {
            return true;
        }
        else {
            std::cout << "There is no scd file in: " << dir << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "Directory dose not existed: " << dir << std::endl;
        return false;
    }
}

/// Class BatchScdDispatcher //////////////////////////////////////////////////////////

BatchScdDispatcher::BatchScdDispatcher(
        ScdSharding* scdSharding,
        const std::string& collectionName,
        const std::string& dispatchTempDir)
: ScdDispatcher(scdSharding)
, collectionName_(collectionName)
, dispatchTempDir_(dispatchTempDir)
{
    bfs::create_directory(dispatchTempDir_);

    ofList_.resize(scdSharding->getMaxShardID()+1, NULL);
    for (unsigned int shardid = scdSharding->getMinShardID();
            shardid <= scdSharding->getMaxShardID(); shardid++)
    {
        std::ostringstream oss;
        oss << dispatchTempDir_<<"/"<<shardid;

        std::string shardScdDir = oss.str();
        bfs::create_directory(shardScdDir);

        shardScdfileMap_.insert(std::make_pair(shardid, shardScdDir));
        ofList_[shardid] = new std::ofstream;
    }
}

BatchScdDispatcher::~BatchScdDispatcher()
{
    for (size_t i = 0; i < ofList_.size(); i++)
    {
        if (ofList_[i])
        {
            ofList_[i]->close();
            delete ofList_[i];
        }
    }
}

bool BatchScdDispatcher::switchFile()
{
    //std::cout<<"switchFile to "<<curScdFileName_<<std::endl;

    for (unsigned int shardid = scdSharding_->getMinShardID();
                shardid <= scdSharding_->getMaxShardID(); shardid++)
    {
        std::ofstream*& rof = ofList_[shardid];

        if (!rof)
            rof = new std::ofstream;

        // close former file
        if (rof->is_open())
            rof->close();

        // open new file
        std::string shardScdFilePath = shardScdfileMap_[shardid]+"/"+curScdFileName_;
        LOG(INFO) << "create scd shard: "<<shardScdFilePath<<std::endl;
        rof->open(shardScdFilePath.c_str(), ios_base::out);
        if (!rof->is_open())
        {
            LOG(INFO) << "Failed to create: "<<shardScdFilePath;
            return false;
        }
    }

    return true;
}

std::ostream& operator<<(std::ostream& out, SCDDoc& scdDoc)
{
    SCDDoc::iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        std::string str;
        (*propertyIter).first.convertString(str, izenelib::util::UString::UTF_8);
        out << "<" << str << ">";
        (*propertyIter).second.convertString(str, izenelib::util::UString::UTF_8);
        out << str << std::endl;
    }

    return out;
}

bool BatchScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    //std::cout<<"BatchScdDispatcher::dispatch_impl shardid: "<<shardid<<std::endl;

    std::ofstream*& rof = ofList_[shardid];

    (*rof) << scdDoc;

    // add shardid property?
    //(*rof) << "<SHARDID>" << shardid << std::endl;

    return true;
}

bool BatchScdDispatcher::finish()
{
    bool ret = false;
    LOG(INFO) << "start SCD dispatching" ;

    // Send splitted scd files in sub dirs to each shard server
    for (unsigned int shardid = scdSharding_->getMinShardID();
            shardid <= scdSharding_->getMaxShardID(); shardid++)
    {
        std::string host;
        unsigned int recvPort;
        if (MasterNodeManagerSingleton::get()->getShardReceiver(shardid, host, recvPort))
        {
            LOG(INFO) << "Transfer scd from "<<shardScdfileMap_[shardid]
                      <<"/ to shard "<<shardid<<" ["<<host<<":"<<recvPort<<"]";
            // thread?
            DataTransfer transfer(host, recvPort);
            if (transfer.syncSend(shardScdfileMap_[shardid], collectionName_+"/scd/index") == 0)
            {
                ret = true; // xxx
                bfs::remove_all(shardScdfileMap_[shardid]);
            }
        }
        else
        {
            //ret = false;
            LOG(ERROR) << "Not found server info for shard "<<shardid;
        }
    }

    LOG(INFO) << "end SCD dispatching" ;
    return ret;
}

/// Class BatchScdDispatcher //////////////////////////////////////////////////////////

bool InstantScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    // TODO, Send scd doc to shard server corresponding to shardid
    // scdDoc format? through open api (client)?

    return false;
}













