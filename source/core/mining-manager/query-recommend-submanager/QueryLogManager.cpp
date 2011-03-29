#include <mining-manager/util/TermUtil.hpp>
#include <mining-manager/util/FSUtil.hpp>
#include "QueryLogManager.h"
#include <bitset>
using namespace sf1r;

QueryLogManager::QueryLogManager(const std::string& path):
path_(path), isOpen_(false), writer_(NULL)
,SUM("SUM"), SUM_TMP("SUM_TMP"), MAIN("MAIN"), TIME("TIME")
    
{
    boost::filesystem::create_directories(path_);
    
    writer_ = new writer_t(path_+"/"+MAIN); 
    writer_->open();
    isOpen_ = true;
    writer_t sumWriter(path_+"/"+SUM);
    sumWriter.open();
    sumWriter.close();
    initTimeFile_(false);
}

QueryLogManager::~QueryLogManager()
{
    close();
}
void QueryLogManager::close()
{
    if(isOpen_)
    {
        writer_->close();
        delete writer_;
        isOpen_ = false;
    }
}

void QueryLogManager::append(const izenelib::util::UString& queryStr)
{
    QueryLogItem item( queryStr);
    uint64_t id = TermUtil::makeQueryId(queryStr);
    boost::mutex::scoped_lock lock(writeMutex_);
    writer_->append(id, item);
}
             
QueryLogManager::reader_t* QueryLogManager::compute(uint64_t& changeCount)
{
    if( FSUtil::exists(path_+"/"+TIME) )
    {
        std::ifstream ifs( (path_+"/"+TIME).c_str() );
        long old = 0L;
        ifs>>old;
        ifs.close();
        long current = time(NULL);
        if( current - old > 3600*24*7 )//more than one week
        {
            FSUtil::del(path_+"/"+SUM);
            writer_t sumWriter(path_+"/"+SUM);
            sumWriter.open();
            sumWriter.close();
            initTimeFile_(true);
        }
    }
    std::string copyFromPath = writer_->getPath();
    std::string copyToPath = FSUtil::getTmpFileFullName(path_);
    {
        boost::mutex::scoped_lock lock(writeMutex_);
        writer_->close();
        delete writer_;
        boost::filesystem::rename( copyFromPath, copyToPath);
        writer_ = new writer_t(path_+"/"+MAIN); 
        writer_->open();
        
    }
    {
        reader_t reader(copyToPath);
        reader.open();
        changeCount = reader.getItemCount();
        reader.close();

    }
    if( changeCount > 0 )
    {
        sorter_t sorter;
        sorter.sort(copyToPath);

        //merge the two sorted file
        FSUtil::del(path_+"/"+SUM_TMP);
        writer_t writer(path_+"/"+SUM_TMP);
        writer.open();
        
        std::vector<QueryLogItem> valueList1;
        std::vector<QueryLogItem> valueList2;
        uint64_t key = 0;
        {
            merger_t merger;
            merger.setObj(copyToPath, path_+"/"+SUM);
            while( merger.next(key, valueList1, valueList2) )
            {
                QueryLogItem item;
                uint32_t i1 = 0;
                uint32_t i2 = 0;
                if( valueList1.size()>0 )
                {
                    item = valueList1[0];
                    i1 = 1;
                }
                else
                {
                    item = valueList2[0];
                    i2 = 1;
                }
                for(;i1<valueList1.size();i1++)
                {
                    item += valueList1[i1];
                }
                for(;i2<valueList2.size();i2++)
                {
                    item += valueList2[i2];
                }
                writer.append(key, item);
            }
        }
        writer.close();
        
        boost::filesystem::remove_all(path_+"/"+SUM);
        boost::filesystem::rename(path_+"/"+SUM_TMP, path_+"/"+SUM);
    }
    boost::filesystem::remove_all(copyToPath);
    reader_t* resultReader_ = new reader_t(path_+"/"+SUM);
    resultReader_->open();
    return resultReader_;
    
}

void QueryLogManager::initTimeFile_(bool replace)
{
    if( replace )
    {
        FSUtil::del(path_+"/"+TIME);
    }
    if( !FSUtil::exists(path_+"/"+TIME) )
    {
        std::ofstream ofs( (path_+"/"+TIME).c_str() );
        long utime = time(NULL);
        ofs<<utime;
        ofs.close();
    }
}

