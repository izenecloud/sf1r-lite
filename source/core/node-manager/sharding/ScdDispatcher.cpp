#include "ScdDispatcher.h"

#include <node-manager/MasterManagerBase.h>
#include <node-manager/DistributeSearchService.h>
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
                LOG(INFO) << "\rProcessed documents: "<<docProcessed<<std::flush;
            }

            if (docProcessed >= docNum && docNum > 0)
                break;

            // for interruption
            boost::this_thread::interruption_point();
        }
        LOG(INFO) << "\rProcessed documents: " << docProcessed;

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
                //cout << "scd file: "<<iter->path().string() << endl;
                fileList.push_back( iter->path().string() );
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
        const std::string& collectionName,
        bool is_dfs_enabled)
: ScdDispatcher(scdSharder)
, collectionName_(collectionName)
, is_dfs_enabled_(is_dfs_enabled)
{
    service_ = Sf1rTopology::getServiceName(Sf1rTopology::SearchService);
}

BatchScdDispatcher::~BatchScdDispatcher()
{
    for (std::map<shardid_t, std::ofstream*>::iterator it = ofList_.begin();
        it != ofList_.end(); ++it)
    {
        if (it->second)
        {
            it->second->close();
            delete it->second;
        }
    }
}

bool BatchScdDispatcher::initialize()
{
    return initTempDir((bfs::path(scdDir_)/bfs::path(DISPATCH_TEMP_DIR)).string());
}

bool BatchScdDispatcher::switchFile()
{
    LOG(INFO) <<" switchFile to "<< curScdFileName_ <<std::endl;

    for (std::map<shardid_t, std::ofstream*>::iterator it = ofList_.begin();
        it != ofList_.end(); ++it)
    {
        std::ofstream*& rof = it->second;
        shardid_t shardid = it->first;

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

std::ostream& operator<<(std::ostream& out, const SCDDoc& scdDoc)
{
    SCDDoc::const_iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        out << "<" << propertyIter->first << ">";
        out << (*propertyIter).second << std::endl;
    }

    return out;
}

bool BatchScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    std::ofstream*& rof = ofList_[shardid];
    (*rof) << scdDoc;

    return true;
}

bool BatchScdDispatcher::finish()
{
    bool ret = true;
    LOG(INFO) << "start SCD dispatching" ;

    if (is_dfs_enabled_)
    {
        LOG(INFO) << "DFS enabled, no need dispatch scd files.";
        return true;
    }
    std::vector<shardid_t> shardids;
    // Send splitted scd files in sub dirs to each shard server
    MasterManagerBase::get()->getCollectionShardids(service_, collectionName_, shardids);
    for (size_t i = 0; i < shardids.size(); ++i)
    {
        shardid_t shardid = shardids[i];
        std::string host;
        unsigned int recvPort;
        if (MasterManagerBase::get()->getShardReceiver(shardid, host, recvPort))
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

    std::vector<shardid_t> shardids;
    MasterManagerBase::get()->getCollectionShardids(service_, collectionName_, shardids);
    for (size_t i = 0; i < shardids.size(); ++i)
    {
        std::ostringstream oss;
        oss << tempDir << shardids[i];

        std::string shardScdDir = oss.str();
        bfs::create_directory(shardScdDir);

        shardScdfileMap_.insert(std::make_pair(shardids[i], shardScdDir));
        ofList_[shardids[i]] = new std::ofstream;
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













