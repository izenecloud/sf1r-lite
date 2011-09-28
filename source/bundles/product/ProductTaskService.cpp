#include "ProductTaskService.h"
#include <bundles/index/IndexTaskService.h>

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <log-manager/ProductInfo.h>

#include <common/SFLogger.h>
#include <common/Utilities.h>
#include <license-manager/LicenseManager.h>
#include <process/common/CollectionMeta.h>
#include <process/common/XmlConfigParser.h>

#include <util/profiler/ProfilerGroup.h>

#include <boost/filesystem.hpp>

#include <la/util/UStringUtil.h>

#include <glog/logging.h>

#include <memory> // for auto_ptr
#include <signal.h>
#include <protect/RestrictMacro.h>
namespace bfs = boost::filesystem;

using namespace izenelib::driver;
using izenelib::util::UString;

namespace
{
/** the directory for scd file backup */
const char* SCD_BACKUP_DIR = "backup";
}

namespace sf1r
{
ProductTaskService::ProductTaskService(
    ProductBundleConfiguration* bundleConfig
    )
    : bundleConfig_(bundleConfig)
    , indexTaskService_(NULL)
    , productSourceField_("")
{
}

ProductTaskService::~ProductTaskService()
{
}


bool ProductTaskService::buildCollection(unsigned int numdoc)
{
    string scdPath = bundleConfig_->collPath_.getScdPath() + "index/";

    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        DLOG(ERROR) << "Index directory is corrupted"<<endl;
        return false;
    }

    DLOG(INFO) << "start BuildCollection"<<endl;

    izenelib::util::ClockTimer timer;

    ScdParser parser(bundleConfig_->encoding_);

    // saves the name of the scd files in the path
    vector<string> scdList;
    try
    {
        if (bfs::is_directory(scdPath) == false)
        {
            DLOG(ERROR) << "SCD Path does not exist. Path "<<scdPath<<endl;
            return false;
        }
    }
    catch(boost::filesystem::filesystem_error& e)
    {
        DLOG(ERROR) << "Error while opening directory "<<e.what()<< endl;
        return false;
    }


    // search the directory for files
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdPath); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename();
            if (parser.checkSCDFormat(fileName) )
            {
                scdList.push_back(itr->path().string() );
                parser.load(scdPath+fileName);
            }
            else
            {
                DLOG(WARNING) << "SCD File not valid "<<fileName <<endl;
            }
        }
    }

    //sort scdList
    sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);

    DLOG(INFO) << "SCD Files in Path processed in given  order. Path "<<scdPath <<endl;
    vector<string>::iterator scd_it;
    for ( scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
        DLOG(INFO) << "SCD File "<< boost::filesystem::path(*scd_it).stem()<<endl;

    try
    {
        // loops the list of SCD files that belongs to this collection
        long proccessedFileSize = 0;
        for ( scd_it = scdList.begin(); scd_it != scdList.end(); scd_it++ )
        {
            size_t pos = scd_it ->rfind("/")+1;
            string filename = scd_it ->substr(pos);

            DLOG(INFO) << "Processing SCD file. "<< boost::filesystem::path(*scd_it).stem()<<endl;

            switch ( parser.checkSCDType(*scd_it) )
            {
            case INSERT_SCD:
            {
                if (doBuildCollection_( *scd_it, 1, numdoc ) == false)
                {
                    //continue;
                }
                DLOG(INFO) << "Indexing Finished"<<endl;

            }
            break;
            case DELETE_SCD:
            {
                if (documentManager_->getMaxDocId()> 0)
                {
                    doBuildCollection_( *scd_it, 3, 0 );
                    DLOG(INFO) << "Delete Finished"<<endl;
                }
                else
                {
                    DLOG(WARNING) << "Indexed documents do not exist. File "<<boost::filesystem::path(*scd_it).stem()<<endl;
                }
            }
            break;
            case UPDATE_SCD:
            {
                if (documentManager_->getMaxDocId()> 0)
                {
                    doBuildCollection_( *scd_it, 2, 0 );
                    DLOG(INFO) << "Update Finished"<<endl;
                }
                else
                {
                    DLOG(WARNING) << "Indexed documents do not exist. File "<<boost::filesystem::path(*scd_it).stem()<<endl;
                }
            }
            break;
            default:
                break;
            }
            parser.load( *scd_it );
            proccessedFileSize += parser.getFileSize();

        } // end of loop for scd files of a collection

        documentManager_->flush();

        idManager_->flush();
    }
    catch (std::exception& e)
    {
        LOG(WARNING) << "exception in indexing or mining: " << e.what();
        return false;
    }

    bfs::path bkDir = bfs::path(scdPath) / SCD_BACKUP_DIR;
    bfs::create_directory(bkDir);
    DLOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;
    for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
    {
        try
        {
            bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
        }
        catch(bfs::filesystem_error& e)
        {
            LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
        }
    }

    DLOG(INFO) << "Archiving Finished! Documents Indexed: " <<documentManager_->getMaxDocId()<<endl;

    DLOG(INFO) << "End BuildCollection: "<< endl;
    DLOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds\n";

    return true;
}

void ProductTaskService::value2SCDDoc(const Value& value, SCDDoc& scddoc)
{
    const Value::ObjectType& objectValue = value.getObject();
    scddoc.resize(objectValue.size());

    std::size_t propertyId = 0;
    for (Value::ObjectType::const_iterator it = objectValue.begin();
         it != objectValue.end(); ++it, ++propertyId)
    {
        scddoc[propertyId].first.assign(it->first, izenelib::util::UString::UTF_8);
        scddoc[propertyId].second.assign(
            asString(it->second),
            izenelib::util::UString::UTF_8
        );
    }
}

bool ProductTaskService::doBuildCollection_(
    const std::string& fileName,
    int op,
    uint32_t numdoc
)
{
    return true;
}

bool ProductTaskService::backup_()
{
    const boost::shared_ptr<Directory>& current
        = directoryRotator_.currentDirectory();
    const boost::shared_ptr<Directory>& next
        = directoryRotator_.nextDirectory();

    // valid pointer
    // && not the same directory
    // && have not copied successfully yet
    if (next
        && current->name() != next->name()
        && ! (next->valid() && next->parentName() == current->name()))
    {
        try
        {
            std::cout << "Copy index dir from "
                      << current->name()
                      << " to "
                      << next->name()
                      << std::endl;
            next->copyFrom(*current);
            return true;
        }
        catch(boost::filesystem::filesystem_error& e)
        {
            DLOG(INFO)<< "Failed to copy index directory "<<e.what()<<endl;
        }

        // try copying but failed
        return false;
    }

    // not copy, always returns true
    return true;
}

}

