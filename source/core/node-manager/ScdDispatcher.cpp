#include "ScdDispatcher.h"

#include <boost/filesystem.hpp>
#include <boost/assert.hpp>


using namespace sf1r;
using namespace boost::filesystem;

ScdDispatcher::ScdDispatcher(ScdSharding* scdSharding)
: scdSharding_(scdSharding), scdEncoding_(izenelib::util::UString::UTF_8)
{
    BOOST_ASSERT(scdSharding_);
}

bool ScdDispatcher::dispatch(const std::string& dir, unsigned int docNum)
{
    std::vector<std::string> scdFileList;
    if (!getScdFileList(dir, scdFileList)) {
        std::cout<<"[ScdDispatcher::dispatch] failed to load SCDs."<<std::endl;
        return false;
    }

    unsigned int docProcessed = 0;
    for (size_t i = 1; i <= scdFileList.size(); i++)
    {
        path scd_path(scdFileList[i-1]);
        curScdFileName_ = scd_path.filename();

        ScdParser scdParser(scdEncoding_);
        if(!scdParser.load(scd_path.string()) )
        {
            std::cerr << "Load scd file failed: " << scd_path.string() << std::endl;
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
                std::cout<<"\rProcessed documents: "<<docProcessed<<std::flush;
            }

            if (docProcessed >= docNum && docNum > 0)
                break;
        }

        if (docProcessed >= docNum  && docNum > 0)
            break;
    }
    //std::cout<<std::endl;

    // post process for batch dispatching
    finish();

    return true;
}

bool ScdDispatcher::getScdFileList(const std::string& dir, std::vector<std::string>& fileList)
{
    fileList.clear();

    if ( exists(dir) )
    {
        if ( !is_directory(dir) ) {
            std::cout << "It's not a directory: " << dir << std::endl;
            return false;
        }

        directory_iterator iterEnd;
        for (directory_iterator iter(dir); iter != iterEnd; iter ++)
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
        std::cout << "File path dose not existed: " << dir << std::endl;
        return false;
    }
}

/// Class BatchScdDispatcher //////////////////////////////////////////////////////////

BatchScdDispatcher::BatchScdDispatcher(ScdSharding* scdSharding)
: ScdDispatcher(scdSharding)
{
    // xxx
    dispatchTempDir_ = "./ScdDispatcherTemp";
    create_directory(dispatchTempDir_);

    ofList_.resize(scdSharding->getMaxShardID()+1, NULL);
    for (unsigned int shardid = scdSharding->getMinShardID();
            shardid <= scdSharding->getMaxShardID(); shardid++)
    {
        std::ostringstream oss;
        oss << dispatchTempDir_<<"/"<<shardid;

        std::string shardScdDir = oss.str();
        create_directory(shardScdDir);

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

    std::string shardScdFilePath = shardScdfileMap_[shardid]+"/"+curScdFileName_;

    std::ofstream*& rof = ofList_[shardid];
    if (!rof->is_open())
    {
        rof->open(shardScdFilePath.c_str(), ios_base::out);
        if (!rof->is_open())
            return false;
    }

    (*rof) << scdDoc;

    /// xxx add shardid property
    //(*rof) << "<SHARDID>" << shardid << std::endl;

    return true;
}

void BatchScdDispatcher::finish()
{
    // TODO, Send splitted scd files in sub dirs to each shard server
}

/// Class BatchScdDispatcher //////////////////////////////////////////////////////////

bool InstantScdDispatcher::dispatch_impl(shardid_t shardid, SCDDoc& scdDoc)
{
    // TODO, Send scd doc to shard server corresponding to shardid
    // scdDoc format? through open api (client)?

    return false;
}













