#include "ScdDispatcher.h"

#include <node-manager/SearchMasterManager.h>
#include <node-manager/RecoveryChecker.h>

#include <net/distribute/DataTransfer2.hpp>

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/assert.hpp>

#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

const static std::string DISPATCH_TEMP_DIR = "dispatch-temp-dir/";

ScdDispatcher::ScdDispatcher(const boost::shared_ptr<ScdSharder>& scdSharder)
    : scdSharder_(scdSharder)
    , scdEncoding_(izenelib::util::UString::UTF_8)
{
    BOOST_ASSERT(scdSharder_);
}

bool ScdDispatcher::dispatch(std::vector<std::string>& scdFileList, const std::string& dir, unsigned int docNum)
{
    LOG(INFO) << "start SCD sharding";

    scdFileList.clear();
    if (!getScdFileList(dir, scdFileList))
        return false;

    // set scd dir and initialize
    scdDir_ = dir;
    if (!initialize())
        return false;

    unsigned int docProcessed = 0;
    for (size_t i = 1; i <= scdFileList.size(); i++)
    {
        bfs::path scd_path(scdFileList[i-1]);
        curScdFileName_ = scd_path.filename().string();
        switchFile();

        ScdParser scdParser(scdEncoding_);
        if(!scdParser.load(scd_path.string()))
        {
            LOG(ERROR) << "Load scd file failed: " << scd_path.string();
            return false;
        }

        for (ScdParser::iterator it = scdParser.begin(); it != scdParser.end(); it++)
        {
            SCDDocPtr pScdDoc = *it;
            shardid_t shardId = scdSharder_->sharding(*pScdDoc);

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

            // for interruption
            boost::this_thread::interruption_point();
        }
        std::cout<<"\rProcessed documents: "<<docProcessed<<std::flush;
        std::cout<<std::endl;

        if (docProcessed >= docNum  && docNum > 0)
            break;
    }

    LOG(INFO) << "end SCD sharding";

    // post process (dispatching)
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
            std::string file_name = iter->path().filename().string();

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
        const boost::shared_ptr<ScdSharder>& scdSharder,
        const std::string& collectionName)
: ScdDispatcher(scdSharder)
, collectionName_(collectionName)
{
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

bool BatchScdDispatcher::initialize()
{
    return initTempDir(scdDir_ + DISPATCH_TEMP_DIR);
}

bool BatchScdDispatcher::switchFile()
{
    //std::cout<<"switchFile to "<<curScdFileName_<<std::endl;

    for (unsigned int shardid = scdSharder_->getMinShardID();
                shardid <= scdSharder_->getMaxShardID(); shardid++)
    {
        std::ofstream*& rof = ofList_[shardid];

        if (!rof)
        {
            continue;
        }

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
        out << "<" << propertyIter->first << ">";
        (*propertyIter).second.convertString(str, izenelib::util::UString::UTF_8);
        out << str << std::endl;
    }

    return out;
}

bool BatchScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    std::ofstream*& rof = ofList_[shardid];
    (*rof) << scdDoc;

    // add shardid property?
    //(*rof) << "<SHARDID>" << shardid << std::endl;

    return true;
}

bool BatchScdDispatcher::finish()
{
    bool ret = true;
    LOG(INFO) << "start SCD dispatching" ;

    // Send splitted scd files in sub dirs to each shard server
    for (unsigned int shardid = scdSharder_->getMinShardID();
            shardid <= scdSharder_->getMaxShardID(); shardid++)
    {
        if (!SearchMasterManager::get()->checkCollectionShardid(collectionName_, shardid))
        {
            continue;
        }

        std::string host;
        unsigned int recvPort;
        if (SearchMasterManager::get()->getShardReceiver(shardid, host, recvPort))
        {
            LOG(INFO) << "Transfer scd from "<<shardScdfileMap_[shardid]
                      <<"/ to shard "<<shardid<<" ["<<host<<":"<<recvPort<<"]";

            izenelib::net::distribute::DataTransfer2 transfer(host, recvPort);
            if (not transfer.syncSend(shardScdfileMap_[shardid], collectionName_ + "/scd/index"))
            {
                ret = false;
                LOG(ERROR) << "Failed to transfer scd"<<shardid;
            }
        }
        else
        {
            ret = false;
            LOG(ERROR) << "Not found server info for shard "<<shardid;
        }
        if (!ret)
            break;
    }

    LOG(INFO) << "end SCD dispatching" ;
    return ret;
}

bool BatchScdDispatcher::initTempDir(const std::string& tempDir)
{
    bfs::remove_all(tempDir);
    bfs::create_directory(tempDir);

    ofList_.resize(scdSharder_->getMaxShardID()+1, NULL);
    for (unsigned int shardid = scdSharder_->getMinShardID();
            shardid <= scdSharder_->getMaxShardID(); shardid++)
    {
        // shards are collection related
        if (!SearchMasterManager::get()->checkCollectionShardid(collectionName_, shardid))
        {
            continue;
        }

        std::ostringstream oss;
        oss << tempDir << shardid;

        std::string shardScdDir = oss.str();
        bfs::create_directory(shardScdDir);

        shardScdfileMap_.insert(std::make_pair(shardid, shardScdDir));
        ofList_[shardid] = new std::ofstream;
    }

    return true;
}

/// Class BatchScdDispatcher //////////////////////////////////////////////////////////

bool InstantScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    // Here intended to directly dispatch the scdDoc to Worker identified by shardid,
    // this can be done by perform a createDocument request through Sf1Driver client.

    return false;
}













