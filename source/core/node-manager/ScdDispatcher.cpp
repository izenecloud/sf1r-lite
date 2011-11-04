#include "ScdDispatcher.h"

#include <boost/filesystem.hpp>
#include <boost/assert.hpp>


using namespace sf1r;
using namespace boost::filesystem;

void DispatchActionToFile::init(ShardingConfig& shardConfig)
{

}

void DispatchActionToFile::dispatch(shardid_t shardid, SCDDoc& scdDoc)
{
    /// todo, dispatching
}

void DispatchActionToFile::finish()
{

}

ScdDispatcher::ScdDispatcher(ScdSharding* scdSharding, DispatchAction* dispatchAction)
: scdSharding_(scdSharding), dispatchAction_(dispatchAction), scdEncoding_(izenelib::util::UString::UTF_8)
{
    BOOST_ASSERT(scdSharding_);
    BOOST_ASSERT(dispatchAction_);

    dispatchAction_->init(scdSharding->getShardingConfig());
}

bool ScdDispatcher::dispatch(const std::string& dir, unsigned int docNum)
{
    std::vector<std::string> scdFileList;
    if (!getScdFileList(dir, scdFileList)) {
        return false;
    }

    unsigned int docProcessed = 0;
    for (size_t i = 1; i <= scdFileList.size(); i++)
    {
        std::string scdFile = scdFileList[i-1];
        ScdParser scdParser(scdEncoding_);
        if(!scdParser.load(scdFile) )
        {
            std::cerr << "Load scd file failed: " << scdFile << std::endl;
            return false;
        }

        for (ScdParser::iterator it = scdParser.begin(); it != scdParser.end(); it++)
        {
            SCDDocPtr pScdDoc = *it;
            shardid_t shardId = scdSharding_->sharding(*pScdDoc);

            // dispatching
            dispatchAction_->dispatch(shardId, *pScdDoc);

            // count
            docProcessed ++;
            //std::cout<<"Processed documents: "<<docProcessed<<std::endl;

            if (docProcessed >= docNum && docNum > 0)
                break;
        }

        if (docProcessed >= docNum  && docNum > 0)
            break;
    }

    dispatchAction_->finish();

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
                if( scd_type == INSERT_SCD ||scd_type == UPDATE_SCD )
                {
                    cout << "scd file: "<<iter->path().string() << endl;
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
